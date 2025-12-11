#include "game.h"

#include <DirectXMath.h>

#include <iostream>

#include "Component/camera_component.h"
#include "Component/renderer_component.h"
#include "Component/transform_component.h"
#include "RenderPass/render_layer.h"
#include "graphic.h"

using namespace DirectX;

void Game::Initialize(Graphic& graphic) {
  graphic_ = &graphic;
  render_system_.Initialize(graphic);

  // Verify DefaultAssets are ready before proceeding
  if (!VerifyDefaultAssets()) {
    std::cerr << "[Game] DefaultAssets verification failed!" << '\n';
    return;
  }

  // Create demo scene using new system
  CreateNewDemoScene();

  std::cout << "[Game] Initialized with " << scene_.GetGameObjectCount() << " game objects" << '\n';
}

bool Game::VerifyDefaultAssets() {
  assert(graphic_);

  // Get DefaultAssets reference
  const auto& defaults = graphic_->GetDefaultAssets();

  // Check default textures
  if (!defaults.GetWhiteTexture().IsValid()) {
    std::cerr << "[Game] DefaultAssets: White texture is invalid!" << '\n';
    return false;
  }

  if (!defaults.GetBlackTexture().IsValid()) {
    std::cerr << "[Game] DefaultAssets: Black texture is invalid!" << '\n';
    return false;
  }

  if (!defaults.GetFlatNormalTexture().IsValid()) {
    std::cerr << "[Game] DefaultAssets: Flat normal texture is invalid!" << '\n';
    return false;
  }

  if (!defaults.GetErrorTexture().IsValid()) {
    std::cerr << "[Game] DefaultAssets: Error texture is invalid!" << '\n';
    return false;
  }

  // Check Rect2D mesh
  if (!defaults.GetRect2DMesh()) {
    std::cerr << "[Game] DefaultAssets: Rect2D mesh is null!" << '\n';
    return false;
  }

  // Check default material instances
  if (!defaults.GetSprite2DDefaultMaterial()) {
    std::cerr << "[Game] DefaultAssets: Sprite2D default material is null!" << '\n';
    return false;
  }

  if (!defaults.GetDebugLineMaterial()) {
    std::cerr << "[Game] DefaultAssets: Debug line material is null!" << '\n';
    return false;
  }

  std::cout << "[Game] DefaultAssets verification passed." << '\n';

  return true;
}

void Game::OnUpdate(float dt) {
  // Begin debug frame - clear previous frame's commands
  render_system_.GetDebugVisualService().BeginFrame();

  scene_.Update(dt);

  // Step 11: Demo debug visual API usage
  auto& debug = render_system_.GetDebugVisualService();

  // 3D Debug visuals
  // Draw axis gizmo at origin
  debug.DrawAxisGizmo({0.0f, 0.0f, 0.0f}, 1.0f);

  // Draw a simple wire box around the sprite
  debug.DrawWireBox({-1.5f, -1.5f, -0.5f}, {1.5f, 1.5f, 0.5f}, DebugColor::Yellow(), DebugDepthMode::TestDepth);

  // Draw some reference lines
  debug.DrawLine3D({-3.0f, 0.0f, 0.0f}, {3.0f, 0.0f, 0.0f}, DebugColor::Cyan());
  debug.DrawLine3D({0.0f, -3.0f, 0.0f}, {0.0f, 3.0f, 0.0f}, DebugColor::Magenta());

  // 2D Debug visuals (UI overlay)
  // Draw a green line at the top of the screen
  debug.DrawLine2D({10.0f, 10.0f}, {310.0f, 10.0f}, DebugColor::Green());

  // Draw a red rectangle
  debug.DrawRect2D({10.0f, 20.0f}, {300.0f, 100.0f}, DebugColor::Red());
}

void Game::OnFixedUpdate(float dt) {
  scene_.FixedUpdate(dt);
}

void Game::OnRender(float) {
  render_system_.RenderFrame(scene_, active_camera_);
}

GameObject* Game::CreateSprite(const SpriteCreateParams& params) {
  // Ensure DefaultAssets are available
  assert(graphic_);
  const auto& defaults = graphic_->GetDefaultAssets();

  // Create GameObject
  GameObject* obj = scene_.CreateGameObject();

  // Transform
  auto* transform = new TransformComponent();
  transform->SetPosition(params.position.x, params.position.y, params.position.z);
  transform->SetScale(params.size.x, params.size.y, params.size.z);
  obj->AddComponent(transform);

  // Renderer
  auto* renderer = new RendererComponent();
  renderer->SetMesh(defaults.GetRect2DMesh().get());

  // Material selection: use provided material or default Sprite2D material
  MaterialInstance* material_to_use = params.material;
  if (!material_to_use) {
    material_to_use = defaults.GetSprite2DDefaultMaterial();
  }

  if (material_to_use) {
    renderer->SetMaterial(material_to_use);
  } else {
    std::cerr << "[Game] Warning: No material available for sprite creation" << '\n';
  }

  // Set rendering properties
  renderer->SetColor(params.color);
  renderer->SetUVTransform(params.uv_transform);
  renderer->SetLayer(params.layer);
  renderer->SetTag(params.tag);
  renderer->SetSortOrder(params.sort_order);

  obj->AddComponent(renderer);

  return obj;
}

void Game::Shutdown() {
  // Shutdown render system first
  render_system_.Shutdown();

  // Clear scene
  scene_.Clear();

  // Reset references
  active_camera_ = nullptr;
  graphic_ = nullptr;

  std::cout << "[Game] Shutdown complete" << '\n';
}

void Game::CreateNewDemoScene() {
  std::cout << "[Game] Creating new demo scene with DefaultAssets and Debug API" << '\n';

  // Create camera using existing method temporarily
  CreateCamera();

  // Create sprite #1: Red tint, normal UV
  SpriteCreateParams sprite1_params;
  sprite1_params.position = {-2.0f, 0.0f, 0.0f};
  sprite1_params.size = {1.5f, 1.5f, 1.0f};
  sprite1_params.layer = RenderLayer::Opaque;
  sprite1_params.color = {1.0f, 0.2f, 0.2f, 1.0f};  // Red tint
  sprite1_params.uv_transform = {0.0f, 0.0f, 1.0f, 1.0f};  // Normal UV

  GameObject* sprite1 = CreateSprite(sprite1_params);
  if (sprite1) {
    sprite1->SetName("RedSprite");
    std::cout << "[Game] Created red tinted sprite" << '\n';
  }

  // Create sprite #2: Green tint, scaled UV
  SpriteCreateParams sprite2_params;
  sprite2_params.position = {0.0f, 0.0f, 0.0f};
  sprite2_params.size = {1.5f, 1.5f, 1.0f};
  sprite2_params.layer = RenderLayer::Opaque;
  sprite2_params.color = {0.2f, 1.0f, 0.2f, 1.0f};  // Green tint
  sprite2_params.uv_transform = {0.0f, 0.0f, 0.5f, 0.5f};  // UV scaled to 0.5 (shows only part of texture)

  GameObject* sprite2 = CreateSprite(sprite2_params);
  if (sprite2) {
    sprite2->SetName("GreenSprite_ScaledUV");
    std::cout << "[Game] Created green tinted sprite with scaled UV" << '\n';
  }

  // Create sprite #3: Blue tint, offset UV
  SpriteCreateParams sprite3_params;
  sprite3_params.position = {2.0f, 0.0f, 0.0f};
  sprite3_params.size = {1.5f, 1.5f, 1.0f};
  sprite3_params.layer = RenderLayer::Opaque;
  sprite3_params.color = {0.2f, 0.2f, 1.0f, 1.0f};  // Blue tint
  sprite3_params.uv_transform = {0.25f, 0.25f, 1.0f, 1.0f};  // UV offset by 0.25

  GameObject* sprite3 = CreateSprite(sprite3_params);
  if (sprite3) {
    sprite3->SetName("BlueSprite_OffsetUV");
    std::cout << "[Game] Created blue tinted sprite with offset UV" << '\n';
  }

  demo_sprite_ = sprite1;  // Keep reference to first sprite
}

void Game::CreateCamera() {
  // Create 3D camera
  GameObject* camera_3d = scene_.CreateGameObject("Camera3D");
  TransformComponent* cam_transform = new TransformComponent();
  cam_transform->SetPosition(0.0f, 0.0f, -5.0f);  // Move back a bit to see the sprite
  camera_3d->AddComponent(cam_transform);

  CameraComponent* camera_component = new CameraComponent();
  camera_component->SetPerspective(45.0f, 16.0f / 9.0f, 0.1f, 1000.0f);  // Use perspective camera for 3D
  camera_3d->AddComponent(camera_component);

  active_camera_ = camera_3d;
  std::cout << "[Game] Created 3D perspective camera" << '\n';
}
