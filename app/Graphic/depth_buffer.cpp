#include "depth_buffer.h"

#include <cassert>

#include "d3dx12.h"

#include "Framework/Logging/logger.h"

DepthBuffer::~DepthBuffer() {
  ReleaseDescriptors();
}

void DepthBuffer::ReleaseDescriptors() {
  if (dsv_allocation_.IsValid() && dsv_allocator_ != nullptr) {
    dsv_allocator_->Free(dsv_allocation_);
  }
  if (srv_allocation_.IsValid() && srv_allocator_ != nullptr) {
    srv_allocator_->Free(srv_allocation_);
  }

  dsv_allocation_ = {};
  srv_allocation_ = {};
  dsv_allocator_ = nullptr;
  srv_allocator_ = nullptr;
}

bool DepthBuffer::Create(ID3D12Device* device,
  UINT width,
  UINT height,
  DescriptorHeapAllocator& dsv_allocator,
  DescriptorHeapAllocator* srv_allocator,
  DXGI_FORMAT format,
  UINT sample_count,
  UINT sample_quality) {
  assert(device != nullptr);
  assert(width > 0 && height > 0);

  ReleaseDescriptors();

  width_ = width;
  height_ = height;
  // Resolve resource/view formats.
  //
  // For depth SRV debug (ImGui/postprocess)
  // - Resource: R32_TYPELESS
  // - DSV:      D32_FLOAT
  // - SRV:      R32_FLOAT
  //
  // Accept either D32_FLOAT or R32_TYPELESS as the caller-facing "format".
  if (format == DXGI_FORMAT_D32_FLOAT || format == DXGI_FORMAT_R32_TYPELESS) {
    resource_format_ = DXGI_FORMAT_R32_TYPELESS;
    dsv_format_ = DXGI_FORMAT_D32_FLOAT;
    srv_format_ = DXGI_FORMAT_R32_FLOAT;
  } else {
    // Default behavior for other depth formats.
    dsv_format_ = format;
    srv_format_ = GetSRVFormat(format);
    resource_format_ = (srv_allocator != nullptr) ? GetTypelessFormat(format) : format;
  }

  // Create clear value
  D3D12_CLEAR_VALUE clear_value = {};
  // Clear format must be the DSV format (never typeless).
  clear_value.Format = dsv_format_;
  clear_value.DepthStencil.Depth = 1.0f;
  clear_value.DepthStencil.Stencil = 0;

  // Determine resource flags
  D3D12_RESOURCE_FLAGS flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
  if (srv_allocator == nullptr) {
    // If no SRV needed, deny shader resource to optimize
    flags |= D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE;
  }

  // Create texture resource
  CD3DX12_RESOURCE_DESC resource_desc =
    CD3DX12_RESOURCE_DESC::Tex2D(resource_format_, width, height, 1, 1, sample_count, sample_quality, flags);

  CD3DX12_HEAP_PROPERTIES heap_props(D3D12_HEAP_TYPE_DEFAULT);

  ComPtr<ID3D12Resource> resource;
  HRESULT hr = device->CreateCommittedResource(&heap_props,
    D3D12_HEAP_FLAG_NONE,
    &resource_desc,
    D3D12_RESOURCE_STATE_DEPTH_WRITE,
    &clear_value,
    IID_PPV_ARGS(resource.GetAddressOf()));

  if (FAILED(hr)) {
    Logger::Logf(LogLevel::Error,
      LogCategory::Resource,
      Logger::Here(),
      "[DepthBuffer] Failed to create depth stencil resource. width={} height={} format={} hr=0x{:08X}",
      width,
      height,
      static_cast<uint32_t>(format),
      static_cast<uint32_t>(hr));
    return false;
  }

  SetResource(resource, D3D12_RESOURCE_STATE_DEPTH_WRITE);

  // Create DSV
  if (!CreateDSV(device, dsv_allocator)) {
    return false;
  }

  // Create SRV if requested
  if (srv_allocator != nullptr) {
    if (!CreateSRV(device, *srv_allocator)) {
      return false;
    }
  }

  SetDebugName("DepthBuffer");
  return true;
}

void DepthBuffer::Clear(ID3D12GraphicsCommandList* command_list, float depth, UINT8 stencil) const {
  assert(command_list != nullptr);
  if (!IsValid()) return;

  command_list->ClearDepthStencilView(dsv_allocation_.cpu, D3D12_CLEAR_FLAG_DEPTH, depth, stencil, 0, nullptr);
}

bool DepthBuffer::CreateDSV(ID3D12Device* device, DescriptorHeapAllocator& dsv_allocator) {
  assert(device != nullptr);
  assert(resource_ != nullptr);

  dsv_allocator_ = &dsv_allocator;

  // Allocate descriptor
  dsv_allocation_ = dsv_allocator.Allocate(1);
  if (!dsv_allocation_.IsValid()) {
    Logger::Log(LogLevel::Error, LogCategory::Resource, "[DepthBuffer] Failed to allocate DSV descriptor");
    return false;
  }

  // Create DSV
  D3D12_DEPTH_STENCIL_VIEW_DESC dsv_desc = {};
  dsv_desc.Format = dsv_format_;
  dsv_desc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
  dsv_desc.Flags = D3D12_DSV_FLAG_NONE;
  dsv_desc.Texture2D.MipSlice = 0;

  device->CreateDepthStencilView(resource_.Get(), &dsv_desc, dsv_allocation_.cpu);

  return true;
}

bool DepthBuffer::CreateSRV(ID3D12Device* device, DescriptorHeapAllocator& srv_allocator) {
  assert(device != nullptr);
  assert(resource_ != nullptr);

  srv_allocator_ = &srv_allocator;

  // Allocate descriptor
  srv_allocation_ = srv_allocator.Allocate(1);
  if (!srv_allocation_.IsValid()) {
    Logger::Log(LogLevel::Error, LogCategory::Resource, "[DepthBuffer] Failed to allocate SRV descriptor");
    return false;
  }

  // Create SRV
  D3D12_SHADER_RESOURCE_VIEW_DESC srv_desc = {};
  srv_desc.Format = srv_format_;
  srv_desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
  srv_desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
  srv_desc.Texture2D.MostDetailedMip = 0;
  srv_desc.Texture2D.MipLevels = 1;
  srv_desc.Texture2D.PlaneSlice = 0;
  srv_desc.Texture2D.ResourceMinLODClamp = 0.0f;

  device->CreateShaderResourceView(resource_.Get(), &srv_desc, srv_allocation_.cpu);

  return true;
}

DXGI_FORMAT DepthBuffer::GetTypelessFormat(DXGI_FORMAT format) {
  switch (format) {
    case DXGI_FORMAT_D32_FLOAT_S8X24_UINT:
      return DXGI_FORMAT_R32G8X24_TYPELESS;
    case DXGI_FORMAT_D32_FLOAT:
      return DXGI_FORMAT_R32_TYPELESS;
    case DXGI_FORMAT_D24_UNORM_S8_UINT:
      return DXGI_FORMAT_R24G8_TYPELESS;
    case DXGI_FORMAT_D16_UNORM:
      return DXGI_FORMAT_R16_TYPELESS;
    default:
      return format;
  }
}

DXGI_FORMAT DepthBuffer::GetSRVFormat(DXGI_FORMAT format) {
  switch (format) {
    case DXGI_FORMAT_D32_FLOAT_S8X24_UINT:
      return DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS;
    case DXGI_FORMAT_D32_FLOAT:
      return DXGI_FORMAT_R32_FLOAT;
    case DXGI_FORMAT_D24_UNORM_S8_UINT:
      return DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
    case DXGI_FORMAT_D16_UNORM:
      return DXGI_FORMAT_R16_UNORM;
    default:
      return format;
  }
}
