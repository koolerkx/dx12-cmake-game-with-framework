#include "fullscreen_pass_helper.h"

#include <DirectXMath.h>

#include <cassert>

#include "Framework/Logging/logger.h"

using namespace DirectX;

struct FullscreenVertex {
  XMFLOAT3 pos;
  XMFLOAT2 uv;
};

bool FullscreenPassHelper::Initialize(ID3D12Device* device, UploadContext& upload_context) {
  assert(device != nullptr);

  if (!CreateFullscreenQuadGeometry(device, upload_context)) {
    Logger::Log(LogLevel::Error, LogCategory::Graphic, "[FullscreenPassHelper] Failed to create fullscreen quad geometry");
    return false;
  }

  Logger::Log(LogLevel::Info, LogCategory::Graphic, "[FullscreenPassHelper] Initialized");
  return true;
}

void FullscreenPassHelper::DrawQuad(
  ID3D12GraphicsCommandList* command_list, ID3D12PipelineState* pso, ID3D12RootSignature* root_signature, const RenderTarget& output) {
  assert(command_list != nullptr);
  assert(pso != nullptr);
  assert(root_signature != nullptr);

  // Set render target
  D3D12_CPU_DESCRIPTOR_HANDLE rtv = output.GetRTV();
  command_list->OMSetRenderTargets(1, &rtv, FALSE, nullptr);

  // Set PSO and root signature
  command_list->SetPipelineState(pso);
  command_list->SetGraphicsRootSignature(root_signature);

  // Bind and draw fullscreen quad
  fullscreen_quad_.Bind(command_list);
  fullscreen_quad_.Draw(command_list);
}

void FullscreenPassHelper::DrawQuadWithTexture(ID3D12GraphicsCommandList* command_list,
  ID3D12PipelineState* pso,
  ID3D12RootSignature* root_signature,
  TextureHandle input,
  const RenderTarget& output,
  TextureManager& texture_manager) {
  assert(command_list != nullptr);
  assert(pso != nullptr);
  assert(root_signature != nullptr);

  // Set render target
  D3D12_CPU_DESCRIPTOR_HANDLE rtv = output.GetRTV();
  command_list->OMSetRenderTargets(1, &rtv, FALSE, nullptr);

  // Set PSO and root signature
  command_list->SetPipelineState(pso);
  command_list->SetGraphicsRootSignature(root_signature);

  // Bind input texture (assume root parameter 0 for descriptor table)
  if (input.IsValid()) {
    const Texture* input_texture = texture_manager.GetTexture(input);
    if (input_texture != nullptr) {
      auto srv = input_texture->GetSRV();
      if (srv.IsValid() && srv.IsShaderVisible()) {
        command_list->SetGraphicsRootDescriptorTable(0, srv.gpu);
      }
    }
  }

  // Bind and draw fullscreen quad
  fullscreen_quad_.Bind(command_list);
  fullscreen_quad_.Draw(command_list);
}

bool FullscreenPassHelper::CreateFullscreenQuadGeometry(ID3D12Device* device, UploadContext& upload_context) {
  // Fullscreen quad vertices (NDC space: -1 to 1)
  // Counter-clockwise winding
  FullscreenVertex vertices[] = {
    {{-1.0f, -1.0f, 0.0f}, {0.0f, 1.0f}},  // bottom-left
    {{-1.0f, 1.0f, 0.0f}, {0.0f, 0.0f}},   // top-left
    {{1.0f, -1.0f, 0.0f}, {1.0f, 1.0f}},   // bottom-right
    {{1.0f, 1.0f, 0.0f}, {1.0f, 0.0f}},    // top-right
  };

  // Create vertex buffer
  vertex_buffer_ = Buffer::CreateAndUploadToDefaultHeapForInit(
    device, upload_context, vertices, sizeof(vertices), Buffer::Type::Vertex, "FullscreenQuad_VertexBuffer");
  if (!vertex_buffer_) {
    Logger::Log(LogLevel::Error, LogCategory::Resource, "[FullscreenPassHelper] Failed to create vertex buffer");
    return false;
  }

  // Indices for two triangles
  uint16_t indices[] = {0, 1, 2, 2, 1, 3};

  // Create index buffer
  index_buffer_ = Buffer::CreateAndUploadToDefaultHeapForInit(
    device, upload_context, indices, sizeof(indices), Buffer::Type::Index, "FullscreenQuad_IndexBuffer");
  if (!index_buffer_) {
    Logger::Log(LogLevel::Error, LogCategory::Resource, "[FullscreenPassHelper] Failed to create index buffer");
    return false;
  }

  // Initialize mesh
  fullscreen_quad_.Initialize(vertex_buffer_, index_buffer_, sizeof(FullscreenVertex), 6, DXGI_FORMAT_R16_UINT);
  fullscreen_quad_.SetDebugName("FullscreenQuad");

  return true;
}
