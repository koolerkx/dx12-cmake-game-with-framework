#include "debug_visual_renderer_2d.h"

#include <cassert>
#include <iostream>
#include <vector>

#include "graphic.h"
#include "pipeline_state_builder.h"
#include "root_signature_builder.h"
#include "shader_manager.h"
#include "vertex_types.h"

bool DebugVisualRenderer2D::Initialize(Graphic& graphic) {
  device_ = graphic.GetDevice();
  graphic_ = &graphic;
  assert(device_ != nullptr);

  // Load shaders
  auto& shader_mgr = graphic.GetShaderManager();
  if (!shader_mgr.HasShader("DebugUIVS")) {
    if (!shader_mgr.LoadShader(L"Content/shaders/debug_ui.vs.cso", ShaderType::Vertex, "DebugUIVS")) {
      std::cerr << "[DebugVisualRenderer2D] Failed to load DebugUIVS shader" << '\n';
      return false;
    }
  }

  if (!shader_mgr.HasShader("DebugUIPS")) {
    if (!shader_mgr.LoadShader(L"Content/shaders/debug_ui.ps.cso", ShaderType::Pixel, "DebugUIPS")) {
      std::cerr << "[DebugVisualRenderer2D] Failed to load DebugUIPS shader" << '\n';
      return false;
    }
  }

  if (!CreateRootSignature()) {
    std::cerr << "[DebugVisualRenderer2D] Failed to create root signature" << '\n';
    return false;
  }

  if (!CreatePipelineState()) {
    std::cerr << "[DebugVisualRenderer2D] Failed to create pipeline state" << '\n';
    return false;
  }

  if (!CreateFrameResources()) {
    std::cerr << "[DebugVisualRenderer2D] Failed to create frame resources" << '\n';
    return false;
  }

  std::cout << "[DebugVisualRenderer2D] Initialized successfully" << '\n';
  return true;
}

void DebugVisualRenderer2D::Shutdown() {
  for (auto& frame_res : frame_resources_) {
    frame_res.vertex_buffer.reset();
  }

  pso_.Reset();
  root_signature_.Reset();
  device_ = nullptr;
  graphic_ = nullptr;
}

void DebugVisualRenderer2D::BeginFrame(uint32_t frame_index) {
  current_frame_index_ = frame_index % FRAME_BUFFER_COUNT;
  vertex_count_ = 0;
}

void DebugVisualRenderer2D::Render(const DebugVisualCommandBuffer2D& commands,
  Graphic& graphic,
  const UISceneData& scene_data,
  const DebugVisualSettings& settings) {
  if (commands.GetTotalCommandCount() == 0) {
    return;
  }

  ID3D12GraphicsCommandList* command_list = graphic.GetCommandList();
  assert(command_list != nullptr);

  // 2D renderer expects no DSV bound (PSO DepthStencilFormat = UNKNOWN)
  {
    auto rtv = graphic.GetMainRTV();
    command_list->OMSetRenderTargets(1, &rtv, FALSE, nullptr);

    auto viewport = graphic.GetScreenViewport();
    auto scissor = graphic.GetScissorRect();
    command_list->RSSetViewports(1, &viewport);
    command_list->RSSetScissorRects(1, &scissor);
  }

  FrameResource& frame_res = frame_resources_[current_frame_index_];
  assert(frame_res.vertex_buffer != nullptr);

  // Expand commands into vertices (with category filtering)
  std::vector<DebugVertex2D> vertices;
  vertices.reserve(commands.lines2D.size() * 2 + commands.rects2D.size() * 8);

  // Add line vertices (filter by category)
  for (const auto& line_cmd : commands.lines2D) {
    if (!settings.IsCategoryEnabled(line_cmd.category)) {
      continue;  // Skip disabled categories
    }
    uint32_t color = line_cmd.color.ToRGBA8();
    vertices.push_back({line_cmd.p0, color});
    vertices.push_back({line_cmd.p1, color});
  }

  // Add rectangle vertices (4 lines per rect, filter by category)
  for (const auto& rect_cmd : commands.rects2D) {
    if (!settings.IsCategoryEnabled(rect_cmd.category)) {
      continue;  // Skip disabled categories
    }
    uint32_t color = rect_cmd.color.ToRGBA8();
    DirectX::XMFLOAT2 tl = rect_cmd.top_left;
    DirectX::XMFLOAT2 br = {tl.x + rect_cmd.size.x, tl.y + rect_cmd.size.y};
    DirectX::XMFLOAT2 tr = {br.x, tl.y};
    DirectX::XMFLOAT2 bl = {tl.x, br.y};

    // Top edge
    vertices.push_back({tl, color});
    vertices.push_back({tr, color});
    // Right edge
    vertices.push_back({tr, color});
    vertices.push_back({br, color});
    // Bottom edge
    vertices.push_back({br, color});
    vertices.push_back({bl, color});
    // Left edge
    vertices.push_back({bl, color});
    vertices.push_back({tl, color});
  }

  vertex_count_ = static_cast<uint32_t>(vertices.size());

  if (vertex_count_ == 0) {
    return;
  }

  if (vertex_count_ > MAX_VERTICES_PER_FRAME) {
    std::cerr << "[DebugVisualRenderer2D] Vertex count exceeds maximum: " << vertex_count_ << '\n';
    vertex_count_ = MAX_VERTICES_PER_FRAME;
  }

  // Upload vertices to buffer
  frame_res.vertex_buffer->Upload(vertices.data(), vertex_count_ * sizeof(DebugVertex2D));

  // Set pipeline state
  command_list->SetPipelineState(pso_.Get());
  command_list->SetGraphicsRootSignature(root_signature_.Get());

  // Set view-projection matrix as root constant
  DirectX::XMFLOAT4X4 view_proj_transposed;
  DirectX::XMStoreFloat4x4(&view_proj_transposed, DirectX::XMMatrixTranspose(scene_data.view_projection_matrix));
  command_list->SetGraphicsRoot32BitConstants(0, 16, &view_proj_transposed, 0);

  // Set vertex buffer
  D3D12_VERTEX_BUFFER_VIEW vbv = frame_res.vertex_buffer->GetVBV(sizeof(DebugVertex2D));
  command_list->IASetVertexBuffers(0, 1, &vbv);
  command_list->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_LINELIST);

  // Draw
  command_list->DrawInstanced(vertex_count_, 1, 0, 0);
}

bool DebugVisualRenderer2D::CreateRootSignature() {
  RootSignatureBuilder builder;
  builder
    .AddRootConstant(16, 0, D3D12_SHADER_VISIBILITY_VERTEX)  // b0: 4x4 matrix (16 floats)
    .AllowInputLayout();

  return builder.Build(device_, root_signature_);
}

bool DebugVisualRenderer2D::CreatePipelineState() {
  assert(graphic_ != nullptr);

  auto& shader_mgr = graphic_->GetShaderManager();
  const ShaderBlob* vs = shader_mgr.GetShader("DebugUIVS");
  const ShaderBlob* ps = shader_mgr.GetShader("DebugUIPS");

  if (!vs || !ps) {
    std::cerr << "[DebugVisualRenderer2D] Required shaders not found" << '\n';
    return false;
  }

  auto input_layout = GetInputLayout_DebugVertex2D();

  PipelineStateBuilder builder;
  builder.SetVertexShader(vs)
    .SetPixelShader(ps)
    .SetInputLayout(input_layout.data(), static_cast<UINT>(input_layout.size()))
    .SetRootSignature(root_signature_.Get())
    .SetPrimitiveTopologyType(D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE)
    .SetRenderTargetFormat(DXGI_FORMAT_R8G8B8A8_UNORM)
    .SetDepthStencilFormat(DXGI_FORMAT_UNKNOWN)
    .SetDepthEnable(false)
    .SetDepthWriteMask(D3D12_DEPTH_WRITE_MASK_ZERO);

  return builder.Build(device_, pso_);
}

bool DebugVisualRenderer2D::CreateFrameResources() {
  for (uint32_t i = 0; i < FRAME_BUFFER_COUNT; ++i) {
    FrameResource& frame_res = frame_resources_[i];

    // Create upload buffer for vertices
    frame_res.vertex_buffer = std::make_unique<Buffer>();
    const size_t buffer_size = MAX_VERTICES_PER_FRAME * sizeof(DebugVertex2D);

    if (!frame_res.vertex_buffer->Create(device_, buffer_size, Buffer::Type::Vertex, D3D12_HEAP_TYPE_UPLOAD)) {
      std::cerr << "[DebugVisualRenderer2D] Failed to create vertex buffer for frame " << i << '\n';
      return false;
    }

    // Get mapped pointer (Buffer already maps it internally for upload heap)
    // We'll use Upload method to write data each frame
  }

  return true;
}
