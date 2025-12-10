struct VSIN {
  float4 pos : POSITION;
  float2 uv : TEXCOORD;
};

struct BasicType {
  float4 svpos : SV_POSITION;
  float2 uv : TEXCOORD;
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