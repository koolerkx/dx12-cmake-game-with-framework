#include "upload_context.h"

#include "Framework/Error/error_helpers.h"
#include "Framework/Logging/logger.h"

#include "fence_manager.h"

bool UploadContext::Initialize(ID3D12Device* device, ID3D12CommandQueue* command_queue, FenceManager* fence_manager) {
  if (device == nullptr || command_queue == nullptr || fence_manager == nullptr) {
    Logger::Log(LogLevel::Error, LogCategory::Validation, "[UploadContext] Initialize: invalid argument(s)." );
    return false;
  }

  device_ = device;
  command_queue_ = command_queue;
  fence_manager_ = fence_manager;

  HRESULT hr = device_->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&command_allocator_));
  if (FAILED(hr) || command_allocator_ == nullptr) {
    (void)ReturnIfFailed(FAILED(hr) ? hr : E_FAIL,
      FrameworkErrorCode::UploadContextInitFailed,
      "UploadContext::CreateCommandAllocator");
    return false;
  }

  hr = device_->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, command_allocator_.Get(), nullptr, IID_PPV_ARGS(&command_list_));
  if (FAILED(hr) || command_list_ == nullptr) {
    (void)ReturnIfFailed(FAILED(hr) ? hr : E_FAIL, FrameworkErrorCode::UploadContextInitFailed, "UploadContext::CreateCommandList");
    return false;
  }

  // CreateCommandList returns an open list; close it so Begin() controls when recording starts.
  hr = command_list_->Close();
  if (FAILED(hr)) {
    (void)ReturnIfFailed(hr, FrameworkErrorCode::UploadContextInitFailed, "UploadContext::CloseInitialCommandList");
    return false;
  }

  initialized_ = true;
  return true;
}

// This helper assumes exclusive ownership of UploadContext for the duration of the call.
void UploadContext::Begin() {
  if (!initialized_) {
    Logger::Log(LogLevel::Warn, LogCategory::Validation, "[UploadContext] Begin called before Initialize.");
    return;
  }

  // Reset allocator then reset command list to enter recording state
  HRESULT hr = command_allocator_->Reset();
  if (FAILED(hr)) {
    Logger::Logf(LogLevel::Error,
      LogCategory::Resource,
      Logger::Here(),
      "[UploadContext] Command allocator reset failed (hr=0x{:08X}).",
      static_cast<uint32_t>(hr));
    return;
  }

  hr = command_list_->Reset(command_allocator_.Get(), nullptr);
  if (FAILED(hr)) {
    Logger::Logf(LogLevel::Error,
      LogCategory::Resource,
      Logger::Here(),
      "[UploadContext] Command list reset failed (hr=0x{:08X}).",
      static_cast<uint32_t>(hr));
    return;
  }
}

void UploadContext::SubmitAndWait() {
  if (!initialized_) {
    Logger::Log(LogLevel::Warn, LogCategory::Validation, "[UploadContext] SubmitAndWait called before Initialize.");
    return;
  }

  if (command_list_ == nullptr) {
    Logger::Log(LogLevel::Error, LogCategory::Validation, "[UploadContext] SubmitAndWait: command_list_ is null.");
    return;
  }

  HRESULT hr = command_list_->Close();
  if (FAILED(hr)) {
    Logger::Logf(LogLevel::Error,
      LogCategory::Resource,
      Logger::Here(),
      "[UploadContext] Command list close failed (hr=0x{:08X}).",
      static_cast<uint32_t>(hr));
    // attempt to continue to execute anyway
  }

  ID3D12CommandList* lists[] = {command_list_.Get()};
  command_queue_->ExecuteCommandLists(1, lists);

  // Use FenceManager to wait for GPU completion
  if (fence_manager_ != nullptr) {
    fence_manager_->WaitForGpu(command_queue_.Get());
  } else {
    Logger::Log(LogLevel::Error, LogCategory::Validation, "[UploadContext] SubmitAndWait: fence_manager_ is null.");
  }
}
