#include "debug_visual_renderer.h"

#include <cassert>

#include "framework_default_assets.h"
#include "graphic.h"
#include "material_instance.h"
#include "material_template.h"

#include "Framework/Logging/logger.h"

void DebugVisualRenderer::Initialize(Graphic& graphic) {
  if (is_initialized_) {
    return;  // Already initialized
  }

  graphic_ = &graphic;

  // Get debug line materials from default assets
  const auto& defaults = graphic.GetDefaultAssets();
  debug_line_material_overlay_ = defaults.GetDebugLineMaterialOverlay();
  debug_line_material_depth_ = defaults.GetDebugLineMaterialDepth();

  if (!debug_line_material_overlay_ || !debug_line_material_depth_) {
    Logger::Log(LogLevel::Error,
      LogCategory::Graphic,
      "[DebugVisualRenderer] Failed to get debug line materials (overlay/depth) from DefaultAssets");
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
    Logger::Log(LogLevel::Error, LogCategory::Graphic, "[DebugVisualRenderer] Failed to create frame buffers");
    return;
  }

  // Create unit segment vertex buffer (shared across all frames)
  if (!CreateUnitSegmentBuffer()) {
    Logger::Log(LogLevel::Error, LogCategory::Graphic, "[DebugVisualRenderer] Failed to create unit segment buffer");
    return;
  }

  is_initialized_ = true;
  Logger::Log(LogLevel::Info, LogCategory::Graphic, "[DebugVisualRenderer] Initialized successfully");
}

void DebugVisualRenderer::Shutdown() {
  if (!is_initialized_) {
    return;
  }

  ReleaseFrameBuffers();

  // Unit segment buffer will be released by its destructor

  debug_line_template_overlay_ = nullptr;
  debug_line_material_overlay_ = nullptr;
  debug_line_template_depth_ = nullptr;
  debug_line_material_depth_ = nullptr;
  graphic_ = nullptr;
  is_initialized_ = false;

  Logger::Log(LogLevel::Info, LogCategory::Graphic, "[DebugVisualRenderer] Shutdown complete");
}

void DebugVisualRenderer::BeginFrame(uint32_t frameIndex) {
  if (!is_initialized_) {
    return;
  }

  current_frame_index_ = (frame_count_ == 0) ? 0u : (frameIndex % frame_count_);
  if (current_frame_index_ < frames_.size()) {
    if (frames_[current_frame_index_]) {
      frames_[current_frame_index_]->Reset();
    }
  }
}

void DebugVisualRenderer::RenderDepthTested(const DebugVisualCommandBuffer& cmds,
  Graphic& graphic,
  const SceneGlobalData& sceneData,
  const Buffer& frame_cb,
  const DebugVisualSettings& settings) {
  ID3D12GraphicsCommandList* cmd_list = graphic.GetCommandList();
  if (!is_initialized_ || !cmd_list || !debug_line_material_depth_ || !debug_line_template_depth_) {
    return;
  }

  // Bind main render targets and viewport/scissor for debug drawing.
  {
    auto rtv = graphic.GetMainRTV();
    auto dsv = graphic.GetMainDSV();
    cmd_list->OMSetRenderTargets(1, &rtv, FALSE, &dsv);

    auto viewport = graphic.GetScreenViewport();
    auto scissor = graphic.GetScissorRect();
    cmd_list->RSSetViewports(1, &viewport);
    cmd_list->RSSetScissorRects(1, &scissor);
  }

  auto& frame = *frames_[current_frame_index_];

  // Fill depth-tested instances
  frame.instance_count = FillInstanceData(cmds, frame.mapped_ptr, MAX_DEBUG_INSTANCES, DebugDepthMode::TestDepth, settings);
  last_frame_instance_count_ = frame.instance_count;
  last_frame_vertex_count_ = frame.instance_count * 2;  // For stats: each instance = 2 vertices
  last_frame_draw_call_count_ = 0;  // Reset at start of depth pass

  if (frame.instance_count == 0) {
    return;
  }

  if (sceneData.scene_cb_gpu_address == 0) {
    Logger::Log(LogLevel::Warn,
      LogCategory::Validation,
      "[DebugVisualRenderer] Skipping depth-tested debug lines because Scene CB address is invalid");
    return;
  }

  const D3D12_GPU_VIRTUAL_ADDRESS cb_address = sceneData.scene_cb_gpu_address ? sceneData.scene_cb_gpu_address : frame_cb.GetGPUAddress();
  if (cb_address == 0) {
    return;
  }

  // Setup vertex buffers: slot 0 = unit segment, slot 1 = instance data
  D3D12_VERTEX_BUFFER_VIEW vbvs[2] = {
    unit_segment_vbv_,
    frame.instance_buffer.GetVBV(sizeof(DebugLineInstanceData))
  };

  cmd_list->SetPipelineState(debug_line_template_depth_->GetPSO());
  cmd_list->SetGraphicsRootSignature(debug_line_template_depth_->GetRootSignature());
  cmd_list->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_LINELIST);
  cmd_list->IASetVertexBuffers(0, 2, vbvs);

  cmd_list->SetGraphicsRootConstantBufferView(0, cb_address);  // b1 - FrameCB (root param 0 now)

  // Draw instanced: 2 vertices per instance, instance_count instances
  cmd_list->DrawInstanced(2, frame.instance_count, 0, 0);
  last_frame_draw_call_count_++;
}

void DebugVisualRenderer::RenderOverlay(const DebugVisualCommandBuffer& cmds,
  Graphic& graphic,
  const SceneGlobalData& sceneData,
  const Buffer& frame_cb,
  const DebugVisualSettings& settings) {
  ID3D12GraphicsCommandList* cmd_list = graphic.GetCommandList();
  if (!is_initialized_ || !cmd_list || !debug_line_material_overlay_ || !debug_line_template_overlay_) {
    return;
  }

  // Bind main render targets and viewport/scissor for debug drawing.
  {
    auto rtv = graphic.GetMainRTV();
    auto dsv = graphic.GetMainDSV();
    cmd_list->OMSetRenderTargets(1, &rtv, FALSE, &dsv);

    auto viewport = graphic.GetScreenViewport();
    auto scissor = graphic.GetScissorRect();
    cmd_list->RSSetViewports(1, &viewport);
    cmd_list->RSSetScissorRects(1, &scissor);
  }

  auto& frame = *frames_[current_frame_index_];

  const UINT start_offset = frame.instance_count;  // Depth-tested instances already written (if any)
  assert(start_offset <= MAX_DEBUG_INSTANCES);
  if (start_offset >= MAX_DEBUG_INSTANCES) {
    return;
  }

  const UINT remaining = MAX_DEBUG_INSTANCES - start_offset;
  DebugLineInstanceData* dst = frame.mapped_ptr + start_offset;

  const UINT overlay_instance_count = FillInstanceData(cmds, dst, remaining, DebugDepthMode::IgnoreDepth, settings);
  assert(start_offset + overlay_instance_count <= MAX_DEBUG_INSTANCES);
  if (overlay_instance_count == 0) {
    // Update stats even if no overlay instances
    return;
  }

  assert(start_offset + overlay_instance_count <= MAX_DEBUG_INSTANCES);
  frame.instance_count += overlay_instance_count;
  last_frame_instance_count_ = frame.instance_count;
  last_frame_vertex_count_ = frame.instance_count * 2;

  const D3D12_GPU_VIRTUAL_ADDRESS cb_address = sceneData.scene_cb_gpu_address ? sceneData.scene_cb_gpu_address : frame_cb.GetGPUAddress();

  // Setup vertex buffers: slot 0 = unit segment, slot 1 = instance data
  D3D12_VERTEX_BUFFER_VIEW vbvs[2] = {
    unit_segment_vbv_,
    frame.instance_buffer.GetVBV(sizeof(DebugLineInstanceData))
  };

  cmd_list->SetPipelineState(debug_line_template_overlay_->GetPSO());
  cmd_list->SetGraphicsRootSignature(debug_line_template_overlay_->GetRootSignature());
  cmd_list->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_LINELIST);
  cmd_list->IASetVertexBuffers(0, 2, vbvs);

  cmd_list->SetGraphicsRootConstantBufferView(0, cb_address);  // b1 - FrameCB (root param 0 now)

  // Draw instanced: 2 vertices per instance, start from start_offset instance
  // Note: startInstanceLocation = start_offset to append after depth-tested instances
  cmd_list->DrawInstanced(2, overlay_instance_count, 0, start_offset);
  last_frame_draw_call_count_++;
}

void DebugVisualRenderer::Render(const DebugVisualCommandBuffer& cmds,
  Graphic& graphic,
  const SceneGlobalData& sceneData,
  const Buffer& frame_cb,
  const DebugVisualSettings& settings) {
  RenderDepthTested(cmds, graphic, sceneData, frame_cb, settings);
  RenderOverlay(cmds, graphic, sceneData, frame_cb, settings);
}

bool DebugVisualRenderer::CreateFrameBuffers() {
  if (!graphic_) {
    return false;
  }

  frame_count_ = Graphic::FrameCount;
  frames_.clear();
  frames_.resize(frame_count_);

  // Create upload buffers for each frame (now storing instance data instead of vertices)
  for (uint32_t i = 0; i < frame_count_; ++i) {
    frames_[i] = std::make_unique<FrameData>();
    auto& frame = *frames_[i];

    // Create upload buffer for instance data
    size_t buffer_size = MAX_DEBUG_INSTANCES * sizeof(DebugLineInstanceData);
    if (!frame.instance_buffer.Create(graphic_->GetDevice(), buffer_size, Buffer::Type::Vertex, D3D12_HEAP_TYPE_UPLOAD)) {
      Logger::Logf(LogLevel::Error,
        LogCategory::Resource,
        Logger::Here(),
        "[DebugVisualRenderer] Failed to create instance buffer for frame {}",
        i);
      return false;
    }

    // Map the buffer persistently
    void* mapped_data = nullptr;
    D3D12_RANGE read_range = {};  // We won't read from upload buffer
    HRESULT hr = frame.instance_buffer.GetResource()->Map(0, &read_range, &mapped_data);

    if (FAILED(hr)) {
      Logger::Logf(LogLevel::Error,
        LogCategory::Resource,
        Logger::Here(),
        "[DebugVisualRenderer] Failed to map instance buffer for frame {}. hr=0x{:08X}",
        i,
        static_cast<uint32_t>(hr));
      return false;
    }

    frame.mapped_ptr = static_cast<DebugLineInstanceData*>(mapped_data);
    frame.is_mapped = true;
    frame.instance_count = 0;
  }

  return true;
}

void DebugVisualRenderer::ReleaseFrameBuffers() {
  for (auto& frame_ptr : frames_) {
    if (!frame_ptr) continue;
    auto& frame = *frame_ptr;
    if (frame.is_mapped && frame.instance_buffer.GetResource()) {
      frame.instance_buffer.GetResource()->Unmap(0, nullptr);
      frame.is_mapped = false;
    }
    frame.mapped_ptr = nullptr;
    frame.instance_count = 0;
    // Buffer destructor will handle resource cleanup
  }
}

// Helper function: Build world matrix for transforming unit segment [-0.5, 0.5] to line segment [p0, p1]
// The unit segment is defined in local space as vertices at (-0.5, 0, 0) and (0.5, 0, 0)
// Returns a row-major float4x4 matrix = Scale * Rotation * Translation
static DirectX::XMFLOAT4X4 BuildLineWorldMatrix(
  const DirectX::XMFLOAT3& p0,
  const DirectX::XMFLOAT3& p1) {
  using namespace DirectX;

  // Compute line properties
  XMVECTOR v0 = XMLoadFloat3(&p0);
  XMVECTOR v1 = XMLoadFloat3(&p1);
  XMVECTOR dir = XMVectorSubtract(v1, v0);
  
  float len = XMVectorGetX(XMVector3Length(dir));
  
  // Handle degenerate case (zero-length or near-zero-length line)
  // Use epsilon to avoid division by zero and NaN propagation
  constexpr float epsilon = 1e-6f;
  if (len < epsilon) {
    // For degenerate lines, return identity matrix at midpoint
    // This prevents NaN/Inf from being uploaded to GPU
    XMVECTOR mid = XMVectorScale(XMVectorAdd(v0, v1), 0.5f);
    XMMATRIX world = XMMatrixTranslationFromVector(mid);
    XMFLOAT4X4 result;
    XMStoreFloat4x4(&result, world);
    return result;
  }

  XMVECTOR xAxis = XMVector3Normalize(dir);
  XMVECTOR mid = XMVectorScale(XMVectorAdd(v0, v1), 0.5f);

  // Build orthonormal basis with X axis aligned to line direction
  // Choose reference up vector to avoid singularity when xAxis is parallel to world up
  XMVECTOR worldUp = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
  float dotUp = fabsf(XMVectorGetX(XMVector3Dot(xAxis, worldUp)));
  if (dotUp > 0.99f) {
    worldUp = XMVectorSet(1.0f, 0.0f, 0.0f, 0.0f);  // Use world X if line is vertical
  }

  XMVECTOR zAxis = XMVector3Normalize(XMVector3Cross(worldUp, xAxis));
  XMVECTOR yAxis = XMVector3Cross(xAxis, zAxis);

  // Construct TRS matrix: world = S * R * T (row-major convention)
  // Scale: X-axis scaled by line length, Y/Z remain 1.0
  XMMATRIX S = XMMatrixScaling(len, 1.0f, 1.0f);
  
  // Rotation: orient local X-axis to line direction
  XMMATRIX R;
  R.r[0] = XMVectorSetW(xAxis, 0.0f);
  R.r[1] = XMVectorSetW(yAxis, 0.0f);
  R.r[2] = XMVectorSetW(zAxis, 0.0f);
  R.r[3] = XMVectorSet(0.0f, 0.0f, 0.0f, 1.0f);

  // Translation: position at line midpoint
  XMMATRIX T = XMMatrixTranslationFromVector(mid);

  // Compose: world = S * R * T
  XMMATRIX world = XMMatrixMultiply(XMMatrixMultiply(S, R), T);

  XMFLOAT4X4 result;
  XMStoreFloat4x4(&result, world);
  return result;
}

bool DebugVisualRenderer::CreateUnitSegmentBuffer() {
  if (!graphic_) {
    return false;
  }

  // Create unit segment VB: 2 vertices at (-0.5, 0, 0) and (0.5, 0, 0)
  DebugUnitSegmentVertex unit_segment[2] = {
    { { -0.5f, 0.0f, 0.0f } },
    { {  0.5f, 0.0f, 0.0f } }
  };

  size_t buffer_size = sizeof(unit_segment);
  
  // Create upload heap buffer for unit segment (can also use default heap with upload copy for optimal performance)
  if (!unit_segment_vb_.Create(graphic_->GetDevice(), buffer_size, Buffer::Type::Vertex, D3D12_HEAP_TYPE_UPLOAD)) {
    Logger::Log(LogLevel::Error,
      LogCategory::Resource,
      "[DebugVisualRenderer] Failed to create unit segment vertex buffer");
    return false;
  }

  // Upload data
  void* mapped_data = nullptr;
  D3D12_RANGE read_range = {};
  HRESULT hr = unit_segment_vb_.GetResource()->Map(0, &read_range, &mapped_data);
  if (FAILED(hr)) {
    Logger::Logf(LogLevel::Error,
      LogCategory::Resource,
      Logger::Here(),
      "[DebugVisualRenderer] Failed to map unit segment buffer. hr=0x{:08X}",
      static_cast<uint32_t>(hr));
    return false;
  }

  memcpy(mapped_data, unit_segment, buffer_size);
  unit_segment_vb_.GetResource()->Unmap(0, nullptr);

  // Setup vertex buffer view
  unit_segment_vbv_ = unit_segment_vb_.GetVBV(sizeof(DebugUnitSegmentVertex));

  return true;
}

UINT DebugVisualRenderer::FillInstanceData(const DebugVisualCommandBuffer& cmds,
  DebugLineInstanceData* instance_buffer,
  UINT max_instances,
  DebugDepthMode depthMode,
  const DebugVisualSettings& settings) {
  if (!instance_buffer) {
    return 0;
  }

  UINT instance_index = 0;

  // Track if we've already warned about overflow in this session
  static bool overflow_warning_shown = false;

  // Convert 3D lines matching the specified depth mode and category filter
  for (const auto& line : cmds.lines3D) {
    if (line.depthMode != depthMode) {
      continue;  // Skip lines not matching this depth mode
    }

    if (!settings.IsCategoryEnabled(line.category)) {
      continue;  // Skip disabled categories
    }

    // Check for overflow - soft fail with one-time warning
    if (instance_index >= max_instances) {
      if (!overflow_warning_shown) {
        Logger::Log(LogLevel::Warn, LogCategory::Validation, 
          "[DebugVisualRenderer] Instance buffer overflow, truncating. This warning will only show once.");
        overflow_warning_shown = true;
      }
      break;  // Stop filling, but don't crash
    }

    // Generate world matrix that transforms unit segment to this line segment
    instance_buffer[instance_index].world = BuildLineWorldMatrix(line.p0, line.p1);
    instance_buffer[instance_index].color = line.color.ToRGBA8();
    // Clear padding to ensure deterministic GPU capture/replay
    instance_buffer[instance_index]._pad[0] = 0;
    instance_buffer[instance_index]._pad[1] = 0;
    instance_buffer[instance_index]._pad[2] = 0;

    instance_index++;
  }

  return instance_index;
}
