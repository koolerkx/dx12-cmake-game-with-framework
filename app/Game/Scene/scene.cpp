#include "scene.h"

GameObject* Scene::CreateGameObject(const std::string& name) {
  auto game_object = std::make_unique<GameObject>(name);
  GameObject* ptr = game_object.get();
  game_objects_.push_back(std::move(game_object));
  return ptr;
}

void Scene::DestroyGameObject(GameObject* obj) {
  auto it = std::find_if(game_objects_.begin(), game_objects_.end(), [obj](const auto& ptr) { return ptr.get() == obj; });

  if (it != game_objects_.end()) {
    game_objects_.erase(it);
  }
}

void Scene::Update(float dt) {
  for (auto& obj : game_objects_) {
    obj->Update(dt);
  }
}

void Scene::FixedUpdate(float dt) {
  for (auto& obj : game_objects_) {
    obj->FixedUpdate(dt);
  }
}

void Scene::Clear() {
  game_objects_.clear();
}
