#pragma once

#include <d3d12.h>

#include "buffer.h"
#include "mesh.h"
#include "render_target.h"
#include "texture_manager.h"

// Helper class for rendering fullscreen quads (post-processing, screen-space effects)
class FullscreenPassHelper {
 public:
  FullscreenPassHelper() = default;
  ~FullscreenPassHelper() = default;

  FullscreenPassHelper(const FullscreenPassHelper&) = delete;
  FullscreenPassHelper& operator=(const FullscreenPassHelper&) = delete;

  // Initialize with device
  bool Initialize(ID3D12Device* device);

  // Draw a fullscreen quad with custom PSO and input texture
  void DrawQuad(
    ID3D12GraphicsCommandList* command_list, ID3D12PipelineState* pso, ID3D12RootSignature* root_signature, const RenderTarget& output);

  // Draw quad with input texture
  void DrawQuadWithTexture(ID3D12GraphicsCommandList* command_list,
    ID3D12PipelineState* pso,
    ID3D12RootSignature* root_signature,
    TextureHandle input,
    const RenderTarget& output,
    TextureManager& texture_manager);

  bool IsValid() const {
    return fullscreen_quad_.IsValid();
  }

 private:
  std::shared_ptr<Buffer> vertex_buffer_;
  std::shared_ptr<Buffer> index_buffer_;
  Mesh fullscreen_quad_;

  bool CreateFullscreenQuadGeometry(ID3D12Device* device);
};
