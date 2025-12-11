#include "buffer.h"

#include <cassert>
#include <cstdint>
#include <cstring>
#include <iostream>

Buffer::~Buffer() {
  Cleanup();
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

  HRESULT hr = device->CreateCommittedResource(&heap_props, D3D12_HEAP_FLAG_NONE, &buffer_desc, initial_state, nullptr, IID_PPV_ARGS(&resource_));

  if (FAILED(hr)) {
    std::cerr << "Buffer::Create - Failed to create buffer resource" << '\n';
    return false;
  }

  current_state_ = initial_state;

  // Map persistent upload buffers
  if (heap_type == D3D12_HEAP_TYPE_UPLOAD) {
    hr = resource_->Map(0, nullptr, &mapped_data_);
    if (FAILED(hr)) {
      std::cerr << "Buffer::Create - Failed to map upload buffer" << '\n';
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
    std::cerr << "Buffer::Upload - Buffer is not mapped. Only upload heap buffers can be uploaded directly" << '\n';
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