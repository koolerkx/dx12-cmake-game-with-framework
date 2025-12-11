//==============================================================================
// debug_line.vs.hlsl
// 
// Purpose: Vertex shader for 3D debug line rendering
// Material: DefaultDebugLine
// 
// Root Signature Layout:
//   param[0] = b0: float4x4 world_matrix (16 x 32-bit constants)
//   param[1] = b1: FrameCB (constant buffer view)
// 
// Features:
// - Local-to-world-to-clip transformation
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

// Per-object world transform (row_major to match CPU upload)
cbuffer PerObjectWorld : register(b0)
{
    row_major float4x4 world_matrix;
};

DebugVertexOutput main(DebugVertexInput input)
{
    DebugVertexOutput output;
    
    // Transform position: Local -> World -> View -> Projection
    // All matrices are uploaded as row_major -> use mul(vector, matrix)
    float4 posW = mul(float4(input.position, 1.0f), world_matrix);
    posW = mul(posW, view);
    output.position = mul(posW, proj);
    
    // Pass through vertex color
    output.color = input.color;
    
    return output;
}