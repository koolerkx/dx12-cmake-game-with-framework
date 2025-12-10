#include "game.h"

#include <iostream>

#include "Component/camera_component.h"
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

  GameObject* camera = scene_.CreateGameObject("Camera");

  auto* cam_transform = new TransformComponent();
  cam_transform->SetPosition(0.0f, 0.0f, -1.0f);
  camera->AddComponent(cam_transform);

  CameraComponent* camera_component = new CameraComponent();
  camera_component->SetOrthographicOffCenter(0.0f, 1920.0f, 1080.0f, 0.0f, 0.1f, 1000.0f);
  camera->AddComponent(camera_component);

  active_camera_ = camera;
  std::cout << "[Game] Initialized with " << scene_.GetGameObjectCount() << " game objects" << '\n';
}

void Game::OnUpdate(float dt) {
  scene_.Update(dt);
}

void Game::OnFixedUpdate(float dt) {
  scene_.FixedUpdate(dt);
}

void Game::OnRender(float) {
  render_system_.RenderFrame(scene_, active_camera_);
}
