#include "graphic.h"

#include <DirectXMath.h>
#include <d3d12.h>
#include <d3dcommon.h>
#include <d3dcompiler.h>
#include <dxgiformat.h>

#include <array>
#include <cstdint>
#include <iostream>

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

  // Create depth buffer using new API (without SRV for simple forward rendering)
  if (!depth_buffer_.Create(device_.Get(),
        frame_buffer_width,
        frame_buffer_height,
        descriptor_heap_manager_.GetDsvAllocator(),
        nullptr,  // No SRV needed for basic forward rendering
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

  // Create pipeline state
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

  test_texture_.ReleaseUploadHeap();

  std::cout << "[Graphic] Initialization complete - Simple Forward Rendering" << '\n';
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
  std::array<float, 4> clear_color = {0.2f, 0.3f, 0.4f, 1.0f};  // Nice blue-gray background
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

  std::cout << "[Graphic] Shutdown complete" << '\n';
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

  std::cerr << "[Graphic] Failed to create device." << '\n';
  return false;
}

bool Graphic::CreateCommandQueue() {
  D3D12_COMMAND_QUEUE_DESC command_queue_desc = {};
  command_queue_desc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
  command_queue_desc.NodeMask = 0;
  command_queue_desc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
  command_queue_desc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;

  auto hr = device_->CreateCommandQueue(&command_queue_desc, IID_PPV_ARGS(&command_queue_));
  if (FAILED(hr) || command_queue_ == nullptr) {
    std::cerr << "[Graphic] Failed to create command queue." << '\n';
    return false;
  }
  return true;
}

bool Graphic::CreateCommandList() {
  auto hr = device_->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, command_allocator_.Get(), nullptr, IID_PPV_ARGS(&command_list_));

  if (FAILED(hr) || command_list_ == nullptr) {
    std::cerr << "[Graphic] Failed to create command list." << '\n';
    return false;
  }

  command_list_->Close();
  return true;
}

bool Graphic::CreateCommandAllocator() {
  auto hr = device_->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&command_allocator_));

  if (FAILED(hr) || command_allocator_ == nullptr) {
    std::cerr << "[Graphic] Failed to create command allocator." << '\n';
    return false;
  }

  return true;
}

bool Graphic::InitializeTestGeometry() {
  // Define test quad vertices
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

  return true;
}

bool Graphic::InitializeTestTexture() {
  // Load texture from file using new Texture class
  if (!test_texture_.LoadFromFile(
        device_.Get(), command_list_.Get(), L"Content/textures/metal_plate_nor_dx_1k.png", descriptor_heap_manager_.GetSrvAllocator())) {
    std::cerr << "[Graphic] Failed to load test texture." << '\n';
    return false;
  }

  test_texture_.SetDebugName("TestTexture");
  std::cout << "[Graphic] Loaded test texture: " << test_texture_.GetWidth() << "x" << test_texture_.GetHeight() << '\n';

  return true;
}

bool Graphic::CreatePipelineState() {
  // Read Shader
  ID3DBlob* vs_blob = nullptr;
  ID3DBlob* ps_blob = nullptr;

  if (FAILED(D3DReadFileToBlob(L"Content/shaders/basic.vs.cso", &vs_blob))) {
    std::cerr << "[Graphic] Failed to read vertex shader" << '\n';
    return false;
  }

  if (FAILED(D3DReadFileToBlob(L"Content/shaders/basic.ps.cso", &ps_blob))) {
    std::cerr << "[Graphic] Failed to read pixel shader" << '\n';
    return false;
  }

  // Root signature
  D3D12_DESCRIPTOR_RANGE desc_tbl_range[1] = {};
  desc_tbl_range[0].NumDescriptors = 1;
  desc_tbl_range[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
  desc_tbl_range[0].BaseShaderRegister = 0;
  desc_tbl_range[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

  D3D12_ROOT_PARAMETER root_param[1] = {};
  root_param[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
  root_param[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
  root_param[0].DescriptorTable.pDescriptorRanges = desc_tbl_range;
  root_param[0].DescriptorTable.NumDescriptorRanges = 1;

  D3D12_STATIC_SAMPLER_DESC sampler_desc[1] = {};
  sampler_desc[0].Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
  sampler_desc[0].AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
  sampler_desc[0].AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
  sampler_desc[0].AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
  sampler_desc[0].MipLODBias = 0;
  sampler_desc[0].MaxAnisotropy = 0;
  sampler_desc[0].ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
  sampler_desc[0].BorderColor = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;
  sampler_desc[0].MinLOD = 0.0f;
  sampler_desc[0].MaxLOD = D3D12_FLOAT32_MAX;
  sampler_desc[0].ShaderRegister = 0;
  sampler_desc[0].RegisterSpace = 0;
  sampler_desc[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

  D3D12_ROOT_SIGNATURE_DESC root_signature_desc = {};
  root_signature_desc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
  root_signature_desc.pParameters = root_param;
  root_signature_desc.NumParameters = 1;
  root_signature_desc.pStaticSamplers = sampler_desc;
  root_signature_desc.NumStaticSamplers = 1;

  ID3DBlob* root_sig_blob = nullptr;
  ID3DBlob* error_blob = nullptr;
  HRESULT hr = D3D12SerializeRootSignature(&root_signature_desc, D3D_ROOT_SIGNATURE_VERSION_1_0, &root_sig_blob, &error_blob);

  if (FAILED(hr) || root_sig_blob == nullptr) {
    std::cerr << "[Graphic] Failed to serialize root signature." << '\n';
    if (error_blob) {
      std::cerr << static_cast<char*>(error_blob->GetBufferPointer()) << '\n';
      error_blob->Release();
    }
    return false;
  }

  hr = device_->CreateRootSignature(0, root_sig_blob->GetBufferPointer(), root_sig_blob->GetBufferSize(), IID_PPV_ARGS(&root_signature_));
  root_sig_blob->Release();

  if (FAILED(hr) || root_signature_ == nullptr) {
    std::cerr << "[Graphic] Failed to create root signature." << '\n';
    return false;
  }

  // Input layout
  D3D12_INPUT_ELEMENT_DESC input_layout[] = {
    {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
    {"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0}};

  // Graphics pipeline
  D3D12_GRAPHICS_PIPELINE_STATE_DESC gpipeline = {};
  gpipeline.pRootSignature = root_signature_.Get();
  gpipeline.VS.pShaderBytecode = vs_blob->GetBufferPointer();
  gpipeline.VS.BytecodeLength = vs_blob->GetBufferSize();
  gpipeline.PS.pShaderBytecode = ps_blob->GetBufferPointer();
  gpipeline.PS.BytecodeLength = ps_blob->GetBufferSize();

  gpipeline.SampleMask = D3D12_DEFAULT_SAMPLE_MASK;

  gpipeline.RasterizerState.MultisampleEnable = FALSE;
  gpipeline.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
  gpipeline.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
  gpipeline.RasterizerState.DepthClipEnable = TRUE;
  gpipeline.RasterizerState.FrontCounterClockwise = FALSE;
  gpipeline.RasterizerState.DepthBias = D3D12_DEFAULT_DEPTH_BIAS;
  gpipeline.RasterizerState.DepthBiasClamp = D3D12_DEFAULT_DEPTH_BIAS_CLAMP;
  gpipeline.RasterizerState.SlopeScaledDepthBias = D3D12_DEFAULT_SLOPE_SCALED_DEPTH_BIAS;
  gpipeline.RasterizerState.AntialiasedLineEnable = FALSE;
  gpipeline.RasterizerState.ForcedSampleCount = 0;
  gpipeline.RasterizerState.ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;

  // Alpha Blend
  gpipeline.BlendState.AlphaToCoverageEnable = FALSE;
  gpipeline.BlendState.IndependentBlendEnable = FALSE;

  D3D12_RENDER_TARGET_BLEND_DESC render_target_blend_desc = {};
  render_target_blend_desc.BlendEnable = FALSE;
  render_target_blend_desc.RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
  render_target_blend_desc.LogicOpEnable = FALSE;

  gpipeline.BlendState.RenderTarget[0] = render_target_blend_desc;

  gpipeline.InputLayout.pInputElementDescs = input_layout;
  gpipeline.InputLayout.NumElements = _countof(input_layout);

  // Depth stencil - Enable depth testing for forward rendering
  gpipeline.DepthStencilState.DepthEnable = TRUE;
  gpipeline.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
  gpipeline.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS;
  gpipeline.DepthStencilState.StencilEnable = FALSE;

  gpipeline.DSVFormat = DXGI_FORMAT_D32_FLOAT;

  gpipeline.IBStripCutValue = D3D12_INDEX_BUFFER_STRIP_CUT_VALUE_DISABLED;
  gpipeline.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;

  gpipeline.NumRenderTargets = 1;
  gpipeline.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;

  gpipeline.SampleDesc.Count = 1;
  gpipeline.SampleDesc.Quality = 0;

  hr = device_->CreateGraphicsPipelineState(&gpipeline, IID_PPV_ARGS(&pipeline_state_));

  vs_blob->Release();
  ps_blob->Release();

  if (FAILED(hr) || pipeline_state_ == nullptr) {
    std::cerr << "[Graphic] Failed to create graphics pipeline state." << '\n';
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

  return true;
}

void Graphic::DrawTestQuad() {
  // Set pipeline state and root signature
  command_list_->SetPipelineState(pipeline_state_.Get());
  command_list_->SetGraphicsRootSignature(root_signature_.Get());

  // Set primitive topology
  command_list_->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

  // Bind vertex and index buffers
  D3D12_VERTEX_BUFFER_VIEW vertex_buffer = vertex_buffer_.GetVBV(sizeof(Vertex));
  D3D12_INDEX_BUFFER_VIEW index_buffer = index_buffer_.GetIBV(DXGI_FORMAT_R16_UINT);

  command_list_->IASetVertexBuffers(0, 1, &vertex_buffer);
  command_list_->IASetIndexBuffer(&index_buffer);

  // Bind texture SRV
  auto srv = test_texture_.GetSRV();
  if (srv.IsValid() && srv.IsShaderVisible()) {
    command_list_->SetGraphicsRootDescriptorTable(0, srv.gpu);
  }

  // Draw
  constexpr int index_count = 6;
  command_list_->DrawIndexedInstanced(index_count, 1, 0, 0, 0);
}
