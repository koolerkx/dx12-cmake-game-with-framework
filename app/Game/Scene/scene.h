#pragma once

#include <memory>
#include <vector>

#include "game_object.h"
#include "scene_renderer.h"

class Scene {
 public:
  Scene() = default;
  ~Scene() = default;

  Scene(const Scene&) = delete;
  Scene& operator=(const Scene&) = delete;

  GameObject* CreateGameObject(const std::string& name = "GameObject");
  void DestroyGameObject(GameObject* obj);

  void Update(float dt);
  void FixedUpdate(float dt);

  void SubmitRenderPackets(SceneRenderer& scene_renderer);

  size_t GetGameObjectCount() const {
    return game_objects_.size();
  }

  void Clear();

 private:
  std::vector<std::unique_ptr<GameObject>> game_objects_;
};
