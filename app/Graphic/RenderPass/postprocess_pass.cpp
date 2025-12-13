#include "postprocess_pass.h"

#include <cassert>

#include "Framework/Logging/logger.h"

bool PostProcessPass::Initialize(ID3D12Device* device) {
  assert(device != nullptr);
  Logger::Log(LogLevel::Info, LogCategory::Graphic, "[PostProcessPass] Initialized");
  return true;
}

void PostProcessPass::Begin(ID3D12GraphicsCommandList* command_list) {
  assert(command_list != nullptr);

  // Transition output target to render target state
  if (output_target_ != nullptr) {
    output_target_->TransitionTo(command_list, D3D12_RESOURCE_STATE_RENDER_TARGET);
  }
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

  // Transition output target to shader resource state for next pass
  if (output_target_ != nullptr) {
    output_target_->TransitionTo(command_list, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
  }
}
