#pragma once

#include "Scene/scene.h"
#include "game_object.h"
#include "debug_visual_service.h"
#include "debug_visual_renderer.h"

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
  DebugVisualService& GetDebugVisualService() { return debug_service_; }
  const DebugVisualService& GetDebugVisualService() const { return debug_service_; }

 private:
  Graphic* graphic_ = nullptr;
  DebugVisualService  debug_service_;
  DebugVisualRenderer debug_renderer_;
  
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
};
