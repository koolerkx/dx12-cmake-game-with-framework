#pragma once

#include <memory>
#include <vector>

#include "Scene/scene.h"
#include "game_object.h"
#include "mesh.h"
#include "render_system.h"
#include "texture_manager.h"

class Graphic;
class MaterialInstance;
class MaterialTemplate;

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

#pragma region Demo // Temp: Demo resources
  std::unique_ptr<Buffer> demo_vertex_buffer_;
  std::unique_ptr<Buffer> demo_index_buffer_;
  std::unique_ptr<Mesh> demo_mesh_;

  MaterialTemplate* demo_material_template_ = nullptr;
  std::unique_ptr<MaterialInstance> demo_material_instance_;

  TextureHandle demo_texture_handle_ = INVALID_TEXTURE_HANDLE;
#pragma endregion Demo

  // Initialization helpers
  bool InitializeDemoResources();
  bool CreateDemoGeometry();
  bool CreateDemoMaterial();
  void CreateDemoScene();
};
