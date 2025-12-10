#include "forward_pass.h"

#include <cassert>
#include <iostream>

bool ForwardPass::Initialize(ID3D12Device* device) {
  assert(device != nullptr);
  std::cout << "[ForwardPass] Initialized" << '\n';
  return true;
}

RenderFilter ForwardPass::GetFilter() const {
  RenderFilter filter;
  filter.layer_mask = RenderLayer::Opaque | RenderLayer::Transparent;
  filter.tag_mask = RenderTag::All;
  filter.tag_exclude_mask = RenderTag::None;
  return filter;
}

void ForwardPass::Begin(ID3D12GraphicsCommandList* command_list) {
  assert(command_list != nullptr);

  // Set render targets
  if (render_target_ != nullptr && depth_buffer_ != nullptr) {
    D3D12_CPU_DESCRIPTOR_HANDLE rtv = render_target_->GetRTV();
    D3D12_CPU_DESCRIPTOR_HANDLE dsv = depth_buffer_->GetDSV();

    command_list->OMSetRenderTargets(1, &rtv, FALSE, &dsv);
  }
}

void ForwardPass::Render(ID3D12GraphicsCommandList* command_list, SceneRenderer& scene_renderer, TextureManager& texture_manager) {
  assert(command_list != nullptr);

  // Get filter for this pass
  RenderFilter filter = GetFilter();

  // Flush scene renderer with filter
  scene_renderer.Flush(command_list, texture_manager, filter);
}

void ForwardPass::End(ID3D12GraphicsCommandList* command_list) {
  // No cleanup needed for forward pass
  (void)command_list;
}
