#pragma once

#include <d3d12.h>

#include "descriptor_heap_allocator.h"
#include "gpu_resource.h"

class Texture : public GpuResource {
 public:
  Texture() = default;
  ~Texture() override = default;

  Texture(const Texture&) = delete;
  Texture& operator=(const Texture&) = delete;

  bool Create(ID3D12Device* device,
    UINT width,
    UINT height,
    DXGI_FORMAT format,
    DescriptorHeapAllocator& src_allocator,
    D3D12_RESOURCE_FLAGS flags = D3D12_RESOURCE_FLAG_NONE,
    UINT mip_levels = 1,
    UINT array_size = 1);

  bool LoadFromFile(
    ID3D12Device* device, ID3D12GraphicsCommandList* command_list, const std::wstring& filepath, DescriptorHeapAllocator& src_allocator);

  bool LoadFromMemory(ID3D12Device* device,
    ID3D12GraphicsCommandList* command_list,
    const void* pixel_data,
    UINT width,
    UINT height,
    DXGI_FORMAT format,
    DescriptorHeapAllocator& srv_allocator,
    UINT row_pitch = 0);

  DescriptorHeapAllocator::Allocation GetSRV() const {
    return srv_allocation_;
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

  UINT GetMipLevels() const {
    return mip_levels_;
  }

  bool IsValid() const override {
    return GpuResource::IsValid() && srv_allocation_.IsValid();
  }

  void ReleaseUploadHeap();

 private:
  DescriptorHeapAllocator::Allocation srv_allocation_ = {};
  UINT width_ = 0;
  UINT height_ = 0;
  DXGI_FORMAT format_ = DXGI_FORMAT_UNKNOWN;
  UINT mip_levels_ = 1;
  UINT array_size_ = 1;

  // Move upload_heap_ to outside the texture
  ComPtr<ID3D12Resource> upload_heap_ = nullptr;

  bool CreateSRV(ID3D12Device* device, DescriptorHeapAllocator& srv_allocator);
};
