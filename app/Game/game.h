#pragma once

#include "Scene/scene.h"
#include "scene_renderer.h"

class Graphic;

class Game {
 public:
  Game() = default;
  ~Game() = default;

  void Initialize(Graphic& graphic);
  void OnUpdate(float dt);
  void OnFixedUpdate(float dt);
  void SubmitRenderPackets(SceneRenderer& scene_renderer);

  Scene& GetScene() {
    return scene_;
  }

 private:
  Scene scene_;
  Graphic* graphic_ = nullptr;
};
