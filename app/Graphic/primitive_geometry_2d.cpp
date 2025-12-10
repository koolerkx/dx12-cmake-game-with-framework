#include "primitive_geometry_2d.h"

#include <array>

#include "buffer.h"
#include "vertex_types.h"

PrimitiveGeometry2D::PrimitiveGeometry2D(ID3D12Device* device) : device_(device) {
}

std::shared_ptr<Mesh> PrimitiveGeometry2D::CreateRect() {
  using V = VertexPositionTexture2D;

  // Rectangle centered at origin with extents [-0.5, 0.5]
  std::array<V, 4> vertices = {
    V{{-0.5f, 0.5f, 0.0f}, {0.0f, 0.0f}},
    V{{0.5f, 0.5f, 0.0f}, {1.0f, 0.0f}},
    V{{0.5f, -0.5f, 0.0f}, {1.0f, 1.0f}},
    V{{-0.5f, -0.5f, 0.0f}, {0.0f, 1.0f}},
  };

  std::array<uint16_t, 6> indices = {0, 1, 2, 0, 2, 3};

  auto vertex_buffer = std::make_shared<Buffer>();
  if (!vertex_buffer->Create(device_, sizeof(vertices), Buffer::Type::Vertex)) {
    return nullptr;
  }
  vertex_buffer->Upload(vertices.data(), sizeof(vertices));
  vertex_buffer->SetDebugName("Rect2D_VertexBuffer");

  auto index_buffer = std::make_shared<Buffer>();
  if (!index_buffer->Create(device_, sizeof(indices), Buffer::Type::Index)) {
    return nullptr;
  }
  index_buffer->Upload(indices.data(), sizeof(indices));
  index_buffer->SetDebugName("Rect2D_IndexBuffer");

  auto mesh = std::make_shared<Mesh>();
  mesh->Initialize(vertex_buffer, index_buffer, sizeof(V), 6, DXGI_FORMAT_R16_UINT, D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
  mesh->SetDebugName("Rect2D");

  return mesh;
}
