#include "ui_pass.h"

#include <cassert>
#include <iostream>

bool UIPass::Initialize(ID3D12Device* device) {
  assert(device != nullptr);
  std::cout << "[UIPass] Initialized" << '\n';
  return true;
}

RenderFilter UIPass::GetFilter() const {
  RenderFilter filter;
  filter.layer_mask = RenderLayer::UI;
  filter.tag_mask = RenderTag::All;
  filter.tag_exclude_mask = RenderTag::None;
  return filter;
}

void UIPass::Begin(ID3D12GraphicsCommandList* command_list) {
  assert(command_list != nullptr);

  // Set render target (no depth buffer for UI)
  if (render_target_ != nullptr) {
    D3D12_CPU_DESCRIPTOR_HANDLE rtv = render_target_->GetRTV();
    command_list->OMSetRenderTargets(1, &rtv, FALSE, nullptr);
  }
}

void UIPass::Render(ID3D12GraphicsCommandList* command_list, SceneRenderer& scene_renderer, TextureManager& texture_manager) {
  assert(command_list != nullptr);

  // Get filter for this pass
  RenderFilter filter = GetFilter();

  // Flush scene renderer with filter
  scene_renderer.Flush(command_list, texture_manager, filter);
}

void UIPass::End(ID3D12GraphicsCommandList* command_list) {
  (void)command_list;
}
