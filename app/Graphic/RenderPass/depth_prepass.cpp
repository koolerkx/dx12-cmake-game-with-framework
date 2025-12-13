#include "depth_prepass.h"

#include <cassert>

#include "Framework/Logging/logger.h"

bool DepthPrepass::Initialize(ID3D12Device* device) {
  assert(device != nullptr);
  Logger::Log(LogLevel::Info, LogCategory::Graphic, "[DepthPrepass] Initialized");
  return true;
}

RenderFilter DepthPrepass::GetFilter() const {
  RenderFilter filter;
  filter.layer_mask = RenderLayer::Opaque;
  filter.tag_mask = RenderTag::Static;  // Only pre-pass static objects
  filter.tag_exclude_mask = RenderTag::None;
  return filter;
}

void DepthPrepass::Begin(ID3D12GraphicsCommandList* command_list) {
  assert(command_list != nullptr);

  if (depth_buffer_ != nullptr) {
    D3D12_CPU_DESCRIPTOR_HANDLE dsv = depth_buffer_->GetDSV();

    // Set null render target, depth-only
    command_list->OMSetRenderTargets(0, nullptr, FALSE, &dsv);

    // Clear depth buffer
    depth_buffer_->Clear(command_list, 1.0f, 0);
  }

  // Set depth-only PSO if available
  if (depth_only_pso_ != nullptr) {
    command_list->SetPipelineState(depth_only_pso_);
  }
}

void DepthPrepass::Render(ID3D12GraphicsCommandList* command_list, SceneRenderer& scene_renderer, TextureManager& texture_manager) {
  assert(command_list != nullptr);

  // Get filter for this pass
  RenderFilter filter = GetFilter();

  // Flush scene renderer with filter
  scene_renderer.Flush(command_list, texture_manager, filter);
}

void DepthPrepass::End(ID3D12GraphicsCommandList* command_list) {
  (void)command_list;
}
