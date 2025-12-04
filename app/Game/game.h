#pragma once

#include "../Graphic/graphic.h"

class Game {
 public:
  explicit Game(Graphic& graphic) : graphic_(graphic) {};
  ~Game() = default;

  void OnUpdate([[maybe_unused]] float dt);
  void OnFixedUpdate([[maybe_unused]] float dt);

 private:
  Graphic& graphic_;
};
