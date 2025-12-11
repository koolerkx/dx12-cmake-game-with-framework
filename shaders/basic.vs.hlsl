#include "basic_type.hlsli"

cbuffer PerObjectWorldPos : register(b0) { float4x4 world_pos; };
cbuffer PerObjectColor : register(b2) { float4 color_tint; };
cbuffer PerObjectUV : register(b3) { float4 uv_transform; };

BasicType main(VSIN input) {
  BasicType output;
  float4 posW = mul(world_pos, float4(input.pos, 1.0f));  // Convert float3 to float4
  posW = mul(posW, view);
  output.svpos = mul(posW, proj);

  // Apply per-object UV transform (offset.xy, scale.xy)
  output.uv = input.uv * uv_transform.zw + uv_transform.xy;
  output.color = color_tint;
  return output;
}