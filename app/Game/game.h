#pragma once

#include "Scene/scene.h"
#include "game_object.h"
#include "render_system.h"

class Graphic;

class Game {
 public:
  Game() = default;
  ~Game() = default;

  void Initialize(Graphic& graphic);
  void OnUpdate(float dt);
  void OnFixedUpdate(float dt);
  void OnRender(float dt);

  Scene& GetScene() {
    return scene_;
  }

 private:
  Scene scene_;
  RenderSystem render_system_;
  Graphic* graphic_ = nullptr;

  GameObject* active_camera_ = nullptr;
};
