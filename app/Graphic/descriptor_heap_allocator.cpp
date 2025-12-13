#include "descriptor_heap_allocator.h"

#include <d3d12.h>

#include <cassert>

#include "Framework/Logging/logger.h"

bool DescriptorHeapAllocator::Initialize(ID3D12Device* device, D3D12_DESCRIPTOR_HEAP_TYPE type, uint32_t capacity, bool shader_visible) {
  assert(device != nullptr);
  assert(capacity > 0);

  type_ = type;
  base_index_ = 0;
  capacity_ = capacity;
  shader_visible_ = shader_visible;
  owns_heap_ = true;
  descriptor_size_ = device->GetDescriptorHandleIncrementSize(type);

  D3D12_DESCRIPTOR_HEAP_DESC heap_desc = {};
  heap_desc.Type = type;
  heap_desc.NumDescriptors = capacity;
  heap_desc.NodeMask = 0;
  heap_desc.Flags = shader_visible_ ? D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE : D3D12_DESCRIPTOR_HEAP_FLAG_NONE;

  HRESULT hr = device->CreateDescriptorHeap(&heap_desc, IID_PPV_ARGS(&heap_));
  if (FAILED(hr)) {
    Logger::Logf(LogLevel::Error,
      LogCategory::Graphic,
      Logger::Here(),
      "[DescriptorHeapAllocator] Initialize: CreateDescriptorHeap failed (hr=0x{:08X}).",
      static_cast<uint32_t>(hr));
    return false;
  }

  heap_start_cpu_ = heap_->GetCPUDescriptorHandleForHeapStart();
  if (shader_visible) {
    heap_start_gpu_ = heap_->GetGPUDescriptorHandleForHeapStart();
  } else {
    heap_start_gpu_.ptr = 0;
  }

  const wchar_t* typeName = nullptr;
  switch (type) {
    case D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV:
      typeName = L"CBV_SRV_UAV";
      break;
    case D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER:
      typeName = L"SAMPLER";
      break;
    case D3D12_DESCRIPTOR_HEAP_TYPE_RTV:
      typeName = L"RTV";
      break;
    case D3D12_DESCRIPTOR_HEAP_TYPE_DSV:
      typeName = L"DSV";
      break;
    default:
      typeName = L"UNKNOWN";
      break;
  }

  std::wstring name = std::wstring(L"DescriptorHeap_") + typeName;
  if (shader_visible) {
    name += L"_ShaderVisible";
  }
  heap_->SetName(name.c_str());

  Reset();
  return true;
}

bool DescriptorHeapAllocator::InitializeFromExistingHeap(ID3D12Device* device,
  ID3D12DescriptorHeap* existing_heap,
  D3D12_DESCRIPTOR_HEAP_TYPE type,
  uint32_t base_index,
  uint32_t capacity,
  bool shader_visible) {
  assert(device != nullptr);
  assert(existing_heap != nullptr);

  type_ = type;
  base_index_ = base_index;
  capacity_ = capacity;
  shader_visible_ = shader_visible;
  owns_heap_ = false;
  descriptor_size_ = device->GetDescriptorHandleIncrementSize(type);

  heap_ = existing_heap;
  heap_start_cpu_ = heap_->GetCPUDescriptorHandleForHeapStart();
  if (shader_visible_) {
    heap_start_gpu_ = heap_->GetGPUDescriptorHandleForHeapStart();
  } else {
    heap_start_gpu_.ptr = 0;
  }

  Reset();
  return true;
}

DescriptorHeapAllocator::Allocation DescriptorHeapAllocator::Allocate(uint32_t count) {
  assert(count > 0);

  // No free block
  if (allocated_ + count > capacity_) {
    Logger::Logf(LogLevel::Error,
      LogCategory::Graphic,
      Logger::Here(),
      "[DescriptorHeapAllocator] Allocate: out of descriptors (requested={}, available={}, free_blocks={}).",
      count,
      (capacity_ - allocated_),
      free_blocks_.size());
    return {};
  }

  Allocation allocation{};
  allocation.count = count;

  auto block = FindBestFitBlock(count);

  // free block found
  if (block != free_blocks_.end()) {
    allocation.index = block->index;
    allocation.cpu = GetCpuHandle(allocation.index);
    allocation.gpu = shader_visible_ ? GetGpuHandle(allocation.index) : D3D12_GPU_DESCRIPTOR_HANDLE{0};

    // remove the best fit from indexing map
    auto size_range = free_blocks_by_size_.equal_range(block->count);
    for (auto it = size_range.first; it != size_range.second; ++it) {
      if (it->second == block) {
        free_blocks_by_size_.erase(it);
        break;
      }
    }

    // put the remaining space back to indexing map
    if (block->count > count) {
      FreeBlock remaining{};
      remaining.index = block->index + count;
      remaining.count = block->count - count;
      block->index = remaining.index;  // inplace update
      block->count = remaining.count;  // inplace update

      free_blocks_by_size_.insert({remaining.count, block});  // update indexing map
    } else {
      free_blocks_.erase(block);
    }

    return allocation;
  }

  allocation.index = allocated_;
  allocation.cpu = GetCpuHandle(allocation.index);
  allocation.gpu = shader_visible_ ? GetGpuHandle(allocation.index) : D3D12_GPU_DESCRIPTOR_HANDLE{0};

  allocated_ += count;

  return allocation;
}

void DescriptorHeapAllocator::Free(const Allocation& allocation) {
  if (!allocation.IsValid() || allocation.count == 0) {
    return;
  }

  FreeBlock new_block{};
  new_block.index = allocation.index;
  new_block.count = allocation.count;

  auto insert_pos = free_blocks_.begin();
  while (insert_pos != free_blocks_.end() && insert_pos->index < new_block.index) {
    ++insert_pos;
  }

  auto inserted_it = free_blocks_.insert(insert_pos, new_block);
  free_blocks_by_size_.insert({new_block.count, inserted_it});

  MergeFreeBlocks();
}

void DescriptorHeapAllocator::Reset() {
  allocated_ = 0;
  free_blocks_.clear();
  free_blocks_by_size_.clear();
}

std::list<DescriptorHeapAllocator::FreeBlock>::iterator DescriptorHeapAllocator::FindBestFitBlock(uint32_t count) {
  auto it = free_blocks_by_size_.lower_bound(count);

  if (it != free_blocks_by_size_.end()) {
    return it->second;
  }

  return free_blocks_.end();
}

void DescriptorHeapAllocator::MergeFreeBlocks() {
  if (free_blocks_.size() < 2) {
    return;
  }

  auto it = free_blocks_.begin();
  while (it != free_blocks_.end()) {
    auto next = std::next(it);

    if (next == free_blocks_.end()) {
      break;
    }

    // Check if current block and next block are adjacent
    if (it->index + it->count == next->index) {
      // Remove both blocks from indexing map
      auto size_range1 = free_blocks_by_size_.equal_range(it->count);
      for (auto size_it = size_range1.first; size_it != size_range1.second; ++size_it) {
        if (size_it->second == it) {
          free_blocks_by_size_.erase(size_it);
          break;
        }
      }

      auto size_range2 = free_blocks_by_size_.equal_range(next->count);
      for (auto size_it = size_range2.first; size_it != size_range2.second; ++size_it) {
        if (size_it->second == next) {
          free_blocks_by_size_.erase(size_it);
          break;
        }
      }

      // Merge blocks
      it->count += next->count;
      free_blocks_.erase(next);

      // Re-insert merged block into size map
      free_blocks_by_size_.insert({it->count, it});

      // Continue checking with the same iterator (in case we can merge more)
    } else {
      ++it;
    }
  }
}

D3D12_CPU_DESCRIPTOR_HANDLE DescriptorHeapAllocator::GetCpuHandle(uint32_t index) const {
  D3D12_CPU_DESCRIPTOR_HANDLE handle = heap_start_cpu_;
  handle.ptr += static_cast<SIZE_T>(base_index_ + index) * descriptor_size_;
  return handle;
}

D3D12_GPU_DESCRIPTOR_HANDLE DescriptorHeapAllocator::GetGpuHandle(uint32_t index) const {
  if (!shader_visible_) {
    return D3D12_GPU_DESCRIPTOR_HANDLE{0};
  }

  D3D12_GPU_DESCRIPTOR_HANDLE handle = heap_start_gpu_;
  handle.ptr += static_cast<UINT64>(base_index_ + index) * descriptor_size_;
  return handle;
}
