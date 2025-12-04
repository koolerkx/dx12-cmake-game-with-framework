#include "fence_manager.h"

#include <cassert>
#include <iostream>

bool FenceManager::Initialize(ID3D12Device* device) {
  assert(device != nullptr);

  HRESULT hr = device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence_));
  if (FAILED(hr)) {
    std::cerr << "[FenceManager] Failed to create fence." << '\n';
    return false;
  }

  fence_value_ = 1;

  fence_event_ = CreateEvent(nullptr, FALSE, FALSE, nullptr);
  if (fence_event_ == nullptr) {
    std::cerr << "[FenceManager] Failed to create fence event." << '\n';
    return false;
  }

  return true;
}

void FenceManager::SignalFence(ID3D12CommandQueue* command_queue) {
  assert(command_queue != nullptr);
  assert(fence_ != nullptr);

  command_queue->Signal(fence_.Get(), fence_value_);
  fence_value_++;
}

void FenceManager::WaitForFenceValue(UINT64 fence_value) {
  assert(fence_ != nullptr);
  assert(fence_event_ != nullptr);

  if (fence_->GetCompletedValue() < fence_value) {
    fence_->SetEventOnCompletion(fence_value, fence_event_);
    WaitForSingleObject(fence_event_, INFINITE);
  }
}

void FenceManager::WaitForGpu(ID3D12CommandQueue* command_queue) {
  assert(command_queue != nullptr);
  assert(fence_ != nullptr);
  assert(fence_event_ != nullptr);

  const UINT64 current_fence_value = fence_value_;
  command_queue->Signal(fence_.Get(), current_fence_value);
  fence_value_++;

  if (fence_->GetCompletedValue() < current_fence_value) {
    fence_->SetEventOnCompletion(current_fence_value, fence_event_);
    WaitForSingleObject(fence_event_, INFINITE);
  }
}

void FenceManager::ShutDown() {
  if (fence_event_ != nullptr) {
    CloseHandle(fence_event_);
    fence_event_ = nullptr;
  }
  fence_.Reset();
}
