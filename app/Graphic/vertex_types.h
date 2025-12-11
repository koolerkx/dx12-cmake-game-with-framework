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
