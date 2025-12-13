#include "ui_pass.h"

#include <cassert>

#include "Framework/Logging/logger.h"

bool UIPass::Initialize(ID3D12Device* device) {
  assert(device != nullptr);
  Logger::Log(LogLevel::Info, LogCategory::Graphic, "[UIPass] Initialized");
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

  // Begin no longer binds RTV directly; binding is declared via GetPassIO().
}

PassIODesc UIPass::GetPassIO() const {
  PassIODesc io;

  // Color: use custom render target if set, otherwise backbuffer (load existing contents)
  io.color.enabled = true;
  if (render_target_ != nullptr) {
    io.color.kind = ColorAttachmentIO::Kind::Custom;
    io.color.target = render_target_;
  } else {
    io.color.kind = ColorAttachmentIO::Kind::Backbuffer;
    io.color.target = nullptr;
  }
  io.color.state = D3D12_RESOURCE_STATE_RENDER_TARGET;
  io.color.clear = false; // load

  // Depth: UI doesn't touch depth
  io.depth.enabled = false;

  return io;
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
