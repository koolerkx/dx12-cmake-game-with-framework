#include "swapchain_manager.h"

#include <d3d12.h>
#include <dxgi.h>
#include <dxgi1_5.h>
#include <dxgiformat.h>
#include <winerror.h>
#include <winnt.h>

#include <cassert>
#include <iostream>
#include <string>

#include "Framework/Error/error_helpers_fast.h"

namespace {
FastErrorCounters g_swapchain_fast_errors{};
}

bool SwapChainManager::Initialize(ID3D12Device* device,
  IDXGIFactory6* factory,
  ID3D12CommandQueue* command_queue,
  HWND hwnd,
  UINT width,
  UINT height,
  uint32_t buffer_count,
  DescriptorHeapManager& descriptor_manager) {
  assert(device != nullptr);
  assert(factory != nullptr);
  assert(command_queue != nullptr);
  assert(hwnd != nullptr);

  device_ = device;
  width_ = width;
  height_ = height;
  buffer_count_ = buffer_count;

  // Query whether DXGI supports present with tearing in windowed mode.
  // This must be set before creating the swap chain so we can set the proper swapchain flags.
  tearing_supported_ = false;
  {
    ComPtr<IDXGIFactory5> factory5;
    // Safe cast: check if the current factory supports IDXGIFactory5 interface
    if (SUCCEEDED(factory->QueryInterface(IID_PPV_ARGS(&factory5)))) {
      BOOL allow_tearing = FALSE;
      HRESULT feature_hr =
        factory5->CheckFeatureSupport(DXGI_FEATURE_PRESENT_ALLOW_TEARING, &allow_tearing, static_cast<UINT>(sizeof(allow_tearing)));
      if (SUCCEEDED(feature_hr) && allow_tearing == TRUE) {
        tearing_supported_ = true;
      }
    }
  }

  DXGI_SWAP_CHAIN_DESC1 swap_chain_desc = {};
  swap_chain_desc.Width = width_;
  swap_chain_desc.Height = height_;
  swap_chain_desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
  swap_chain_desc.Stereo = false;
  swap_chain_desc.SampleDesc.Count = 1;
  swap_chain_desc.SampleDesc.Quality = 0;
  swap_chain_desc.BufferUsage = DXGI_USAGE_BACK_BUFFER;
  swap_chain_desc.BufferCount = buffer_count_;
  swap_chain_desc.Scaling = DXGI_SCALING_STRETCH;
  swap_chain_desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
  swap_chain_desc.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;
  swap_chain_desc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
  if (tearing_supported_) {
    swap_chain_desc.Flags |= DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING;
  }

  ComPtr<IDXGISwapChain1> swap_chain1;
  HRESULT hr = factory->CreateSwapChainForHwnd(command_queue, hwnd, &swap_chain_desc, nullptr, nullptr, swap_chain1.GetAddressOf());

  if (FAILED(hr) || swap_chain1 == nullptr) {
    std::cerr << "[SwapChainManager] Failed to create swap chain." << '\n';
    return false;
  }

  swap_chain1.As(&swap_chain_);

  if (FAILED(hr) || swap_chain_ == nullptr) {
    std::cerr << "[SwapChainManager] Failed to create swap chain." << '\n';
    return false;
  }

  if (!CreateBackBufferViews(descriptor_manager)) {
    std::cerr << "[SwapChainManager] Failed to create back buffer views" << '\n';
  }

  return true;
}

bool SwapChainManager::CreateBackBufferViews(DescriptorHeapManager& descriptor_manager) {
  auto& rtvAlloc = descriptor_manager.GetRtvAllocator();
  backbuffer_targets_.clear();

  for (uint32_t i = 0; i < buffer_count_; ++i) {
    backbuffer_targets_.push_back(RenderTarget{});

    ComPtr<ID3D12Resource> buffer;
    HRESULT hr = swap_chain_->GetBuffer(i, IID_PPV_ARGS(buffer.GetAddressOf()));
    if (FAILED(hr)) {
      std::cerr << "Failed to get back buffer " << i << '\n';
      return false;
    }

    if (!backbuffer_targets_[i].CreateFromResource(device_, buffer.Get(), rtvAlloc, DXGI_FORMAT_R8G8B8A8_UNORM)) {
      std::cerr << "Failed to create RenderTarget for back buffer " << i << '\n';
      return false;
    }

    std::wstring name = L"BackBuffer_" + std::to_wstring(i);
    backbuffer_targets_[i].SetDebugName(name);
  }

  return true;
}

void SwapChainManager::TransitionToRenderTarget(ID3D12GraphicsCommandList* command_list) {
  UINT index = GetCurrentBackBufferIndex();

  D3D12_RESOURCE_BARRIER barrier = {};
  barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
  barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
  barrier.Transition.pResource = backbuffer_targets_[index].GetResource();
  barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
  barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
  barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;

  command_list->ResourceBarrier(1, &barrier);
}

void SwapChainManager::TransitionToPresent(ID3D12GraphicsCommandList* command_list) {
  UINT index = GetCurrentBackBufferIndex();

  D3D12_RESOURCE_BARRIER barrier = {};
  barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
  barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
  barrier.Transition.pResource = backbuffer_targets_[index].GetResource();
  barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
  barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
  barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;

  command_list->ResourceBarrier(1, &barrier);
}

void SwapChainManager::Present(UINT syncInterval, UINT flags) {
  if (swap_chain_) {
    const HRESULT hr = swap_chain_->Present(syncInterval, flags);
    const uint32_t extra_marker = (static_cast<uint32_t>(syncInterval & 0xFFFFu) << 16) | (static_cast<uint32_t>(flags) & 0xFFFFu);
    (void)ReturnIfFailedFast(hr, ContextId::Graphic_Present_SwapChainPresent, extra_marker, &g_swapchain_fast_errors);
  }
}

void SwapChainManager::ReleaseBackBuffers() {
  backbuffer_targets_.clear();
}

bool SwapChainManager::Resize(UINT width, UINT height, uint32_t buffer_count, DescriptorHeapManager& descriptor_manager) {
  assert(swap_chain_ != nullptr);

  buffer_count_ = buffer_count;
  ReleaseBackBuffers();
  UINT swapchain_flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
  if (tearing_supported_) {
    swapchain_flags |= DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING;
  }
  HRESULT hr = swap_chain_->ResizeBuffers(buffer_count_, width, height, DXGI_FORMAT_R8G8B8A8_UNORM, swapchain_flags);

  if (FAILED(hr)) {
    std::cerr << "[SwapChainManager] Failed to resize swap chain." << '\n';
    return false;
  }

  width_ = width;
  height_ = height;

  return CreateBackBufferViews(descriptor_manager);
}
