#pragma once

#include <vector>

#include "RenderPass/scene_renderer.h"
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

  void BuildRenderQueues(Scene& scene, std::vector<RenderPacket>& world_packets, std::vector<RenderPacket>& ui_packets);
  void RenderDebugVisuals(SceneRenderer& scene_renderer);
  void RenderDebugVisuals2D(uint32_t frame_index);

  void RenderWorldPass(Scene& scene,
    GameObject* active_camera,
    RenderPassManager& rpm,
    SceneRenderer& scene_renderer,
    const std::vector<RenderPacket>& world_packets);

  void RenderUIPass(RenderPassManager& rpm, SceneRenderer& scene_renderer, const std::vector<RenderPacket>& ui_packets);

  void SetupWorldSceneData(GameObject* active_camera, SceneRenderer& scene_renderer, SceneData& out_scene_data);
};
