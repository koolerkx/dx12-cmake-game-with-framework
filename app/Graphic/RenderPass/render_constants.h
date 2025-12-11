// Helper functions for setting per-object constants and frame constant buffer
#pragma once

#include <d3d12.h>
#include <DirectXMath.h>

#include "buffer.h"

namespace RenderHelpers {
// Root signature layout for Sprite2D material (DefaultSprite2D):
// param[0] = b0 (world matrix)       - 16 x 32-bit constants
// param[1] = b2 (color tint)         - 4 x 32-bit constants
// param[2] = b3 (UV transform)       - 4 x 32-bit constants
// param[3] = b1 (frame CB)           - CBV
// param[4] = t0 (texture)            - descriptor table

inline void SetPerObjectConstants(ID3D12GraphicsCommandList* cmd, const DirectX::XMFLOAT4X4& world, const DirectX::XMFLOAT4& color, const DirectX::XMFLOAT4& uv_transform) {
  // world is 16 floats (4x4 matrix), packed as row-major; root param index 0 (b0)
  cmd->SetGraphicsRoot32BitConstants(0, 16, &world, 0);
  // color is 4 floats; root parameter index 1 (b2)
  cmd->SetGraphicsRoot32BitConstants(1, 4, &color, 0);
  // uv_transform is 4 floats; root parameter index 2 (b3)
  cmd->SetGraphicsRoot32BitConstants(2, 4, &uv_transform, 0);
}

inline void SetFrameConstants(ID3D12GraphicsCommandList* cmd, const Buffer& frame_cb) {
  // Frame CB is bound to root parameter index 3 (b1)
  if (frame_cb.IsValid()) {
    cmd->SetGraphicsRootConstantBufferView(3, frame_cb.GetGPUAddress());
  }
}

inline void SetFrameConstants(ID3D12GraphicsCommandList* cmd, D3D12_GPU_VIRTUAL_ADDRESS cb_address) {
  // Frame CB is bound to root parameter index 3 (b1)
  if (cb_address != 0) {
    cmd->SetGraphicsRootConstantBufferView(3, cb_address);
  }
}
}  // namespace RenderHelpers
