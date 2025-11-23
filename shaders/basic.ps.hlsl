#include "basic_type.hlsli"

float4 main(BasicType input) : SV_TARGET { return float4(input.uv, 1, 1); }