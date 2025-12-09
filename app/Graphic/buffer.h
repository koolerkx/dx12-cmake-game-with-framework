#pragma once

#include <d3d12.h>

#include <cstdint>

#include "gpu_resource.h"

class Buffer : public GpuResource {
 public:
  enum class Type { Vertex, Index, Constant, Structured, RawBuffer };

  Buffer() = default;
  ~Buffer() override;

  Buffer(const Buffer&) = delete;
  Buffer& operator=(const Buffer&) = delete;

  bool Create(ID3D12Device* device, size_t size, Type type, D3D12_HEAP_TYPE heap_type = D3D12_HEAP_TYPE_UPLOAD);

  void Upload(const void* data, size_t size);

  // View getters
  D3D12_VERTEX_BUFFER_VIEW GetVBV(uint32_t stride) const;
  D3D12_INDEX_BUFFER_VIEW GetIBV(DXGI_FORMAT format) const;
  D3D12_GPU_VIRTUAL_ADDRESS GetGPUAddress() const;

  size_t GetSize() const {
    return size_;
  }
  Type GetType() const {
    return type_;
  }

  bool IsValid() const override {
    return resource_ != nullptr;
  }

 private:
  void* mapped_data_ = nullptr;  // For persistent upload buffers
  size_t size_ = 0;
  Type type_ = Type::Vertex;
  D3D12_HEAP_TYPE heap_type_ = D3D12_HEAP_TYPE_UPLOAD;

  void Cleanup();
};
