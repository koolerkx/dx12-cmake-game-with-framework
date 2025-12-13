#include "descriptor_heap_manager.h"

#include <array>
#include <cassert>

#include "Framework/Logging/logger.h"

static void SplitCapacity(uint32_t total, uint32_t parts, std::vector<uint32_t>& out_sizes) {
  out_sizes.clear();
  if (parts == 0) {
    return;
  }
  out_sizes.resize(parts, 0);

  const uint32_t base = total / parts;
  const uint32_t rem = total % parts;
  for (uint32_t i = 0; i < parts; ++i) {
    out_sizes[i] = base + (i < rem ? 1u : 0u);
  }
}

bool DescriptorHeapManager::Initalize(ID3D12Device* device, uint32_t frame_count) {
  assert(device != nullptr);
  frame_count_ = (frame_count == 0) ? 1u : frame_count;
  current_frame_index_ = 0;

  if (!rtv_heap_.Initialize(device, D3D12_DESCRIPTOR_HEAP_TYPE_RTV, config_.rtv_capacity, false)) {
    Logger::Log(LogLevel::Error, LogCategory::Graphic, "[DescriptorHeapManager] Failed to initialize RTV heap.");
    return false;
  }
  if (!dsv_heap_.Initialize(device, D3D12_DESCRIPTOR_HEAP_TYPE_DSV, config_.dsv_capacity, false)) {
    Logger::Log(LogLevel::Error, LogCategory::Graphic, "[DescriptorHeapManager] Failed to initialize DSV heap.");
    return false;
  }
  if (!srv_heap_.Initialize(device, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, config_.srv_capacity, true)) {
    Logger::Log(LogLevel::Error, LogCategory::Graphic, "[DescriptorHeapManager] Failed to initialize SRV heap.");
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
    Logger::Log(LogLevel::Error, LogCategory::Graphic, "[DescriptorHeapManager] Failed to initialize SRV static sub-allocator.");
    return false;
  }

  // Split the dynamic suffix into per-frame slices so BeginFrame() can safely reset only
  // the current frame's transient descriptors under frames-in-flight.
  srv_dynamic_frames_.clear();
  srv_dynamic_frames_.resize(frame_count_);
  std::vector<uint32_t> srv_slice_sizes;
  SplitCapacity(dynamic_capacity, frame_count_, srv_slice_sizes);

  uint32_t srv_offset = 0;
  for (uint32_t i = 0; i < frame_count_; ++i) {
    srv_dynamic_frames_[i] = std::make_unique<DescriptorHeapAllocator>();
    const uint32_t slice_capacity = srv_slice_sizes[i];
    if (!srv_dynamic_frames_[i]->InitializeFromExistingHeap(
          device,
          srv_heap_.GetHeap(),
          D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,
          static_reserved + srv_offset,
          slice_capacity,
          true)) {
      Logger::Logf(LogLevel::Error,
        LogCategory::Graphic,
        Logger::Here(),
        "[DescriptorHeapManager] Failed to initialize SRV dynamic sub-allocator (frame slice {}).",
        i);
      return false;
    }
    srv_offset += slice_capacity;
  }

  if (!sampler_heap_.Initialize(device, D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER, config_.sampler_capacity, true)) {
    Logger::Log(LogLevel::Error, LogCategory::Graphic, "[DescriptorHeapManager] Failed to initialize sampler heap.");
    return false;
  }

  sampler_frames_.clear();
  sampler_frames_.resize(frame_count_);
  std::vector<uint32_t> sampler_slice_sizes;
  SplitCapacity(config_.sampler_capacity, frame_count_, sampler_slice_sizes);
  uint32_t sampler_offset = 0;
  for (uint32_t i = 0; i < frame_count_; ++i) {
    sampler_frames_[i] = std::make_unique<DescriptorHeapAllocator>();
    const uint32_t slice_capacity = sampler_slice_sizes[i];
    if (!sampler_frames_[i]->InitializeFromExistingHeap(
          device,
          sampler_heap_.GetHeap(),
          D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER,
          sampler_offset,
          slice_capacity,
          true)) {
      Logger::Logf(LogLevel::Error,
        LogCategory::Graphic,
        Logger::Here(),
        "[DescriptorHeapManager] Failed to initialize sampler sub-allocator (frame slice {}).",
        i);
      return false;
    }
    sampler_offset += slice_capacity;
  }

  return true;
}

void DescriptorHeapManager::BeginFrame(uint32_t frame_index) {
  if (frame_count_ == 0) {
    current_frame_index_ = 0;
  } else {
    current_frame_index_ = frame_index % frame_count_;
  }

  // Only reset this frame's slices (frames-in-flight safe).
  if (current_frame_index_ < srv_dynamic_frames_.size()) {
    if (srv_dynamic_frames_[current_frame_index_]) {
      srv_dynamic_frames_[current_frame_index_]->Reset();
    }
  }
  if (current_frame_index_ < sampler_frames_.size()) {
    if (sampler_frames_[current_frame_index_]) {
      sampler_frames_[current_frame_index_]->Reset();
    }
  }
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
  uint32_t srv_dyn_allocated = 0;
  uint32_t srv_dyn_capacity = 0;
  for (const auto& alloc : srv_dynamic_frames_) {
    if (!alloc) continue;
    srv_dyn_allocated += alloc->GetAllocated();
    srv_dyn_capacity += alloc->GetCapacity();
  }

  uint32_t sampler_allocated = 0;
  uint32_t sampler_capacity = 0;
  for (const auto& alloc : sampler_frames_) {
    if (!alloc) continue;
    sampler_allocated += alloc->GetAllocated();
    sampler_capacity += alloc->GetCapacity();
  }

  Logger::Logf(LogLevel::Info,
    LogCategory::Graphic,
    Logger::Here(),
    "=== Descriptor Heap Statistics ===\nRTV Heap: {}/{}\nDSV Heap: {}/{}\nSRV Heap Static (persistent): {}/{}\nSRV Heap Dynamic (per-frame slices): {}/{}\nSampler Heap (per-frame slices): {}/{}\n==================================",
    rtv_heap_.GetAllocated(),
    rtv_heap_.GetCapacity(),
    dsv_heap_.GetAllocated(),
    dsv_heap_.GetCapacity(),
    srv_static_heap_.GetAllocated(),
    srv_static_heap_.GetCapacity(),
    srv_dyn_allocated,
    srv_dyn_capacity,
    sampler_allocated,
    sampler_capacity);
}
