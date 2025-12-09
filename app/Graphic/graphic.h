#pragma once

#include <basetsd.h>
#include <d3d12.h>
#include <dxgi1_6.h>

#include "buffer.h"
#include "depth_buffer.h"
#include "descriptor_heap_manager.h"
#include "fence_manager.h"
#include "material_instance.h"
#include "material_manager.h"
#include "material_template.h"
#include "mesh.h"
#include "scene_renderer.h"
#include "shader_manager.h"
#include "swapchain_manager.h"
#include "texture_manager.h"
#include "types.h"

class Graphic {
 public:
  Graphic() = default;
  ~Graphic() = default;

  bool Initalize(HWND hwnd, UINT frame_buffer_width, UINT frame_buffer_height);
  void BeginRender();
  void EndRender();
  void Shutdown();

  // Forward rendering
  void DrawTestQuad();

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

  UINT frame_buffer_width_ = 0;
  UINT frame_buffer_height_ = 0;

  // Rendering system
  ShaderManager shader_manager_;
  ComPtr<ID3D12RootSignature> root_signature_ = nullptr;
  ComPtr<ID3D12PipelineState> pipeline_state_ = nullptr;
  SceneRenderer scene_renderer_;

  // Viewport and scissor
  D3D12_VIEWPORT viewport_ = {};
  D3D12_RECT scissor_rect_ = {};

  // Test material
  MaterialTemplate* test_material_template_ = nullptr;
  std::unique_ptr<MaterialInstance> test_material_instance_ = nullptr;

  // Test resources
  Buffer vertex_buffer_;
  Buffer index_buffer_;
  Mesh test_mesh_;
  TextureHandle test_texture_handle_ = INVALID_TEXTURE_HANDLE;  // Temporary, assigned to material

  // Initialization helpers
  bool EnableDebugLayer();
  bool CreateFactory();
  bool CreateDevice();
  bool CreateCommandQueue();
  bool CreateCommandList();
  bool CreateCommandAllocator();

  bool InitializeTestGeometry();
  bool InitializeTestTexture();
  bool CreateTestMaterial();

  bool LoadShaders();
  bool CreateRootSignature();
  bool CreatePipelineState();
};
