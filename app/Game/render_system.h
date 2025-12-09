#pragma once

#include "Scene/scene.h"
#include "graphic.h"

class RenderSystem {
 public:
  RenderSystem() = default;
  ~RenderSystem() = default;

  void Initialize(Graphic& graphic) {
    graphic_ = &graphic;
  }

  void RenderFrame(Scene& scene);

 private:
  Graphic* graphic_ = nullptr;
  void Submit(Scene& scene);
};
