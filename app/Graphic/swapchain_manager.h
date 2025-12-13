#pragma once

#include <d3d12.h>
#include <dxgi1_5.h>
#include <dxgi1_6.h>

#include <vector>

#include "Framework/types.h"
#include "descriptor_heap_manager.h"
#include "render_target.h"


class SwapChainManager {
 public:
  SwapChainManager() = default;
  ~SwapChainManager() = default;
  SwapChainManager(const SwapChainManager&) = delete;
  SwapChainManager& operator=(const SwapChainManager&) = delete;

  bool Initialize(ID3D12Device* device,
    IDXGIFactory6* factory,
    ID3D12CommandQueue* command_queue,
    HWND hwnd,
    UINT width,
    UINT height,
    uint32_t buffer_count,
    DescriptorHeapManager& descriptor_manager);

  UINT GetCurrentBackBufferIndex() const {
    return swap_chain_ ? swap_chain_->GetCurrentBackBufferIndex() : 0;
  }

  // Alias for frame index usage
  UINT GetCurrentFrameIndex() const {
    return GetCurrentBackBufferIndex();
  }

  D3D12_CPU_DESCRIPTOR_HANDLE GetCurrentRTV() const {
    return GetCurrentRenderTarget()->GetRTV();
  }

  RenderTarget* GetRenderTarget(uint32_t frame_index) {
    if (backbuffer_render_targets_.empty() || buffer_count_ == 0) {
      return nullptr;
    }
    const uint32_t index = frame_index % buffer_count_;
    return &backbuffer_render_targets_[index];
  }

  const RenderTarget* GetRenderTarget(uint32_t frame_index) const {
    if (backbuffer_render_targets_.empty() || buffer_count_ == 0) {
      return nullptr;
    }
    const uint32_t index = frame_index % buffer_count_;
    return &backbuffer_render_targets_[index];
  }

  RenderTarget* GetCurrentRenderTarget() {
    return GetRenderTarget(GetCurrentBackBufferIndex());
  }

  const RenderTarget* GetCurrentRenderTarget() const {
    return GetRenderTarget(GetCurrentBackBufferIndex());
  }

  ID3D12Resource* GetCurrentBackBuffer() const {
    return GetCurrentRenderTarget()->GetResource();
  }

  void TransitionToRenderTarget(ID3D12GraphicsCommandList* command_list);
  void TransitionToPresent(ID3D12GraphicsCommandList* command_list);

  void Present(UINT syncInterval = 1, UINT flags = 0);

  bool IsTearingSupported() const {
    return tearing_supported_;
  }

  bool IsFullscreenExclusive() const {
    if (!swap_chain_) {
      return false;
    }
    BOOL fullscreen = FALSE;
    if (FAILED(swap_chain_->GetFullscreenState(&fullscreen, nullptr))) {
      return false;
    }
    return fullscreen == TRUE;
  }

  IDXGISwapChain4* GetSwapChain() const {
    return swap_chain_.Get();
  }

  UINT GetWidth() const {
    return width_;
  }

  // Backward-compatible typo alias.
  UINT GetWdith() const {
    return GetWidth();
  }
  UINT GetHeight() const {
    return height_;
  }

  bool Resize(UINT width, UINT height, uint32_t buffer_count, DescriptorHeapManager& descriptor_manager);

 private:
  ComPtr<IDXGISwapChain4> swap_chain_ = nullptr;
  std::vector<RenderTarget> backbuffer_render_targets_;

  UINT width_ = 0;
  UINT height_ = 0;
  uint32_t buffer_count_ = 0;

  bool tearing_supported_ = false;

  ID3D12Device* device_ = nullptr;

  bool CreateBackBufferRenderTargets(DescriptorHeapManager& descriptor_manager);

  /**
   * @note used for resize
   */
  void ReleaseBackBufferRenderTargets();
};
