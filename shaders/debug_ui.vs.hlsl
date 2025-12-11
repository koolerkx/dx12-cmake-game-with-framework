// Debug UI Vertex Shader (2D screen-space)
// Input: screen-space pixel coordinates
// Output: NDC coordinates for rasterization

struct VSInput {
  float2 position : POSITION;
  float4 color : COLOR;
};

struct VSOutput {
  float4 position : SV_POSITION;
  float4 color : COLOR;
};

// Root constants: ortho projection (screen-space to NDC)
// b0: float4x4 viewProjMatrix (orthographic projection)
cbuffer UIConstants : register(b0) {
  float4x4 viewProjMatrix;
};

VSOutput main(VSInput input) {
  VSOutput output;
  
  // Transform from screen-space to NDC using orthographic projection
  // Input position is in pixel coordinates (e.g., 0-1920, 0-1080)
  output.position = mul(float4(input.position, 0.0f, 1.0f), viewProjMatrix);
  output.color = input.color;
  
  return output;
}
