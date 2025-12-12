#pragma once

#include <basetsd.h>

#include "d3d12.h"
#include "types.h"

class FenceManager {
 public:
  FenceManager() = default;
  ~FenceManager() {
    ShutDown();
  }

  FenceManager(const FenceManager&) = delete;
  FenceManager& operator=(const FenceManager&) = delete;

  bool Initialize(ID3D12Device* device);
  void WaitForGpu(ID3D12CommandQueue* command_queue);
  void SignalFence(ID3D12CommandQueue* command_queue);

  // Wait for a specific fence value to complete.
  // Intended for frame-slot reuse (frames-in-flight), not as a global GPU flush.
  void WaitForFenceValue(UINT64 fence_value);

  UINT64 GetCurrentFenceValue() const {
    return fence_value_;
  }

  UINT64 GetCompletedFenceValue() const {
    return (fence_ != nullptr) ? fence_->GetCompletedValue() : 0;
  }

  bool IsValid() const {
    return fence_ != nullptr && fence_event_ != nullptr;
  }

 private:
  ComPtr<ID3D12Fence> fence_ = nullptr;
  HANDLE fence_event_ = nullptr;
  UINT64 fence_value_ = 0;

  void ShutDown();
};
