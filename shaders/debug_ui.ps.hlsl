//==============================================================================
// debug_ui.ps.hlsl
// 
// Purpose: Pixel shader for 2D UI debug rendering (screen-space)
// Used by: DebugVisualRenderer2D
// 
// Features:
// - Pass-through vertex color (no textures)
//==============================================================================

struct PSInput {
  float4 position : SV_POSITION;
  float4 color : COLOR;
};

float4 main(PSInput input) : SV_TARGET {
  return input.color;
}
