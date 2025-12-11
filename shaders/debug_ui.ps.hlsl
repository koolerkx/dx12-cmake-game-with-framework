// Debug UI Pixel Shader (2D screen-space)
// Simple pass-through pixel shader

struct PSInput {
  float4 position : SV_POSITION;
  float4 color : COLOR;
};

float4 main(PSInput input) : SV_TARGET {
  return input.color;
}
