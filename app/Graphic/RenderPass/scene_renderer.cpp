#include "scene_renderer.h"

#include <algorithm>
#include <cassert>
#include <iostream>

#include "RenderPass/render_constants.h"

bool SceneRenderer::Initialize(ID3D12Device* device, uint32_t frame_count) {
  frame_count_ = (frame_count == 0) ? 1u : frame_count;
  const size_t per_frame_size = kMaxSceneUpdatesPerFrame * kAlignedSceneDataSize;
  const size_t total_size = static_cast<size_t>(frame_count_) * per_frame_size;

  bool result = frame_cb_.Create(device, total_size, Buffer::Type::Constant, D3D12_HEAP_TYPE_UPLOAD);

  if (!result) {
    std::cerr << "[SceneRenderer] Failed to create frame constant buffer." << '\n';
    return false;
  }
  frame_cb_.SetDebugName("Scene_FrameCB");
  return true;
}

void SceneRenderer::BeginFrame(uint32_t frame_index) {
  current_frame_index_ = (frame_count_ == 0) ? 0u : (frame_index % frame_count_);

  const size_t per_frame_size = kMaxSceneUpdatesPerFrame * kAlignedSceneDataSize;
  current_frame_base_offset_ = static_cast<size_t>(current_frame_index_) * per_frame_size;
  current_cb_offset_ = current_frame_base_offset_;
  current_scene_data_gpu_address_ = 0;
}

void SceneRenderer::Submit(const RenderPacket& packet) {
  if (!packet.IsValid()) {
    std::cerr << "[SceneRenderer] Warning: Invalid render packet submitted" << '\n';
    return;
  }

  packets_.push_back(packet);
}

void SceneRenderer::Flush(ID3D12GraphicsCommandList* command_list, TextureManager& texture_manager, const RenderFilter& filter) {
  assert(command_list != nullptr);

  if (packets_.empty()) {
    return;
  }

  // Filter packets
  std::vector<RenderPacket> filtered_packets;
  filtered_packets.reserve(packets_.size());

  for (const auto& packet : packets_) {
    if (filter.Match(packet)) {
      filtered_packets.push_back(packet);
    }
  }

  if (filtered_packets.empty()) {
    return;
  }

  // Generate sort keys for filtered packets
  for (auto& packet : filtered_packets) {
    packet.sort_key = GenerateSortKey(packet);
  }

  // Sort filtered packets using custom comparator that incorporates sort_order for UI layer
  std::sort(filtered_packets.begin(), filtered_packets.end(), [](const RenderPacket& a, const RenderPacket& b) {
    // First by layer
    if (a.layer != b.layer) {
      return static_cast<int>(a.layer) < static_cast<int>(b.layer);
    }

    // For UI layer, sort by sort_order (lower values first)
    if (a.layer == RenderLayer::UI) {
      if (a.sort_order != b.sort_order) {
        return a.sort_order < b.sort_order;
      }
    }

    // Then by sort_key (which includes material, texture, etc.)
    return a.sort_key < b.sort_key;
  });

  // Track state changes
  MaterialTemplate* current_template = nullptr;
  size_t draw_calls = 0;
  size_t pso_switches = 0;

  // Execute render packets
  for (const auto& packet : filtered_packets) {
    // Check if we need to switch PSO
    MaterialTemplate* packet_template = packet.material->GetTemplate();
    if (packet_template != current_template) {
      current_template = packet_template;

      // Set PSO and root signature
      command_list->SetPipelineState(current_template->GetPSO());
      command_list->SetGraphicsRootSignature(current_template->GetRootSignature());

      // Bind frame constant buffer (b1)
      RenderHelpers::SetFrameConstants(command_list, current_scene_data_gpu_address_);
      ++pso_switches;
    }

    // Bind material (PSO, root signature, textures)
    packet.material->Bind(command_list, texture_manager);

    // Bind mesh (vertex/index buffers, topology)
    packet.mesh->Bind(command_list);

    // Set per-object constants (b0), color (b2), and UV transform (b3)
    RenderHelpers::SetPerObjectConstants(command_list, packet.world, packet.color, packet.uv_transform);

    // Draw
    packet.mesh->Draw(command_list);
    ++draw_calls;
  }

  // Update statistics
  draw_call_count_ += draw_calls;
  pso_switch_count_ += pso_switches;
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

bool SceneRenderer::SetSceneData(const SceneData& scene_data) {
  if (!frame_cb_.IsValid()) {
    return false;
  }

  const size_t per_frame_size = kMaxSceneUpdatesPerFrame * kAlignedSceneDataSize;
  const size_t frame_end = current_frame_base_offset_ + per_frame_size;
  if (current_cb_offset_ + kAlignedSceneDataSize > frame_end) {
    std::cerr << "[SceneRenderer] Error: Exceeded max SetSceneData calls per frame (kMaxSceneUpdatesPerFrame)" << '\n';
    return false;
  }

  frame_cb_.Upload(&scene_data, sizeof(SceneData), current_cb_offset_);

  current_scene_data_gpu_address_ = frame_cb_.GetGPUAddress() + current_cb_offset_;
  current_cb_offset_ += kAlignedSceneDataSize;

  return true;
}

uint64_t SceneRenderer::GenerateSortKey(const RenderPacket& packet) const {
  // Sort key layout (64 bits):
  // [8 bits: Layer] [24 bits: Template/PSO ptr low bits] [24 bits: Texture index] [8 bits: Material ptr low bits]

  uint64_t key = 0;

  // Layer (highest priority to keep passes grouped; each pass typically filters by layer)
  uint64_t layer_bits = static_cast<uint64_t>(static_cast<uint8_t>(packet.layer)) & 0xFF;
  key |= (layer_bits << 56);

  // PSO/Template (next priority - minimize PSO switches)
  MaterialTemplate* template_ptr = packet.material->GetTemplate();
  uint64_t template_hash = reinterpret_cast<uint64_t>(template_ptr);
  key |= ((template_hash & 0xFFFFFF) << 32);

  // Texture index (next priority - batch identical textures together)
  uint32_t texture_index = 0xFFFFFF;  // invalid default
  if (template_ptr != nullptr) {
    // Prefer an "albedo" slot if present, otherwise use the first slot
    const TextureSlotDefinition* slot_def = template_ptr->GetTextureSlot("albedo");
    if (slot_def == nullptr) {
      slot_def = template_ptr->GetTextureSlotByIndex(0);
    }
    if (slot_def != nullptr) {
      TextureHandle handle = packet.material->GetTexture(slot_def->name);
      if (handle.IsValid()) {
        texture_index = handle.index & 0xFFFFFF;
      }
    }
  }
  key |= (static_cast<uint64_t>(texture_index & 0xFFFFFF) << 8);

  // Material pointer (final priority)
  uint64_t material_hash = reinterpret_cast<uint64_t>(packet.material);
  key |= (material_hash & 0xFF);

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
