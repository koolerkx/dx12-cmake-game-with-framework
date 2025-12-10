#pragma once

#include "Component/camera_component.h"
#include "Scene/scene.h"
#include "game_object.h"
#include "graphic.h"

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
  void Submit(Scene& scene);
};
