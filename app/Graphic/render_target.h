#pragma once

#include <dxgiformat.h>

#include <array>

#include "d3d12.h"
#include "descriptor_heap_allocator.h"
#include "gpu_resource.h"


class RenderTarget : public GpuResource {
 public:
  RenderTarget() = default;
  ~RenderTarget() override = default;

  RenderTarget(const RenderTarget&) = delete;
  RenderTarget& operator=(const RenderTarget&) = delete;

  bool Create(ID3D12Device* device,
    UINT width,
    UINT height,
    DXGI_FORMAT format,
    DescriptorHeapAllocator& rtv_allocator,
    DescriptorHeapAllocator* srv_allocator = nullptr,
    const float* clear_color = nullptr,
    UINT sample_count = 1,
    UINT sample_quality = 0);

  bool CreateFromResource(
    ID3D12Device* device, ID3D12Resource* resource, DescriptorHeapAllocator& rtv_allocator, DXGI_FORMAT rtv_format = DXGI_FORMAT_UNKNOWN);

  void Clear(ID3D12GraphicsCommandList* command_list) const;
  void Clear(ID3D12GraphicsCommandList* command_list, const float* clear_color) const;

  D3D12_CPU_DESCRIPTOR_HANDLE GetRTV() const {
    return rtv_allocation_.cpu;
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

  const std::array<float, 4>& GetClearColor() const {
    return clear_color_;
  }

  void SetClearColor(const float* color);

  bool IsValid() const override {
    return GpuResource::IsValid() && rtv_allocation_.IsValid();
  }

 private:
  DescriptorHeapAllocator::Allocation rtv_allocation_ = {};
  DescriptorHeapAllocator::Allocation srv_allocation_ = {};  // Optional

  UINT width_ = 0;
  UINT height_ = 0;
  DXGI_FORMAT format_ = DXGI_FORMAT_UNKNOWN;
  std::array<float, 4> clear_color_ = {0.0f, 0.0f, 0.0f, 1.0f};

  bool CreateRTV(ID3D12Device* device, DescriptorHeapAllocator& rtv_allocator, DXGI_FORMAT rtv_format);
  bool CreateSRV(ID3D12Device* device, DescriptorHeapAllocator& srv_allocator);
};
