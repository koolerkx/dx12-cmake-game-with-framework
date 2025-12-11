#include "graphic.h"

#include <array>
#include <iostream>

#include "RenderPass/forward_pass.h"
#include "RenderPass/ui_pass.h"

bool Graphic::Initialize(HWND hwnd, UINT frame_buffer_width, UINT frame_buffer_height) {
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

  // Initialize primitive geometry 2D
  primitive_geometry_2d_ = std::make_unique<PrimitiveGeometry2D>(device_.Get());

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

  // Initialize upload context for one-shot resource uploads
  if (!upload_context_.Initialize(device_.Get(), command_queue_.Get(), &fence_manager_)) {
    MessageBoxW(nullptr, L"Graphic: Failed to initialize upload context", init_error_caption.c_str(), MB_OK | MB_ICONERROR);
    return false;
  }

  if (!render_pass_manager_.Initialize(device_.Get())) {
    MessageBoxW(nullptr, L"Graphic: Failed to initialize render pass manager", init_error_caption.c_str(), MB_OK | MB_ICONERROR);
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

  if (!InitializeRenderPasses()) {
    MessageBoxW(nullptr, L"Graphic: Failed to initialize render passes", init_error_caption.c_str(), MB_OK | MB_ICONERROR);
    return false;
  }

  // Initialize framework default assets (textures, meshes, debug materials)
  default_assets_ = std::make_unique<FrameworkDefaultAssets>();
  default_assets_->Initialize(*this);

  std::cout << "[Graphic] Initialization complete - Render Pass Architecture" << '\n';
  return true;
}

bool Graphic::InitializeRenderPasses() {
  auto forward_pass = std::make_unique<ForwardPass>();
  forward_pass->Initialize(device_.Get());

  // Use swap chain back buffer as render target
  // The back buffer will be set dynamically in BeginFrame
  forward_pass->SetDepthBuffer(&depth_buffer_);

  render_pass_manager_.RegisterPass("Forward", std::move(forward_pass));

  // Create UI Pass
  auto ui_pass = std::make_unique<UIPass>();
  ui_pass->Initialize(device_.Get());

  render_pass_manager_.RegisterPass("UI", std::move(ui_pass));

  // Cache pass pointers for efficient access
  forward_pass_ = static_cast<ForwardPass*>(render_pass_manager_.GetPass("Forward"));
  ui_pass_ = static_cast<UIPass*>(render_pass_manager_.GetPass("UI"));

  std::cout << "[Graphic] Registered " << render_pass_manager_.GetPassCount() << " render passes" << '\n';
  return true;
}

void Graphic::ExecuteImmediate(const std::function<void(ID3D12GraphicsCommandList*)>& recordFunc) {
  if (!recordFunc) return;
  if (!upload_context_.IsInitialized()) return;

  upload_context_.Begin();
  ID3D12GraphicsCommandList* cmd = upload_context_.GetCommandList();

  // Bind descriptor heaps if code recording requires shader-visible heaps
  descriptor_heap_manager_.SetDescriptorHeaps(cmd);

  // Let caller record commands
  recordFunc(cmd);

  // Close/execute and wait
  upload_context_.SubmitAndWait();
}

void Graphic::BeginFrame() {
  // Reset command allocator and list
  command_allocator_->Reset();
  command_list_->Reset(command_allocator_.Get(), nullptr);

  // Reset descriptor heaps for this frame
  descriptor_heap_manager_.BeginFrame();
  descriptor_heap_manager_.SetDescriptorHeaps(command_list_.Get());

  // Transition swap chain back buffer to render target state
  swap_chain_manager_.TransitionToRenderTarget(command_list_.Get());

  // Get current render targets
  auto* backbufferRT = swap_chain_manager_.GetCurrentRenderTarget();

  // Set viewport and scissor rect
  command_list_->RSSetViewports(1, &viewport_);
  command_list_->RSSetScissorRects(1, &scissor_rect_);

  // Clear render target and depth buffer
  std::array<float, 4> clear_color = {0.2f, 0.3f, 0.4f, 1.0f};
  backbufferRT->Clear(command_list_.Get(), clear_color.data());
  depth_buffer_.Clear(command_list_.Get(), 1.0f, 0);

  // Note: OMSetRenderTargets is now handled by each RenderPass::Begin() to bind their specific RT/DSV

  // Update render passes with current back buffer (wrapped in RenderTarget)
  if (forward_pass_) {
    forward_pass_->SetRenderTarget(backbufferRT);
    forward_pass_->SetDepthBuffer(&depth_buffer_);
  }
  if (ui_pass_) {
    ui_pass_->SetRenderTarget(backbufferRT);
  }
}

void Graphic::RenderFrame() {
  // Execute all render passes through the pass manager
  // The pass manager will handle filtering and executing each pass
  render_pass_manager_.RenderFrame(command_list_.Get(), texture_manager_);

  // Clear render queue for next frame
  render_pass_manager_.Clear();
}

void Graphic::EndFrame() {
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
  render_pass_manager_.PrintStats();

  // Shutdown framework default assets before clearing managers so they can
  // release references into managers during shutdown if needed.
  if (default_assets_) {
    default_assets_->Shutdown();
    default_assets_.reset();
  }

  // Clean up managers
  shader_manager_.Clear();
  texture_manager_.Clear();
  material_manager_.Clear();

  std::cout << "[Graphic] Shutdown complete" << '\n';
}

D3D12_CPU_DESCRIPTOR_HANDLE Graphic::GetMainRTV() const {
  return swap_chain_manager_.GetCurrentRTV();
}

D3D12_CPU_DESCRIPTOR_HANDLE Graphic::GetMainDSV() const {
  return depth_buffer_.GetDSV();
}

D3D12_VIEWPORT Graphic::GetScreenViewport() const {
  return viewport_;
}

D3D12_RECT Graphic::GetScissorRect() const {
  return scissor_rect_;
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
