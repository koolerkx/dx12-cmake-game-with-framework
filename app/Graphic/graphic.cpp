#include "graphic.h"

#include <winerror.h>

#include "types.h"

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
  if (!CreateCommandQueue()) {
    MessageBoxW(nullptr, L"Graphic: Failed to create command queue", init_error_caption.c_str(), MB_OK | MB_ICONERROR);
    return false;
  }

  if (!CreateSwapChain(hwnd, frame_buffer_height_, frame_buffer_height_)) {
    MessageBoxW(nullptr, L"Graphic: Failed to create swap chain", init_error_caption.c_str(), MB_OK | MB_ICONERROR);
    return false;
  }

  if (!CreateDescriptorHeapForFrameBuffer()) {
    MessageBoxW(nullptr, L"Graphic: Failed to create descriptor heap", init_error_caption.c_str(), MB_OK | MB_ICONERROR);
    return false;
  }
  if (!CreateRTVForFameBuffer()) {
    MessageBoxW(nullptr, L"Graphic: Failed to create RTV", init_error_caption.c_str(), MB_OK | MB_ICONERROR);
    return false;
  }
  if (!CreateDSVForFrameBuffer()) {
    MessageBoxW(nullptr, L"Graphic: Failed to create DSV", init_error_caption.c_str(), MB_OK | MB_ICONERROR);
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

  if (!CreateSynchronizationWithGPUObject()) {
    MessageBoxW(nullptr, L"Graphic: Failed to create fence", init_error_caption.c_str(), MB_OK | MB_ICONERROR);
    return false;
  }

  return true;
}

void Graphic::BeginRender() {
  frame_index_ = swap_chain_->GetCurrentBackBufferIndex();
  command_allocator_->Reset();

  command_list_->Reset(command_allocator_.Get(), nullptr);

  D3D12_RESOURCE_BARRIER BarrierDesc = {};
  BarrierDesc.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
  BarrierDesc.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
  BarrierDesc.Transition.pResource = _backBuffers[frame_index_].Get();
  BarrierDesc.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
  BarrierDesc.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
  BarrierDesc.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
  command_list_->ResourceBarrier(1, &BarrierDesc);

  auto rtvH = rtv_heaps_->GetCPUDescriptorHandleForHeapStart();
  rtvH.ptr += static_cast<ULONG_PTR>(frame_index_ * device_->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV));
  command_list_->OMSetRenderTargets(1, &rtvH, false, nullptr);

  float clearColor[] = {1.0f, 1.0f, 0.0f, 1.0f};
  command_list_->ClearRenderTargetView(rtvH, clearColor, 0, nullptr);

  BarrierDesc.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
  BarrierDesc.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
  command_list_->ResourceBarrier(1, &BarrierDesc);

  command_list_->Close();

  ID3D12CommandList* cmdlists[] = {command_list_.Get()};
  command_queue_->ExecuteCommandLists(1, cmdlists);
  command_queue_->Signal(fence_.Get(), ++fence_val_);

  if (fence_->GetCompletedValue() != fence_val_) {
    auto event = CreateEvent(nullptr, false, false, nullptr);
    fence_->SetEventOnCompletion(fence_val_, event);
    WaitForSingleObject(event, INFINITE);
    CloseHandle(event);
  }

  swap_chain_->Present(1, 0);
}

void Graphic::EndRender() {
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

  for (auto adapter : adapters) {
    DXGI_ADAPTER_DESC adapter_desc = {};
    adapter->GetDesc(&adapter_desc);
    if (std::wstring desc_str = adapter_desc.Description; desc_str.find(L"NVIDIA") != std::string::npos) {
      tmpAdapter = adapter;
      break;
    }
  }

  D3D_FEATURE_LEVEL levels[] = {
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

  std::cerr << "Failed to create device." << std::endl;
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
    std::cerr << "Failed to create command queue." << std::endl;
    return false;
  }
  return true;
}

bool Graphic::CreateCommandList() {
  auto hr = device_->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, command_allocator_.Get(), nullptr, IID_PPV_ARGS(&command_list_));

  if (FAILED(hr) || command_list_ == nullptr) {
    std::cerr << "Failed to create command list." << std::endl;
    return false;
  }

  command_list_->Close();
  return true;
}

bool Graphic::CreateCommandAllocator() {
  auto hr = device_->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&command_allocator_));

  if (FAILED(hr) || command_allocator_ == nullptr) {
    std::cerr << "Failed to create command allocator." << std::endl;
    return false;
  }
  return true;
}

bool Graphic::CreateSwapChain(HWND hwnd, UINT frameBufferWidth, UINT frameBufferHeight) {
  DXGI_SWAP_CHAIN_DESC1 swap_chain_desc = {};
  swap_chain_desc.Width = frameBufferWidth;
  swap_chain_desc.Height = frameBufferHeight;
  swap_chain_desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
  swap_chain_desc.Stereo = false;
  swap_chain_desc.SampleDesc.Count = 1;
  swap_chain_desc.SampleDesc.Quality = 0;
  swap_chain_desc.BufferUsage = DXGI_USAGE_BACK_BUFFER;
  swap_chain_desc.BufferCount = 2;
  swap_chain_desc.Scaling = DXGI_SCALING_STRETCH;
  swap_chain_desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
  swap_chain_desc.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;
  swap_chain_desc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;

  ComPtr<IDXGISwapChain1> swap_chain1;
  HRESULT hr =
    dxgi_factory_->CreateSwapChainForHwnd(command_queue_.Get(), hwnd, &swap_chain_desc, nullptr, nullptr, (swap_chain1.GetAddressOf()));

  if (FAILED(hr) || swap_chain1 == nullptr) {
    std::cerr << "Failed to create swap chain." << std::endl;
    return false;
  }

  swap_chain1.As(&swap_chain_);

  if (FAILED(hr) || swap_chain_ == nullptr) {
    std::cerr << "Failed to create swap chain." << std::endl;
    return false;
  }

  return true;
}

bool Graphic::CreateDescriptorHeapForFrameBuffer() {
  D3D12_DESCRIPTOR_HEAP_DESC heap_desc = {};
  heap_desc.NumDescriptors = FRAME_BUFFER_COUNT;
  heap_desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
  heap_desc.NodeMask = 0;
  heap_desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;

  auto hr = device_->CreateDescriptorHeap(&heap_desc, IID_PPV_ARGS(&rtv_heaps_));

  rtv_descriptor_size_ = device_->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

  if (FAILED(hr) || rtv_heaps_ == nullptr) {
    std::cerr << "Failed to create RTV descriptor heap." << std::endl;
    return false;
  }

  heap_desc.NumDescriptors = 1;
  heap_desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
  hr = device_->CreateDescriptorHeap(&heap_desc, IID_PPV_ARGS(&dsv_heaps_));
  if (FAILED(hr) || dsv_heaps_ == nullptr) {
    std::cerr << "Failed to create DSV descriptor heap." << std::endl;
    return false;
  }

  dsv_descriptor_size_ = device_->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);

  return true;
}

bool Graphic::CreateRTVForFameBuffer() {
  if (swap_chain_ == nullptr) {
    std::cerr << "Failed to create RTV for frame buffer, swap_chain_ is nullptr." << std::endl;
    return false;
  }

  if (rtv_heaps_ == nullptr) {
    std::cerr << "Failed to create RTV for frame buffer, rtv_heaps_ is nullptr." << std::endl;
    return false;
  }

  DXGI_SWAP_CHAIN_DESC swcDesc = {};
  swap_chain_->GetDesc(&swcDesc);

  D3D12_CPU_DESCRIPTOR_HANDLE handle = rtv_heaps_->GetCPUDescriptorHandleForHeapStart();
  for (size_t i = 0; i < swcDesc.BufferCount; ++i) {
    swap_chain_->GetBuffer(static_cast<UINT>(i), IID_PPV_ARGS(_backBuffers[i].GetAddressOf()));
    device_->CreateRenderTargetView(_backBuffers[i].Get(), nullptr, handle);
    handle.ptr += rtv_descriptor_size_;
  }

  return true;
}

bool Graphic::CreateDSVForFrameBuffer() {
  D3D12_CLEAR_VALUE dsvClearValue;
  dsvClearValue.Format = DXGI_FORMAT_D32_FLOAT;
  dsvClearValue.DepthStencil.Depth = 1.0f;
  dsvClearValue.DepthStencil.Stencil = 0;

  CD3DX12_RESOURCE_DESC desc(D3D12_RESOURCE_DIMENSION_TEXTURE2D,
    0,
    frame_buffer_width_,
    frame_buffer_height_,
    1,
    1,
    DXGI_FORMAT_D32_FLOAT,
    1,
    0,
    D3D12_TEXTURE_LAYOUT_UNKNOWN,
    D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL | D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE);

  auto heapProp = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
  auto hr = device_->CreateCommittedResource(&heapProp,
    D3D12_HEAP_FLAG_NONE,
    &desc,
    D3D12_RESOURCE_STATE_DEPTH_WRITE,
    &dsvClearValue,
    IID_PPV_ARGS(depth_stencil_buffer_.GetAddressOf()));

  if (FAILED(hr) || depth_stencil_buffer_ == nullptr) {
    std::cerr << "Failed to create depth stencil buffer." << std::endl;
    return false;
  }

  D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle = dsv_heaps_->GetCPUDescriptorHandleForHeapStart();

  device_->CreateDepthStencilView(depth_stencil_buffer_.Get(), nullptr, dsvHandle);

  return true;
}

bool Graphic::CreateSynchronizationWithGPUObject() {
  auto hr = device_->CreateFence(fence_val_, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(fence_.GetAddressOf()));

  if (FAILED(hr) || fence_ == nullptr) {
    std::cerr << "Failed to create fence." << std::endl;
    return false;
  }

  fence_val_ = 1;

  fence_event_ = CreateEvent(nullptr, FALSE, FALSE, nullptr);
  if (fence_event_ == nullptr) {
    std::cerr << "Failed to create fence event." << std::endl;
    return false;
  }
  return true;
}
