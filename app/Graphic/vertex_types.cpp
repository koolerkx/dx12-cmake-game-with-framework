#include "vertex_types.h"

static const D3D12_INPUT_ELEMENT_DESC s_inputLayout_VertexPositionTexture2D[] = {
  {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
  {"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
};

std::span<const D3D12_INPUT_ELEMENT_DESC> GetInputLayout_VertexPositionTexture2D() {
  return std::span<const D3D12_INPUT_ELEMENT_DESC>(s_inputLayout_VertexPositionTexture2D, std::size(s_inputLayout_VertexPositionTexture2D));
}

static const D3D12_INPUT_ELEMENT_DESC s_inputLayout_DebugVertex[] = {
  {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
  {"COLOR", 0, DXGI_FORMAT_R8G8B8A8_UNORM, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
};

std::span<const D3D12_INPUT_ELEMENT_DESC> GetInputLayout_DebugVertex() {
  return std::span<const D3D12_INPUT_ELEMENT_DESC>(s_inputLayout_DebugVertex, std::size(s_inputLayout_DebugVertex));
}

static const D3D12_INPUT_ELEMENT_DESC s_inputLayout_DebugVertex2D[] = {
  {"POSITION", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
  {"COLOR", 0, DXGI_FORMAT_R8G8B8A8_UNORM, 0, 8, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
};

std::span<const D3D12_INPUT_ELEMENT_DESC> GetInputLayout_DebugVertex2D() {
  return std::span<const D3D12_INPUT_ELEMENT_DESC>(s_inputLayout_DebugVertex2D, std::size(s_inputLayout_DebugVertex2D));
}

// Input layout for instanced debug line rendering
// World matrix is represented as 4 separate WORLD semantics (WORLD0-WORLD3),
// each a FLOAT4 (16 bytes), forming a complete row-major float4x4 matrix (64 bytes total)
// This matches D3D12 input layout requirements for matrix instancing
static const D3D12_INPUT_ELEMENT_DESC s_inputLayout_DebugLineInstanced[] = {
  // Slot 0: Unit segment vertex (per-vertex)
  {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
  
  // Slot 1: Per-instance data (instancing step rate = 1)
  // World matrix as a single float4x4 (64 bytes)
  {"WORLD", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, 0,  D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA, 1},
  {"WORLD", 1, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, 16, D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA, 1},
  {"WORLD", 2, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, 32, D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA, 1},
  {"WORLD", 3, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, 48, D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA, 1},
  
  // Color (R8G8B8A8_UNORM automatically normalized to float4 in shader)
  {"COLOR", 0, DXGI_FORMAT_R8G8B8A8_UNORM, 1, 64, D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA, 1},
};

std::span<const D3D12_INPUT_ELEMENT_DESC> GetInputLayout_DebugLineInstanced() {
  return std::span<const D3D12_INPUT_ELEMENT_DESC>(s_inputLayout_DebugLineInstanced, std::size(s_inputLayout_DebugLineInstanced));
}
