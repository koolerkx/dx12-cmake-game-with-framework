#pragma once

#include <cstdint>
#include <memory>
#include <vector>

#include "d3d12.h"
#include "descriptor_heap_allocator.h"

constexpr int DEFAULT_RTV_CAPACITY = 256;
constexpr int DEFAULT_DSV_CAPACITY = 64;
constexpr int DEFAULT_SRV_CAPACITY = 4096;
constexpr int DEFAULT_SAMPLER_CAPACITY = 256;
constexpr int DEFAULT_SRV_PERSISTENT_RESERVED = 2048;

class DescriptorHeapManager {
 public:
  DescriptorHeapManager() = default;
  ~DescriptorHeapManager() = default;

  DescriptorHeapManager(const DescriptorHeapManager&) = delete;
  DescriptorHeapManager& operator=(const DescriptorHeapManager&) = delete;

  bool Initalize(ID3D12Device* device, uint32_t frame_count);
  void BeginFrame(uint32_t frame_index);
  void SetDescriptorHeaps(ID3D12GraphicsCommandList* command_list);

  DescriptorHeapAllocator& GetRtvAllocator() {
    return rtv_heap_;
  }
  DescriptorHeapAllocator& GetDsvAllocator() {
    return dsv_heap_;
  }
  // Persistent SRV allocations (textures, long-lived SRVs). Never reset during frames.
  DescriptorHeapAllocator& GetSrvPersistentAllocator() {
    return srv_persistent_heap_;
  }
  // Transient SRV allocations (per-frame slices, reset at BeginFrame).
  DescriptorHeapAllocator& GetSrvTransientAllocator() {
    return *srv_transient_frames_[current_frame_index_];
  }
  // Transient sampler allocations (per-frame slices, reset at BeginFrame).
  DescriptorHeapAllocator& GetSamplerTransientAllocator() {
    return *sampler_transient_frames_[current_frame_index_];
  }

  void PrintStats() const;

 private:
  DescriptorHeapAllocator rtv_heap_;
  DescriptorHeapAllocator dsv_heap_;

  // Persistent means static, transient means dynamic here.
  // Underlying SRV heap (owned) + two sub-allocators (persistent prefix + transient suffix slices).
  DescriptorHeapAllocator srv_heap_;
  DescriptorHeapAllocator srv_persistent_heap_;
  std::vector<std::unique_ptr<DescriptorHeapAllocator>> srv_transient_frames_;

  // Underlying sampler heap (owned) + per-frame sub-allocators (slices).
  DescriptorHeapAllocator sampler_heap_;
  std::vector<std::unique_ptr<DescriptorHeapAllocator>> sampler_transient_frames_;

  uint32_t frame_count_ = 1;
  uint32_t current_frame_index_ = 0;

  struct Config {
    uint32_t rtv_capacity = DEFAULT_RTV_CAPACITY;
    uint32_t dsv_capacity = DEFAULT_DSV_CAPACITY;

    uint32_t srv_capacity = DEFAULT_SRV_CAPACITY;
    uint32_t srv_persistent_reserved = DEFAULT_SRV_PERSISTENT_RESERVED;
    uint32_t sampler_capacity = DEFAULT_SAMPLER_CAPACITY;
  };

  Config config_;
};
