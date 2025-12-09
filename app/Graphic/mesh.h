#pragma once

#include <d3d12.h>
#include <dxgiformat.h>

#include <cstdint>
#include <string>

#include "buffer.h"

// Simple mesh representation with vertex and index buffers
class Mesh {
 public:
  Mesh() = default;
  ~Mesh() = default;

  Mesh(const Mesh&) = delete;
  Mesh& operator=(const Mesh&) = delete;

  // Initialize with existing buffers
  void Initialize(Buffer* vertex_buffer,
    Buffer* index_buffer,
    uint32_t vertex_stride,
    uint32_t index_count,
    DXGI_FORMAT index_format = DXGI_FORMAT_R16_UINT,
    D3D_PRIMITIVE_TOPOLOGY topology = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

  // Bind mesh for rendering
  void Bind(ID3D12GraphicsCommandList* command_list) const;

  // Draw mesh
  void Draw(ID3D12GraphicsCommandList* command_list) const;

  // Getters
  uint32_t GetIndexCount() const {
    return index_count_;
  }

  uint32_t GetVertexStride() const {
    return vertex_stride_;
  }

  D3D_PRIMITIVE_TOPOLOGY GetTopology() const {
    return topology_;
  }

  bool IsValid() const {
    return vertex_buffer_ != nullptr && index_buffer_ != nullptr && index_count_ > 0;
  }

  void SetDebugName(const std::string& name) {
    debug_name_ = name;
  }

  const std::string& GetDebugName() const {
    return debug_name_;
  }

 private:
  Buffer* vertex_buffer_ = nullptr;
  Buffer* index_buffer_ = nullptr;

  uint32_t vertex_stride_ = 0;
  uint32_t index_count_ = 0;
  DXGI_FORMAT index_format_ = DXGI_FORMAT_R16_UINT;
  D3D_PRIMITIVE_TOPOLOGY topology_ = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;

  std::string debug_name_;
};
