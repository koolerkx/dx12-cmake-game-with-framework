#include "render_pass_manager.h"

#include <cassert>

#include "Framework/Logging/logger.h"

#include "RenderPass/render_pass.h"

bool RenderPassManager::Initialize(ID3D12Device* device, uint32_t frame_count, UploadContext& upload_context) {
  assert(device != nullptr);

  // Initialize shared scene renderer
  if (!scene_renderer_.Initialize(device, frame_count)) {
    Logger::Log(LogLevel::Error, LogCategory::Graphic, "[RenderPassManager] Failed to initialize scene renderer.");
    return false;
  }

  // Initialize fullscreen pass helper
  if (!fullscreen_helper_.Initialize(device, upload_context)) {
    Logger::Log(LogLevel::Error, LogCategory::Graphic, "[RenderPassManager] Failed to initialize fullscreen pass helper.");
    return false;
  }

  Logger::Log(LogLevel::Info, LogCategory::Graphic, "[RenderPassManager] Initialized.");
  return true;
}

void RenderPassManager::RegisterPass(const std::string& name, std::unique_ptr<RenderPass> pass) {
  assert(pass != nullptr);

  RenderPass* pass_ptr = pass.get();
  passes_.push_back(std::move(pass));
  pass_map_[name] = pass_ptr;

  Logger::Logf(LogLevel::Info, LogCategory::Graphic, Logger::Here(), "[RenderPassManager] Registered pass: {}", name);
}

RenderPass* RenderPassManager::GetPass(const std::string& name) {
  auto it = pass_map_.find(name);
  if (it != pass_map_.end()) {
    return it->second;
  }
  return nullptr;
}

void RenderPassManager::SubmitPacket(const RenderPacket& packet) {
  if (!packet.IsValid()) {
    Logger::Log(LogLevel::Warn, LogCategory::Validation, "[RenderPassManager] Invalid render packet submitted.");
    return;
  }

  render_queue_.push_back(packet);
}

void RenderPassManager::SubmitToPass(const std::string& name, const RenderPacket& packet) {
  if (!packet.IsValid()) {
    Logger::Logf(LogLevel::Warn,
      LogCategory::Validation,
      Logger::Here(),
      "[RenderPassManager] Invalid render packet submitted to pass '{}'.",
      name);
    return;
  }

  RenderPass* pass = GetPass(name);
  if (!pass) {
    Logger::Logf(LogLevel::Warn,
      LogCategory::Validation,
      Logger::Here(),
      "[RenderPassManager] Pass '{}' not found; falling back to unified queue.",
      name);
    render_queue_.push_back(packet);
    return;
  }

  pass_queues_[pass].push_back(packet);
}

void RenderPassManager::RenderFrame(ID3D12GraphicsCommandList* command_list, TextureManager& texture_manager) {
  assert(command_list != nullptr);

  if (render_queue_.empty()) {
    // Even if unified queue is empty, per-pass queues may have items
    bool any_direct_packets = false;
    for (const auto& pair : pass_queues_) {
      if (!pair.second.empty()) {
        any_direct_packets = true;
        break;
      }
    }
    if (!any_direct_packets) {
      return;
    }
  }

  // Execute each enabled pass in order
  for (const auto& pass : passes_) {
    if (!pass->IsEnabled()) {
      continue;
    }

    // Clear scene renderer and feed packets from unified queue or pass-specific queue
    scene_renderer_.Clear();

    // Unified queue flows through all passes
    for (const auto& packet : render_queue_) {
      scene_renderer_.Submit(packet);
    }
    // Pass-specific queue applies only to this pass
    auto pass_queue_it = pass_queues_.find(pass.get());
    if (pass_queue_it != pass_queues_.end()) {
      for (const auto& packet : pass_queue_it->second) {
        scene_renderer_.Submit(packet);
      }
    }

    // Begin pass
    pass->Begin(command_list);

    // Render pass
    pass->Render(command_list, scene_renderer_, texture_manager);

    // End pass
    pass->End(command_list);
  }
}

void RenderPassManager::Clear() {
  render_queue_.clear();
  pass_queues_.clear();
  scene_renderer_.Clear();
  scene_renderer_.ResetStats();
}

void RenderPassManager::PrintStats() const {
  std::string enabled;
  enabled.reserve(passes_.size() * 24);
  for (const auto& pass : passes_) {
    enabled += std::format("  - {}: {}\n", pass->GetName(), (pass->IsEnabled() ? "Enabled" : "Disabled"));
  }

  Logger::Logf(LogLevel::Info,
    LogCategory::Graphic,
    Logger::Here(),
    "=== Render Pass Manager Statistics ===\nTotal Packets: {}\nRegistered Passes: {}\n\nEnabled Passes:\n{}======================================",
    render_queue_.size(),
    passes_.size(),
    enabled);

  // Print scene renderer stats
  scene_renderer_.PrintStats();
}
