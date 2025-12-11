#pragma once

#include <d3d12.h>
#include <dxgi1_5.h>
#include <dxgi1_6.h>

#include <vector>

#include "descriptor_heap_manager.h"
#include "render_target.h"
#include "types.h"

class SwapChainManager {
 public:
  static constexpr UINT BUFFER_COUNT = 2;

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

  RenderTarget* GetCurrentRenderTarget() {
    UINT index = GetCurrentBackBufferIndex();
    return &backbuffer_targets_[index];
  }

  const RenderTarget* GetCurrentRenderTarget() const {
    UINT index = GetCurrentBackBufferIndex();
    return &backbuffer_targets_[index];
  }

  ID3D12Resource* GetCurrentBackBuffer() const {
    return GetCurrentRenderTarget()->GetResource();
  }

  void TransitionToRenderTarget(ID3D12GraphicsCommandList* command_list);
  void TransitionToPresent(ID3D12GraphicsCommandList* command_list);

  void Present(UINT syncInterval = 1, UINT flags = 0);

  IDXGISwapChain4* GetSwapChain() const {
    return swap_chain_.Get();
  }

  UINT GetWdith() const {
    return width_;
  }
  UINT GetHeight() const {
    return height_;
  }

  bool Resize(UINT width, UINT height, DescriptorHeapManager& descriptor_manager);

 private:
  ComPtr<IDXGISwapChain4> swap_chain_ = nullptr;
  std::vector<RenderTarget> backbuffer_targets_;

  UINT width_ = 0;
  UINT height_ = 0;

  ID3D12Device* device_ = nullptr;

  bool CreateBackBufferViews(DescriptorHeapManager& descriptor_manager);

  /**
   * @note used for resize
   */
  void ReleaseBackBuffers();
};
