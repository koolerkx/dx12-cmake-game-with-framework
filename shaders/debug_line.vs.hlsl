//==============================================================================
// debug_line.vs.hlsl
//
// Purpose: Vertex shader for 3D debug line rendering using hardware instancing
// Material: DefaultDebugLine
//
// Root Signature Layout:
//   param[0] = b1: FrameCB (constant buffer view)
//
// Input Layout:
//   Slot 0 (per-vertex): Unit segment vertex position
//   Slot 1 (per-instance): World matrix (float4x4) + Color (float4)
//
// Features:
// - Hardware instancing: each line is a single instance
// - Unit segment [-0.5, 0.5] transformed by per-instance world matrix
// - Per-instance color support
//==============================================================================

#include "basic_type.hlsli"

// Instanced debug line vertex shader input
struct DebugVertexInput {
  float3 position : POSITION; // Unit segment vertex (from slot 0)

  // Per-instance world matrix (from slot 1)
  // Note: Declared as 4 separate float4 semantics due to input layout
  // constraints
  float4 world0 : WORLD0;
  float4 world1 : WORLD1;
  float4 world2 : WORLD2;
  float4 world3 : WORLD3;

  float4 color : COLOR; // Per-instance color (from slot 1)
};

// Debug line vertex shader output / pixel shader input
struct DebugVertexOutput {
  float4 position : SV_POSITION;
  float4 color : COLOR;
};

DebugVertexOutput main(DebugVertexInput input) {
  DebugVertexOutput output;

  // Reconstruct world matrix from 4 float4 rows
  // Note: row_major is a storage qualifier, not used in constructor
  row_major float4x4 world_matrix =
      float4x4(input.world0, input.world1, input.world2, input.world3);

  // Transform position: Local -> World -> View -> Projection
  // All matrices are uploaded as row_major -> use mul(vector, matrix)
  float4 posW = mul(float4(input.position, 1.0f), world_matrix);
  posW = mul(posW, view);
  output.position = mul(posW, proj);

  // Pass through instance color
  output.color = input.color;

  return output;
}
