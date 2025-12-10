#include "game.h"

#include <DirectXMath.h>
#include <d3d12.h>

#include <iostream>

#include "Component/camera_component.h"
#include "Component/renderer_component.h"
#include "Component/transform_component.h"
#include "RenderPass/render_layer.h"
#include "buffer.h"
#include "graphic.h"
#include "material_instance.h"
#include "material_template.h"
#include "pipeline_state_builder.h"
#include "root_signature_builder.h"

using namespace DirectX;

struct Vertex {
  XMFLOAT3 pos;
  XMFLOAT2 uv;
};

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
  // Create vertex buffer
  demo_vertex_buffer_ = std::make_unique<Buffer>();
  demo_index_buffer_ = std::make_unique<Buffer>();
  demo_mesh_ = std::make_unique<Mesh>();

  // Define vertices for a simple quad
  Vertex vertices[] = {
    {{0, 100, 0.0f}, {0.0f, 1.0f}},    // bottom-left
    {{0, 0, 0.0f}, {0.0f, 0.0f}},      // top-left
    {{100, 100, 0.0f}, {1.0f, 1.0f}},  // bottom-right
    {{100, 0, 0.0f}, {1.0f, 0.0f}},    // top-right
  };

  if (!demo_vertex_buffer_->Create(graphic_->GetDevice(), sizeof(vertices), Buffer::Type::Vertex)) {
    std::cerr << "[Game] Failed to create vertex buffer" << '\n';
    return false;
  }
  demo_vertex_buffer_->Upload(vertices, sizeof(vertices));
  demo_vertex_buffer_->SetDebugName("DemoQuad_VertexBuffer");

  // Define indices
  uint16_t indices[] = {0, 1, 2, 2, 1, 3};

  if (!demo_index_buffer_->Create(graphic_->GetDevice(), sizeof(indices), Buffer::Type::Index)) {
    std::cerr << "[Game] Failed to create index buffer" << '\n';
    return false;
  }
  demo_index_buffer_->Upload(indices, sizeof(indices));
  demo_index_buffer_->SetDebugName("DemoQuad_IndexBuffer");

  // Initialize mesh
  demo_mesh_->Initialize(demo_vertex_buffer_.get(), demo_index_buffer_.get(), sizeof(Vertex), 6, DXGI_FORMAT_R16_UINT);
  demo_mesh_->SetDebugName("DemoQuad");

  std::cout << "[Game] Created demo geometry" << '\n';
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
    .AddRootConstant(16, 0, D3D12_SHADER_VISIBILITY_VERTEX)                                    // b0 - Object constants
    .AddRootCBV(1, D3D12_SHADER_VISIBILITY_ALL)                                                // b1 - Frame CB
    .AddDescriptorTable(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0, D3D12_SHADER_VISIBILITY_PIXEL)  // t0 - Texture
    .AddStaticSampler(0, D3D12_FILTER_MIN_MAG_MIP_LINEAR, D3D12_TEXTURE_ADDRESS_MODE_WRAP, D3D12_SHADER_VISIBILITY_PIXEL)  // s0
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
    .UseForwardRenderingDefaults();

  if (!pso_builder.Build(graphic_->GetDevice(), pso)) {
    std::cerr << "[Game] Failed to build pipeline state" << '\n';
    return false;
  }

  // Load texture using a short-lived upload command list
  TextureLoadParams tex_params;
  tex_params.file_path = L"Content/textures/metal_plate_nor_dx_1k.png";
  tex_params.force_srgb = false;

  graphic_->ExecuteImmediate([this, &texture_manager, tex_params](ID3D12GraphicsCommandList* cmd_list) {
    demo_texture_handle_ = texture_manager.LoadTexture(cmd_list, tex_params);
  });
  if (!demo_texture_handle_.IsValid()) {
    std::cerr << "[Game] Failed to load demo texture" << '\n';
    return false;
  }

  // Create material template
  std::vector<TextureSlotDefinition> texture_slots;
  texture_slots.push_back({"albedo", 2, D3D12_SHADER_VISIBILITY_PIXEL});

  demo_material_template_ = material_manager.CreateTemplate("DemoMaterial", pso.Get(), root_signature.Get(), texture_slots);
  if (demo_material_template_ == nullptr) {
    std::cerr << "[Game] Failed to create material template" << '\n';
    return false;
  }

  // Create material instance
  demo_material_instance_ = std::make_unique<MaterialInstance>();
  if (!demo_material_instance_->Initialize(demo_material_template_)) {
    std::cerr << "[Game] Failed to initialize material instance" << '\n';
    return false;
  }

  demo_material_instance_->SetTexture("albedo", demo_texture_handle_);

  std::cout << "[Game] Created demo material" << '\n';
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

  // Create demo quad object (3D scene - Opaque layer)
  GameObject* demo_quad_3d = scene_.CreateGameObject("DemoQuad3D");

  auto* transform_3d = new TransformComponent();
  transform_3d->SetPosition(50.0f, 50.0f, 0.0f);
  transform_3d->SetScale(1.0f);
  demo_quad_3d->AddComponent(transform_3d);

  auto* renderer_3d = new RendererComponent();
  renderer_3d->SetMesh(demo_mesh_.get());
  renderer_3d->SetMaterial(demo_material_instance_.get());
  renderer_3d->SetLayer(RenderLayer::Opaque);  // Mark as opaque for forward pass
  renderer_3d->SetTag(RenderTag::Static | RenderTag::Lit);
  demo_quad_3d->AddComponent(renderer_3d);

  // Create UI quad object (UI layer)
  GameObject* demo_quad_ui = scene_.CreateGameObject("DemoQuadUI");

  auto* transform_ui = new TransformComponent();
  transform_ui->SetPosition(200.0f, 200.0f, 0.0f);
  transform_ui->SetScale(0.5f);
  demo_quad_ui->AddComponent(transform_ui);

  auto* renderer_ui = new RendererComponent();
  renderer_ui->SetMesh(demo_mesh_.get());
  renderer_ui->SetMaterial(demo_material_instance_.get());
  renderer_ui->SetLayer(RenderLayer::UI);  // Mark as UI for UI pass
  renderer_ui->SetTag(RenderTag::Static | RenderTag::Unlit);
  demo_quad_ui->AddComponent(renderer_ui);

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
