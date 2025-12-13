#pragma once

#include <DirectXMath.h>
#include <d3d12.h>

#include <memory>
#include <vector>

#include "buffer.h"
#include "debug_visual_service.h"
#include "vertex_types.h"

// Forward declarations
class Graphic;
class MaterialTemplate;
class MaterialInstance;

// Scene data passed to debug renderer for view/projection matrices
struct SceneGlobalData {
  DirectX::XMMATRIX view_matrix;
  DirectX::XMMATRIX projection_matrix;
  DirectX::XMMATRIX view_projection_matrix;
  DirectX::XMFLOAT3 camera_position;
  float padding1;
  DirectX::XMFLOAT3 camera_forward;
  float padding2;
  D3D12_GPU_VIRTUAL_ADDRESS scene_cb_gpu_address = 0;  // GPU address of frame/scene constant buffer
  // Add other scene constants as needed
};

// DX12 backend for rendering debug visuals
class DebugVisualRenderer {
 public:
  DebugVisualRenderer() = default;
  ~DebugVisualRenderer() = default;

  // Non-copyable
  DebugVisualRenderer(const DebugVisualRenderer&) = delete;
  DebugVisualRenderer& operator=(const DebugVisualRenderer&) = delete;

  // Initialize the renderer with reference to graphics system
  void Initialize(Graphic& gfx);
  void Shutdown();

  // Call this at the beginning of each frame
  void BeginFrame(uint32_t frameIndex);

  // Render depth-tested debug lines
  void RenderDepthTested(const DebugVisualCommandBuffer& cmds,
    Graphic& graphic,
    const SceneGlobalData& sceneData,
    const Buffer& frame_cb,
    const DebugVisualSettings& settings);

  // Render overlay (ignore depth) debug lines
  void RenderOverlay(const DebugVisualCommandBuffer& cmds,
    Graphic& graphic,
    const SceneGlobalData& sceneData,
    const Buffer& frame_cb,
    const DebugVisualSettings& settings);

  // Helper to render both depth-tested then overlay (for compatibility)
  void Render(const DebugVisualCommandBuffer& cmds,
    Graphic& graphic,
    const SceneGlobalData& sceneData,
    const Buffer& frame_cb,
    const DebugVisualSettings& settings);

  // Statistics and diagnostics
  uint32_t GetLastFrameVertexCount() const {
    return last_frame_vertex_count_;
  }
  uint32_t GetLastFrameInstanceCount() const {
    return last_frame_instance_count_;
  }
  uint32_t GetLastFrameDrawCallCount() const {
    return last_frame_draw_call_count_;
  }
  bool IsInitialized() const {
    return is_initialized_;
  }

 private:
  static constexpr UINT MAX_DEBUG_INSTANCES = 64 * 1024;  // 64K line instances max per frame (was 64K vertices = 32K lines)

  // Per-frame data
  struct FrameData {
    Buffer instance_buffer;  // Upload heap buffer for instance data
    DebugLineInstanceData* mapped_ptr = nullptr;
    UINT instance_count = 0;
    bool is_mapped = false;

    void Reset() {
      instance_count = 0;
      // Keep buffer mapped persistently
    }
  };

  // Internal state
  bool is_initialized_ = false;
  Graphic* graphic_ = nullptr;
  uint32_t frame_count_ = 1;
  uint32_t current_frame_index_ = 0;
  
  // Frame statistics for diagnostics
  uint32_t last_frame_vertex_count_ = 0;      // Total vertices rendered (instances * 2)
  uint32_t last_frame_instance_count_ = 0;    // Total line instances rendered
  uint32_t last_frame_draw_call_count_ = 0;   // Number of draw calls issued

  // Unit segment vertex buffer (shared across all frames)
  // Contains 2 vertices: (-0.5, 0, 0) and (0.5, 0, 0)
  Buffer unit_segment_vb_;
  D3D12_VERTEX_BUFFER_VIEW unit_segment_vbv_{};

  // Per-frame buffers
  std::vector<std::unique_ptr<FrameData>> frames_;

  // Material system references (non-owning)
  MaterialTemplate* debug_line_template_overlay_ = nullptr;
  MaterialInstance* debug_line_material_overlay_ = nullptr;
  MaterialTemplate* debug_line_template_depth_ = nullptr;
  MaterialInstance* debug_line_material_depth_ = nullptr;

  // Internal helpers
  bool CreateFrameBuffers();
  bool CreateUnitSegmentBuffer();
  void ReleaseFrameBuffers();
  UINT FillInstanceData(const DebugVisualCommandBuffer& cmds,
    DebugLineInstanceData* instance_buffer,
    UINT max_instances,
    DebugDepthMode depthMode,
    const DebugVisualSettings& settings);
};
