#pragma once

#include <d3d12.h>

#include "Framework/types.h"

class FenceManager;

class UploadContext {
 public:
  UploadContext() = default;
  ~UploadContext() = default;

  UploadContext(const UploadContext&) = delete;
  UploadContext& operator=(const UploadContext&) = delete;

  UploadContext(UploadContext&&) = default;
  UploadContext& operator=(UploadContext&&) = default;

  // Initialize with device, command queue and fence manager (for waiting)
  bool Initialize(ID3D12Device* device, ID3D12CommandQueue* command_queue, FenceManager* fence_manager);

  // Prepare a new one-shot command allocator / command list for recording uploads
  void Begin();

  // Returns the currently open command list. Valid only after Begin().
  ID3D12GraphicsCommandList* GetCommandList() const {
    return command_list_.Get();
  }

  // Close, execute and wait for completion
  void SubmitAndWait();

  bool IsInitialized() const {
    return initialized_;
  }

 private:
  bool initialized_ = false;

  ComPtr<ID3D12Device> device_;
  ComPtr<ID3D12CommandQueue> command_queue_;
  FenceManager* fence_manager_ = nullptr;

  ComPtr<ID3D12CommandAllocator> command_allocator_;
  ComPtr<ID3D12GraphicsCommandList> command_list_;
};
