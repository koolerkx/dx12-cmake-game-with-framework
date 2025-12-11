//==============================================================================
// basic_type.hlsli
// 
// Purpose: Shared types and resources for basic sprite rendering
// Used by: DefaultSprite2D material template
// 
// Root Signature Layout:
//   param[0] = b0: float4x4 world_pos      (16 x 32-bit constants)
//   param[1] = b2: float4 color_tint       (4 x 32-bit constants)
//   param[2] = b3: float4 uv_transform     (4 x 32-bit constants, format: offset.xy, scale.xy)
//   param[3] = b1: FrameCB                 (constant buffer view)
//   param[4] = t0: Texture2D               (descriptor table SRV)
//   sampler s0: Static sampler (POINT, WRAP)
//==============================================================================

struct VSIN {
  float3 pos : POSITION;  // Vertex position (local space)
  float2 uv : TEXCOORD;   // Texture coordinates
};

struct BasicType {
  float4 svpos : SV_POSITION;
  float2 uv : TEXCOORD;
  float4 color : COLOR;
};

Texture2D<float4> tex : register(t0);
SamplerState smp : register(s0);

cbuffer FrameCB : register(b1) {
  float4x4 view;
  float4x4 proj;
  float4x4 view_proj;
  float4x4 inv_view_pos;
  float3 camera_pos;
  float pad;
};