#pragma once

#include <DirectXMath.h>

#include <memory>
#include <string>
#include <vector>

#include "RenderPass/render_layer.h"
#include "Scene/scene.h"
#include "game_object.h"
#include "mesh.h"
#include "render_system.h"
#include "texture_manager.h"

class Graphic;
class MaterialInstance;
class MaterialTemplate;

struct SpriteParams {
  DirectX::XMFLOAT2 position = {0.0f, 0.0f};
  DirectX::XMFLOAT2 size = {1.0f, 1.0f};

  float sort_order = 0.0f;  // Sorting hint for UI/2D rendering order, higher values render in front (or lower, depending on convention)

  TextureHandle texture = INVALID_TEXTURE_HANDLE;             // Optional: specific texture to use
  MaterialInstance* material = nullptr;                       // Optional: provide a pre-allocated material instance
  RenderLayer layer = RenderLayer::UI;                        // Default layer
  RenderTag tag = RenderTag::Static;                          // Default tag
  DirectX::XMFLOAT4 color = {1.0f, 1.0f, 1.0f, 1.0f};         // Optional tint color
  DirectX::XMFLOAT4 uv_transform = {0.0f, 0.0f, 1.0f, 1.0f};  // offset.xy, scale.xy
};

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
  std::shared_ptr<Mesh> sprite_mesh_;

  MaterialTemplate* sprite_material_template_ = nullptr;
  std::unique_ptr<MaterialInstance> sprite_material_instance_;
  std::unique_ptr<MaterialInstance> sprite_material_instance_2_;

  TextureHandle sprite_texture_handle_ = INVALID_TEXTURE_HANDLE;
  TextureHandle sprite_texture_handle_2_ = INVALID_TEXTURE_HANDLE;
  // Storage for dynamically created per-sprite material instances (owned by Game)
  std::vector<std::unique_ptr<MaterialInstance>> sprite_material_instances_;
#pragma endregion Demo

  // Initialization helpers
  bool InitializeDemoResources();
  bool CreateDemoGeometry();
  bool CreateDemoMaterial();
  void CreateDemoScene();

  // Create a sprite using the shared quad mesh and material.
  // Material selection order: params.material -> texture (new instance) -> default instance.
  GameObject* CreateSprite(const std::string& name, const SpriteParams& params);
};
