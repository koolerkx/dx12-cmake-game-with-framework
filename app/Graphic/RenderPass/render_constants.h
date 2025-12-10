// Helper functions for setting per-object constants and frame constant buffer
#pragma once

#include <d3d12.h>
#include <DirectXMath.h>

#include "buffer.h"

namespace RenderHelpers {
inline void SetPerObjectConstants(ID3D12GraphicsCommandList* cmd, const DirectX::XMFLOAT4X4& world, const DirectX::XMFLOAT4& color, const DirectX::XMFLOAT4& uv_transform) {
  // world is 16 floats (4x4 matrix), packed as row-major; root param index 0 is reserved for per-object constants
  cmd->SetGraphicsRoot32BitConstants(0, 16, &world, 0);
  // color is 4 floats; root parameter index 1 is reserved for per-object color
  cmd->SetGraphicsRoot32BitConstants(1, 4, &color, 0);
  // uv_transform is 4 floats; root parameter index 2 is reserved for per-object UV transform
  cmd->SetGraphicsRoot32BitConstants(2, 4, &uv_transform, 0);
}

inline void SetFrameConstants(ID3D12GraphicsCommandList* cmd, const Buffer& frame_cb) {
  // Frame CB is bound to root parameter index 3
  if (frame_cb.IsValid()) {
    cmd->SetGraphicsRootConstantBufferView(3, frame_cb.GetGPUAddress());
  }
}
}  // namespace RenderHelpers
