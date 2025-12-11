//==============================================================================
// basic.ps.hlsl
// 
// Purpose: Pixel shader for basic sprite rendering
// Material: DefaultSprite2D
// 
// Features:
// - Texture sampling
// - Color tint multiplication
//==============================================================================

#include "basic_type.hlsli"

float4 main(BasicType input) : SV_TARGET {
  // Sample texture and apply color tint
  float4 texCol = tex.Sample(smp, input.uv);
  return texCol * input.color;
}
