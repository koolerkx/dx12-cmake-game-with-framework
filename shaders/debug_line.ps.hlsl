// Debug line pixel shader input (from vertex shader)
struct DebugVertexOutput
{
    float4 position : SV_POSITION;
    float4 color    : COLOR;
};

// Debug line pixel shader output
float4 main(DebugVertexOutput input) : SV_TARGET
{
    // Simply output the interpolated vertex color
    return input.color;
}
