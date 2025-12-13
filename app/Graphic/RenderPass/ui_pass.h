#pragma once

#include "render_pass.h"
#include "render_target.h"

// UI rendering pass - renders UI elements with orthographic projection
class UIPass : public RenderPass {
 public:
  UIPass() = default;
  ~UIPass() override = default;

  bool Initialize(ID3D12Device* device) override;

  RenderFilter GetFilter() const override;

  void Begin(ID3D12GraphicsCommandList* command_list) override;

  PassIODesc GetPassIO() const override;

  void Render(ID3D12GraphicsCommandList* command_list, SceneRenderer& scene_renderer, TextureManager& texture_manager) override;

  void End(ID3D12GraphicsCommandList* command_list) override;

  const char* GetName() const override {
    return "UIPass";
  }

  // Set render target
  // NOTE: This setter now influences `GetPassIO()`. If a custom
  // `render_target` is provided, `GetPassIO()` will return a `Kind::Custom`
  // color attachment referencing it so the manager will transition/clear/bind it.
  void SetRenderTarget(RenderTarget* render_target) {
    render_target_ = render_target;
  }

 private:
  RenderTarget* render_target_ = nullptr;
};
