#pragma once

#include <d3d12.h>

#include <vector>

#include "Framework/types.h"

class RootSignatureBuilder {
 public:
  RootSignatureBuilder() = default;
  ~RootSignatureBuilder() = default;

  // Root parameter builders
  RootSignatureBuilder& AddDescriptorTable(D3D12_DESCRIPTOR_RANGE_TYPE range_type,
    UINT num_descriptors,
    UINT base_shader_register,
    D3D12_SHADER_VISIBILITY visibility = D3D12_SHADER_VISIBILITY_ALL);

  RootSignatureBuilder& AddRootConstant(
    UINT num_32bit_values, UINT shader_register, D3D12_SHADER_VISIBILITY visibility = D3D12_SHADER_VISIBILITY_ALL);

  RootSignatureBuilder& AddRootCBV(UINT shader_register, D3D12_SHADER_VISIBILITY visibility = D3D12_SHADER_VISIBILITY_ALL);

  RootSignatureBuilder& AddRootSRV(UINT shader_register, D3D12_SHADER_VISIBILITY visibility = D3D12_SHADER_VISIBILITY_ALL);

  RootSignatureBuilder& AddRootUAV(UINT shader_register, D3D12_SHADER_VISIBILITY visibility = D3D12_SHADER_VISIBILITY_ALL);

  // Static sampler builders
  RootSignatureBuilder& AddStaticSampler(UINT shader_register,
    D3D12_FILTER filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR,
    D3D12_TEXTURE_ADDRESS_MODE address_mode = D3D12_TEXTURE_ADDRESS_MODE_WRAP,
    D3D12_SHADER_VISIBILITY visibility = D3D12_SHADER_VISIBILITY_PIXEL);

  // Flags
  RootSignatureBuilder& AllowInputLayout();
  RootSignatureBuilder& DenyVertexShaderRootAccess();
  RootSignatureBuilder& DenyPixelShaderRootAccess();

  // Build final root signature
  bool Build(ID3D12Device* device, ComPtr<ID3D12RootSignature>& out_root_signature);

  // Reset builder for reuse
  void Reset();

 private:
  struct DescriptorTableEntry {
    std::vector<D3D12_DESCRIPTOR_RANGE> ranges;
    D3D12_SHADER_VISIBILITY visibility;
  };

  std::vector<D3D12_ROOT_PARAMETER> root_parameters_;
  std::vector<DescriptorTableEntry> descriptor_tables_;  // Keep ranges alive
  std::vector<D3D12_STATIC_SAMPLER_DESC> static_samplers_;
  D3D12_ROOT_SIGNATURE_FLAGS flags_ = D3D12_ROOT_SIGNATURE_FLAG_NONE;
};
