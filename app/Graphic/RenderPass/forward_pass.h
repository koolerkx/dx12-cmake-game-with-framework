#pragma once

#include "depth_buffer.h"
#include "render_pass.h"
#include "render_target.h"


// Forward rendering pass for opaque and transparent objects
class ForwardPass : public RenderPass {
 public:
  ForwardPass() = default;
  ~ForwardPass() override = default;

  bool Initialize(ID3D12Device* device) override;

  RenderFilter GetFilter() const override;

  void Begin(ID3D12GraphicsCommandList* command_list) override;

  void Render(ID3D12GraphicsCommandList* command_list, SceneRenderer& scene_renderer, TextureManager& texture_manager) override;

  void End(ID3D12GraphicsCommandList* command_list) override;

  const char* GetName() const override {
    return "ForwardPass";
  }

  // Set render targets
  void SetRenderTarget(RenderTarget* render_target) {
    render_target_ = render_target;
  }

  void SetDepthBuffer(DepthBuffer* depth_buffer) {
    depth_buffer_ = depth_buffer;
  }

 private:
  RenderTarget* render_target_ = nullptr;
  DepthBuffer* depth_buffer_ = nullptr;
};
