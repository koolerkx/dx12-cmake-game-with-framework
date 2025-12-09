#include "graphic.h"

#include <DirectXMath.h>
#include <d3d12.h>
#include <d3dcommon.h>
#include <dxgiformat.h>

#include <array>
#include <cstdint>
#include <iostream>

#include "pipeline_state_builder.h"
#include "root_signature_builder.h"
#include "scene_renderer.h"
#include "texture.h"
#include "types.h"

using namespace DirectX;

struct Vertex {
  XMFLOAT3 pos;
  XMFLOAT2 uv;
};

bool Graphic::Initalize(HWND hwnd, UINT frame_buffer_width, UINT frame_buffer_height) {
  frame_buffer_width_ = frame_buffer_width;
  frame_buffer_height_ = frame_buffer_height;

  std::wstring init_error_caption = L"Graphic Initialization Error";

  if (!CreateFactory()) {
    MessageBoxW(nullptr, L"Graphic: Failed to create factory", init_error_caption.c_str(), MB_OK | MB_ICONERROR);
    return false;
  }
  if (!CreateDevice()) {
    MessageBoxW(nullptr, L"Graphic: Failed to create device", init_error_caption.c_str(), MB_OK | MB_ICONERROR);
    return false;
  }
  if (!descriptor_heap_manager_.Initalize(device_.Get())) {
    MessageBoxW(nullptr, L"Graphic: Failed to initialize descriptor heap manager", init_error_caption.c_str(), MB_OK | MB_ICONERROR);
    return false;
  }

  if (!texture_manager_.Initialize(device_.Get(), &descriptor_heap_manager_.GetSrvAllocator(), 1024)) {
    MessageBoxW(nullptr, L"Graphic: Failed to initialize texture manager", init_error_caption.c_str(), MB_OK | MB_ICONERROR);
    return false;
  }

  if (!CreateCommandQueue()) {
    MessageBoxW(nullptr, L"Graphic: Failed to create command queue", init_error_caption.c_str(), MB_OK | MB_ICONERROR);
    return false;
  }
  if (!CreateCommandAllocator()) {
    MessageBoxW(nullptr, L"Graphic: Failed to create command allocator", init_error_caption.c_str(), MB_OK | MB_ICONERROR);
    return false;
  }
  if (!CreateCommandList()) {
    MessageBoxW(nullptr, L"Graphic: Failed to create command list", init_error_caption.c_str(), MB_OK | MB_ICONERROR);
    return false;
  }

  if (!swap_chain_manager_.Initialize(device_.Get(),
        dxgi_factory_.Get(),
        command_queue_.Get(),
        hwnd,
        frame_buffer_width,
        frame_buffer_height,
        descriptor_heap_manager_)) {
    return false;
  }

  // Create depth buffer
  if (!depth_buffer_.Create(device_.Get(),
        frame_buffer_width,
        frame_buffer_height,
        descriptor_heap_manager_.GetDsvAllocator(),
        nullptr,
        DXGI_FORMAT_D32_FLOAT)) {
    MessageBoxW(nullptr, L"Graphic: Failed to create depth buffer", init_error_caption.c_str(), MB_OK | MB_ICONERROR);
    return false;
  }
  depth_buffer_.SetDebugName("SceneDepthBuffer");

  if (!fence_manager_.Initialize(device_.Get())) {
    return false;
  }

  // Initialize test geometry
  if (!InitializeTestGeometry()) {
    MessageBoxW(nullptr, L"Graphic: Failed to initialize test geometry", init_error_caption.c_str(), MB_OK | MB_ICONERROR);
    return false;
  }

  // Load shaders using ShaderManager
  if (!LoadShaders()) {
    MessageBoxW(nullptr, L"Graphic: Failed to load shaders", init_error_caption.c_str(), MB_OK | MB_ICONERROR);
    return false;
  }

  // Create root signature using RootSignatureBuilder
  if (!CreateRootSignature()) {
    MessageBoxW(nullptr, L"Graphic: Failed to create root signature", init_error_caption.c_str(), MB_OK | MB_ICONERROR);
    return false;
  }

  // Create pipeline state using PipelineStateBuilder
  if (!CreatePipelineState()) {
    MessageBoxW(nullptr, L"Graphic: Failed to create pipeline state", init_error_caption.c_str(), MB_OK | MB_ICONERROR);
    return false;
  }

  // Load textures
  command_allocator_->Reset();
  command_list_->Reset(command_allocator_.Get(), nullptr);

  if (!InitializeTestTexture()) {
    MessageBoxW(nullptr, L"Graphic: Failed to initialize test texture", init_error_caption.c_str(), MB_OK | MB_ICONERROR);
    return false;
  }

  command_list_->Close();
  std::array<ID3D12CommandList*, 1> cmdlists = {command_list_.Get()};
  command_queue_->ExecuteCommandLists(1, cmdlists.data());
  fence_manager_.WaitForGpu(command_queue_.Get());

  // Create test material
  if (!CreateTestMaterial()) {
    MessageBoxW(nullptr, L"Graphic: Failed to create test material", init_error_caption.c_str(), MB_OK | MB_ICONERROR);
    return false;
  }

  std::cout << "[Graphic] Initialization complete - Forward Rendering with Abstracted Pipeline" << '\n';
  return true;
}

void Graphic::BeginRender() {
  // Reset command allocator and list
  command_allocator_->Reset();
  command_list_->Reset(command_allocator_.Get(), nullptr);

  // Reset descriptor heaps for this frame
  descriptor_heap_manager_.BeginFrame();
  descriptor_heap_manager_.SetDescriptorHeaps(command_list_.Get());

  // Transition swap chain back buffer to render target state
  swap_chain_manager_.TransitionToRenderTarget(command_list_.Get());

  // Get current render targets
  D3D12_CPU_DESCRIPTOR_HANDLE rtv = swap_chain_manager_.GetCurrentRTV();
  D3D12_CPU_DESCRIPTOR_HANDLE dsv = depth_buffer_.GetDSV();

  // Set viewport and scissor rect
  command_list_->RSSetViewports(1, &viewport_);
  command_list_->RSSetScissorRects(1, &scissor_rect_);

  // Clear render target and depth buffer
  std::array<float, 4> clear_color = {0.2f, 0.3f, 0.4f, 1.0f};
  command_list_->ClearRenderTargetView(rtv, clear_color.data(), 0, nullptr);
  depth_buffer_.Clear(command_list_.Get(), 1.0f, 0);

  // Bind render targets
  command_list_->OMSetRenderTargets(1, &rtv, FALSE, &dsv);
}

void Graphic::EndRender() {
  // Transition swap chain back buffer to present state
  swap_chain_manager_.TransitionToPresent(command_list_.Get());

  // Execute command list
  command_list_->Close();

  std::array<ID3D12CommandList*, 1> cmdlists = {command_list_.Get()};
  command_queue_->ExecuteCommandLists(static_cast<UINT>(cmdlists.size()), cmdlists.data());

  // Wait for GPU to finish
  fence_manager_.WaitForGpu(command_queue_.Get());

  // Present
  swap_chain_manager_.Present(1, 0);
}

void Graphic::Shutdown() {
  // Wait for GPU to finish all work
  fence_manager_.WaitForGpu(command_queue_.Get());

  // Print statistics before cleanup
  texture_manager_.PrintStats();
  material_manager_.PrintStats();
  scene_renderer_.PrintStats();

  // Clean up managers
  shader_manager_.Clear();
  texture_manager_.Clear();
  material_manager_.Clear();

  std::cout << "[Graphic] Shutdown complete" << '\n';
}

void Graphic::FlushRenderQueue() {
  // Execute render queue (sort and draw)
  scene_renderer_.Flush(command_list_.Get(), texture_manager_);
}

bool Graphic::EnableDebugLayer() {
  ID3D12Debug* debugLayer = nullptr;

  if (FAILED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugLayer)))) {
    return false;
  }

  debugLayer->EnableDebugLayer();
  debugLayer->Release();

  return true;
}

bool Graphic::CreateFactory() {
#if defined(DEBUG) || defined(_DEBUG)
  EnableDebugLayer();
  UINT dxgi_factory_flag = DXGI_CREATE_FACTORY_DEBUG;
#else
  UINT dxgi_factory_flag = 0;
#endif

  auto hr = CreateDXGIFactory2(dxgi_factory_flag, IID_PPV_ARGS(&dxgi_factory_));
  if (FAILED(hr)) {
    std::cerr << "[Graphic] Failed to create dxgi factory." << '\n';
    return false;
  }
  return true;
}

bool Graphic::CreateDevice() {
  std::vector<ComPtr<IDXGIAdapter>> adapters;
  ComPtr<IDXGIAdapter> tmpAdapter = nullptr;

  for (int i = 0; dxgi_factory_->EnumAdapters(i, &tmpAdapter) != DXGI_ERROR_NOT_FOUND; ++i) {
    adapters.push_back(tmpAdapter);
  }

  for (const auto& adapter : adapters) {
    DXGI_ADAPTER_DESC adapter_desc = {};
    adapter->GetDesc(&adapter_desc);
    if (std::wstring desc_str = adapter_desc.Description; desc_str.find(L"NVIDIA") != std::string::npos) {
      tmpAdapter = adapter;
      break;
    }
  }

  std::array<D3D_FEATURE_LEVEL, 5> levels = {
    D3D_FEATURE_LEVEL_12_2,
    D3D_FEATURE_LEVEL_12_1,
    D3D_FEATURE_LEVEL_12_0,
    D3D_FEATURE_LEVEL_11_1,
    D3D_FEATURE_LEVEL_11_0,
  };

  for (auto level : levels) {
    auto hr = D3D12CreateDevice(tmpAdapter.Get(), level, IID_PPV_ARGS(&device_));
    if (SUCCEEDED(hr) && device_ != nullptr) {
      return true;
    }
  }

  std::cerr << "[Graphic] Failed to create D3D12 device." << '\n';
  return false;
}

bool Graphic::CreateCommandQueue() {
  D3D12_COMMAND_QUEUE_DESC queue_desc = {};
  queue_desc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
  queue_desc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
  queue_desc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
  queue_desc.NodeMask = 0;

  HRESULT hr = device_->CreateCommandQueue(&queue_desc, IID_PPV_ARGS(&command_queue_));

  if (FAILED(hr) || command_queue_ == nullptr) {
    std::cerr << "[Graphic] Failed to create command queue." << '\n';
    return false;
  }

  return true;
}

bool Graphic::CreateCommandAllocator() {
  HRESULT hr = device_->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&command_allocator_));

  if (FAILED(hr) || command_allocator_ == nullptr) {
    std::cerr << "[Graphic] Failed to create command allocator." << '\n';
    return false;
  }
  return true;
}

bool Graphic::CreateCommandList() {
  HRESULT hr =
    device_->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, command_allocator_.Get(), nullptr, IID_PPV_ARGS(&command_list_));

  if (FAILED(hr) || command_list_ == nullptr) {
    std::cerr << "[Graphic] Failed to create command list." << '\n';
    return false;
  }

  command_list_->Close();
  return true;
}

bool Graphic::InitializeTestGeometry() {
  // Define vertices
  Vertex vertices[] = {
    {{-0.4f, -0.7f, 0.0f}, {0.0f, 1.0f}},  // bottom-left
    {{-0.4f, 0.7f, 0.0f}, {0.0f, 0.0f}},   // top-left
    {{0.4f, -0.7f, 0.0f}, {1.0f, 1.0f}},   // bottom-right
    {{0.4f, 0.7f, 0.0f}, {1.0f, 0.0f}},    // top-right
  };

  // Create vertex buffer
  if (!vertex_buffer_.Create(device_.Get(), sizeof(vertices), Buffer::Type::Vertex)) {
    std::cerr << "[Graphic] Failed to create vertex buffer." << '\n';
    return false;
  }
  vertex_buffer_.Upload(vertices, sizeof(vertices));
  vertex_buffer_.SetDebugName("TestQuad_VertexBuffer");

  // Define indices
  uint16_t indices[] = {0, 1, 2, 2, 1, 3};

  // Create index buffer
  if (!index_buffer_.Create(device_.Get(), sizeof(indices), Buffer::Type::Index)) {
    std::cerr << "[Graphic] Failed to create index buffer." << '\n';
    return false;
  }
  index_buffer_.Upload(indices, sizeof(indices));
  index_buffer_.SetDebugName("TestQuad_IndexBuffer");

  // Initialize mesh
  test_mesh_.Initialize(&vertex_buffer_, &index_buffer_, sizeof(Vertex), 6, DXGI_FORMAT_R16_UINT);
  test_mesh_.SetDebugName("TestQuad");

  return true;
}

bool Graphic::InitializeTestTexture() {
  // Define texture loading parameters
  TextureLoadParams params;
  params.file_path = L"Content/textures/metal_plate_nor_dx_1k.png";
  params.generate_mips = false;
  params.force_srgb = false;

  // Load texture through TextureManager
  test_texture_handle_ = texture_manager_.LoadTexture(command_list_.Get(), params);

  if (!test_texture_handle_.IsValid()) {
    std::cerr << "[Graphic] Failed to load test texture." << '\n';
    return false;
  }

  // Get texture for info logging
  const Texture* texture = texture_manager_.GetTexture(test_texture_handle_);
  if (texture) {
    std::cout << "[Graphic] Loaded test texture: " << texture->GetWidth() << "x" << texture->GetHeight() << '\n';
  }

  return true;
}

bool Graphic::LoadShaders() {
  // Load vertex shader
  if (!shader_manager_.LoadShader(L"Content/shaders/basic.vs.cso", ShaderType::Vertex, "BasicVS")) {
    std::cerr << "[Graphic] Failed to load vertex shader" << '\n';
    return false;
  }

  // Load pixel shader
  if (!shader_manager_.LoadShader(L"Content/shaders/basic.ps.cso", ShaderType::Pixel, "BasicPS")) {
    std::cerr << "[Graphic] Failed to load pixel shader" << '\n';
    return false;
  }

  std::cout << "[Graphic] Shaders loaded: " << shader_manager_.GetShaderCount() << " shaders" << '\n';
  return true;
}

bool Graphic::CreateRootSignature() {
  // Use RootSignatureBuilder to create root signature
  RootSignatureBuilder builder;

  builder
    .AddDescriptorTable(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0, D3D12_SHADER_VISIBILITY_PIXEL)  // t0 - texture
    .AddStaticSampler(0, D3D12_FILTER_MIN_MAG_MIP_LINEAR, D3D12_TEXTURE_ADDRESS_MODE_WRAP, D3D12_SHADER_VISIBILITY_PIXEL)
    .AllowInputLayout();

  if (!builder.Build(device_.Get(), root_signature_)) {
    std::cerr << "[Graphic] Failed to build root signature" << '\n';
    return false;
  }

  std::cout << "[Graphic] Root signature created using RootSignatureBuilder" << '\n';
  return true;
}

bool Graphic::CreatePipelineState() {
  // Get shaders from shader manager
  const ShaderBlob* vs = shader_manager_.GetShader("BasicVS");
  const ShaderBlob* ps = shader_manager_.GetShader("BasicPS");

  if (!vs || !ps) {
    std::cerr << "[Graphic] Shaders not loaded" << '\n';
    return false;
  }

  // Use PipelineStateBuilder to create PSO
  PipelineStateBuilder builder;

  builder.SetRootSignature(root_signature_.Get())
    .SetVertexShader(vs)
    .SetPixelShader(ps)
    .AddInputElement("POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0)
    .AddInputElement("TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0)
    .UseForwardRenderingDefaults();  // Apply forward rendering defaults

  if (!builder.Build(device_.Get(), pipeline_state_)) {
    std::cerr << "[Graphic] Failed to build pipeline state" << '\n';
    return false;
  }

  // Setup viewport
  viewport_.Width = static_cast<FLOAT>(frame_buffer_width_);
  viewport_.Height = static_cast<FLOAT>(frame_buffer_height_);
  viewport_.TopLeftX = 0;
  viewport_.TopLeftY = 0;
  viewport_.MaxDepth = 1.0f;
  viewport_.MinDepth = 0.0f;

  scissor_rect_.top = 0;
  scissor_rect_.left = 0;
  scissor_rect_.right = frame_buffer_width_;
  scissor_rect_.bottom = frame_buffer_height_;

  std::cout << "[Graphic] Pipeline state created using PipelineStateBuilder" << '\n';
  return true;
}

bool Graphic::CreateTestMaterial() {
  // Define texture slots for this material
  std::vector<TextureSlotDefinition> texture_slots;
  texture_slots.push_back({"albedo", 0, D3D12_SHADER_VISIBILITY_PIXEL});  // Root parameter 0

  // Create material template
  test_material_template_ = material_manager_.CreateTemplate("BasicMaterial", pipeline_state_.Get(), root_signature_.Get(), texture_slots);

  if (test_material_template_ == nullptr) {
    std::cerr << "[Graphic] Failed to create material template" << '\n';
    return false;
  }

  // Create material instance
  test_material_instance_ = std::make_unique<MaterialInstance>();
  if (!test_material_instance_->Initialize(test_material_template_)) {
    std::cerr << "[Graphic] Failed to initialize material instance" << '\n';
    return false;
  }

  // Assign texture to material
  test_material_instance_->SetTexture("albedo", test_texture_handle_);

  test_material_template_->PrintInfo();
  test_material_instance_->PrintInfo();

  return true;
}

void Graphic::DrawTestQuad() {
  // Clear previous frame packets
  scene_renderer_.Clear();
  scene_renderer_.ResetStats();

  // Submit render packet
  RenderPacket packet;
  packet.mesh = &test_mesh_;
  packet.material = test_material_instance_.get();
  packet.transform = XMMatrixIdentity();

  scene_renderer_.Submit(packet);

  // Flush render queue (sorts and executes)
  scene_renderer_.Flush(command_list_.Get(), texture_manager_);
}
