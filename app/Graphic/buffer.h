#pragma once

#include <d3d12.h>

#include <cstdint>
#include <memory>
#include <string>

#include "gpu_resource.h"

class UploadContext;

class Buffer : public GpuResource {
 public:
  enum class Type { Vertex, Index, Constant, Structured, RawBuffer };

  Buffer() = default;
  ~Buffer() override;

  Buffer(const Buffer&) = delete;
  Buffer& operator=(const Buffer&) = delete;

  bool Create(ID3D12Device* device, size_t size, Type type, D3D12_HEAP_TYPE heap_type = D3D12_HEAP_TYPE_UPLOAD);

  // BLOCKING helper. Handles staging lifetime internally.
  // INITIALIZATION ONLY: calls SubmitAndWait(); do not use for runtime/streaming.
  static std::shared_ptr<Buffer> CreateAndUploadToDefaultHeapForInit(
    ID3D12Device* device,
    UploadContext& upload_context,
    const void* data,
    size_t size_in_bytes,
    Type type,
    const std::string& debug_name = "");

  void Upload(const void* data, size_t size);

  void Upload(const void* data, size_t size, size_t offset);

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
