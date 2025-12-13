#pragma once

#include <d3d12.h>

#include <cstdint>
#include <memory>
#include <vector>

#include "buffer.h"
#include "debug_visual_service.h"
#include "Framework/types.h"

class Graphic;
struct DebugVertex2D;

// Scene data for 2D UI rendering
struct UISceneData {
  DirectX::XMMATRIX view_projection_matrix;  // Orthographic projection for UI
};

// Renderer for 2D debug visuals in UI/screen space
class DebugVisualRenderer2D {
 public:
  static constexpr UINT MAX_VERTICES_PER_FRAME = 10000;

  DebugVisualRenderer2D() = default;
  ~DebugVisualRenderer2D() = default;

  bool Initialize(Graphic& graphic);
  void Shutdown();

  void BeginFrame(uint32_t frame_index);
  void Render(const DebugVisualCommandBuffer2D& commands,
    Graphic& graphic,
    const UISceneData& scene_data,
    const DebugVisualSettings& settings);

 private:
  struct FrameResource {
    std::unique_ptr<Buffer> vertex_buffer;
  };

  ID3D12Device* device_ = nullptr;
  Graphic* graphic_ = nullptr;  // Store reference for shader access
  uint32_t frame_count_ = 1;
  std::vector<FrameResource> frame_resources_;
  uint32_t current_frame_index_ = 0;
  uint32_t vertex_count_ = 0;

  ComPtr<ID3D12RootSignature> root_signature_;
  ComPtr<ID3D12PipelineState> pso_;

  bool CreateRootSignature();
  bool CreatePipelineState();
  bool CreateFrameResources();
};
