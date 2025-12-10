#include "pipeline_state_builder.h"

#include <cstring>
#include <iostream>

void PipelineStateBuilder::InitializeDefaults() {
  std::memset(&desc_, 0, sizeof(desc_));

  // Default rasterizer state
  desc_.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
  desc_.RasterizerState.CullMode = D3D12_CULL_MODE_BACK;
  desc_.RasterizerState.FrontCounterClockwise = FALSE;
  desc_.RasterizerState.DepthBias = D3D12_DEFAULT_DEPTH_BIAS;
  desc_.RasterizerState.DepthBiasClamp = D3D12_DEFAULT_DEPTH_BIAS_CLAMP;
  desc_.RasterizerState.SlopeScaledDepthBias = D3D12_DEFAULT_SLOPE_SCALED_DEPTH_BIAS;
  desc_.RasterizerState.DepthClipEnable = TRUE;
  desc_.RasterizerState.MultisampleEnable = FALSE;
  desc_.RasterizerState.AntialiasedLineEnable = FALSE;
  desc_.RasterizerState.ForcedSampleCount = 0;
  desc_.RasterizerState.ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;

  // Default blend state
  desc_.BlendState.AlphaToCoverageEnable = FALSE;
  desc_.BlendState.IndependentBlendEnable = FALSE;

  for (UINT i = 0; i < D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT; ++i) {
    desc_.BlendState.RenderTarget[i].BlendEnable = FALSE;
    desc_.BlendState.RenderTarget[i].LogicOpEnable = FALSE;
    desc_.BlendState.RenderTarget[i].SrcBlend = D3D12_BLEND_ONE;
    desc_.BlendState.RenderTarget[i].DestBlend = D3D12_BLEND_ZERO;
    desc_.BlendState.RenderTarget[i].BlendOp = D3D12_BLEND_OP_ADD;
    desc_.BlendState.RenderTarget[i].SrcBlendAlpha = D3D12_BLEND_ONE;
    desc_.BlendState.RenderTarget[i].DestBlendAlpha = D3D12_BLEND_ZERO;
    desc_.BlendState.RenderTarget[i].BlendOpAlpha = D3D12_BLEND_OP_ADD;
    desc_.BlendState.RenderTarget[i].LogicOp = D3D12_LOGIC_OP_NOOP;
    desc_.BlendState.RenderTarget[i].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
  }

  // Default depth stencil state
  desc_.DepthStencilState.DepthEnable = TRUE;
  desc_.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
  desc_.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS;
  desc_.DepthStencilState.StencilEnable = FALSE;
  desc_.DepthStencilState.StencilReadMask = D3D12_DEFAULT_STENCIL_READ_MASK;
  desc_.DepthStencilState.StencilWriteMask = D3D12_DEFAULT_STENCIL_WRITE_MASK;

  // Default sample desc
  desc_.SampleDesc.Count = 1;
  desc_.SampleDesc.Quality = 0;

  // Default sample mask
  desc_.SampleMask = D3D12_DEFAULT_SAMPLE_MASK;

  // Default primitive topology
  desc_.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;

  // Default strip cut value
  desc_.IBStripCutValue = D3D12_INDEX_BUFFER_STRIP_CUT_VALUE_DISABLED;
}

// ========== Shader Setup ==========

PipelineStateBuilder& PipelineStateBuilder::SetVertexShader(const ShaderBlob* shader) {
  if (shader && shader->IsValid()) {
    desc_.VS = shader->GetBytecode();
  }
  return *this;
}

PipelineStateBuilder& PipelineStateBuilder::SetPixelShader(const ShaderBlob* shader) {
  if (shader && shader->IsValid()) {
    desc_.PS = shader->GetBytecode();
  }
  return *this;
}

PipelineStateBuilder& PipelineStateBuilder::SetGeometryShader(const ShaderBlob* shader) {
  if (shader && shader->IsValid()) {
    desc_.GS = shader->GetBytecode();
  }
  return *this;
}

PipelineStateBuilder& PipelineStateBuilder::SetHullShader(const ShaderBlob* shader) {
  if (shader && shader->IsValid()) {
    desc_.HS = shader->GetBytecode();
  }
  return *this;
}

PipelineStateBuilder& PipelineStateBuilder::SetDomainShader(const ShaderBlob* shader) {
  if (shader && shader->IsValid()) {
    desc_.DS = shader->GetBytecode();
  }
  return *this;
}

// ========== Root Signature ==========

PipelineStateBuilder& PipelineStateBuilder::SetRootSignature(ID3D12RootSignature* root_signature) {
  desc_.pRootSignature = root_signature;
  return *this;
}

// ========== Input Layout ==========

PipelineStateBuilder& PipelineStateBuilder::AddInputElement(const char* semantic_name,
  UINT semantic_index,
  DXGI_FORMAT format,
  UINT input_slot,
  UINT aligned_byte_offset,
  D3D12_INPUT_CLASSIFICATION input_slot_class,
  UINT instance_data_step_rate) {
  D3D12_INPUT_ELEMENT_DESC element = {};
  element.SemanticName = semantic_name;
  element.SemanticIndex = semantic_index;
  element.Format = format;
  element.InputSlot = input_slot;
  element.AlignedByteOffset = aligned_byte_offset;
  element.InputSlotClass = input_slot_class;
  element.InstanceDataStepRate = instance_data_step_rate;

  input_elements_.push_back(element);

  desc_.InputLayout.pInputElementDescs = input_elements_.data();
  desc_.InputLayout.NumElements = static_cast<UINT>(input_elements_.size());

  return *this;
}

PipelineStateBuilder& PipelineStateBuilder::SetInputLayout(const D3D12_INPUT_ELEMENT_DESC* input_elements, UINT num_elements) {
  input_elements_.clear();
  input_elements_.assign(input_elements, input_elements + num_elements);

  desc_.InputLayout.pInputElementDescs = input_elements_.data();
  desc_.InputLayout.NumElements = num_elements;

  return *this;
}

// ========== Rasterizer State ==========

PipelineStateBuilder& PipelineStateBuilder::SetFillMode(D3D12_FILL_MODE fill_mode) {
  desc_.RasterizerState.FillMode = fill_mode;
  return *this;
}

PipelineStateBuilder& PipelineStateBuilder::SetCullMode(D3D12_CULL_MODE cull_mode) {
  desc_.RasterizerState.CullMode = cull_mode;
  return *this;
}

PipelineStateBuilder& PipelineStateBuilder::SetFrontCounterClockwise(bool is_front_ccw) {
  desc_.RasterizerState.FrontCounterClockwise = is_front_ccw ? TRUE : FALSE;
  return *this;
}

PipelineStateBuilder& PipelineStateBuilder::SetDepthBias(INT depth_bias, FLOAT depth_bias_clamp, FLOAT slope_scaled_depth_bias) {
  desc_.RasterizerState.DepthBias = depth_bias;
  desc_.RasterizerState.DepthBiasClamp = depth_bias_clamp;
  desc_.RasterizerState.SlopeScaledDepthBias = slope_scaled_depth_bias;
  return *this;
}

PipelineStateBuilder& PipelineStateBuilder::SetDepthClipEnable(bool enable) {
  desc_.RasterizerState.DepthClipEnable = enable ? TRUE : FALSE;
  return *this;
}

PipelineStateBuilder& PipelineStateBuilder::SetMultisampleEnable(bool enable) {
  desc_.RasterizerState.MultisampleEnable = enable ? TRUE : FALSE;
  return *this;
}

PipelineStateBuilder& PipelineStateBuilder::SetAntialiasedLineEnable(bool enable) {
  desc_.RasterizerState.AntialiasedLineEnable = enable ? TRUE : FALSE;
  return *this;
}

// ========== Blend State ==========

PipelineStateBuilder& PipelineStateBuilder::SetBlendEnable(bool enable, UINT render_target_index) {
  if (render_target_index < D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT) {
    desc_.BlendState.RenderTarget[render_target_index].BlendEnable = enable ? TRUE : FALSE;
  }
  return *this;
}

PipelineStateBuilder& PipelineStateBuilder::SetBlendOp(D3D12_BLEND_OP blend_op, UINT render_target_index) {
  if (render_target_index < D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT) {
    desc_.BlendState.RenderTarget[render_target_index].BlendOp = blend_op;
    desc_.BlendState.RenderTarget[render_target_index].BlendOpAlpha = blend_op;
  }
  return *this;
}

PipelineStateBuilder& PipelineStateBuilder::SetBlendFactors(
  D3D12_BLEND src_blend, D3D12_BLEND dest_blend, D3D12_BLEND src_blend_alpha, D3D12_BLEND dest_blend_alpha, UINT render_target_index) {
  if (render_target_index < D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT) {
    desc_.BlendState.RenderTarget[render_target_index].SrcBlend = src_blend;
    desc_.BlendState.RenderTarget[render_target_index].DestBlend = dest_blend;
    desc_.BlendState.RenderTarget[render_target_index].SrcBlendAlpha = src_blend_alpha;
    desc_.BlendState.RenderTarget[render_target_index].DestBlendAlpha = dest_blend_alpha;
  }
  return *this;
}

PipelineStateBuilder& PipelineStateBuilder::SetAlphaToCoverageEnable(bool enable) {
  desc_.BlendState.AlphaToCoverageEnable = enable ? TRUE : FALSE;
  return *this;
}

PipelineStateBuilder& PipelineStateBuilder::SetIndependentBlendEnable(bool enable) {
  desc_.BlendState.IndependentBlendEnable = enable ? TRUE : FALSE;
  return *this;
}

// ========== Depth Stencil State ==========

PipelineStateBuilder& PipelineStateBuilder::SetDepthEnable(bool enable) {
  desc_.DepthStencilState.DepthEnable = enable ? TRUE : FALSE;
  return *this;
}

PipelineStateBuilder& PipelineStateBuilder::SetDepthWriteMask(D3D12_DEPTH_WRITE_MASK mask) {
  desc_.DepthStencilState.DepthWriteMask = mask;
  return *this;
}

PipelineStateBuilder& PipelineStateBuilder::SetDepthFunc(D3D12_COMPARISON_FUNC func) {
  desc_.DepthStencilState.DepthFunc = func;
  return *this;
}

PipelineStateBuilder& PipelineStateBuilder::SetStencilEnable(bool enable) {
  desc_.DepthStencilState.StencilEnable = enable ? TRUE : FALSE;
  return *this;
}

PipelineStateBuilder& PipelineStateBuilder::SetStencilReadMask(UINT8 mask) {
  desc_.DepthStencilState.StencilReadMask = mask;
  return *this;
}

PipelineStateBuilder& PipelineStateBuilder::SetStencilWriteMask(UINT8 mask) {
  desc_.DepthStencilState.StencilWriteMask = mask;
  return *this;
}

// ========== Render Target Formats ==========

PipelineStateBuilder& PipelineStateBuilder::SetRenderTargetFormat(DXGI_FORMAT format, UINT index) {
  if (index < D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT) {
    desc_.RTVFormats[index] = format;
    if (index >= desc_.NumRenderTargets) {
      desc_.NumRenderTargets = index + 1;
    }
  }
  return *this;
}

PipelineStateBuilder& PipelineStateBuilder::SetRenderTargetFormats(const DXGI_FORMAT* formats, UINT num_render_targets) {
  desc_.NumRenderTargets = num_render_targets;
  for (UINT i = 0; i < num_render_targets && i < D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT; ++i) {
    desc_.RTVFormats[i] = formats[i];
  }
  return *this;
}

PipelineStateBuilder& PipelineStateBuilder::SetDepthStencilFormat(DXGI_FORMAT format) {
  desc_.DSVFormat = format;
  return *this;
}

// ========== Primitive Topology ==========

PipelineStateBuilder& PipelineStateBuilder::SetPrimitiveTopologyType(D3D12_PRIMITIVE_TOPOLOGY_TYPE topology_type) {
  desc_.PrimitiveTopologyType = topology_type;
  return *this;
}

// ========== Sample Desc ==========

PipelineStateBuilder& PipelineStateBuilder::SetSampleDesc(UINT count, UINT quality) {
  desc_.SampleDesc.Count = count;
  desc_.SampleDesc.Quality = quality;
  return *this;
}

// ========== Preset Configurations ==========

PipelineStateBuilder& PipelineStateBuilder::UseForwardRenderingDefaults() {
  // Standard forward rendering: single RT, depth testing enabled
  SetRenderTargetFormat(DXGI_FORMAT_R8G8B8A8_UNORM, 0);
  SetDepthStencilFormat(DXGI_FORMAT_D32_FLOAT);

  SetDepthEnable(true);
  SetDepthWriteMask(D3D12_DEPTH_WRITE_MASK_ALL);
  SetDepthFunc(D3D12_COMPARISON_FUNC_LESS);

  SetCullMode(D3D12_CULL_MODE_BACK);
  SetFillMode(D3D12_FILL_MODE_SOLID);

  SetBlendEnable(false, 0);

  return *this;
}

PipelineStateBuilder& PipelineStateBuilder::UseDeferredGBufferDefaults() {
  // Deferred G-Buffer pass: multiple render targets, depth write enabled
  // Typical G-Buffer: Albedo, Normal, Roughness/Metallic, Emission
  SetRenderTargetFormat(DXGI_FORMAT_R8G8B8A8_UNORM, 0);      // Albedo
  SetRenderTargetFormat(DXGI_FORMAT_R16G16B16A16_FLOAT, 1);  // Normal
  SetRenderTargetFormat(DXGI_FORMAT_R8G8B8A8_UNORM, 2);      // Roughness/Metallic
  SetRenderTargetFormat(DXGI_FORMAT_R16G16B16A16_FLOAT, 3);  // Emission
  SetDepthStencilFormat(DXGI_FORMAT_D32_FLOAT);

  SetDepthEnable(true);
  SetDepthWriteMask(D3D12_DEPTH_WRITE_MASK_ALL);
  SetDepthFunc(D3D12_COMPARISON_FUNC_LESS);

  SetCullMode(D3D12_CULL_MODE_BACK);
  SetFillMode(D3D12_FILL_MODE_SOLID);

  // No blending in G-Buffer pass
  SetBlendEnable(false, 0);
  SetBlendEnable(false, 1);
  SetBlendEnable(false, 2);
  SetBlendEnable(false, 3);

  return *this;
}

PipelineStateBuilder& PipelineStateBuilder::UseDeferredLightingDefaults() {
  // Deferred lighting pass: single RT, depth read-only, additive blending
  SetRenderTargetFormat(DXGI_FORMAT_R16G16B16A16_FLOAT, 0);  // HDR lighting buffer
  SetDepthStencilFormat(DXGI_FORMAT_D32_FLOAT);

  SetDepthEnable(true);
  SetDepthWriteMask(D3D12_DEPTH_WRITE_MASK_ZERO);  // Read-only depth
  SetDepthFunc(D3D12_COMPARISON_FUNC_GREATER_EQUAL);

  SetCullMode(D3D12_CULL_MODE_FRONT);  // Render back faces for light volumes
  SetFillMode(D3D12_FILL_MODE_SOLID);

  // Additive blending for light accumulation
  SetBlendEnable(true, 0);
  SetBlendFactors(D3D12_BLEND_ONE, D3D12_BLEND_ONE, D3D12_BLEND_ONE, D3D12_BLEND_ONE, 0);
  SetBlendOp(D3D12_BLEND_OP_ADD, 0);

  return *this;
}

PipelineStateBuilder& PipelineStateBuilder::UsePostProcessDefaults() {
  // Post-process pass: single RT, no depth, fullscreen quad
  SetRenderTargetFormat(DXGI_FORMAT_R8G8B8A8_UNORM, 0);

  SetDepthEnable(false);
  SetDepthWriteMask(D3D12_DEPTH_WRITE_MASK_ZERO);

  SetCullMode(D3D12_CULL_MODE_NONE);
  SetFillMode(D3D12_FILL_MODE_SOLID);

  SetBlendEnable(false, 0);

  return *this;
}

PipelineStateBuilder& PipelineStateBuilder::UseUIDefaults() {
  SetRenderTargetFormat(DXGI_FORMAT_R8G8B8A8_UNORM, 0);
  SetDepthStencilFormat(DXGI_FORMAT_D32_FLOAT);

  SetDepthEnable(false);
  SetDepthWriteMask(D3D12_DEPTH_WRITE_MASK_ZERO);

  SetCullMode(D3D12_CULL_MODE_NONE);
  SetFillMode(D3D12_FILL_MODE_SOLID);

  SetBlendEnable(true, 0);
  SetBlendFactors(D3D12_BLEND_SRC_ALPHA, D3D12_BLEND_INV_SRC_ALPHA, D3D12_BLEND_SRC_ALPHA, D3D12_BLEND_INV_SRC_ALPHA, 0);

  return *this;
}

PipelineStateBuilder& PipelineStateBuilder::UseForwardTransparentDefaults() {
  SetRenderTargetFormat(DXGI_FORMAT_R8G8B8A8_UNORM, 0);
  SetDepthStencilFormat(DXGI_FORMAT_D32_FLOAT);

  SetDepthEnable(true);
  SetDepthWriteMask(D3D12_DEPTH_WRITE_MASK_ZERO);
  SetDepthFunc(D3D12_COMPARISON_FUNC_LESS_EQUAL);

  SetCullMode(D3D12_CULL_MODE_BACK);
  SetFillMode(D3D12_FILL_MODE_SOLID);

  SetBlendEnable(true, 0);
  SetBlendFactors(D3D12_BLEND_SRC_ALPHA, D3D12_BLEND_INV_SRC_ALPHA, D3D12_BLEND_SRC_ALPHA, D3D12_BLEND_INV_SRC_ALPHA, 0);

  return *this;
}

// ========== Build ==========

bool PipelineStateBuilder::Build(ID3D12Device* device, ComPtr<ID3D12PipelineState>& out_pso) {
  if (device == nullptr) {
    std::cerr << "[PipelineStateBuilder] Device is null" << '\n';
    return false;
  }

  if (desc_.pRootSignature == nullptr) {
    std::cerr << "[PipelineStateBuilder] Root signature not set" << '\n';
    return false;
  }

  if (desc_.VS.pShaderBytecode == nullptr) {
    std::cerr << "[PipelineStateBuilder] Vertex shader not set" << '\n';
    return false;
  }

  HRESULT hr = device->CreateGraphicsPipelineState(&desc_, IID_PPV_ARGS(out_pso.GetAddressOf()));

  if (FAILED(hr)) {
    std::cerr << "[PipelineStateBuilder] Failed to create graphics pipeline state" << '\n';
    return false;
  }

  return true;
}

void PipelineStateBuilder::Reset() {
  input_elements_.clear();
  InitializeDefaults();
}
