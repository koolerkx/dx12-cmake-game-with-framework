#pragma once

#include "Scene/scene.h"
#include "debug_visual_renderer.h"
#include "debug_visual_renderer_2d.h"
#include "debug_visual_service.h"
#include "game_object.h"


class Graphic;
class RenderPassManager;
class SceneRenderer;

class RenderSystem {
 public:
  RenderSystem() = default;
  ~RenderSystem() = default;

  void Initialize(Graphic& graphic);
  void Shutdown();

  void RenderFrame(Scene& scene, GameObject* active_camera);

  // Access to debug visual system
  DebugVisualService& GetDebugVisualService() {
    return debug_service_;
  }
  const DebugVisualService& GetDebugVisualService() const {
    return debug_service_;
  }

  // Access to debug visual settings
  DebugVisualSettings& GetDebugSettings() {
    return debug_settings_;
  }
  const DebugVisualSettings& GetDebugSettings() const {
    return debug_settings_;
  }

 private:
  Graphic* graphic_ = nullptr;
  DebugVisualService debug_service_;
  DebugVisualSettings debug_settings_;  // Debug visual filtering/toggle settings
  DebugVisualRenderer debug_renderer_;
  DebugVisualRenderer2D debug_renderer_2d_;

  // Cached camera data for debug rendering
  struct CachedCameraData {
    DirectX::XMMATRIX view_matrix;
    DirectX::XMMATRIX projection_matrix;
    DirectX::XMMATRIX view_projection_matrix;
    DirectX::XMFLOAT3 camera_position;
    bool is_valid = false;
  } cached_camera_data_;

  void Submit(Scene& scene, RenderPassManager& render_pass_manager);
  void RenderDebugVisuals(SceneRenderer& scene_renderer);
  void RenderDebugVisuals2D(uint32_t frame_index);
};
