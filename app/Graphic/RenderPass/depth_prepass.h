#pragma once

#include "depth_buffer.h"
#include "render_pass.h"

// Depth pre-pass - renders depth-only for early-z optimization
class DepthPrepass : public RenderPass {
 public:
  DepthPrepass() = default;
  ~DepthPrepass() override = default;

  bool Initialize(ID3D12Device* device) override;

  RenderFilter GetFilter() const override;

  void Begin(ID3D12GraphicsCommandList* command_list) override;

  void Render(ID3D12GraphicsCommandList* command_list, SceneRenderer& scene_renderer, TextureManager& texture_manager) override;

  void End(ID3D12GraphicsCommandList* command_list) override;

  const char* GetName() const override {
    return "DepthPrepass";
  }

  // Set depth buffer
  void SetDepthBuffer(DepthBuffer* depth_buffer) {
    depth_buffer_ = depth_buffer;
  }

  // Set depth-only PSO
  void SetDepthOnlyPSO(ID3D12PipelineState* pso) {
    depth_only_pso_ = pso;
  }

 private:
  DepthBuffer* depth_buffer_ = nullptr;
  ID3D12PipelineState* depth_only_pso_ = nullptr;
};
