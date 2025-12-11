#pragma once

#include <d3d12.h>

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "RenderPass/fullscreen_pass_helper.h"
#include "RenderPass/render_pass.h"
#include "RenderPass/scene_renderer.h"

class RenderPassManager {
 public:
  RenderPassManager() = default;
  ~RenderPassManager() = default;

  RenderPassManager(const RenderPassManager&) = delete;
  RenderPassManager& operator=(const RenderPassManager&) = delete;

  // Initialize manager with device
  bool Initialize(ID3D12Device* device);

  // Register a render pass
  void RegisterPass(const std::string& name, std::unique_ptr<RenderPass> pass);

  // Get a pass by name
  RenderPass* GetPass(const std::string& name);

  // Submit render packet to unified queue
  void SubmitPacket(const RenderPacket& packet);

  // Submit directly to a specific pass by name (skips unified queue)
  void SubmitToPass(const std::string& pass_name, const RenderPacket& packet);

  // Execute all enabled passes
  void RenderFrame(ID3D12GraphicsCommandList* command_list, TextureManager& texture_manager);

  // Clear render queue
  void Clear();

  // Get shared scene renderer
  SceneRenderer& GetSceneRenderer() {
    return scene_renderer_;
  }

  // Get fullscreen pass helper
  FullscreenPassHelper& GetFullscreenHelper() {
    return fullscreen_helper_;
  }

  // Statistics
  size_t GetPacketCount() const {
    return render_queue_.size();
  }

  size_t GetPassCount() const {
    return passes_.size();
  }

  void PrintStats() const;

 private:
  std::vector<RenderPacket> render_queue_;
  std::unordered_map<RenderPass*, std::vector<RenderPacket>> pass_queues_;
  std::vector<std::unique_ptr<RenderPass>> passes_;
  std::unordered_map<std::string, RenderPass*> pass_map_;

  SceneRenderer scene_renderer_;
  FullscreenPassHelper fullscreen_helper_;
};
