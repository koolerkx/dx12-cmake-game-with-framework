#include "buffer.h"

#include <cassert>
#include <cstdint>
#include <cstring>

#include "Framework/Logging/logger.h"

#include "Framework/utils.h"
#include "upload_context.h"

Buffer::~Buffer() {
  Cleanup();
}

std::shared_ptr<Buffer> Buffer::CreateAndUploadToDefaultHeapForInit(
  ID3D12Device* device, UploadContext& upload_context, const void* data, size_t size_in_bytes, Type type, const std::string& debug_name) {
  if (device == nullptr) {
    Logger::Log(LogLevel::Error, LogCategory::Validation, "Buffer::CreateAndUploadToDefaultHeapForInit: device is null.");
    return nullptr;
  }
  if (!upload_context.IsInitialized()) {
    Logger::Log(LogLevel::Error,
      LogCategory::Validation,
      "Buffer::CreateAndUploadToDefaultHeapForInit: upload_context is not initialized.");
    return nullptr;
  }
  if (data == nullptr || size_in_bytes == 0) {
    Logger::Log(LogLevel::Error, LogCategory::Validation, "Buffer::CreateAndUploadToDefaultHeapForInit: invalid data/size.");
    return nullptr;
  }
  if (type != Type::Vertex && type != Type::Index) {
    Logger::Log(LogLevel::Error,
      LogCategory::Validation,
      "Buffer::CreateAndUploadToDefaultHeapForInit: only Vertex/Index are supported in init-only helper.");
    return nullptr;
  }

  auto result = std::make_shared<Buffer>();
  result->Cleanup();

  result->size_ = size_in_bytes;
  result->type_ = type;
  result->heap_type_ = D3D12_HEAP_TYPE_DEFAULT;
  result->mapped_data_ = nullptr;

  // Create DEFAULT heap buffer in COPY_DEST
  D3D12_HEAP_PROPERTIES default_heap_props = {};
  default_heap_props.Type = D3D12_HEAP_TYPE_DEFAULT;

  D3D12_RESOURCE_DESC buffer_desc = {};
  buffer_desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
  buffer_desc.Width = result->size_;
  buffer_desc.Height = 1;
  buffer_desc.DepthOrArraySize = 1;
  buffer_desc.MipLevels = 1;
  buffer_desc.Format = DXGI_FORMAT_UNKNOWN;
  buffer_desc.SampleDesc.Count = 1;
  buffer_desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
  buffer_desc.Flags = D3D12_RESOURCE_FLAG_NONE;

  ComPtr<ID3D12Resource> default_resource;
  HRESULT hr = device->CreateCommittedResource(&default_heap_props,
    D3D12_HEAP_FLAG_NONE,
    &buffer_desc,
    D3D12_RESOURCE_STATE_COPY_DEST,
    nullptr,
    IID_PPV_ARGS(default_resource.GetAddressOf()));

  if (FAILED(hr) || default_resource == nullptr) {
    Logger::Logf(LogLevel::Error,
      LogCategory::Resource,
      Logger::Here(),
      "Buffer::CreateAndUploadToDefaultHeapForInit: failed to create DEFAULT buffer (hr=0x{:08X}).",
      static_cast<uint32_t>(FAILED(hr) ? hr : E_FAIL));
    return nullptr;
  }

  result->SetResource(default_resource, D3D12_RESOURCE_STATE_COPY_DEST);

  if (!debug_name.empty()) {
    result->SetDebugName(debug_name);
  }

  // Create staging UPLOAD heap buffer
  D3D12_HEAP_PROPERTIES upload_heap_props = {};
  upload_heap_props.Type = D3D12_HEAP_TYPE_UPLOAD;

  ComPtr<ID3D12Resource> staging_resource;
  hr = device->CreateCommittedResource(&upload_heap_props,
    D3D12_HEAP_FLAG_NONE,
    &buffer_desc,
    D3D12_RESOURCE_STATE_GENERIC_READ,
    nullptr,
    IID_PPV_ARGS(staging_resource.GetAddressOf()));

  if (FAILED(hr) || staging_resource == nullptr) {
    Logger::Logf(LogLevel::Error,
      LogCategory::Resource,
      Logger::Here(),
      "Buffer::CreateAndUploadToDefaultHeapForInit: failed to create UPLOAD staging buffer (hr=0x{:08X}).",
      static_cast<uint32_t>(FAILED(hr) ? hr : E_FAIL));
    return nullptr;
  }

  if (!debug_name.empty()) {
    staging_resource->SetName(Utf8ToWstring(debug_name + "_Staging").c_str());
  }

  // Copy CPU -> staging
  void* mapped = nullptr;
  hr = staging_resource->Map(0, nullptr, &mapped);
  if (FAILED(hr) || mapped == nullptr) {
    Logger::Logf(LogLevel::Error,
      LogCategory::Resource,
      Logger::Here(),
      "Buffer::CreateAndUploadToDefaultHeapForInit: failed to map staging buffer (hr=0x{:08X}).",
      static_cast<uint32_t>(FAILED(hr) ? hr : E_FAIL));
    return nullptr;
  }
  std::memcpy(mapped, data, size_in_bytes);
  staging_resource->Unmap(0, nullptr);

  // Atomic blocking submission: Begin -> Record -> SubmitAndWait
  upload_context.Begin();
  ID3D12GraphicsCommandList* cmd = upload_context.GetCommandList();
  if (cmd == nullptr) {
    Logger::Log(LogLevel::Error,
      LogCategory::Validation,
      "Buffer::CreateAndUploadToDefaultHeapForInit: upload_context returned null command list.");
    return nullptr;
  }

  cmd->CopyBufferRegion(default_resource.Get(), 0, staging_resource.Get(), 0, size_in_bytes);

  const D3D12_RESOURCE_STATES final_state =
    (type == Type::Vertex) ? D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER : D3D12_RESOURCE_STATE_INDEX_BUFFER;
  result->TransitionTo(cmd, final_state);

  // INITIALIZATION ONLY: blocking wait makes staging lifetime safe to release on return.
  upload_context.SubmitAndWait();

  return result;
}

bool Buffer::Create(ID3D12Device* device, size_t size, Type type, D3D12_HEAP_TYPE heap_type) {
  assert(device != nullptr);
  assert(size > 0);

  Cleanup();

  size_ = size;
  type_ = type;
  heap_type_ = heap_type;

  // Align constant buffer size to 256 bytes
  if (type == Type::Constant) {
    size_ = (size + 255) & ~255;
  }

  D3D12_HEAP_PROPERTIES heap_props = {};
  heap_props.Type = heap_type;

  D3D12_RESOURCE_DESC buffer_desc = {};
  buffer_desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
  buffer_desc.Width = size_;
  buffer_desc.Height = 1;
  buffer_desc.DepthOrArraySize = 1;
  buffer_desc.MipLevels = 1;
  buffer_desc.Format = DXGI_FORMAT_UNKNOWN;
  buffer_desc.SampleDesc.Count = 1;
  buffer_desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
  buffer_desc.Flags = D3D12_RESOURCE_FLAG_NONE;

  // Allow UAV for structured/raw buffers if needed
  if (type == Type::Structured || type == Type::RawBuffer) {
    buffer_desc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
  }

  D3D12_RESOURCE_STATES initial_state;
  if (heap_type == D3D12_HEAP_TYPE_UPLOAD) {
    initial_state = D3D12_RESOURCE_STATE_GENERIC_READ;
  } else if (heap_type == D3D12_HEAP_TYPE_READBACK) {
    initial_state = D3D12_RESOURCE_STATE_COPY_DEST;
  } else {
    initial_state = D3D12_RESOURCE_STATE_COMMON;
  }

  HRESULT hr =
    device->CreateCommittedResource(&heap_props, D3D12_HEAP_FLAG_NONE, &buffer_desc, initial_state, nullptr, IID_PPV_ARGS(&resource_));

  if (FAILED(hr)) {
    Logger::Logf(LogLevel::Error,
      LogCategory::Resource,
      Logger::Here(),
      "Buffer::Create: CreateCommittedResource failed (hr=0x{:08X}).",
      static_cast<uint32_t>(hr));
    return false;
  }

  current_state_ = initial_state;

  // Map persistent upload buffers
  if (heap_type == D3D12_HEAP_TYPE_UPLOAD) {
    hr = resource_->Map(0, nullptr, &mapped_data_);
    if (FAILED(hr)) {
      Logger::Logf(LogLevel::Error,
        LogCategory::Resource,
        Logger::Here(),
        "Buffer::Create: Map failed for upload heap buffer (hr=0x{:08X}).",
        static_cast<uint32_t>(hr));
      Cleanup();
      return false;
    }
  }

  return true;
}

void Buffer::Upload(const void* data, size_t size) {
  Upload(data, size, 0);
}

void Buffer::Upload(const void* data, size_t size, size_t offset) {
  assert(data != nullptr);
  assert(offset <= size_);
  assert(size <= size_);
  assert(offset + size <= size_);

  if (mapped_data_ == nullptr) {
    Logger::Log(LogLevel::Error,
      LogCategory::Validation,
      "Buffer::Upload: buffer is not mapped (only upload heap buffers can be uploaded directly)." );
    return;
  }

  auto* dst = reinterpret_cast<std::uint8_t*>(mapped_data_) + offset;
  std::memcpy(dst, data, size);
}

D3D12_VERTEX_BUFFER_VIEW Buffer::GetVBV(uint32_t stride) const {
  assert(type_ == Type::Vertex);
  assert(resource_ != nullptr);

  D3D12_VERTEX_BUFFER_VIEW vbv = {};
  vbv.BufferLocation = resource_->GetGPUVirtualAddress();
  vbv.SizeInBytes = static_cast<UINT>(size_);
  vbv.StrideInBytes = stride;

  return vbv;
}

D3D12_INDEX_BUFFER_VIEW Buffer::GetIBV(DXGI_FORMAT format) const {
  assert(type_ == Type::Index);
  assert(resource_ != nullptr);

  D3D12_INDEX_BUFFER_VIEW ibv = {};
  ibv.BufferLocation = resource_->GetGPUVirtualAddress();
  ibv.SizeInBytes = static_cast<UINT>(size_);
  ibv.Format = format;

  return ibv;
}

D3D12_GPU_VIRTUAL_ADDRESS Buffer::GetGPUAddress() const {
  if (resource_ != nullptr) {
    return resource_->GetGPUVirtualAddress();
  }
  return 0;
}

void Buffer::Cleanup() {
  if (mapped_data_ != nullptr && resource_ != nullptr) {
    resource_->Unmap(0, nullptr);
    mapped_data_ = nullptr;
  }

  resource_.Reset();
  size_ = 0;
  current_state_ = D3D12_RESOURCE_STATE_COMMON;
}
