#pragma once

#include <DirectXMath.h>
#include <d3d12.h>

#include <array>

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

  // Render all debug commands accumulated in the command buffer
  void Render(const DebugVisualCommandBuffer& cmds,
    ID3D12GraphicsCommandList* cmd_list,
    const SceneGlobalData& sceneData,
    const Buffer& frame_cb,
    const DebugVisualSettings& settings);

  // Statistics
  uint32_t GetLastFrameVertexCount() const {
    return last_frame_vertex_count_;
  }
  bool IsInitialized() const {
    return is_initialized_;
  }

 private:
  static constexpr UINT MAX_DEBUG_VERTICES = 64 * 1024;  // 64K vertices max per frame
  static constexpr int FRAME_BUFFER_COUNT = 2;

  // Per-frame data
  struct FrameData {
    Buffer vertex_buffer;  // Upload heap buffer for this frame
    DebugVertex* mapped_ptr = nullptr;
    UINT vertex_count = 0;
    bool is_mapped = false;

    void Reset() {
      vertex_count = 0;
      // Keep buffer mapped persistently
    }
  };

  // Internal state
  bool is_initialized_ = false;
  Graphic* graphic_ = nullptr;
  uint32_t current_frame_index_ = 0;
  uint32_t last_frame_vertex_count_ = 0;

  // Per-frame buffers
  std::array<FrameData, FRAME_BUFFER_COUNT> frames_;

  // Material system references (non-owning)
  MaterialTemplate* debug_line_template_ = nullptr;
  MaterialInstance* debug_line_material_ = nullptr;
  MaterialTemplate* debug_line_depth_template_ = nullptr;
  MaterialInstance* debug_line_depth_material_ = nullptr;

  // Internal helpers
  bool CreateFrameBuffers();
  void ReleaseFrameBuffers();
  UINT ConvertCommandsToVertices(const DebugVisualCommandBuffer& cmds,
    DebugVertex* vertex_buffer,
    UINT max_vertices,
    DebugDepthMode depthMode,
    const DebugVisualSettings& settings);
};
