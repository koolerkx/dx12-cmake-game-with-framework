#include "game.h"

#include <DirectXMath.h>

#include <iostream>

#include "Component/camera_component.h"
#include "Component/renderer_component.h"
#include "Component/transform_component.h"
#include "RenderPass/render_layer.h"
#include "debug_visual_service.h"
#include "graphic.h"

using namespace DirectX;

namespace {
constexpr const char* kBlockTestWorldMaterialInstance = "BlockTest_World";
constexpr const char* kBlockTestUIMaterialInstance = "BlockTest_UI";
}  // namespace

void Game::Initialize(Graphic& graphic) {
  graphic_ = &graphic;

#if defined(_DEBUG) || defined(DEBUG)
  graphic_->SetVSync(false);
#endif

  render_system_.Initialize(graphic);

  // Verify DefaultAssets are ready before proceeding
  if (!VerifyDefaultAssets()) {
    std::cerr << "[Game] DefaultAssets verification failed!" << '\n';
    return;
  }

  // Load block_test.png and create a dedicated material instance for world + UI.
  graphic.ExecuteImmediate([&](ID3D12GraphicsCommandList* cmd) {
    TextureLoadParams params;
    params.file_path = L"Content/textures/block_test.png";
    params.force_srgb = false;
    block_test_texture_ = graphic.GetTextureManager().LoadTexture(cmd, params);
  });

  if (!block_test_texture_.IsValid()) {
    std::cerr << "[Game] Failed to load Content/textures/block_test.png; block test sprites will use default textures" << '\n';
  } else {
    const auto& defaults = graphic_->GetDefaultAssets();
    auto& material_mgr = graphic_->GetMaterialManager();

    if (defaults.GetSpriteWorldOpaqueMaterial() && defaults.GetSpriteWorldOpaqueMaterial()->GetTemplate()) {
      block_test_world_material_ =
        material_mgr.CreateInstance(kBlockTestWorldMaterialInstance, defaults.GetSpriteWorldOpaqueMaterial()->GetTemplate());
      if (block_test_world_material_) {
        block_test_world_material_->SetTexture("BaseColor", block_test_texture_);
      }
    }

    if (defaults.GetSpriteUIMaterial() && defaults.GetSpriteUIMaterial()->GetTemplate()) {
      block_test_ui_material_ = material_mgr.CreateInstance(kBlockTestUIMaterialInstance, defaults.GetSpriteUIMaterial()->GetTemplate());
      if (block_test_ui_material_) {
        block_test_ui_material_->SetTexture("BaseColor", block_test_texture_);
      }
    }
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

  if (!defaults.GetDebugLineMaterialOverlay()) {
    std::cerr << "[Game] DefaultAssets: Debug line overlay material is null!" << '\n';
    return false;
  }

  if (!defaults.GetDebugLineMaterialDepth()) {
    std::cerr << "[Game] DefaultAssets: Debug line depth material is null!" << '\n';
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
  // Draw axis gizmo at origin - try both modes to see the difference
  debug.DrawAxisGizmo({0.0f, 0.0f, 0.0f}, 100.0f, DebugDepthMode::TestDepth);  // Change to TestDepth to see depth testing

  // Draw a simple wire box around the sprite (AABB version)
  debug.DrawWireBox({-1.5f, -1.5f, -0.5f}, {1.5f, 1.5f, 0.5f}, DebugColor::Yellow(), DebugDepthMode::TestDepth);

  // Draw some reference lines
  debug.DrawLine3D({-3.0f, 0.0f, 0.0f}, {3.0f, 0.0f, 0.0f}, DebugColor::Cyan());
  debug.DrawLine3D({0.0f, -3.0f, 0.0f}, {0.0f, 3.0f, 0.0f}, DebugColor::Magenta());

  // ============================================================================
  // Phase 2 Wire Primitives Demo (Task 2.1-2.3)
  // ============================================================================

  // Identity quaternion for no rotation
  DirectX::XMFLOAT4 identity_quat(0.0f, 0.0f, 0.0f, 1.0f);

  // Oriented wire box (rotated 45 degrees around Y axis for visual verification)
  DirectX::XMVECTOR rotAxis = DirectX::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
  DirectX::XMVECTOR quat = DirectX::XMQuaternionRotationAxis(rotAxis, DirectX::XM_PIDIV4);  // 45 degrees
  DirectX::XMFLOAT4 rotation_quat;
  DirectX::XMStoreFloat4(&rotation_quat, quat);
  debug.DrawWireBox({0.0f, 0.0f, 0.0f}, rotation_quat, {2.0f, 2.0f, 2.0f}, DebugColor::Green(), DebugDepthMode::TestDepth);

  // Wire sphere (demonstrate different segment quality)
  debug.DrawWireSphere({3.0f, 0.0f, 0.0f}, 0.8f, DebugSegments::S16, DebugColor::Blue(), DebugDepthMode::TestDepth);
  debug.DrawWireSphere({5.0f, 0.0f, 0.0f}, 0.8f, DebugSegments::S32, DebugColor::Cyan(), DebugDepthMode::TestDepth);

  // Wire cylinder (Y-axis aligned)
  debug.DrawWireCylinder(
    {0.0f, 3.0f, 0.0f}, identity_quat, 0.5f, 2.0f, DebugAxis::Y, DebugSegments::S24, DebugColor::Red(), DebugDepthMode::TestDepth);

  // Wire cylinder (Z-axis aligned, rotated)
  debug.DrawWireCylinder(
    {3.0f, 3.0f, 0.0f}, rotation_quat, 0.5f, 2.5f, DebugAxis::Z, DebugSegments::S24, DebugColor::Magenta(), DebugDepthMode::TestDepth);

  // Wire capsule (normal height > 2*radius)
  debug.DrawWireCapsule(
    {0.0f, -3.0f, 0.0f}, identity_quat, 0.5f, 2.5f, DebugAxis::Y, DebugSegments::S24, DebugColor::Yellow(), DebugDepthMode::TestDepth);

  // Wire capsule (degenerate case: height <= 2*radius, should fall back to sphere)
  debug.DrawWireCapsule(
    {3.0f, -3.0f, 0.0f}, identity_quat, 0.6f, 1.0f, DebugAxis::Y, DebugSegments::S24, DebugColor::White(), DebugDepthMode::TestDepth);

  // 2D Debug visuals (UI overlay)
  // Draw a green line at the top of the screen
  debug.DrawLine2D({10.0f, 10.0f}, {1000.0f, 10.0f}, DebugColor::Green());

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
    // Route to default material based on layer: UI gets UI material (no depth), world gets depth-tested sprite material.
    material_to_use = HasLayer(params.layer, RenderLayer::UI) ? defaults.GetSpriteUIMaterial() : defaults.GetSpriteWorldOpaqueMaterial();
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

  // Block-test sprite in world (Forward pass layer)
  if (block_test_world_material_) {
    SpriteCreateParams block_world;
    block_world.position = {0.0f, 0.0f, 1.0f};
    block_world.size = {1.5f, 1.5f, 1.0f};
    block_world.layer = RenderLayer::Opaque;
    block_world.material = block_test_world_material_;
    block_world.color = {1.0f, 1.0f, 1.0f, 1.0f};
    block_world.uv_transform = {0.0f, 0.0f, 1.0f, 1.0f};
    GameObject* sprite = CreateSprite(block_world);
    if (sprite) {
      sprite->SetName("BlockTest_WorldSprite");
      std::cout << "[Game] Created block_test world sprite" << '\n';
    }
  }

  // Block-test sprite in UI (UIPass)
  if (block_test_ui_material_) {
    SpriteCreateParams block_ui;
    block_ui.position = {50.0f, 50.0f, 0.0f};  // screen-space pixels (orthographic UI pass)
    block_ui.size = {256.0f, 256.0f, 1.0f};
    block_ui.layer = RenderLayer::UI;
    block_ui.material = block_test_ui_material_;
    block_ui.sort_order = 1000.0f;
    block_ui.color = {1.0f, 1.0f, 1.0f, 1.0f};
    block_ui.uv_transform = {0.0f, 0.0f, 1.0f, 1.0f};
    GameObject* sprite = CreateSprite(block_ui);
    if (sprite) {
      sprite->SetName("BlockTest_UISprite");
      std::cout << "[Game] Created block_test UI sprite" << '\n';
    }
  }

  // Create sprite #1: Red tint, normal UV
  SpriteCreateParams sprite1_params;
  sprite1_params.position = {-4.0f, 0.0f, 0.0f};  // z 改成 1.0f
  sprite1_params.size = {1.5f, 1.5f, 1.0f};
  sprite1_params.layer = RenderLayer::Opaque;
  sprite1_params.color = {1.0f, 0.2f, 0.2f, 1.0f};         // Red tint
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
  sprite2_params.color = {0.2f, 1.0f, 0.2f, 1.0f};         // Green tint
  sprite2_params.uv_transform = {0.0f, 0.0f, 0.5f, 0.5f};  // UV scaled to 0.5 (shows only part of texture)

  GameObject* sprite2 = CreateSprite(sprite2_params);
  if (sprite2) {
    sprite2->SetName("GreenSprite_ScaledUV");
    std::cout << "[Game] Created green tinted sprite with scaled UV" << '\n';
  }

  // Create sprite #3: Blue tint, offset UV
  SpriteCreateParams sprite3_params;
  sprite3_params.position = {4.0f, 0.0f, 0.0f};
  sprite3_params.size = {1.5f, 1.5f, 1.0f};
  sprite3_params.layer = RenderLayer::Opaque;
  sprite3_params.color = {0.2f, 0.2f, 1.0f, 1.0f};           // Blue tint
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
  // Position camera so that sprites around the origin are in front of it
  cam_transform->SetPosition(-3.0f, 3.0f, -5.0f);
  float rotx = 30.0f;
  float roty = 30.0f;
  cam_transform->SetRotation(DirectX::XMConvertToRadians(rotx), DirectX::XMConvertToRadians(roty), 0.0f);

  camera_3d->AddComponent(cam_transform);

  CameraComponent* camera_component = new CameraComponent();
  camera_component->SetPerspective(45.0f, 16.0f / 9.0f, 0.1f, 1000.0f);  // Use perspective camera for 3D
  camera_3d->AddComponent(camera_component);

  active_camera_ = camera_3d;
  std::cout << "[Game] Created 3D perspective camera" << '\n';
}
