#pragma once

#include <DirectXMath.h>

#include "RenderPass/render_layer.h"
#include "Scene/scene.h"
#include "game_object.h"
#include "render_system.h"
#include "texture_manager.h"

class Graphic;
class MaterialInstance;
class MaterialTemplate;

// Updated sprite creation parameters structure
struct SpriteCreateParams {
  DirectX::XMFLOAT3 position = {0.0f, 0.0f, 0.0f};
  DirectX::XMFLOAT3 size = {1.0f, 1.0f, 1.0f};

  float sort_order = 0.0f;  // Sorting hint for UI/2D rendering order

  MaterialInstance* material = nullptr;                       // Optional: provide a pre-allocated material instance
  RenderLayer layer = RenderLayer::UI;                        // Default layer
  RenderTag tag = RenderTag::Static;                          // Default tag
  DirectX::XMFLOAT4 color = {1.0f, 1.0f, 1.0f, 1.0f};         // Optional tint color
  DirectX::XMFLOAT4 uv_transform = {0.0f, 0.0f, 1.0f, 1.0f};  // offset.xy, scale.xy
};

// Primitive mesh creation parameters structure
struct PrimitiveCreateParams {
  DirectX::XMFLOAT3 position = {0.0f, 0.0f, 0.0f};
  DirectX::XMFLOAT4 rotation_quat = {0.0f, 0.0f, 0.0f, 1.0f};  // Identity quaternion
  DirectX::XMFLOAT3 scale = {1.0f, 1.0f, 1.0f};
  
  MaterialInstance* material = nullptr;  // Optional: if null, uses SpriteWorldOpaqueMaterial as fallback
  RenderLayer layer = RenderLayer::Opaque;
  RenderTag tag = RenderTag::Dynamic;
  float sort_order = 0.0f;
  std::string name;
};

// Primitive type enumeration for unified creation API
enum class PrimitiveType {
  Cube,
  Cylinder
  // Sphere, Capsule can be added later
};

class Game {
 public:
  Game() = default;
  ~Game() = default;

  void Initialize(Graphic& graphic);
  void Shutdown();  // Step 11: Add explicit shutdown method
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
  GameObject* demo_sprite_ = nullptr;  // Current demo sprite for reference

  // Block test assets (human verification for SRV heap correctness)
  TextureHandle block_test_texture_ = INVALID_TEXTURE_HANDLE;
  MaterialInstance* block_test_world_material_ = nullptr;
  MaterialInstance* block_test_ui_material_ = nullptr;

  // Initialization helpers
  bool VerifyDefaultAssets();  // DefaultAssets sanity check
  void CreateNewDemoScene();   // New demo scene using DefaultAssets + Debug API
  void CreateCamera();         // Helper to create a basic camera

  // Create sprite using DefaultAssets
  GameObject* CreateSprite(const SpriteCreateParams& params);
  
  // Create primitive meshes using DefaultAssets
  GameObject* CreateCube(const PrimitiveCreateParams& params);
  GameObject* CreateCylinder(const PrimitiveCreateParams& params);
  GameObject* CreatePrimitive(PrimitiveType type, const PrimitiveCreateParams& params);
  
  // Material creation helper for primitives
  // Creates a MaterialInstance with the specified BaseColor texture
  // Returns nullptr if creation fails
  MaterialInstance* CreateMaterialInstanceForPrimitive(TextureHandle base_color_texture, const std::string& instance_name);
};
