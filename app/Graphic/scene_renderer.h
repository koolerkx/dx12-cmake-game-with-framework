#pragma once

#include <DirectXMath.h>
#include <d3d12.h>

#include <cstdint>
#include <vector>

#include "buffer.h"
#include "material_instance.h"
#include "mesh.h"
#include "texture_manager.h"

using namespace DirectX;

struct SceneData {
  DirectX::XMFLOAT4X4 viewMatrix;
  DirectX::XMFLOAT4X4 projMatrix;
  DirectX::XMFLOAT4X4 viewProjMatrix;
  DirectX::XMFLOAT4X4 invViewProjMatrix;

  DirectX::XMFLOAT3 cameraPosition;
  float padding;
};

// Lightweight render packet submitted to the scene renderer
struct RenderPacket {
  Mesh* mesh = nullptr;
  MaterialInstance* material = nullptr;
  XMMATRIX transform = XMMatrixIdentity();

  // Sorting key helpers
  uint64_t sort_key = 0;

  bool IsValid() const {
    return mesh != nullptr && material != nullptr && mesh->IsValid() && material->IsValid();
  }
};

// SceneRenderer: Collects render packets, sorts them, and executes draw calls
class SceneRenderer {
 public:
  SceneRenderer() = default;
  ~SceneRenderer() = default;

  SceneRenderer(const SceneRenderer&) = delete;
  SceneRenderer& operator=(const SceneRenderer&) = delete;

  bool Initialize(ID3D12Device* device);

  // Submit a render packet
  void Submit(const RenderPacket& packet);

  // Sort and execute all render packets
  void Flush(ID3D12GraphicsCommandList* command_list, TextureManager& texture_manager);

  // Clear all packets (call after flush or at frame start)
  void Clear();

  // Set FrameCB, Scene Data
  bool SetSceneData(const SceneData& scene_data);

  // Statistics
  size_t GetPacketCount() const {
    return packets_.size();
  }

  size_t GetDrawCallCount() const {
    return draw_call_count_;
  }

  size_t GetPSOSwitchCount() const {
    return pso_switch_count_;
  }

  void ResetStats() {
    draw_call_count_ = 0;
    pso_switch_count_ = 0;
  }

  void PrintStats() const;

 private:
  std::vector<RenderPacket> packets_;

  // Statistics
  size_t draw_call_count_ = 0;
  size_t pso_switch_count_ = 0;

  // Sorting
  void SortPackets();
  uint64_t GenerateSortKey(const RenderPacket& packet) const;

  Buffer frame_cb_;
};
