#include "vertex_types.h"

static const D3D12_INPUT_ELEMENT_DESC s_inputLayout_VertexPositionTexture2D[] = {
  {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
  {"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
};

std::span<const D3D12_INPUT_ELEMENT_DESC> GetInputLayout_VertexPositionTexture2D() {
  return std::span<const D3D12_INPUT_ELEMENT_DESC>(s_inputLayout_VertexPositionTexture2D, std::size(s_inputLayout_VertexPositionTexture2D));
}
