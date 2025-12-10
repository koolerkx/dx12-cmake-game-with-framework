#include "upload_context.h"

#include <iostream>

#include "fence_manager.h"

bool UploadContext::Initialize(ID3D12Device* device, ID3D12CommandQueue* command_queue, FenceManager* fence_manager) {
  if (device == nullptr || command_queue == nullptr || fence_manager == nullptr) {
    std::cerr << "[UploadContext] Initialize: invalid argument(s)" << std::endl;
    return false;
  }

  device_ = device;
  command_queue_ = command_queue;
  fence_manager_ = fence_manager;

  HRESULT hr = device_->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&command_allocator_));
  if (FAILED(hr) || command_allocator_ == nullptr) {
    std::cerr << "[UploadContext] Failed to create command allocator (hr=0x" << std::hex << hr << ")" << std::dec << std::endl;
    return false;
  }

  hr = device_->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, command_allocator_.Get(), nullptr, IID_PPV_ARGS(&command_list_));
  if (FAILED(hr) || command_list_ == nullptr) {
    std::cerr << "[UploadContext] Failed to create command list (hr=0x" << std::hex << hr << ")" << std::dec << std::endl;
    return false;
  }

  // CreateCommandList returns an open list; close it so Begin() controls when recording starts.
  hr = command_list_->Close();
  if (FAILED(hr)) {
    std::cerr << "[UploadContext] Failed to close initial command list (hr=0x" << std::hex << hr << ")" << std::dec << std::endl;
    return false;
  }

  initialized_ = true;
  return true;
}

void UploadContext::Begin() {
  if (!initialized_) {
    std::cerr << "[UploadContext] Begin called before Initialize" << std::endl;
    return;
  }

  // Reset allocator then reset command list to enter recording state
  HRESULT hr = command_allocator_->Reset();
  if (FAILED(hr)) {
    std::cerr << "[UploadContext] Failed to reset command allocator (hr=0x" << std::hex << hr << ")" << std::dec << std::endl;
    return;
  }

  hr = command_list_->Reset(command_allocator_.Get(), nullptr);
  if (FAILED(hr)) {
    std::cerr << "[UploadContext] Failed to reset command list (hr=0x" << std::hex << hr << ")" << std::dec << std::endl;
    return;
  }
}

void UploadContext::SubmitAndWait() {
  if (!initialized_) {
    std::cerr << "[UploadContext] SubmitAndWait called before Initialize" << std::endl;
    return;
  }

  if (command_list_ == nullptr) {
    std::cerr << "[UploadContext] SubmitAndWait: command_list_ is null" << std::endl;
    return;
  }

  HRESULT hr = command_list_->Close();
  if (FAILED(hr)) {
    std::cerr << "[UploadContext] Failed to close command list (hr=0x" << std::hex << hr << ")" << std::dec << std::endl;
    // attempt to continue to execute anyway
  }

  ID3D12CommandList* lists[] = {command_list_.Get()};
  command_queue_->ExecuteCommandLists(1, lists);

  // Use FenceManager to wait for GPU completion
  if (fence_manager_ != nullptr) {
    fence_manager_->WaitForGpu(command_queue_.Get());
  } else {
    std::cerr << "[UploadContext] SubmitAndWait: fence_manager_ is null" << std::endl;
  }
}
