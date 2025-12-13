#pragma once

#include <d3d12.h>

#include "scene_renderer.h"

class RenderTarget;
class DepthBuffer;

// Describe load/clear behavior for a color attachment
struct ColorAttachmentIO {
  enum class Kind { Backbuffer, Custom } kind = Kind::Backbuffer;
  RenderTarget* target = nullptr; // valid when kind==Custom
  bool enabled = false; // whether this attachment is used
  D3D12_RESOURCE_STATES state = D3D12_RESOURCE_STATE_COMMON;
  // true = clear at begin, false = load existing contents
  bool clear = false;
};

// Describe load/clear behavior for a depth attachment (main depth or custom)
struct DepthAttachmentIO {
  enum class Kind { None, MainDepth, Custom } kind = Kind::None;
  DepthBuffer* target = nullptr; // valid when kind==Custom
  bool enabled = false; // whether depth is used
  D3D12_RESOURCE_STATES state = D3D12_RESOURCE_STATE_COMMON;
  bool clear = false;
  float clear_depth = 1.0f;
  uint8_t clear_stencil = 0;
};

// Overall IO description for a pass (color + depth)
struct PassIODesc {
  ColorAttachmentIO color;
  DepthAttachmentIO depth;
  // If true, manager will apply Graphic's default viewport and scissor rect.
  // If false, the pass is responsible for setting its own viewport/scissor.
  bool use_default_viewport_scissor = true;
};

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

  // Describe the IO requirements of this pass (which attachments, states, load/clear)
  // used for auto set the state
  virtual PassIODesc GetPassIO() const = 0;

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
