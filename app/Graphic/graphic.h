#pragma once

#include <basetsd.h>
#include <d3d12.h>
#include <dxgi1_6.h>

#include <cstdint>
#include <iostream>
#include <vector>

#include "d3dx12.h"
#include "types.h"

class Graphic {
 public:
  Graphic() = default;
  ~Graphic() = default;

  bool Initalize(HWND hwnd, UINT frame_buffer_width, UINT frame_buffer_height);
  void BeginRender();
  void EndRender();

  static constexpr int FRAME_BUFFER_COUNT = 2;

 private:
  ComPtr<ID3D12Device5> device_ = nullptr;  /// @note D3D Device, RTX graphic card required
  ComPtr<IDXGIFactory6> dxgi_factory_ = nullptr;
  ComPtr<IDXGISwapChain4> swap_chain_ = nullptr;

  ComPtr<ID3D12CommandAllocator> command_allocator_ = nullptr;
  ComPtr<ID3D12GraphicsCommandList> command_list_ = nullptr;

  ComPtr<ID3D12CommandQueue> command_queue_ = nullptr;

  ComPtr<ID3D12DescriptorHeap> rtv_heaps_ = nullptr;
  ComPtr<ID3D12DescriptorHeap> dsv_heaps_ = nullptr;

  int current_back_buffer_index_ = 0;

  UINT frame_buffer_width_ = 0;
  UINT frame_buffer_height_ = 0;

  UINT rtv_descriptor_size_;
  UINT dsv_descriptor_size_;
  D3D12_CPU_DESCRIPTOR_HANDLE current_frame_buffer_rtv_handle_;
  D3D12_CPU_DESCRIPTOR_HANDLE current_frame_buffer_dsv_handle_;

  std::vector<ComPtr<ID3D12Resource>> _backBuffers{FRAME_BUFFER_COUNT, nullptr};
  ComPtr<ID3D12Resource> depth_stencil_buffer_ = nullptr;

  // Pipeline State
  ComPtr<ID3D12RootSignature> root_signature_ = nullptr;
  ComPtr<ID3D12PipelineState> pipeline_state_ = nullptr;

  // Viewport
  D3D12_VIEWPORT viewport_ = {};
  D3D12_RECT scissor_rect_ = {};

  // Buffer
  D3D12_VERTEX_BUFFER_VIEW vertex_buffer_view_ = {};
  D3D12_INDEX_BUFFER_VIEW index_buffer_view_ = {};

  // texture
  ComPtr<ID3D12DescriptorHeap> texture_descriptor_heap_ = nullptr;

  // GPU Synchronization
  UINT frame_index_ = 0;
  HANDLE fence_event_ = nullptr;

  ComPtr<ID3D12Fence> fence_ = nullptr;
  UINT64 fence_val_ = 0;

  // Initialization
  bool EnableDebugLayer();
  bool CreateFactory();
  bool CreateDevice();
  bool CreateCommandQueue();

  bool CreateCommandList();
  bool CreateCommandAllocator();

  bool CreateSwapChain(HWND hwnd, UINT frameBufferWidth, UINT frameBufferHeight);

  bool CreateDescriptorHeapForFrameBuffer();
  bool CreateRTVForFameBuffer();
  bool CreateDSVForFrameBuffer();

  bool CreateSynchronizationWithGPUObject();
};
