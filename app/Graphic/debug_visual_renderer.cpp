#include "debug_visual_renderer.h"

#include <iostream>

#include "framework_default_assets.h"
#include "graphic.h"
#include "material_instance.h"
#include "material_template.h"

void DebugVisualRenderer::Initialize(Graphic& gfx) {
  if (is_initialized_) {
    return;  // Already initialized
  }

  graphic_ = &gfx;

  // Get debug line materials from default assets
  const auto& defaults = gfx.GetDefaultAssets();
  debug_line_material_ = defaults.GetDebugLineMaterial();
  debug_line_depth_material_ = defaults.GetDebugLineDepthMaterial();

  if (!debug_line_material_ || !debug_line_depth_material_) {
    std::cerr << "[DebugVisualRenderer] Failed to get debug line materials from DefaultAssets" << '\n';
    return;
  }

  debug_line_template_ = debug_line_material_->GetTemplate();
  debug_line_depth_template_ = debug_line_depth_material_->GetTemplate();

  // Create per-frame upload buffers
  if (!CreateFrameBuffers()) {
    std::cerr << "[DebugVisualRenderer] Failed to create frame buffers" << '\n';
    return;
  }

  is_initialized_ = true;
  std::cout << "[DebugVisualRenderer] Initialized successfully" << '\n';
}

void DebugVisualRenderer::Shutdown() {
  if (!is_initialized_) {
    return;
  }

  ReleaseFrameBuffers();

  debug_line_template_ = nullptr;
  debug_line_material_ = nullptr;
  debug_line_depth_template_ = nullptr;
  debug_line_depth_material_ = nullptr;
  graphic_ = nullptr;
  is_initialized_ = false;

  std::cout << "[DebugVisualRenderer] Shutdown complete" << '\n';
}

void DebugVisualRenderer::BeginFrame(uint32_t frameIndex) {
  if (!is_initialized_) {
    return;
  }

  current_frame_index_ = frameIndex % FRAME_BUFFER_COUNT;
  frames_[current_frame_index_].Reset();
}

void DebugVisualRenderer::Render(const DebugVisualCommandBuffer& cmds,
  ID3D12GraphicsCommandList* cmd_list,
  const SceneGlobalData& /* sceneData */,
  const Buffer& frame_cb,
  const DebugVisualSettings& settings) {
  if (!is_initialized_ || !cmd_list || !debug_line_material_ || !debug_line_depth_material_) {
    return;
  }

  auto& current_frame = frames_[current_frame_index_];

  // Identity world matrix for debug primitives (currently using world-space coordinates)
  // Note: No transpose needed - shader expects row-major, we store row-major
  DirectX::XMFLOAT4X4 identity_world;
  DirectX::XMStoreFloat4x4(&identity_world, DirectX::XMMatrixIdentity());

  // Set vertex buffer (shared by both passes)
  D3D12_VERTEX_BUFFER_VIEW vbv = current_frame.vertex_buffer.GetVBV(sizeof(DebugVertex));

  // ========================================
  // Pass 1: Render IgnoreDepth lines (overlay - no depth test)
  // ========================================
  DebugVertex* vertex_ptr = current_frame.mapped_ptr;
  UINT overlay_vertex_count = ConvertCommandsToVertices(cmds, vertex_ptr, MAX_DEBUG_VERTICES, DebugDepthMode::IgnoreDepth, settings);

  if (overlay_vertex_count > 0) {
    ID3D12PipelineState* pso = debug_line_template_->GetPSO();
    ID3D12RootSignature* root_signature = debug_line_template_->GetRootSignature();

    cmd_list->SetPipelineState(pso);
    cmd_list->SetGraphicsRootSignature(root_signature);
    cmd_list->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_LINELIST);
    cmd_list->IASetVertexBuffers(0, 1, &vbv);

    // Bind root constants and constant buffer
    cmd_list->SetGraphicsRoot32BitConstants(0, 16, &identity_world, 0);        // b0 - world matrix
    cmd_list->SetGraphicsRootConstantBufferView(1, frame_cb.GetGPUAddress());  // b1 - FrameCB

    cmd_list->DrawInstanced(overlay_vertex_count, 1, 0, 0);
  }

  // ========================================
  // Pass 2: Render TestDepth lines (depth-tested)
  // ========================================
  vertex_ptr += overlay_vertex_count;
  UINT depth_vertex_count =
    ConvertCommandsToVertices(cmds, vertex_ptr, MAX_DEBUG_VERTICES - overlay_vertex_count, DebugDepthMode::TestDepth, settings);

  if (depth_vertex_count > 0) {
    ID3D12PipelineState* pso = debug_line_depth_template_->GetPSO();
    ID3D12RootSignature* root_signature = debug_line_depth_template_->GetRootSignature();

    cmd_list->SetPipelineState(pso);
    cmd_list->SetGraphicsRootSignature(root_signature);
    cmd_list->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_LINELIST);
    cmd_list->IASetVertexBuffers(0, 1, &vbv);

    // Bind root constants and constant buffer
    cmd_list->SetGraphicsRoot32BitConstants(0, 16, &identity_world, 0);        // b0 - world matrix
    cmd_list->SetGraphicsRootConstantBufferView(1, frame_cb.GetGPUAddress());  // b1 - FrameCB

    cmd_list->DrawInstanced(depth_vertex_count, 1, overlay_vertex_count, 0);
  }

  // Track stats
  UINT total_vertices = overlay_vertex_count + depth_vertex_count;
  current_frame.vertex_count = total_vertices;
  last_frame_vertex_count_ = total_vertices;
}

bool DebugVisualRenderer::CreateFrameBuffers() {
  if (!graphic_) {
    return false;
  }

  // Create upload buffers for each frame
  for (int i = 0; i < FRAME_BUFFER_COUNT; ++i) {
    auto& frame = frames_[i];

    // Create upload buffer
    size_t buffer_size = MAX_DEBUG_VERTICES * sizeof(DebugVertex);
    if (!frame.vertex_buffer.Create(graphic_->GetDevice(), buffer_size, Buffer::Type::Vertex, D3D12_HEAP_TYPE_UPLOAD)) {
      std::cerr << "[DebugVisualRenderer] Failed to create vertex buffer for frame " << i << '\n';
      return false;
    }

    // Map the buffer persistently
    void* mapped_data = nullptr;
    D3D12_RANGE read_range = {};  // We won't read from upload buffer
    HRESULT hr = frame.vertex_buffer.GetResource()->Map(0, &read_range, &mapped_data);

    if (FAILED(hr)) {
      std::cerr << "[DebugVisualRenderer] Failed to map vertex buffer for frame " << i << '\n';
      return false;
    }

    frame.mapped_ptr = static_cast<DebugVertex*>(mapped_data);
    frame.is_mapped = true;
    frame.vertex_count = 0;
  }

  return true;
}

void DebugVisualRenderer::ReleaseFrameBuffers() {
  for (auto& frame : frames_) {
    if (frame.is_mapped && frame.vertex_buffer.GetResource()) {
      frame.vertex_buffer.GetResource()->Unmap(0, nullptr);
      frame.is_mapped = false;
    }
    frame.mapped_ptr = nullptr;
    frame.vertex_count = 0;
    // Buffer destructor will handle resource cleanup
  }
}

UINT DebugVisualRenderer::ConvertCommandsToVertices(const DebugVisualCommandBuffer& cmds,
  DebugVertex* vertex_buffer,
  UINT max_vertices,
  DebugDepthMode depthMode,
  const DebugVisualSettings& settings) {
  if (!vertex_buffer) {
    return 0;
  }

  UINT vertex_index = 0;

  // Convert 3D lines matching the specified depth mode and category filter
  for (const auto& line : cmds.lines3D) {
    if (line.depthMode != depthMode) {
      continue;  // Skip lines not matching this depth mode
    }

    if (!settings.IsCategoryEnabled(line.category)) {
      continue;  // Skip disabled categories
    }

    // Check for overflow
    if (vertex_index + 2 > max_vertices) {
      std::cerr << "[DebugVisualRenderer] Vertex overflow, truncating." << '\n';
      break;
    }

    // First vertex
    vertex_buffer[vertex_index].position = line.p0;
    vertex_buffer[vertex_index].color = line.color.ToRGBA8();
    vertex_index++;

    // Second vertex
    vertex_buffer[vertex_index].position = line.p1;
    vertex_buffer[vertex_index].color = line.color.ToRGBA8();
    vertex_index++;
  }

  return vertex_index;
}
