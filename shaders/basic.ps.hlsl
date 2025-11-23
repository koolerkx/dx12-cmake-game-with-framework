#include "basic_type.hlsli"

float4 main(BasicType input) : SV_TARGET {
  return float4(tex.Sample(smp, input.uv));
}
