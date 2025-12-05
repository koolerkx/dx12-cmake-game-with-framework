#pragma once

#include <d3d12.h>
#include <dxgiformat.h>

#include "descriptor_heap_allocator.h"
#include "gpu_resource.h"

class DepthBuffer : public GpuResource {
 public:
  DepthBuffer() = default;
  ~DepthBuffer() override = default;

  DepthBuffer(const DepthBuffer&) = delete;
  DepthBuffer& operator=(const DepthBuffer&) = delete;

  // Create depth buffer with optional SRV (for shadow mapping, SSAO, etc.)
  bool Create(ID3D12Device* device,
    UINT width,
    UINT height,
    DescriptorHeapAllocator& dsv_allocator,
    DescriptorHeapAllocator* srv_allocator = nullptr,
    DXGI_FORMAT format = DXGI_FORMAT_D32_FLOAT,
    UINT sample_count = 1,
    UINT sample_quality = 0);

  // Clear the depth buffer
  void Clear(ID3D12GraphicsCommandList* command_list, float depth = 1.0f, UINT8 stencil = 0) const;

  // Accessors
  D3D12_CPU_DESCRIPTOR_HANDLE GetDSV() const {
    return dsv_allocation_.cpu;
  }

  DescriptorHeapAllocator::Allocation GetSRV() const {
    return srv_allocation_;
  }

  bool HasSRV() const {
    return srv_allocation_.IsValid();
  }

  UINT GetWidth() const {
    return width_;
  }

  UINT GetHeight() const {
    return height_;
  }

  DXGI_FORMAT GetFormat() const {
    return format_;
  }

  DXGI_FORMAT GetSRVFormat() const {
    return srv_format_;
  }

  bool IsValid() const override {
    return GpuResource::IsValid() && dsv_allocation_.IsValid();
  }

 private:
  DescriptorHeapAllocator::Allocation dsv_allocation_ = {};
  DescriptorHeapAllocator::Allocation srv_allocation_ = {};  // Optional

  UINT width_ = 0;
  UINT height_ = 0;
  DXGI_FORMAT format_ = DXGI_FORMAT_D32_FLOAT;      // Texture format
  DXGI_FORMAT srv_format_ = DXGI_FORMAT_R32_FLOAT;  // SRV format (if needed)

  bool CreateDSV(ID3D12Device* device, DescriptorHeapAllocator& dsv_allocator);
  bool CreateSRV(ID3D12Device* device, DescriptorHeapAllocator& srv_allocator);

  // Helper to determine typeless and SRV formats
  static DXGI_FORMAT GetTypelessFormat(DXGI_FORMAT format);
  static DXGI_FORMAT GetSRVFormat(DXGI_FORMAT format);
};
