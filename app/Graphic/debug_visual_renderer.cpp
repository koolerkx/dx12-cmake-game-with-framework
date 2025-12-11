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

  // Get debug line material from default assets
  const auto& defaults = gfx.GetDefaultAssets();
  debug_line_material_ = defaults.GetDebugLineMaterial();

  if (!debug_line_material_) {
    std::cerr << "[DebugVisualRenderer] Failed to get debug line material from DefaultAssets" << '\n';
    return;
  }

  debug_line_template_ = debug_line_material_->GetTemplate();

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

void DebugVisualRenderer::Render(
  const DebugVisualCommandBuffer& cmds, ID3D12GraphicsCommandList* cmd_list, const SceneGlobalData& sceneData) {
  if (!is_initialized_ || !cmd_list || !debug_line_material_) {
    return;
  }

  auto& current_frame = frames_[current_frame_index_];

  // Convert commands to vertices
  UINT vertex_count = ConvertCommandsToVertices(cmds, current_frame.mapped_ptr, MAX_DEBUG_VERTICES);

  if (vertex_count == 0) {
    return;  // Nothing to draw
  }

  current_frame.vertex_count = vertex_count;
  last_frame_vertex_count_ = vertex_count;

  // Set up rendering state
  ID3D12PipelineState* pso = debug_line_template_->GetPSO();
  ID3D12RootSignature* root_signature = debug_line_template_->GetRootSignature();

  cmd_list->SetPipelineState(pso);
  cmd_list->SetGraphicsRootSignature(root_signature);

  // Set primitive topology for lines
  cmd_list->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_LINELIST);

  // Set vertex buffer
  D3D12_VERTEX_BUFFER_VIEW vbv = current_frame.vertex_buffer.GetVBV(sizeof(DebugVertex));
  cmd_list->IASetVertexBuffers(0, 1, &vbv);

  // Set scene constants (view/projection matrices)
  // Assuming root parameter 0 is the scene CBV
  struct SceneConstants {
    DirectX::XMMATRIX view_projection;
  } scene_constants;

  scene_constants.view_projection = DirectX::XMMatrixTranspose(sceneData.view_projection_matrix);
  cmd_list->SetGraphicsRoot32BitConstants(0, sizeof(SceneConstants) / 4, &scene_constants, 0);

  // Draw all vertices as lines
  cmd_list->DrawInstanced(vertex_count, 1, 0, 0);
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

UINT DebugVisualRenderer::ConvertCommandsToVertices(const DebugVisualCommandBuffer& cmds, DebugVertex* vertex_buffer, UINT max_vertices) {
  if (!vertex_buffer) {
    return 0;
  }

  UINT vertex_index = 0;
  UINT required_vertices = static_cast<UINT>(cmds.lines3D.size() * 2);  // 2 vertices per line

  // Check for overflow
  if (required_vertices > max_vertices) {
    std::cerr << "[DebugVisualRenderer] Vertex overflow: " << required_vertices << " > " << max_vertices << ", truncating." << '\n';
    required_vertices = max_vertices;
  }

  // Convert 3D lines to vertex pairs
  size_t lines_to_process = required_vertices / 2;
  for (size_t i = 0; i < lines_to_process && i < cmds.lines3D.size(); ++i) {
    const auto& line = cmds.lines3D[i];

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
