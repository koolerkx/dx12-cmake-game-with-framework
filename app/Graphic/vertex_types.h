#pragma once

#include <DirectXMath.h>
#include <d3d12.h>

#include <span>

struct VertexPositionTexture2D {
  DirectX::XMFLOAT3 position;
  DirectX::XMFLOAT2 texcoord;
};

std::span<const D3D12_INPUT_ELEMENT_DESC> GetInputLayout_VertexPositionTexture2D();

// Debug vertex structure for debug lines and wireframes
struct DebugVertex {
  DirectX::XMFLOAT3 position;
  uint32_t          color; // RGBA8
};

std::span<const D3D12_INPUT_ELEMENT_DESC> GetInputLayout_DebugVertex();

// Debug vertex structure for 2D UI debug visuals (screen-space)
struct DebugVertex2D {
  DirectX::XMFLOAT2 position;
  uint32_t          color; // RGBA8
};

std::span<const D3D12_INPUT_ELEMENT_DESC> GetInputLayout_DebugVertex2D();

// Unit segment vertex for instanced debug line rendering
// Represents a line segment from (-0.5, 0, 0) to (0.5, 0, 0) in local space
struct DebugUnitSegmentVertex {
  DirectX::XMFLOAT3 position;
};

// Per-instance data for debug line rendering
// Each instance represents a single line segment with its own transform and color
struct DebugLineInstanceData {
  DirectX::XMFLOAT4X4 world;  // Row-major transformation matrix
  uint32_t color;             // RGBA8 color
  uint32_t _pad[3];           // Padding to 16-byte alignment (total 80 bytes)
};

// Input layout for instanced debug line rendering
// Slot 0: per-vertex unit segment position (POSITION semantic)
// Slot 1: per-instance data with the following semantics:
//   - WORLD0-WORLD3: 4 consecutive float4 semantics forming a row-major 4x4 matrix (64 bytes)
//   - COLOR: R8G8B8A8_UNORM auto-normalized to float4 (4 bytes)
// Total instance stride: 80 bytes (64 + 4 + 12 padding)
std::span<const D3D12_INPUT_ELEMENT_DESC> GetInputLayout_DebugLineInstanced();
