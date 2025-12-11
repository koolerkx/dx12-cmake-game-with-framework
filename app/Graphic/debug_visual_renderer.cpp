#include "debug_visual_renderer.h"

#include <cassert>
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
  debug_line_material_overlay_ = defaults.GetDebugLineMaterialOverlay();
  debug_line_material_depth_ = defaults.GetDebugLineMaterialDepth();

  if (!debug_line_material_overlay_ || !debug_line_material_depth_) {
    std::cerr << "[DebugVisualRenderer] Failed to get debug line materials (overlay/depth) from DefaultAssets" << '\n';
    return;
  }

  debug_line_template_overlay_ = defaults.GetDebugLineTemplateOverlay();
  debug_line_template_depth_ = defaults.GetDebugLineTemplateDepth();

  // Fallback: derive templates from materials if template getters are unavailable
  if (!debug_line_template_overlay_ && debug_line_material_overlay_) {
    debug_line_template_overlay_ = debug_line_material_overlay_->GetTemplate();
  }
  if (!debug_line_template_depth_ && debug_line_material_depth_) {
    debug_line_template_depth_ = debug_line_material_depth_->GetTemplate();
  }

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

  debug_line_template_overlay_ = nullptr;
  debug_line_material_overlay_ = nullptr;
  debug_line_template_depth_ = nullptr;
  debug_line_material_depth_ = nullptr;
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

void DebugVisualRenderer::RenderDepthTested(const DebugVisualCommandBuffer& cmds,
  ID3D12GraphicsCommandList* cmd_list,
  const SceneGlobalData& sceneData,
  const Buffer& frame_cb,
  const DebugVisualSettings& settings) {
  if (!is_initialized_ || !cmd_list || !debug_line_material_depth_ || !debug_line_template_depth_) {
    return;
  }

  auto& frame = frames_[current_frame_index_];

  // Fill depth-tested vertices first
  frame.vertex_count = FillVertexData(cmds, frame.mapped_ptr, MAX_DEBUG_VERTICES, DebugDepthMode::TestDepth, settings);
  last_frame_vertex_count_ = frame.vertex_count;

  if (frame.vertex_count == 0) {
    return;
  }

  if (sceneData.scene_cb_gpu_address == 0) {
    std::cerr << "[DebugVisualRenderer] Skipping depth-tested debug lines because Scene CB address is invalid" << '\n';
    return;
  }

  const D3D12_GPU_VIRTUAL_ADDRESS cb_address = sceneData.scene_cb_gpu_address ? sceneData.scene_cb_gpu_address : frame_cb.GetGPUAddress();
  if (cb_address == 0) {
    return;
  }
  if (cb_address == 0) {
    return;
  }

  DirectX::XMFLOAT4X4 identity_world;
  DirectX::XMStoreFloat4x4(&identity_world, DirectX::XMMatrixIdentity());

  D3D12_VERTEX_BUFFER_VIEW vbv = frame.vertex_buffer.GetVBV(sizeof(DebugVertex));

  cmd_list->SetPipelineState(debug_line_template_depth_->GetPSO());
  cmd_list->SetGraphicsRootSignature(debug_line_template_depth_->GetRootSignature());
  cmd_list->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_LINELIST);
  cmd_list->IASetVertexBuffers(0, 1, &vbv);

  cmd_list->SetGraphicsRoot32BitConstants(0, 16, &identity_world, 0);  // b0 - world matrix
  cmd_list->SetGraphicsRootConstantBufferView(1, cb_address);          // b1 - FrameCB

  cmd_list->DrawInstanced(frame.vertex_count, 1, 0, 0);
}

void DebugVisualRenderer::RenderOverlay(const DebugVisualCommandBuffer& cmds,
  ID3D12GraphicsCommandList* cmd_list,
  const SceneGlobalData& sceneData,
  const Buffer& frame_cb,
  const DebugVisualSettings& settings) {
  if (!is_initialized_ || !cmd_list || !debug_line_material_overlay_ || !debug_line_template_overlay_) {
    return;
  }

  auto& frame = frames_[current_frame_index_];

  const UINT start_offset = frame.vertex_count;  // Depth-tested vertices already written (if any)
  assert(start_offset <= MAX_DEBUG_VERTICES);
  if (start_offset >= MAX_DEBUG_VERTICES) {
    return;
  }

  const UINT remaining = MAX_DEBUG_VERTICES - start_offset;
  DebugVertex* dst = frame.mapped_ptr + start_offset;

  const UINT overlay_vertex_count = FillVertexData(cmds, dst, remaining, DebugDepthMode::IgnoreDepth, settings);
  assert(start_offset + overlay_vertex_count <= MAX_DEBUG_VERTICES);
  if (overlay_vertex_count == 0) {
    last_frame_vertex_count_ = frame.vertex_count;
    return;
  }

  assert(start_offset + overlay_vertex_count <= MAX_DEBUG_VERTICES);
  frame.vertex_count += overlay_vertex_count;
  last_frame_vertex_count_ = frame.vertex_count;

  const D3D12_GPU_VIRTUAL_ADDRESS cb_address = sceneData.scene_cb_gpu_address ? sceneData.scene_cb_gpu_address : frame_cb.GetGPUAddress();

  DirectX::XMFLOAT4X4 identity_world;
  DirectX::XMStoreFloat4x4(&identity_world, DirectX::XMMatrixIdentity());

  D3D12_VERTEX_BUFFER_VIEW vbv = frame.vertex_buffer.GetVBV(sizeof(DebugVertex));

  cmd_list->SetPipelineState(debug_line_template_overlay_->GetPSO());
  cmd_list->SetGraphicsRootSignature(debug_line_template_overlay_->GetRootSignature());
  cmd_list->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_LINELIST);
  cmd_list->IASetVertexBuffers(0, 1, &vbv);

  cmd_list->SetGraphicsRoot32BitConstants(0, 16, &identity_world, 0);  // b0 - world matrix
  cmd_list->SetGraphicsRootConstantBufferView(1, cb_address);          // b1 - FrameCB

  // Note: firstVertexLocation = start_offset to append after depth-tested vertices
  cmd_list->DrawInstanced(overlay_vertex_count, 1, start_offset, 0);
}

void DebugVisualRenderer::Render(const DebugVisualCommandBuffer& cmds,
  ID3D12GraphicsCommandList* cmd_list,
  const SceneGlobalData& sceneData,
  const Buffer& frame_cb,
  const DebugVisualSettings& settings) {
  RenderDepthTested(cmds, cmd_list, sceneData, frame_cb, settings);
  RenderOverlay(cmds, cmd_list, sceneData, frame_cb, settings);
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

UINT DebugVisualRenderer::FillVertexData(const DebugVisualCommandBuffer& cmds,
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
      assert(false && "DebugVisualRenderer vertex buffer overflow");
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
