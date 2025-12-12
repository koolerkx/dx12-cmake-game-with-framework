//==============================================================================
// debug_ui.vs.hlsl
// 
// Purpose: Vertex shader for 2D UI debug rendering (screen-space)
// Used by: DebugVisualRenderer2D
// 
// Root Signature Layout:
//   param[0] = b0: float4x4 viewProjMatrix (16 x 32-bit constants)
// 
// Features:
// - Transform from screen-space pixel coordinates to NDC
// - Per-vertex color support
// - Top-left origin coordinate system (0,0 = top-left corner)
//==============================================================================

struct VSInput {
  float2 position : POSITION;
  float4 color : COLOR;
};

struct VSOutput {
  float4 position : SV_POSITION;
  float4 color : COLOR;
};

// Orthographic projection matrix for UI (screen-space to NDC)
cbuffer UIConstants : register(b0) {
  row_major float4x4 viewProjMatrix;
};

VSOutput main(VSInput input) {
  VSOutput output;
  
  // Transform from screen-space to NDC using orthographic projection
  // Input position is in pixel coordinates (e.g., 0-1920, 0-1080)
  output.position = mul(float4(input.position, 0.0f, 1.0f), viewProjMatrix);
  output.color = input.color;
  
  return output;
}
