#include "postprocess_pass.h"

#include <cassert>

#include "Framework/Logging/logger.h"

bool PostProcessPass::Initialize(ID3D12Device* device) {
  assert(device != nullptr);
  Logger::Log(LogLevel::Info, LogCategory::Graphic, "[PostProcessPass] Initialized");
  // Disable by default until wired into the pass manager
  SetEnabled(false);
  return true;
}

void PostProcessPass::Begin(ID3D12GraphicsCommandList* command_list) {
  assert(command_list != nullptr);
  // IO (transitions/clears/binding) are handled by RenderPassManager via GetPassIO().
}

void PostProcessPass::Render(ID3D12GraphicsCommandList* command_list, SceneRenderer& scene_renderer, TextureManager& texture_manager) {
  assert(command_list != nullptr);

  // Post-processing uses fullscreen quad, not scene geometry
  (void)scene_renderer;

  if (fullscreen_helper_ == nullptr || output_target_ == nullptr || pso_ == nullptr || root_signature_ == nullptr) {
    Logger::Log(LogLevel::Error, LogCategory::Validation, "[PostProcessPass] Missing required resources");
    return;
  }

  // Draw fullscreen quad with input texture
  if (input_texture_.IsValid()) {
    fullscreen_helper_->DrawQuadWithTexture(command_list, pso_, root_signature_, input_texture_, *output_target_, texture_manager);
  } else {
    fullscreen_helper_->DrawQuad(command_list, pso_, root_signature_, *output_target_);
  }
}

void PostProcessPass::End(ID3D12GraphicsCommandList* command_list) {
  assert(command_list != nullptr);
  // IO transitions are handled by RenderPassManager.
}

PassIODesc PostProcessPass::GetPassIO() const {
  PassIODesc io;

  // Color: if we have an explicit output target, mark a color attachment (non-backbuffer)
  // Post-process renders to a color target; default to backbuffer when output_target_ is null.
  io.color.enabled = true;
  if (output_target_ != nullptr) {
    io.color.kind = ColorAttachmentIO::Kind::Custom;
    io.color.target = output_target_;
  } else {
    io.color.kind = ColorAttachmentIO::Kind::Backbuffer;
    io.color.target = nullptr;
  }
  io.color.state = D3D12_RESOURCE_STATE_RENDER_TARGET;
  io.color.clear = false;

  // Depth: postprocess typically doesn't write depth
  io.depth.enabled = false;

  return io;
}
