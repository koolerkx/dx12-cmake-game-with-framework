#include "mesh.h"

#include <cassert>

void Mesh::Initialize(std::shared_ptr<Buffer> vertex_buffer,
  std::shared_ptr<Buffer> index_buffer,
  uint32_t vertex_stride,
  uint32_t index_count,
  DXGI_FORMAT index_format,
  D3D_PRIMITIVE_TOPOLOGY topology) {
  assert(vertex_buffer != nullptr);
  assert(index_buffer != nullptr);
  assert(vertex_stride > 0);
  assert(index_count > 0);

  vertex_buffer_ = vertex_buffer;
  index_buffer_ = index_buffer;
  vertex_stride_ = vertex_stride;
  index_count_ = index_count;
  index_format_ = index_format;
  topology_ = topology;
}

void Mesh::Bind(ID3D12GraphicsCommandList* command_list) const {
  assert(command_list != nullptr);
  assert(IsValid());

  // Set topology
  command_list->IASetPrimitiveTopology(topology_);

  // Bind vertex buffer
  D3D12_VERTEX_BUFFER_VIEW vbv = vertex_buffer_->GetVBV(vertex_stride_);
  command_list->IASetVertexBuffers(0, 1, &vbv);

  // Bind index buffer
  D3D12_INDEX_BUFFER_VIEW ibv = index_buffer_->GetIBV(index_format_);
  command_list->IASetIndexBuffer(&ibv);
}

void Mesh::Draw(ID3D12GraphicsCommandList* command_list) const {
  assert(command_list != nullptr);
  assert(IsValid());

  command_list->DrawIndexedInstanced(index_count_, 1, 0, 0, 0);
}
