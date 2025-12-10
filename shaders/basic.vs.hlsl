#include "basic_type.hlsli"

cbuffer PerObjectConstants : register(b0) { float4x4 world_pos; };

BasicType main(VSIN input) {
  BasicType output;
  float4 posW = input.pos;
  posW = mul(posW, view);
  output.svpos = mul(posW, proj);

  output.uv = input.uv;
  return output;
}