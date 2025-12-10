#pragma once

#include "fullscreen_pass_helper.h"
#include "render_pass.h"
#include "render_target.h"

// Post-processing pass using fullscreen quad
class PostProcessPass : public RenderPass {
 public:
  PostProcessPass() = default;
  ~PostProcessPass() override = default;

  bool Initialize(ID3D12Device* device) override;

  RenderFilter GetFilter() const override {
    // Post-process doesn't use geometry, return empty filter
    RenderFilter filter;
    filter.layer_mask = RenderLayer::None;
    return filter;
  }

  void Begin(ID3D12GraphicsCommandList* command_list) override;

  void Render(ID3D12GraphicsCommandList* command_list, SceneRenderer& scene_renderer, TextureManager& texture_manager) override;

  void End(ID3D12GraphicsCommandList* command_list) override;

  const char* GetName() const override {
    return "PostProcessPass";
  }

  // Set input and output
  void SetInputTexture(TextureHandle input) {
    input_texture_ = input;
  }

  void SetOutputTarget(RenderTarget* output) {
    output_target_ = output;
  }

  // Set PSO and root signature for post-processing
  void SetPSO(ID3D12PipelineState* pso) {
    pso_ = pso;
  }

  void SetRootSignature(ID3D12RootSignature* root_signature) {
    root_signature_ = root_signature;
  }

  void SetFullscreenHelper(FullscreenPassHelper* helper) {
    fullscreen_helper_ = helper;
  }

 private:
  TextureHandle input_texture_ = INVALID_TEXTURE_HANDLE;
  RenderTarget* output_target_ = nullptr;
  ID3D12PipelineState* pso_ = nullptr;
  ID3D12RootSignature* root_signature_ = nullptr;
  FullscreenPassHelper* fullscreen_helper_ = nullptr;
};
