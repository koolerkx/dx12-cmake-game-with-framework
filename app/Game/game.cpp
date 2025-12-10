#include "game.h"

#include <DirectXMath.h>
#include <d3d12.h>

#include <iostream>

#include "Component/camera_component.h"
#include "Component/renderer_component.h"
#include "Component/transform_component.h"
#include "RenderPass/render_layer.h"
#include "graphic.h"
#include "material_instance.h"
#include "material_template.h"
#include "pipeline_state_builder.h"
#include "root_signature_builder.h"

using namespace DirectX;

void Game::Initialize(Graphic& graphic) {
  graphic_ = &graphic;
  render_system_.Initialize(graphic);

  // Initialize demo resources (geometry, materials, textures)
  if (!InitializeDemoResources()) {
    std::cerr << "[Game] Failed to initialize demo resources!" << '\n';
    return;
  }

  // Create demo scene
  CreateDemoScene();

  std::cout << "[Game] Initialized with " << scene_.GetGameObjectCount() << " game objects" << '\n';
}

bool Game::InitializeDemoResources() {
  if (!CreateDemoGeometry()) {
    std::cerr << "[Game] Failed to create demo geometry" << '\n';
    return false;
  }

  if (!CreateDemoMaterial()) {
    std::cerr << "[Game] Failed to create demo material" << '\n';
    return false;
  }

  return true;
}

bool Game::CreateDemoGeometry() {
  sprite_mesh_ = graphic_->GetPrimitiveGeometry2D().CreateRect();

  std::cout << "[Game] Created sprite geometry" << '\n';
  return true;
}

bool Game::CreateDemoMaterial() {
  ShaderManager& shader_manager = graphic_->GetShaderManager();
  MaterialManager& material_manager = graphic_->GetMaterialManager();
  TextureManager& texture_manager = graphic_->GetTextureManager();

  // Load shaders
  if (!shader_manager.LoadShader(L"Content/shaders/basic.vs.cso", ShaderType::Vertex, "BasicVS")) {
    std::cerr << "[Game] Failed to load vertex shader" << '\n';
    return false;
  }

  if (!shader_manager.LoadShader(L"Content/shaders/basic.ps.cso", ShaderType::Pixel, "BasicPS")) {
    std::cerr << "[Game] Failed to load pixel shader" << '\n';
    return false;
  }

  // Create root signature
  ComPtr<ID3D12RootSignature> root_signature;
  RootSignatureBuilder rs_builder;
  rs_builder
    .AddRootConstant(16, 0, D3D12_SHADER_VISIBILITY_VERTEX)  // b0 - Object constants
    .AddRootConstant(4, 2, D3D12_SHADER_VISIBILITY_VERTEX)   // b2 - Per-object color tint
    .AddRootConstant(4, 3, D3D12_SHADER_VISIBILITY_VERTEX)   // b3 - Per-object UV transform (offset.xy, scale.xy)
    .AddRootCBV(1, D3D12_SHADER_VISIBILITY_ALL)              // b1 - Frame CB
    .AddDescriptorTable(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0, D3D12_SHADER_VISIBILITY_PIXEL)                             // t0 - Texture
    .AddStaticSampler(0, D3D12_FILTER_MIN_MAG_MIP_POINT, D3D12_TEXTURE_ADDRESS_MODE_WRAP, D3D12_SHADER_VISIBILITY_PIXEL)  // s0
    .AllowInputLayout();

  if (!rs_builder.Build(graphic_->GetDevice(), root_signature)) {
    std::cerr << "[Game] Failed to build root signature" << '\n';
    return false;
  }

  // Create pipeline state
  ComPtr<ID3D12PipelineState> pso;
  PipelineStateBuilder pso_builder;
  const ShaderBlob* vs = shader_manager.GetShader("BasicVS");
  const ShaderBlob* ps = shader_manager.GetShader("BasicPS");

  pso_builder.SetRootSignature(root_signature.Get())
    .SetVertexShader(vs)
    .SetPixelShader(ps)
    .AddInputElement("POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0)
    .AddInputElement("TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0)
    .UseUIDefaults();

  if (!pso_builder.Build(graphic_->GetDevice(), pso)) {
    std::cerr << "[Game] Failed to build pipeline state" << '\n';
    return false;
  }

  // Load texture using a short-lived upload command list
  TextureLoadParams tex_params;
  tex_params.file_path = L"Content/textures/block_test.png";
  tex_params.force_srgb = false;

  graphic_->ExecuteImmediate([this, &texture_manager, tex_params](ID3D12GraphicsCommandList* cmd_list) {
    sprite_texture_handle_ = texture_manager.LoadTexture(cmd_list, tex_params);
  });
  if (!sprite_texture_handle_.IsValid()) {
    std::cerr << "[Game] Failed to load demo texture" << '\n';
    return false;
  }

  // Create material template
  std::vector<TextureSlotDefinition> texture_slots;
  // Root parameter index 4: after added root constants (b0, b2, b3) and frame CB (b1)
  texture_slots.push_back({"albedo", 4, D3D12_SHADER_VISIBILITY_PIXEL});

  sprite_material_template_ = material_manager.CreateTemplate("SpriteMaterial", pso.Get(), root_signature.Get(), texture_slots);
  if (sprite_material_template_ == nullptr) {
    std::cerr << "[Game] Failed to create material template" << '\n';
    return false;
  }

  // Create material instance
  sprite_material_instance_ = std::make_unique<MaterialInstance>();
  if (!sprite_material_instance_->Initialize(sprite_material_template_)) {
    std::cerr << "[Game] Failed to initialize material instance" << '\n';
    return false;
  }

  sprite_material_instance_->SetTexture("albedo", sprite_texture_handle_);

  // Create a second simple texture (empty) to test texture switching
  sprite_texture_handle_2_ = texture_manager.CreateEmptyTexture(128, 128, DXGI_FORMAT_R8G8B8A8_UNORM);
  if (sprite_texture_handle_2_.IsValid()) {
    sprite_material_instance_2_ = std::make_unique<MaterialInstance>();
    if (sprite_material_instance_2_->Initialize(sprite_material_template_)) {
      sprite_material_instance_2_->SetTexture("albedo", sprite_texture_handle_2_);
    } else {
      sprite_material_instance_2_.reset();
      std::cerr << "[Game] Warning: Failed to initialize second sprite material instance" << '\n';
    }
  } else {
    std::cerr << "[Game] Warning: Failed to create second sprite texture" << '\n';
  }

  std::cout << "[Game] Created sprite material" << '\n';
  return true;
}

void Game::CreateDemoScene() {
  // Create 3D camera
  GameObject* camera_3d = scene_.CreateGameObject("Camera3D");
  auto* cam_transform = new TransformComponent();
  cam_transform->SetPosition(0.0f, 0.0f, -1.0f);
  camera_3d->AddComponent(cam_transform);

  CameraComponent* camera_component = new CameraComponent();
  camera_component->SetOrthographicOffCenter(0.0f, 1920.0f, 1080.0f, 0.0f, 0.1f, 1000.0f);
  camera_3d->AddComponent(camera_component);

  active_camera_ = camera_3d;

  // // Create sprite quad object (Background - Opaque layer) using helper API
  // SpriteParams bg_params;
  // bg_params.position = DirectX::XMFLOAT2(960.0f, 540.0f);
  // bg_params.size = DirectX::XMFLOAT2(1920.0f, 1080.0f);
  // bg_params.texture = sprite_texture_handle_;
  // bg_params.layer = RenderLayer::Opaque;
  // bg_params.tag = RenderTag::Static | RenderTag::Lit;
  // CreateSprite("SpriteQuad3D", bg_params);

  // // Create UI sprite quad object (UI layer, corner) using helper API
  // SpriteParams ui_params;
  // ui_params.position = DirectX::XMFLOAT2(192.0f, 256.0f);
  // ui_params.size = DirectX::XMFLOAT2(128.0f, 128.0f);
  // ui_params.texture = sprite_texture_handle_;
  // ui_params.layer = RenderLayer::UI;
  // ui_params.tag = RenderTag::Static | RenderTag::Unlit;
  // ui_params.color = DirectX::XMFLOAT4(1.0f, 0.5f, 0.5f, 1.0f);
  // CreateSprite("SpriteQuadUI", ui_params);

  // // Test: create several world sprites with different sizes, positions and tags
  // for (int i = -2; i <= 2; ++i) {
  //   SpriteParams world_p;
  //   world_p.position = DirectX::XMFLOAT2(960.0f + i * 200.0f, 540.0f);
  //   world_p.size = DirectX::XMFLOAT2(150.0f + (i + 2) * 40.0f, 150.0f + (i + 2) * 40.0f);
  //   world_p.texture = sprite_texture_handle_;
  //   world_p.layer = RenderLayer::Opaque;
  //   // Alternate Lit/Unlit tags
  //   world_p.tag = (i % 2 == 0) ? RenderTag::Lit : RenderTag::Unlit;
  //   world_p.color = DirectX::XMFLOAT4(1.0f, 1.0f - (0.2f * (i + 2)), 1.0f - (0.1f * (i + 2)), 1.0f);
  //   CreateSprite("WorldSprite_" + std::to_string(i + 3), world_p);
  // }

  // // Test: create multiple UI sprites in a row to verify ordering and batching
  // for (int i = 0; i < 10; ++i) {
  //   SpriteParams p;
  //   p.position = DirectX::XMFLOAT2(50.0f + i * 90.0f, 60.0f);
  //   p.size = DirectX::XMFLOAT2(64.0f, 64.0f);
  //   // Alternate materials for texture switching test
  //   p.material = (i % 3 == 0 && sprite_material_instance_2_) ? sprite_material_instance_2_.get() : sprite_material_instance_.get();
  //   p.material = sprite_material_instance_2_.get();
  //   p.layer = RenderLayer::UI;
  //   p.tag = RenderTag::Static | RenderTag::Unlit;
  //   // Every third sprite uses a UV offset to show atlas-like effect
  //   if (i % 3 == 0) {
  //     p.uv_transform = DirectX::XMFLOAT4(0.0f, 0.0f, 0.5f, 0.5f);  // Top-left quarter
  //   }
  //   // Give a distinct tint for testing
  //   p.color = DirectX::XMFLOAT4(1.0f - 0.08f * i, 0.7f, 0.7f + 0.02f * i, 1.0f);
  //   CreateSprite("UISprite_" + std::to_string(i), p);
  // }

  // Test: overlapping UI sprites with different sort_order to verify rendering order
  // Red sprite with sort_order 0 (drawn first, behind others)
  SpriteParams red_sprite;
  red_sprite.position = DirectX::XMFLOAT2(960.0f, 540.0f);
  red_sprite.size = DirectX::XMFLOAT2(200.0f, 200.0f);
  red_sprite.color = DirectX::XMFLOAT4(1.0f, 0.0f, 0.0f, 0.7f);
  red_sprite.sort_order = 0.0f;
  red_sprite.layer = RenderLayer::UI;
  red_sprite.tag = RenderTag::Static | RenderTag::Unlit;
  CreateSprite("TestRedSprite", red_sprite);

  // Green sprite with sort_order 1 (drawn second, in front of red)
  SpriteParams green_sprite = red_sprite;
  green_sprite.color = DirectX::XMFLOAT4(0.0f, 1.0f, 0.0f, 0.7f);
  green_sprite.sort_order = 1.0f;
  CreateSprite("TestGreenSprite", green_sprite);

  // Blue sprite with sort_order 2 (drawn last, in front of all)
  SpriteParams blue_sprite = red_sprite;
  blue_sprite.color = DirectX::XMFLOAT4(0.0f, 0.0f, 1.0f, 0.7f);
  blue_sprite.sort_order = 2.0f;
  CreateSprite("TestBlueSprite", blue_sprite);

  std::cout << "[Game] Created demo scene with " << scene_.GetGameObjectCount() << " objects" << '\n';
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

GameObject* Game::CreateSprite(const std::string& name, const SpriteParams& params) {
  // Sanity: ensure mesh exists
  if (sprite_mesh_ == nullptr) {
    std::cerr << "[Game] Error: sprite_mesh_ not initialized when creating sprite: " << name << '\n';
    return nullptr;
  }

  // Create GameObject
  GameObject* obj = scene_.CreateGameObject(name);

  // Transform
  auto* transform = new TransformComponent();
  transform->SetPosition(params.position.x, params.position.y, params.sort_order);
  transform->SetScale(params.size.x, params.size.y, 1.0f);
  obj->AddComponent(transform);

  // Renderer
  auto* renderer = new RendererComponent();
  renderer->SetMesh(sprite_mesh_.get());

  // Material selection/creation
  MaterialInstance* material_to_use = nullptr;
  if (params.material != nullptr) {
    material_to_use = params.material;
  } else if (params.texture != INVALID_TEXTURE_HANDLE) {
    // Create a new MaterialInstance from the sprite_material_template_
    if (sprite_material_template_ == nullptr) {
      std::cerr << "[Game] Error: sprite_material_template_ not initialized. Using default." << '\n';
      material_to_use = sprite_material_instance_.get();
    } else {
      auto mat_inst = std::make_unique<MaterialInstance>();
      if (mat_inst->Initialize(sprite_material_template_)) {
        mat_inst->SetTexture("albedo", params.texture);
        material_to_use = mat_inst.get();
        // Keep ownership in Game so the instance stays alive
        sprite_material_instances_.push_back(std::move(mat_inst));
      } else {
        std::cerr << "[Game] Error: Failed to initialize material instance for sprite: " << name << '\n';
        material_to_use = sprite_material_instance_.get();
      }
    }
  } else {
    // Default to the shared sprite material instance
    material_to_use = sprite_material_instance_.get();
  }

  renderer->SetMaterial(material_to_use);

  // Set color tint
  renderer->SetColor(params.color);
  renderer->SetUVTransform(params.uv_transform);

  // Layer & Tag
  renderer->SetLayer(params.layer);
  renderer->SetTag(params.tag);
  renderer->SetSortOrder(params.sort_order);

  obj->AddComponent(renderer);

  return obj;
}
