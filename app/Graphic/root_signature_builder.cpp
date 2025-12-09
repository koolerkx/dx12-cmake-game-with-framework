#include "root_signature_builder.h"

#include <d3d12.h>

#include <iostream>

RootSignatureBuilder& RootSignatureBuilder::AddDescriptorTable(
  D3D12_DESCRIPTOR_RANGE_TYPE range_type, UINT num_descriptors, UINT base_shader_register, D3D12_SHADER_VISIBILITY visibility) {
  // Create descriptor range
  D3D12_DESCRIPTOR_RANGE range = {};
  range.RangeType = range_type;
  range.NumDescriptors = num_descriptors;
  range.BaseShaderRegister = base_shader_register;
  range.RegisterSpace = 0;
  range.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

  // Create descriptor table entry
  DescriptorTableEntry table_entry;
  table_entry.ranges.push_back(range);
  table_entry.visibility = visibility;

  descriptor_tables_.push_back(table_entry);

  // Create root parameter
  D3D12_ROOT_PARAMETER root_param = {};
  root_param.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
  root_param.ShaderVisibility = visibility;
  root_param.DescriptorTable.NumDescriptorRanges = 1;
  root_param.DescriptorTable.pDescriptorRanges = descriptor_tables_.back().ranges.data();

  root_parameters_.push_back(root_param);

  return *this;
}

RootSignatureBuilder& RootSignatureBuilder::AddRootConstant(
  UINT num_32bit_values, UINT shader_register, D3D12_SHADER_VISIBILITY visibility) {
  D3D12_ROOT_PARAMETER root_param = {};
  root_param.ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
  root_param.ShaderVisibility = visibility;
  root_param.Constants.ShaderRegister = shader_register;
  root_param.Constants.RegisterSpace = 0;
  root_param.Constants.Num32BitValues = num_32bit_values;

  root_parameters_.push_back(root_param);
  return *this;
}

RootSignatureBuilder& RootSignatureBuilder::AddRootCBV(UINT shader_register, D3D12_SHADER_VISIBILITY visibility) {
  D3D12_ROOT_PARAMETER root_param = {};
  root_param.ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
  root_param.ShaderVisibility = visibility;
  root_param.Descriptor.ShaderRegister = shader_register;
  root_param.Descriptor.RegisterSpace = 0;

  root_parameters_.push_back(root_param);
  return *this;
}

RootSignatureBuilder& RootSignatureBuilder::AddRootSRV(UINT shader_register, D3D12_SHADER_VISIBILITY visibility) {
  D3D12_ROOT_PARAMETER root_param = {};
  root_param.ParameterType = D3D12_ROOT_PARAMETER_TYPE_SRV;
  root_param.ShaderVisibility = visibility;
  root_param.Descriptor.ShaderRegister = shader_register;
  root_param.Descriptor.RegisterSpace = 0;

  root_parameters_.push_back(root_param);
  return *this;
}

RootSignatureBuilder& RootSignatureBuilder::AddRootUAV(UINT shader_register, D3D12_SHADER_VISIBILITY visibility) {
  D3D12_ROOT_PARAMETER root_param = {};
  root_param.ParameterType = D3D12_ROOT_PARAMETER_TYPE_UAV;
  root_param.ShaderVisibility = visibility;
  root_param.Descriptor.ShaderRegister = shader_register;
  root_param.Descriptor.RegisterSpace = 0;

  root_parameters_.push_back(root_param);
  return *this;
}

RootSignatureBuilder& RootSignatureBuilder::AddStaticSampler(
  UINT shader_register, D3D12_FILTER filter, D3D12_TEXTURE_ADDRESS_MODE address_mode, D3D12_SHADER_VISIBILITY visibility) {
  D3D12_STATIC_SAMPLER_DESC sampler_desc = {};
  sampler_desc.Filter = filter;
  sampler_desc.AddressU = address_mode;
  sampler_desc.AddressV = address_mode;
  sampler_desc.AddressW = address_mode;
  sampler_desc.MipLODBias = 0;
  sampler_desc.MaxAnisotropy = 0;
  sampler_desc.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
  sampler_desc.BorderColor = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;
  sampler_desc.MinLOD = 0.0f;
  sampler_desc.MaxLOD = D3D12_FLOAT32_MAX;
  sampler_desc.ShaderRegister = shader_register;
  sampler_desc.RegisterSpace = 0;
  sampler_desc.ShaderVisibility = visibility;

  static_samplers_.push_back(sampler_desc);
  return *this;
}

RootSignatureBuilder& RootSignatureBuilder::AllowInputLayout() {
  flags_ |= D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
  return *this;
}

RootSignatureBuilder& RootSignatureBuilder::DenyVertexShaderRootAccess() {
  flags_ |= D3D12_ROOT_SIGNATURE_FLAG_DENY_VERTEX_SHADER_ROOT_ACCESS;
  return *this;
}

RootSignatureBuilder& RootSignatureBuilder::DenyPixelShaderRootAccess() {
  flags_ |= D3D12_ROOT_SIGNATURE_FLAG_DENY_PIXEL_SHADER_ROOT_ACCESS;
  return *this;
}

bool RootSignatureBuilder::Build(ID3D12Device* device, ComPtr<ID3D12RootSignature>& out_root_signature) {
  if (device == nullptr) {
    std::cerr << "[RootSignatureBuilder] Device is null" << '\n';
    return false;
  }

  // Create root signature desc
  D3D12_ROOT_SIGNATURE_DESC root_signature_desc = {};
  root_signature_desc.NumParameters = static_cast<UINT>(root_parameters_.size());
  root_signature_desc.pParameters = root_parameters_.empty() ? nullptr : root_parameters_.data();
  root_signature_desc.NumStaticSamplers = static_cast<UINT>(static_samplers_.size());
  root_signature_desc.pStaticSamplers = static_samplers_.empty() ? nullptr : static_samplers_.data();
  root_signature_desc.Flags = flags_;

  // Serialize
  ComPtr<ID3DBlob> signature_blob;
  ComPtr<ID3DBlob> error_blob;
  HRESULT hr = D3D12SerializeRootSignature(
    &root_signature_desc, D3D_ROOT_SIGNATURE_VERSION_1_0, signature_blob.GetAddressOf(), error_blob.GetAddressOf());

  if (FAILED(hr)) {
    std::cerr << "[RootSignatureBuilder] Failed to serialize root signature" << '\n';
    if (error_blob) {
      std::cerr << static_cast<char*>(error_blob->GetBufferPointer()) << '\n';
    }
    return false;
  }

  // Create root signature
  hr = device->CreateRootSignature(
    0, signature_blob->GetBufferPointer(), signature_blob->GetBufferSize(), IID_PPV_ARGS(out_root_signature.GetAddressOf()));

  if (FAILED(hr)) {
    std::cerr << "[RootSignatureBuilder] Failed to create root signature" << '\n';
    return false;
  }

  return true;
}

void RootSignatureBuilder::Reset() {
  root_parameters_.clear();
  descriptor_tables_.clear();
  static_samplers_.clear();
  flags_ = D3D12_ROOT_SIGNATURE_FLAG_NONE;
}
