#pragma once

#include <cstdint>

#include "d3d12.h"
#include "descriptor_heap_allocator.h"

constexpr int DEFAULT_RTV_CAPACITY = 256;
constexpr int DEFAULT_DSV_CAPACITY = 64;
constexpr int DEFAULT_SRV_CAPACITY = 4096;
constexpr int DEFAULT_SAMPLER_CAPACITY = 256;

class DescriptorHeapManager {
 public:
  DescriptorHeapManager() = default;
  ~DescriptorHeapManager() = default;

  DescriptorHeapManager(const DescriptorHeapManager&) = delete;
  DescriptorHeapManager& operator=(const DescriptorHeapManager&) = delete;

  bool Initalize(ID3D12Device* device);
  void BeginFrame();
  void SetDescriptorHeaps(ID3D12GraphicsCommandList* command_list);

  DescriptorHeapAllocator& GetRtvAllocator() {
    return rtv_heap_;
  }
  DescriptorHeapAllocator& GetDsvAllocator() {
    return dsv_heap_;
  }
  DescriptorHeapAllocator& GetSrvAllocator() {
    return srv_heap_;
  }
  DescriptorHeapAllocator& GetSamplerAllocator() {
    return sampler_heap_;
  }

  void PrintStats() const;

 private:
  DescriptorHeapAllocator rtv_heap_;
  DescriptorHeapAllocator dsv_heap_;

  DescriptorHeapAllocator srv_heap_;
  DescriptorHeapAllocator sampler_heap_;

  struct Config {
    uint32_t rtv_capacity = DEFAULT_RTV_CAPACITY;
    uint32_t dsv_capacity = DEFAULT_DSV_CAPACITY;

    uint32_t srv_capacity = DEFAULT_SRV_CAPACITY;
    uint32_t sampler_capacity = DEFAULT_SAMPLER_CAPACITY;
  };

  Config config_;
};
