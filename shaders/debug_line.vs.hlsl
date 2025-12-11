//==============================================================================
// debug_line.vs.hlsl
// 
// Purpose: Vertex shader for 3D debug line rendering
// Material: DefaultDebugLine
// 
// Root Signature Layout:
//   param[0] = b0: float4x4 ViewProjectionMatrix (16 x 32-bit constants)
// 
// Features:
// - Direct transform from world space to clip space
// - Per-vertex color support
//==============================================================================

#include "basic_type.hlsli"

// Debug line vertex shader input
struct DebugVertexInput
{
    float3 position : POSITION;
    float4 color    : COLOR;
};

// Debug line vertex shader output / pixel shader input
struct DebugVertexOutput
{
    float4 position : SV_POSITION;
    float4 color    : COLOR;
};

// Scene constants - using root constants for better performance
cbuffer SceneConstants : register(b0)
{
    float4x4 ViewProjectionMatrix;
};

DebugVertexOutput main(DebugVertexInput input)
{
    DebugVertexOutput output;
    
    // Transform vertex position to clip space
    output.position = mul(float4(input.position, 1.0f), ViewProjectionMatrix);
    
    // Pass through vertex color
    output.color = input.color;
    
    return output;
}