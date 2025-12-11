//==============================================================================
// basic.vs.hlsl
// 
// Purpose: Vertex shader for basic sprite rendering with color tint and UV transform
// Material: DefaultSprite2D
// 
// Features:
// - World-View-Projection transformation
// - Per-object color tint
// - Per-object UV transform (scale and offset)
//==============================================================================

#include "basic_type.hlsli"

cbuffer PerObjectWorldPos : register(b0) { float4x4 world_pos; };
cbuffer PerObjectColor : register(b2) { float4 color_tint; };
cbuffer PerObjectUV : register(b3) { float4 uv_transform; };  // (offset.xy, scale.xy)

BasicType main(VSIN input) {
  BasicType output;
  
  // Transform position: Local -> World -> View -> Projection
  float4 posW = mul(world_pos, float4(input.pos, 1.0f));
  posW = mul(posW, view);
  output.svpos = mul(posW, proj);

  // Apply UV transform: uv' = uv * scale + offset
  output.uv = input.uv * uv_transform.zw + uv_transform.xy;
  
  // Pass through color tint
  output.color = color_tint;
  
  return output;
}