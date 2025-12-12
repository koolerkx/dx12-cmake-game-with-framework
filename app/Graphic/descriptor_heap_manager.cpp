#include "descriptor_heap_manager.h"

#include <array>
#include <cassert>
#include <iostream>

bool DescriptorHeapManager::Initalize(ID3D12Device* device) {
  assert(device != nullptr);

  if (!rtv_heap_.Initialize(device, D3D12_DESCRIPTOR_HEAP_TYPE_RTV, config_.rtv_capacity, false)) {
    std::cerr << "Failed to initialize RTV heap" << '\n';
    return false;
  }
  if (!dsv_heap_.Initialize(device, D3D12_DESCRIPTOR_HEAP_TYPE_DSV, config_.dsv_capacity, false)) {
    std::cerr << "Failed to initialize DSV heap" << '\n';
    return false;
  }
  if (!srv_heap_.Initialize(device, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, config_.srv_capacity, true)) {
    std::cerr << "Failed to initialize SRV heap" << '\n';
    return false;
  }

  // Split SRV heap into a persistent prefix and a per-frame resettable suffix.
  // Ensure we always leave at least 1 descriptor for the dynamic region.
  uint32_t static_reserved = config_.srv_static_reserved;
  if (config_.srv_capacity <= 1) {
    static_reserved = 0;
  } else {
    const uint32_t max_static = config_.srv_capacity - 1;
    if (static_reserved > max_static) {
      static_reserved = max_static;
    }
  }
  const uint32_t dynamic_capacity = config_.srv_capacity - static_reserved;

  if (!srv_static_heap_.InitializeFromExistingHeap(
        device, srv_heap_.GetHeap(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 0, static_reserved, true)) {
    std::cerr << "Failed to initialize SRV static sub-allocator" << '\n';
    return false;
  }
  if (!srv_dynamic_heap_.InitializeFromExistingHeap(
        device, srv_heap_.GetHeap(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, static_reserved, dynamic_capacity, true)) {
    std::cerr << "Failed to initialize SRV dynamic sub-allocator" << '\n';
    return false;
  }

  if (!sampler_heap_.Initialize(device, D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER, config_.sampler_capacity, true)) {
    std::cerr << "Failed to initialize sampler heap" << '\n';
    return false;
  }

  return true;
}

void DescriptorHeapManager::BeginFrame() {
  // Only reset the dynamic portion of the SRV heap.
  srv_dynamic_heap_.Reset();
  sampler_heap_.Reset();
}

void DescriptorHeapManager::SetDescriptorHeaps(ID3D12GraphicsCommandList* command_list) {
  assert(command_list != nullptr);

  std::array<ID3D12DescriptorHeap*, 2> heaps = {
    srv_heap_.GetHeap(),
    sampler_heap_.GetHeap(),
  };

  command_list->SetDescriptorHeaps(static_cast<UINT>(heaps.size()), heaps.data());
}

void DescriptorHeapManager::PrintStats() const {
  std::cout << "\n=== Descriptor Heap Statistics ===" << '\n';
  std::cout << "RTV Heap: " << rtv_heap_.GetAllocated() << "/" << rtv_heap_.GetCapacity() << '\n';
  std::cout << "DSV Heap: " << dsv_heap_.GetAllocated() << "/" << dsv_heap_.GetCapacity() << '\n';
  std::cout << "SRV Heap Static (persistent): " << srv_static_heap_.GetAllocated() << "/" << srv_static_heap_.GetCapacity() << '\n';
  std::cout << "SRV Heap Dynamic (per-frame): " << srv_dynamic_heap_.GetAllocated() << "/" << srv_dynamic_heap_.GetCapacity() << '\n';
  std::cout << "Sampler Heap (per-frame): " << sampler_heap_.GetAllocated() << "/" << sampler_heap_.GetCapacity() << '\n';
  std::cout << "==================================\n" << '\n';
}
