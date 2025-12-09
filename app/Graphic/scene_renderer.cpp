#include "scene_renderer.h"

#include <algorithm>
#include <cassert>
#include <iostream>

void SceneRenderer::Submit(const RenderPacket& packet) {
  if (!packet.IsValid()) {
    std::cerr << "[SceneRenderer] Warning: Invalid render packet submitted" << '\n';
    return;
  }

  packets_.push_back(packet);
}

void SceneRenderer::Flush(ID3D12GraphicsCommandList* command_list, TextureManager& texture_manager) {
  assert(command_list != nullptr);

  if (packets_.empty()) {
    return;
  }

  // Sort packets for optimal rendering
  SortPackets();

  // Track state changes
  MaterialTemplate* current_template = nullptr;
  size_t draw_calls = 0;
  size_t pso_switches = 0;

  // Execute render packets
  for (const auto& packet : packets_) {
    // Check if we need to switch PSO
    MaterialTemplate* packet_template = packet.material->GetTemplate();
    if (packet_template != current_template) {
      current_template = packet_template;
      ++pso_switches;
    }

    // Bind material (PSO, root signature, textures)
    packet.material->Bind(command_list, texture_manager);

    // Bind mesh (vertex/index buffers, topology)
    packet.mesh->Bind(command_list);

    // TODO: Set transform constant buffer here
    // For now, we skip transform upload since it requires upload buffer management
    // This will be implemented in a future iteration

    // Draw
    packet.mesh->Draw(command_list);
    ++draw_calls;
  }

  // Update statistics
  draw_call_count_ = draw_calls;
  pso_switch_count_ = pso_switches;
}

void SceneRenderer::Clear() {
  packets_.clear();
}

void SceneRenderer::SortPackets() {
  // Generate sort keys for all packets
  for (auto& packet : packets_) {
    packet.sort_key = GenerateSortKey(packet);
  }

  // Sort by key (PSO first, then material, then depth)
  std::sort(packets_.begin(), packets_.end(), [](const RenderPacket& a, const RenderPacket& b) { return a.sort_key < b.sort_key; });
}

uint64_t SceneRenderer::GenerateSortKey(const RenderPacket& packet) const {
  // Sort key layout (64 bits):
  // [32 bits: PSO pointer hash] [24 bits: Material pointer hash] [8 bits: depth bucket]

  uint64_t key = 0;

  // PSO/Template (high priority - minimize PSO switches)
  MaterialTemplate* template_ptr = packet.material->GetTemplate();
  uint64_t template_hash = reinterpret_cast<uint64_t>(template_ptr);
  key |= (template_hash & 0xFFFFFFFF) << 32;

  // Material (medium priority - batch same materials)
  uint64_t material_hash = reinterpret_cast<uint64_t>(packet.material);
  key |= (material_hash & 0xFFFFFF) << 8;

  // Depth bucket (low priority - optional front-to-back/back-to-front)
  // For now, just use 0 since we don't have camera depth calculation
  uint8_t depth_bucket = 0;
  key |= depth_bucket;

  return key;
}

void SceneRenderer::PrintStats() const {
  std::cout << "\n=== Scene Renderer Statistics ===" << '\n';
  std::cout << "Packets Submitted: " << packets_.size() << '\n';
  std::cout << "Draw Calls: " << draw_call_count_ << '\n';
  std::cout << "PSO Switches: " << pso_switch_count_ << '\n';

  if (draw_call_count_ > 0) {
    float batching_efficiency = 1.0f - (static_cast<float>(pso_switch_count_) / static_cast<float>(draw_call_count_));
    std::cout << "Batching Efficiency: " << (batching_efficiency * 100.0f) << "%" << '\n';
  }

  std::cout << "=================================\n" << '\n';
}
