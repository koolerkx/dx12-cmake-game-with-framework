#include "basic_type.hlsli"

cbuffer PerObjectConstants : register(b0) { float4x4 world_pos; };

BasicType main(VSIN input) {
  BasicType output;
  output.svpos = mul(world_pos, input.pos);
  output.uv = input.uv;
  return output;
}