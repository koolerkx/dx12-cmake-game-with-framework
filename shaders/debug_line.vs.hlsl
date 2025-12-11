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

// Scene constants - using root constants instead of CBV for better performance
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