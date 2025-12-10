#include "basic_type.hlsli"

float4 main(BasicType input) : SV_TARGET {
  float4 texCol = tex.Sample(smp, input.uv);
  return texCol * input.color;
}
