#include "forward_pass.h"

#include <cassert>

#include "Framework/Logging/logger.h"

bool ForwardPass::Initialize(ID3D12Device* device) {
  assert(device != nullptr);
  Logger::Log(LogLevel::Info, LogCategory::Graphic, "[ForwardPass] Initialized");
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
  // Begin no longer binds RTV/DSV directly; binding is declared via GetPassIO().
}

PassIODesc ForwardPass::GetPassIO() const {
  PassIODesc io;

  // Color: use custom render target if set, otherwise backbuffer
  io.color.enabled = true;
  if (render_target_ != nullptr) {
    io.color.kind = ColorAttachmentIO::Kind::Custom;
    io.color.target = render_target_;
  } else {
    io.color.kind = ColorAttachmentIO::Kind::Backbuffer;
    io.color.target = nullptr;
  }
  io.color.state = D3D12_RESOURCE_STATE_RENDER_TARGET;
  io.color.clear = true;  // forward pass typically clears the render target

  // Depth: use custom depth buffer if set, otherwise main depth
  io.depth.enabled = true;
  if (depth_buffer_ != nullptr) {
    io.depth.kind = DepthAttachmentIO::Kind::Custom;
    io.depth.target = depth_buffer_;
  } else {
    io.depth.kind = DepthAttachmentIO::Kind::MainDepth;
    io.depth.target = nullptr;
  }
  io.depth.state = D3D12_RESOURCE_STATE_DEPTH_WRITE;
  io.depth.clear = true;
  io.depth.clear_depth = 1.0f;
  io.depth.clear_stencil = 0;

  return io;
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
