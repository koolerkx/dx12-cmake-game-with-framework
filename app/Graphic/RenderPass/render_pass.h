#pragma once

#include <d3d12.h>

#include "scene_renderer.h"

// Base class for all render passes
class RenderPass {
 public:
  RenderPass() = default;
  virtual ~RenderPass() = default;

  RenderPass(const RenderPass&) = delete;
  RenderPass& operator=(const RenderPass&) = delete;

  // Initialize resources needed for this pass
  virtual bool Initialize(ID3D12Device*) {
    return true;
  }

  // Get filter for this pass (which packets to render)
  virtual RenderFilter GetFilter() const = 0;

  // Setup render targets and states before rendering
  virtual void Begin(ID3D12GraphicsCommandList* command_list) = 0;

  // Render the pass
  virtual void Render(ID3D12GraphicsCommandList* command_list, SceneRenderer& scene_renderer, TextureManager& texture_manager) = 0;

  // Clean up after rendering
  virtual void End(ID3D12GraphicsCommandList* command_list) = 0;

  // Get pass name for debugging
  virtual const char* GetName() const = 0;

  // Enable/disable this pass
  void SetEnabled(bool enabled) {
    enabled_ = enabled;
  }

  bool IsEnabled() const {
    return enabled_;
  }

 protected:
  bool enabled_ = true;
};
