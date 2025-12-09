#include "render_system.h"

#include <cassert>

#include "Component/renderer_component.h"
#include "graphic.h"
#include "scene_renderer.h"

void RenderSystem::RenderFrame(Scene& scene) {
  assert(graphic_ != nullptr);

  graphic_->BeginRender();

  SceneRenderer& scene_renderer = graphic_->GetSceneRenderer();
  scene_renderer.Clear();
  scene_renderer.ResetStats();

  Submit(scene);
  graphic_->FlushRenderQueue();

  graphic_->EndRender();
}

void RenderSystem::Submit(Scene& scene) {
  const auto& game_objects = scene.GetGameObjects();

  for (const auto& game_object : game_objects) {
    if (!game_object->IsActive()) {
      continue;
    }

    auto* renderer = game_object->GetComponent<RendererComponent>();
    if (!renderer) {
      continue;
    }

    renderer->OnRender(graphic_->GetSceneRenderer());
  }
}
