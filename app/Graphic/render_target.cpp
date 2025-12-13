#include "render_target.h"

#include <cassert>
#include <cstring>

#include "d3dx12.h"

#include "Framework/Logging/logger.h"

bool RenderTarget::Create(ID3D12Device* device,
  UINT width,
  UINT height,
  DXGI_FORMAT format,
  DescriptorHeapAllocator& rtv_allocator,
  DescriptorHeapAllocator* srv_allocator,
  const float* clear_color,
  UINT sample_count,
  UINT sample_quality) {
  assert(device != nullptr);
  assert(width > 0 && height > 0);

  width_ = width;
  height_ = height;
  format_ = format;

  if (clear_color != nullptr) {
    SetClearColor(clear_color);
  }

  // Create clear value
  D3D12_CLEAR_VALUE clear_value = {};
  clear_value.Format = format;
  std::memcpy(clear_value.Color, clear_color_.data(), sizeof(clear_color_));

  // Determine resource flags
  D3D12_RESOURCE_FLAGS flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
  if (srv_allocator == nullptr) {
    // If no SRV needed, deny shader resource to optimize
    flags |= D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE;
  }

  // Create texture resource
  CD3DX12_RESOURCE_DESC resource_desc = CD3DX12_RESOURCE_DESC::Tex2D(format, width, height, 1, 1, sample_count, sample_quality, flags);

  CD3DX12_HEAP_PROPERTIES heap_props(D3D12_HEAP_TYPE_DEFAULT);

  ComPtr<ID3D12Resource> resource;
  HRESULT hr = device->CreateCommittedResource(&heap_props,
    D3D12_HEAP_FLAG_NONE,
    &resource_desc,
    D3D12_RESOURCE_STATE_RENDER_TARGET,
    &clear_value,
    IID_PPV_ARGS(resource.GetAddressOf()));

  if (FAILED(hr)) {
    Logger::Logf(LogLevel::Error,
      LogCategory::Resource,
      Logger::Here(),
      "[RenderTarget] Failed to create render target resource. width={} height={} format={} hr=0x{:08X}",
      width,
      height,
      static_cast<uint32_t>(format),
      static_cast<uint32_t>(hr));
    return false;
  }

  SetResource(resource, D3D12_RESOURCE_STATE_RENDER_TARGET);

  // Create RTV
  if (!CreateRTV(device, rtv_allocator, format)) {
    return false;
  }

  // Create SRV if requested
  if (srv_allocator != nullptr) {
    if (!CreateSRV(device, *srv_allocator)) {
      return false;
    }
  }

  return true;
}

bool RenderTarget::CreateFromResource(
  ID3D12Device* device, ID3D12Resource* resource, DescriptorHeapAllocator& rtv_allocator, DXGI_FORMAT rtv_format) {
  assert(device != nullptr);
  assert(resource != nullptr);

  // Get resource description
  D3D12_RESOURCE_DESC resource_desc = resource->GetDesc();
  width_ = static_cast<UINT>(resource_desc.Width);
  height_ = resource_desc.Height;
  format_ = resource_desc.Format;

  // Set resource
  ComPtr<ID3D12Resource> resource_ptr;
  resource_ptr.Attach(resource);
  resource->AddRef();  // AddRef because Attach doesn't add reference

  SetResource(resource_ptr, D3D12_RESOURCE_STATE_PRESENT);

  // Create RTV
  DXGI_FORMAT view_format = (rtv_format != DXGI_FORMAT_UNKNOWN) ? rtv_format : format_;
  if (!CreateRTV(device, rtv_allocator, view_format)) {
    return false;
  }

  return true;
}

void RenderTarget::Clear(ID3D12GraphicsCommandList* command_list) const {
  assert(command_list != nullptr);
  if (!IsValid()) return;

  command_list->ClearRenderTargetView(rtv_allocation_.cpu, clear_color_.data(), 0, nullptr);
}

void RenderTarget::Clear(ID3D12GraphicsCommandList* command_list, const float* clear_color) const {
  assert(command_list != nullptr);
  assert(clear_color != nullptr);
  if (!IsValid()) return;

  command_list->ClearRenderTargetView(rtv_allocation_.cpu, clear_color, 0, nullptr);
}

void RenderTarget::SetClearColor(const float* color) {
  assert(color != nullptr);
  std::memcpy(clear_color_.data(), color, sizeof(clear_color_));
}

bool RenderTarget::CreateRTV(ID3D12Device* device, DescriptorHeapAllocator& rtv_allocator, DXGI_FORMAT rtv_format) {
  assert(device != nullptr);
  assert(resource_ != nullptr);

  // Allocate descriptor
  rtv_allocation_ = rtv_allocator.Allocate(1);
  if (!rtv_allocation_.IsValid()) {
    Logger::Log(LogLevel::Error, LogCategory::Resource, "[RenderTarget] Failed to allocate RTV descriptor");
    return false;
  }

  // Create RTV
  D3D12_RENDER_TARGET_VIEW_DESC rtv_desc = {};
  rtv_desc.Format = rtv_format;
  rtv_desc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
  rtv_desc.Texture2D.MipSlice = 0;
  rtv_desc.Texture2D.PlaneSlice = 0;

  device->CreateRenderTargetView(resource_.Get(), &rtv_desc, rtv_allocation_.cpu);

  return true;
}

bool RenderTarget::CreateSRV(ID3D12Device* device, DescriptorHeapAllocator& srv_allocator) {
  assert(device != nullptr);
  assert(resource_ != nullptr);

  // Allocate descriptor
  srv_allocation_ = srv_allocator.Allocate(1);
  if (!srv_allocation_.IsValid()) {
    Logger::Log(LogLevel::Error, LogCategory::Resource, "[RenderTarget] Failed to allocate SRV descriptor");
    return false;
  }

  // Create SRV
  D3D12_SHADER_RESOURCE_VIEW_DESC srv_desc = {};
  srv_desc.Format = format_;
  srv_desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
  srv_desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
  srv_desc.Texture2D.MostDetailedMip = 0;
  srv_desc.Texture2D.MipLevels = 1;
  srv_desc.Texture2D.PlaneSlice = 0;
  srv_desc.Texture2D.ResourceMinLODClamp = 0.0f;

  device->CreateShaderResourceView(resource_.Get(), &srv_desc, srv_allocation_.cpu);

  return true;
}
