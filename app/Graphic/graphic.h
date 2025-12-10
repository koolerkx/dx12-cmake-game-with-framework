#pragma once

#include <basetsd.h>
#include <d3d12.h>
#include <dxgi1_6.h>

#include <functional>
#include <memory>

#include "RenderPass/forward_pass.h"
#include "RenderPass/render_pass_manager.h"
#include "RenderPass/ui_pass.h"
#include "depth_buffer.h"
#include "descriptor_heap_manager.h"
#include "fence_manager.h"
#include "material_manager.h"
#include "primitive_geometry_2d.h"
#include "shader_manager.h"
#include "swapchain_manager.h"
#include "texture_manager.h"
#include "types.h"
#include "upload_context.h"

class Scene;

class Graphic {
 public:
  Graphic() = default;
  ~Graphic() = default;

  bool Initialize(HWND hwnd, UINT frame_buffer_width, UINT frame_buffer_height);
  void BeginFrame();
  void RenderFrame();
  void EndFrame();
  void Shutdown();

  // Execute a short-lived command list for one-shot work (uploads, copies)
  void ExecuteImmediate(const std::function<void(ID3D12GraphicsCommandList*)>& recordFunc);

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

  UINT GetFrameBufferWidth() const {
    return frame_buffer_width_;
  }

  UINT GetFrameBufferHeight() const {
    return frame_buffer_height_;
  }

  static constexpr int FRAME_BUFFER_COUNT = 2;

 private:
  // Core D3D12 objects
  ComPtr<ID3D12Device5> device_ = nullptr;
  ComPtr<IDXGIFactory6> dxgi_factory_ = nullptr;

  ComPtr<ID3D12CommandAllocator> command_allocator_ = nullptr;
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

  // Viewport and scissor
  D3D12_VIEWPORT viewport_ = {};
  D3D12_RECT scissor_rect_ = {};

  // Initialization helpers
  bool EnableDebugLayer();
  bool CreateFactory();
  bool CreateDevice();
  bool CreateCommandQueue();
  bool CreateCommandList();
  bool CreateCommandAllocator();
  bool InitializeRenderPasses();
};
