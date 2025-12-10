#pragma once

#include "Scene/scene.h"
#include "game_object.h"

class Graphic;
class RenderPassManager;

class RenderSystem {
 public:
  RenderSystem() = default;
  ~RenderSystem() = default;

  void Initialize(Graphic& graphic) {
    graphic_ = &graphic;
  }

  void RenderFrame(Scene& scene, GameObject* active_camera);

 private:
  Graphic* graphic_ = nullptr;
  void Submit(Scene& scene, RenderPassManager& render_pass_manager);
};
