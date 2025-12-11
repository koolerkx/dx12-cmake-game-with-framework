#include "render_pass_manager.h"

#include <cassert>
#include <iostream>

bool RenderPassManager::Initialize(ID3D12Device* device) {
  assert(device != nullptr);

  // Initialize shared scene renderer
  if (!scene_renderer_.Initialize(device)) {
    std::cerr << "[RenderPassManager] Failed to initialize scene renderer" << '\n';
    return false;
  }

  // Initialize fullscreen pass helper
  if (!fullscreen_helper_.Initialize(device)) {
    std::cerr << "[RenderPassManager] Failed to initialize fullscreen pass helper" << '\n';
    return false;
  }

  std::cout << "[RenderPassManager] Initialized" << '\n';
  return true;
}

void RenderPassManager::RegisterPass(const std::string& name, std::unique_ptr<RenderPass> pass) {
  assert(pass != nullptr);

  RenderPass* pass_ptr = pass.get();
  passes_.push_back(std::move(pass));
  pass_names_.push_back(name);
  pass_map_[name] = pass_ptr;

  std::cout << "[RenderPassManager] Registered pass: " << name << '\n';
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
    std::cerr << "[RenderPassManager] Warning: Invalid render packet submitted" << '\n';
    return;
  }

  render_queue_.push_back(packet);
}

void RenderPassManager::SubmitPacketToPass(const std::string& pass_name, const RenderPacket& packet) {
  if (!packet.IsValid()) {
    std::cerr << "[RenderPassManager] Warning: Invalid render packet submitted to pass: " << pass_name << '\n';
    return;
  }

  pass_queues_[pass_name].push_back(packet);
}

void RenderPassManager::RenderFrame(ID3D12GraphicsCommandList* command_list, TextureManager& texture_manager) {
  assert(command_list != nullptr);

  // Execute each enabled pass in order
  for (size_t i = 0; i < passes_.size(); ++i) {
    const auto& pass = passes_[i];
    if (!pass->IsEnabled()) {
      continue;
    }

    // Submit only packets targeted for this pass (or fall back to shared queue)
    scene_renderer_.Clear();
    const std::string& pass_name = pass_names_[i];
    auto queue_it = pass_queues_.find(pass_name);
    const auto& queue = (queue_it != pass_queues_.end()) ? queue_it->second : render_queue_;
    for (const auto& packet : queue) {
      scene_renderer_.Submit(packet);
    }

    // Skip empty queues
    if (queue.empty()) {
      continue;
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
  std::cout << "\n=== Render Pass Manager Statistics ===" << '\n';
  std::cout << "Total Packets: " << render_queue_.size() << '\n';
  std::cout << "Registered Passes: " << passes_.size() << '\n';

  std::cout << "\nEnabled Passes:" << '\n';
  for (const auto& pass : passes_) {
    std::cout << "  - " << pass->GetName() << ": " << (pass->IsEnabled() ? "Enabled" : "Disabled") << '\n';
  }

  std::cout << "======================================\n" << '\n';

  // Print scene renderer stats
  scene_renderer_.PrintStats();
}
