#pragma once

#include <d3d12.h>

#include <vector>

#include "shader_manager.h"
#include "types.h"

class PipelineStateBuilder {
 public:
  PipelineStateBuilder() {
    InitializeDefaults();
  }
  ~PipelineStateBuilder() = default;

  // Shader setup
  PipelineStateBuilder& SetVertexShader(const ShaderBlob* shader);
  PipelineStateBuilder& SetPixelShader(const ShaderBlob* shader);
  PipelineStateBuilder& SetGeometryShader(const ShaderBlob* shader);
  PipelineStateBuilder& SetHullShader(const ShaderBlob* shader);
  PipelineStateBuilder& SetDomainShader(const ShaderBlob* shader);

  // Root signature
  PipelineStateBuilder& SetRootSignature(ID3D12RootSignature* root_signature);

  // Input layout
  PipelineStateBuilder& AddInputElement(const char* semantic_name,
    UINT semantic_index,
    DXGI_FORMAT format,
    UINT input_slot,
    UINT aligned_byte_offset = D3D12_APPEND_ALIGNED_ELEMENT,
    D3D12_INPUT_CLASSIFICATION input_slot_class = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,
    UINT instance_data_step_rate = 0);

  PipelineStateBuilder& SetInputLayout(const D3D12_INPUT_ELEMENT_DESC* input_elements, UINT num_elements);

  // Rasterizer state
  PipelineStateBuilder& SetFillMode(D3D12_FILL_MODE fill_mode);
  PipelineStateBuilder& SetCullMode(D3D12_CULL_MODE cull_mode);
  PipelineStateBuilder& SetFrontCounterClockwise(bool is_front_ccw);
  PipelineStateBuilder& SetDepthBias(INT depth_bias, FLOAT depth_bias_clamp, FLOAT slope_scaled_depth_bias);
  PipelineStateBuilder& SetDepthClipEnable(bool enable);
  PipelineStateBuilder& SetMultisampleEnable(bool enable);
  PipelineStateBuilder& SetAntialiasedLineEnable(bool enable);

  // Blend state
  PipelineStateBuilder& SetBlendEnable(bool enable, UINT render_target_index = 0);
  PipelineStateBuilder& SetBlendOp(D3D12_BLEND_OP blend_op, UINT render_target_index = 0);
  PipelineStateBuilder& SetBlendFactors(
    D3D12_BLEND src_blend, D3D12_BLEND dest_blend, D3D12_BLEND src_blend_alpha, D3D12_BLEND dest_blend_alpha, UINT render_target_index = 0);
  PipelineStateBuilder& SetAlphaToCoverageEnable(bool enable);
  PipelineStateBuilder& SetIndependentBlendEnable(bool enable);

  // Depth stencil state
  PipelineStateBuilder& SetDepthEnable(bool enable);
  PipelineStateBuilder& SetDepthWriteMask(D3D12_DEPTH_WRITE_MASK mask);
  PipelineStateBuilder& SetDepthFunc(D3D12_COMPARISON_FUNC func);
  PipelineStateBuilder& SetStencilEnable(bool enable);
  PipelineStateBuilder& SetStencilReadMask(UINT8 mask);
  PipelineStateBuilder& SetStencilWriteMask(UINT8 mask);

  // Render target formats
  PipelineStateBuilder& SetRenderTargetFormat(DXGI_FORMAT format, UINT index = 0);
  PipelineStateBuilder& SetRenderTargetFormats(const DXGI_FORMAT* formats, UINT num_render_targets);
  PipelineStateBuilder& SetDepthStencilFormat(DXGI_FORMAT format);

  // Primitive topology
  PipelineStateBuilder& SetPrimitiveTopologyType(D3D12_PRIMITIVE_TOPOLOGY_TYPE topology_type);

  // Sample desc
  PipelineStateBuilder& SetSampleDesc(UINT count, UINT quality);

  // Preset configurations for common scenarios
  PipelineStateBuilder& UseForwardRenderingDefaults();
  PipelineStateBuilder& UseDeferredGBufferDefaults();
  PipelineStateBuilder& UseDeferredLightingDefaults();
  PipelineStateBuilder& UsePostProcessDefaults();
  PipelineStateBuilder& UseUIDefaults();
  PipelineStateBuilder& UseForwardTransparentDefaults();
  PipelineStateBuilder& UseOverlayDefaults();  // For debug overlays and UI elements without depth

  // Build final PSO
  bool Build(ID3D12Device* device, ComPtr<ID3D12PipelineState>& out_pso);

  // Reset for reuse
  void Reset();

 private:
  D3D12_GRAPHICS_PIPELINE_STATE_DESC desc_ = {};
  std::vector<D3D12_INPUT_ELEMENT_DESC> input_elements_;

  void InitializeDefaults();
};
