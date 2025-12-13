#pragma once

#include <basetsd.h>
#include <d3d12.h>
#include <dxgi1_6.h>

#include <array>
#include <cstdint>
#include <functional>
#include <memory>

#include "Framework/types.h"
#include "RenderPass/forward_pass.h"
#include "RenderPass/render_pass_manager.h"
#include "RenderPass/ui_pass.h"
#include "depth_buffer.h"
#include "descriptor_heap_manager.h"
#include "fence_manager.h"
#include "framework_default_assets.h"
#include "gpu_resource.h"
#include "material_manager.h"
#include "primitive_geometry_2d.h"
#include "shader_manager.h"
#include "swapchain_manager.h"
#include "texture_manager.h"
#include "upload_context.h"

class Scene;
struct ID3D12InfoQueue;

class Graphic {
 public:
  Graphic() = default;
  ~Graphic() = default;

  void Initialize(HWND hwnd, UINT frame_buffer_width, UINT frame_buffer_height);
  void BeginFrame();
  void RenderFrame();
  void EndFrame();
  bool Resize(UINT frame_buffer_width, UINT frame_buffer_height);
  void Shutdown();

  // RenderSystem decides WHEN; Graphic handles HOW (encapsulation).
  void Transition(GpuResource* resource, D3D12_RESOURCE_STATES new_state);
  void Clear(RenderTarget* rt, const float* clear_color);
  void Clear(DepthBuffer* depth, float depth_val, uint8_t stencil_val);
  // Legacy wrapper: delegates to the canonical RenderFrame() path in Graphic.
  // Retained for compatibility; prefer calling RenderFrame() directly.
  void RenderPasses();

  // Execute a short-lived command list for one-shot work (uploads, copies)
  void ExecuteImmediate(const std::function<void(ID3D12GraphicsCommandList*)>& recordFunc);

  void SetVSync(bool enabled) {
    vsync_enabled_ = enabled;
  }

  // Access upload context for one-shot uploads
  UploadContext& GetUploadContext() {
    return upload_context_;
  }
  const UploadContext& GetUploadContext() const {
    return upload_context_;
  }

  RenderPassManager& GetRenderPassManager() {
    return render_pass_manager_;
  }

  TextureManager& GetTextureManager() {
    return texture_manager_;
  }

  MaterialManager& GetMaterialManager() {
    return material_manager_;
  }

  ShaderManager& GetShaderManager() {
    return shader_manager_;
  }

  PrimitiveGeometry2D& GetPrimitiveGeometry2D() {
    return *primitive_geometry_2d_;
  }

  ID3D12Device* GetDevice() const {
    return device_.Get();
  }

  ID3D12GraphicsCommandList* GetCommandList() const {
    return command_list_.Get();
  }

  const FrameworkDefaultAssets& GetDefaultAssets() const {
    return *default_assets_;
  }

  UINT GetFrameBufferWidth() const {
    return frame_buffer_width_;
  }

  UINT GetFrameBufferHeight() const {
    return frame_buffer_height_;
  }

  UINT GetCurrentFrameIndex() const {
    return frame_index_;
  }

  // Barrier helpers (state-tracked wrappers)
  RenderTarget* GetBackBufferRenderTarget() {
    return swap_chain_manager_.GetRenderTarget(frame_index_);
  }
  const RenderTarget* GetBackBufferRenderTarget() const {
    return swap_chain_manager_.GetRenderTarget(frame_index_);
  }

  DepthBuffer* GetDepthBuffer() {
    return &depth_buffer_;
  }
  const DepthBuffer* GetDepthBuffer() const {
    return &depth_buffer_;
  }

  // Access to main render targets and viewport state
  D3D12_CPU_DESCRIPTOR_HANDLE GetMainRTV() const;
  D3D12_CPU_DESCRIPTOR_HANDLE GetMainDSV() const;
  D3D12_VIEWPORT GetScreenViewport() const;
  D3D12_RECT GetScissorRect() const;

  // How many frames can be in-flight (CPU recording vs GPU executing).
  // This is the single source of truth for per-frame-slot resources.
  static constexpr uint32_t FrameCount = 2;

 private:
  // Core D3D12 objects
  ComPtr<ID3D12Device5> device_ = nullptr;
  ComPtr<IDXGIFactory6> dxgi_factory_ = nullptr;
  ComPtr<ID3D12InfoQueue> info_queue_ = nullptr;

  uint32_t frame_index_ = 0;
  std::array<ComPtr<ID3D12CommandAllocator>, FrameCount> command_allocators_ = {};
  std::array<uint64_t, FrameCount> frame_fence_values_ = {};
  ComPtr<ID3D12GraphicsCommandList> command_list_ = nullptr;
  ComPtr<ID3D12CommandQueue> command_queue_ = nullptr;

  // Resource management
  DescriptorHeapManager descriptor_heap_manager_;
  SwapChainManager swap_chain_manager_;
  DepthBuffer depth_buffer_;
  FenceManager fence_manager_;
  TextureManager texture_manager_;
  MaterialManager material_manager_;
  ShaderManager shader_manager_;
  std::unique_ptr<PrimitiveGeometry2D> primitive_geometry_2d_;

  // Upload context for one-shot resource uploads
  UploadContext upload_context_;

  RenderPassManager render_pass_manager_;

  // Cached pass pointers for efficient access
  ForwardPass* forward_pass_ = nullptr;
  UIPass* ui_pass_ = nullptr;

  UINT frame_buffer_width_ = 0;
  UINT frame_buffer_height_ = 0;

  bool vsync_enabled_ = true;

  // Viewport and scissor
  D3D12_VIEWPORT viewport_ = {};
  D3D12_RECT scissor_rect_ = {};

  // Initialization helpers
  bool EnableDebugLayer();
  HRESULT CreateFactory();
  HRESULT CreateDevice();
  HRESULT CreateCommandQueue();
  HRESULT CreateCommandList();
  HRESULT CreateCommandAllocator();
  void InitializeRenderPasses();
  void ProcessD3D12InfoQueueMessages();

  // Default framework assets (textures, basic meshes, debug materials)
  std::unique_ptr<FrameworkDefaultAssets> default_assets_;
};
