#include "graphic.h"

#include <array>
#include <format>
#include <vector>

#include "Framework/Error/error_helpers_fast.h"
#include "Framework/Error/framework_error.h"
#include "RenderPass/forward_pass.h"
#include "RenderPass/ui_pass.h"

namespace {
FastErrorCounters g_graphic_fast_errors{};
}

void Graphic::Transition(GpuResource* resource, D3D12_RESOURCE_STATES new_state) {
  if (!resource) return;
  resource->TransitionTo(command_list_.Get(), new_state);
}

void Graphic::Clear(RenderTarget* rt, const float* clear_color) {
  if (!rt || !clear_color) return;
  rt->Clear(command_list_.Get(), clear_color);
}

void Graphic::Clear(DepthBuffer* depth, float depth_val, uint8_t stencil_val) {
  if (!depth) return;
  depth->Clear(command_list_.Get(), depth_val, stencil_val);
}

void Graphic::RenderPasses() {
  render_pass_manager_.RenderFrame(command_list_.Get(), texture_manager_);
}

void Graphic::Initialize(HWND hwnd, UINT frame_buffer_width, UINT frame_buffer_height) {
  frame_buffer_width_ = frame_buffer_width;
  frame_buffer_height_ = frame_buffer_height;

  FrameworkDx::ThrowIfFailed(CreateFactory(), FrameworkErrorCode::DxgiFactoryCreateFailed, "CreateFactory");
  FrameworkDx::ThrowIfFailed(CreateDevice(), FrameworkErrorCode::D3d12DeviceCreateFailed, "CreateDevice");

  // Initialize primitive geometry 2D
  primitive_geometry_2d_ = std::make_unique<PrimitiveGeometry2D>(device_.Get());

  if (!descriptor_heap_manager_.Initalize(device_.Get(), FrameCount)) {
    FrameworkFail::Throw(
      FrameworkErrorDomain::Graphic, FrameworkErrorCode::DescriptorHeapManagerInitFailed, "DescriptorHeapManager::Initialize");
  }

  // Textures are persistent; allocate their SRVs from the static region.
  if (!texture_manager_.Initialize(device_.Get(), &descriptor_heap_manager_.GetSrvStaticAllocator(), 1024)) {
    FrameworkFail::Throw(FrameworkErrorDomain::Resource, FrameworkErrorCode::TextureManagerInitFailed, "TextureManager::Initialize");
  }

  FrameworkDx::ThrowIfFailed(CreateCommandQueue(), FrameworkErrorCode::CommandQueueCreateFailed, "CreateCommandQueue");
  FrameworkDx::ThrowIfFailed(CreateCommandAllocator(), FrameworkErrorCode::CommandAllocatorCreateFailed, "CreateCommandAllocator");
  FrameworkDx::ThrowIfFailed(CreateCommandList(), FrameworkErrorCode::CommandListCreateFailed, "CreateCommandList");

  if (!swap_chain_manager_.Initialize(device_.Get(),
        dxgi_factory_.Get(),
        command_queue_.Get(),
        hwnd,
        frame_buffer_width,
        frame_buffer_height,
        FrameCount,
        descriptor_heap_manager_)) {
    FrameworkFail::Throw(FrameworkErrorDomain::Graphic, FrameworkErrorCode::SwapchainInitFailed, "SwapChainManager::Initialize");
  }

  // Create depth buffer
  if (!depth_buffer_.Create(device_.Get(),
        frame_buffer_width,
        frame_buffer_height,
        descriptor_heap_manager_.GetDsvAllocator(),
        &descriptor_heap_manager_.GetSrvStaticAllocator(),
        DXGI_FORMAT_R32_TYPELESS)) {
    FrameworkFail::Throw(FrameworkErrorDomain::Graphic, FrameworkErrorCode::DepthBufferCreateFailed, "DepthBuffer::Create");
  }
  depth_buffer_.SetDebugName("SceneDepthBuffer");

  if (!fence_manager_.Initialize(device_.Get())) {
    FrameworkFail::Throw(FrameworkErrorDomain::Graphic, FrameworkErrorCode::FenceManagerInitFailed, "FenceManager::Initialize");
  }

  // Initialize upload context for one-shot resource uploads
  if (!upload_context_.Initialize(device_.Get(), command_queue_.Get(), &fence_manager_)) {
    FrameworkFail::Throw(FrameworkErrorDomain::Resource, FrameworkErrorCode::UploadContextInitFailed, "UploadContext::Initialize");
  }

  if (!render_pass_manager_.Initialize(device_.Get(), FrameCount, upload_context_)) {
    FrameworkFail::Throw(FrameworkErrorDomain::Graphic, FrameworkErrorCode::RenderPassManagerInitFailed, "RenderPassManager::Initialize");
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

  InitializeRenderPasses();

  // Initialize framework default assets (textures, meshes, debug materials)
  default_assets_ = std::make_unique<FrameworkDefaultAssets>();
  default_assets_->Initialize(*this);

  Logger::Log(LogLevel::Info, LogCategory::Graphic, "Graphic initialized.");
}

void Graphic::InitializeRenderPasses() {
  auto forward_pass = std::make_unique<ForwardPass>();
  if (!forward_pass->Initialize(device_.Get())) {
    FrameworkFail::Throw(FrameworkErrorDomain::Graphic, FrameworkErrorCode::RenderPassInitFailed, "ForwardPass::Initialize");
  }

  // Use swap chain back buffer as render target
  // The back buffer will be set dynamically in BeginFrame
  forward_pass->SetDepthBuffer(&depth_buffer_);

  render_pass_manager_.RegisterPass("Forward", std::move(forward_pass));

  // Create UI Pass
  auto ui_pass = std::make_unique<UIPass>();
  if (!ui_pass->Initialize(device_.Get())) {
    FrameworkFail::Throw(FrameworkErrorDomain::Graphic, FrameworkErrorCode::RenderPassInitFailed, "UIPass::Initialize");
  }

  render_pass_manager_.RegisterPass("UI", std::move(ui_pass));

  // Cache pass pointers for efficient access
  forward_pass_ = static_cast<ForwardPass*>(render_pass_manager_.GetPass("Forward"));
  ui_pass_ = static_cast<UIPass*>(render_pass_manager_.GetPass("UI"));

  Logger::Logf(LogLevel::Info, LogCategory::Graphic, Logger::Here(), "Registered {} render passes.", render_pass_manager_.GetPassCount());
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
  // Pin the frame index for the entire frame (BeginFrame..EndFrame).
  frame_index_ = swap_chain_manager_.GetCurrentBackBufferIndex();

#if defined(_DEBUG) || defined(DEBUG)
  // Throttled sync diagnostics: helps confirm frames-in-flight behavior without spamming logs.
  // Expectation: completed value typically lags behind the most recent signaled value.
  static uint32_t s_debug_frame_counter = 0;
  if ((s_debug_frame_counter++ % 120u) == 0u) {
    Logger::Log(LogLevel::Debug,
      LogCategory::Graphic,
      std::format("[FrameSync] frame={} frame_index={} slot_fence={} completed={}",
        s_debug_frame_counter,
        frame_index_,
        frame_fence_values_[frame_index_],
        fence_manager_.GetCompletedFenceValue()));
  }
#endif

  // Wait only for the previous use of this frame slot (not a full GPU flush).
  fence_manager_.WaitForFenceValue(frame_fence_values_[frame_index_]);

  // Reset the per-frame allocator and command list for recording.
  {
    const HRESULT hr_alloc = command_allocators_[frame_index_]->Reset();
    if (ReturnIfFailedFast(hr_alloc, ContextId::Graphic_BeginFrame_ResetCommandAllocator, frame_index_, &g_graphic_fast_errors)) {
      return;
    }

    const HRESULT hr_list = command_list_->Reset(command_allocators_[frame_index_].Get(), nullptr);
    if (ReturnIfFailedFast(hr_list, ContextId::Graphic_BeginFrame_ResetCommandList, frame_index_, &g_graphic_fast_errors)) {
      return;
    }
  }

  // Reset descriptor heaps for this frame
  descriptor_heap_manager_.BeginFrame(frame_index_);
  descriptor_heap_manager_.SetDescriptorHeaps(command_list_.Get());

  // Get current render targets
  auto* backbufferRT = swap_chain_manager_.GetRenderTarget(frame_index_);

  // Set viewport and scissor rect
  command_list_->RSSetViewports(1, &viewport_);
  command_list_->RSSetScissorRects(1, &scissor_rect_);

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
  // Execute command list
  {
    const HRESULT hr_close = command_list_->Close();
    if (ReturnIfFailedFast(hr_close, ContextId::Graphic_EndFrame_CloseCommandList, frame_index_, &g_graphic_fast_errors)) {
      return;
    }
  }

  std::array<ID3D12CommandList*, 1> cmdlists = {command_list_.Get()};
  command_queue_->ExecuteCommandLists(static_cast<UINT>(cmdlists.size()), cmdlists.data());

  // Signal completion for this frame slot (no per-frame flush).
  const uint64_t signal_value = fence_manager_.GetCurrentFenceValue();
  fence_manager_.SignalFence(command_queue_.Get());
  frame_fence_values_[frame_index_] = signal_value;

  // Present
  const UINT sync_interval = vsync_enabled_ ? 1u : 0u;
  UINT present_flags = 0u;
  if (!vsync_enabled_ && swap_chain_manager_.IsTearingSupported() && !swap_chain_manager_.IsFullscreenExclusive()) {
    present_flags |= DXGI_PRESENT_ALLOW_TEARING;
  }
  swap_chain_manager_.Present(sync_interval, present_flags);
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

  Logger::Log(LogLevel::Info, LogCategory::Graphic, "Graphic shutdown complete.");
}

D3D12_CPU_DESCRIPTOR_HANDLE Graphic::GetMainRTV() const {
  const RenderTarget* rt = swap_chain_manager_.GetRenderTarget(frame_index_);
  return rt ? rt->GetRTV() : D3D12_CPU_DESCRIPTOR_HANDLE{};
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

HRESULT Graphic::CreateFactory() {
#if defined(DEBUG) || defined(_DEBUG)
  EnableDebugLayer();
  UINT dxgi_factory_flag = DXGI_CREATE_FACTORY_DEBUG;
#else
  UINT dxgi_factory_flag = 0;
#endif

  const HRESULT hr = CreateDXGIFactory2(dxgi_factory_flag, IID_PPV_ARGS(&dxgi_factory_));
  return hr;
}

HRESULT Graphic::CreateDevice() {
  std::vector<ComPtr<IDXGIAdapter>> adapters;
  ComPtr<IDXGIAdapter> tmpAdapter = nullptr;

  for (int i = 0; dxgi_factory_->EnumAdapters(i, &tmpAdapter) != DXGI_ERROR_NOT_FOUND; ++i) {
    adapters.push_back(tmpAdapter);
  }

  for (const auto& adapter : adapters) {
    DXGI_ADAPTER_DESC adapter_desc = {};
    adapter->GetDesc(&adapter_desc);
    if (std::wstring desc_str = adapter_desc.Description; desc_str.find(L"NVIDIA") != std::wstring::npos) {
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
    const HRESULT hr = D3D12CreateDevice(tmpAdapter.Get(), level, IID_PPV_ARGS(&device_));
    if (SUCCEEDED(hr) && device_ != nullptr) {
      return S_OK;
    }
  }

  return E_FAIL;
}

HRESULT Graphic::CreateCommandQueue() {
  D3D12_COMMAND_QUEUE_DESC queue_desc = {};
  queue_desc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
  queue_desc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
  queue_desc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
  queue_desc.NodeMask = 0;

  HRESULT hr = device_->CreateCommandQueue(&queue_desc, IID_PPV_ARGS(&command_queue_));

  if (FAILED(hr) || command_queue_ == nullptr) {
    return FAILED(hr) ? hr : E_FAIL;
  }

  return S_OK;
}

HRESULT Graphic::CreateCommandAllocator() {
  for (uint32_t i = 0; i < FrameCount; ++i) {
    HRESULT hr = device_->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&command_allocators_[i]));
    if (FAILED(hr) || command_allocators_[i] == nullptr) {
      return FAILED(hr) ? hr : E_FAIL;
    }
  }
  return S_OK;
}

HRESULT Graphic::CreateCommandList() {
  HRESULT hr =
    device_->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, command_allocators_[0].Get(), nullptr, IID_PPV_ARGS(&command_list_));

  if (FAILED(hr) || command_list_ == nullptr) {
    return FAILED(hr) ? hr : E_FAIL;
  }

  command_list_->Close();
  return S_OK;
}
