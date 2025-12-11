//==============================================================================
// debug_line.ps.hlsl
// 
// Purpose: Pixel shader for 3D debug line rendering
// Material: DefaultDebugLine
// 
// Features:
// - Pass-through vertex color (no textures)
//==============================================================================

// Debug line pixel shader input (from vertex shader)
struct DebugVertexOutput
{
    float4 position : SV_POSITION;
    float4 color    : COLOR;
};

float4 main(DebugVertexOutput input) : SV_TARGET
{
    // Output the interpolated vertex color
    return input.color;
}
