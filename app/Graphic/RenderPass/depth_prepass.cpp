#include "depth_prepass.h"

#include <cassert>

#include "Framework/Logging/logger.h"

bool DepthPrepass::Initialize(ID3D12Device* device) {
  assert(device != nullptr);
  Logger::Log(LogLevel::Info, LogCategory::Graphic, "[DepthPrepass] Initialized");
  // Disable by default until integrated into the pass sequence
  SetEnabled(false);
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
  // Binding and clears are handled centrally by RenderPassManager via PassIODesc.
  // Here we only set the depth-only PSO if available.
  if (depth_only_pso_ != nullptr) {
    command_list->SetPipelineState(depth_only_pso_);
  }
}

PassIODesc DepthPrepass::GetPassIO() const {
  PassIODesc io;
  // No color target
  io.color.enabled = false;

  // Depth: use main depth buffer, depth-write and clear
  io.depth.enabled = true;
  io.depth.kind = DepthAttachmentIO::Kind::MainDepth;
  io.depth.target = nullptr;
  io.depth.state = D3D12_RESOURCE_STATE_DEPTH_WRITE;
  io.depth.clear = true;
  io.depth.clear_depth = 1.0f;
  io.depth.clear_stencil = 0;

  return io;
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
