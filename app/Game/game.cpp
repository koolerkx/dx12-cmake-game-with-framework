#include "game.h"

#include <iostream>

#include "Component/renderer_component.h"
#include "Component/transform_component.h"
#include "graphic.h"

void Game::Initialize(Graphic& graphic) {
  graphic_ = &graphic;
  render_system_.Initialize(graphic);

  // Create test game object with components
  GameObject* test_object = scene_.CreateGameObject("TestQuad");

  // Add transform component
  auto* transform = new TransformComponent();
  transform->SetPosition(5.0f, 0.0f, 0.0f);
  transform->SetScale(1.0f);
  test_object->AddComponent(transform);

  // Add renderer component
  auto* renderer = new RendererComponent();
  renderer->SetMesh(graphic.GetTestMesh());
  renderer->SetMaterial(graphic.GetTestMaterial());
  test_object->AddComponent(renderer);

  std::cout << "[Game] Initialized with " << scene_.GetGameObjectCount() << " game objects" << '\n';
}

void Game::OnUpdate(float dt) {
  scene_.Update(dt);
}

void Game::OnFixedUpdate(float dt) {
  scene_.FixedUpdate(dt);
}

void Game::OnRender(float) {
  render_system_.RenderFrame(scene_);
}
