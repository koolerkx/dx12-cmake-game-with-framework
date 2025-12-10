#include "texture.h"

#include <WICTextureLoader12.h>
#include <d3d12.h>

#include <cassert>
#include <iostream>
#include <memory>

#include "d3dx12.h"
#include "utils.h"

bool Texture::Create(ID3D12Device* device,
  UINT width,
  UINT height,
  DXGI_FORMAT format,
  DescriptorHeapAllocator& srv_allocator,
  D3D12_RESOURCE_FLAGS flags,
  UINT mip_levels,
  UINT array_size) {
  assert(device != nullptr);
  assert(width > 0 && height > 0);

  width_ = width;
  height_ = height;
  format_ = format;
  mip_levels_ = mip_levels;
  array_size_ = array_size;

  // Create texture resource
  CD3DX12_RESOURCE_DESC texture_desc =
    CD3DX12_RESOURCE_DESC::Tex2D(format, width, height, static_cast<UINT16>(array_size), static_cast<UINT16>(mip_levels), 1, 0, flags);

  CD3DX12_HEAP_PROPERTIES heap_props(D3D12_HEAP_TYPE_DEFAULT);

  ComPtr<ID3D12Resource> resource;
  HRESULT hr = device->CreateCommittedResource(
    &heap_props, D3D12_HEAP_FLAG_NONE, &texture_desc, D3D12_RESOURCE_STATE_COMMON, nullptr, IID_PPV_ARGS(resource.GetAddressOf()));

  if (FAILED(hr)) {
    std::cerr << "[Texture] Failed to create texture resource" << '\n';
    return false;
  }

  SetResource(resource, D3D12_RESOURCE_STATE_COMMON);

  // Create SRV
  if (!CreateSRV(device, srv_allocator)) {
    return false;
  }

  return true;
}

bool Texture::LoadFromFile(
  ID3D12Device* device, ID3D12GraphicsCommandList* command_list, const std::wstring& file_path, DescriptorHeapAllocator& srv_allocator) {
  assert(device != nullptr);
  assert(command_list != nullptr);

  ComPtr<ID3D12Resource> resource;
  ComPtr<ID3D12Resource> upload_heap;
  std::unique_ptr<uint8_t[]> decoded_data;
  D3D12_SUBRESOURCE_DATA subresource_data;

  HRESULT hr = DirectX::LoadWICTextureFromFileEx(device,
    file_path.c_str(),
    0,
    D3D12_RESOURCE_FLAG_NONE,
    DirectX::WIC_LOADER_DEFAULT,
    resource.GetAddressOf(),
    decoded_data,
    subresource_data);

  if (FAILED(hr)) {
    std::cerr << "[Texture] Failed to load texture: " << utils::WstringToUtf8(file_path) << '\n';
    return false;
  }

  // Get texture description
  D3D12_RESOURCE_DESC resource_desc = resource->GetDesc();
  width_ = static_cast<UINT>(resource_desc.Width);
  height_ = static_cast<UINT>(resource_desc.Height);
  format_ = resource_desc.Format;
  mip_levels_ = static_cast<UINT>(resource_desc.MipLevels);
  array_size_ = static_cast<UINT>(resource_desc.DepthOrArraySize);

  // Create upload heap
  const UINT64 upload_buffer_size = GetRequiredIntermediateSize(resource.Get(), 0, 1);

  CD3DX12_HEAP_PROPERTIES upload_heap_props(D3D12_HEAP_TYPE_UPLOAD);
  CD3DX12_RESOURCE_DESC upload_buffer_desc = CD3DX12_RESOURCE_DESC::Buffer(upload_buffer_size);

  hr = device->CreateCommittedResource(&upload_heap_props,
    D3D12_HEAP_FLAG_NONE,
    &upload_buffer_desc,
    D3D12_RESOURCE_STATE_GENERIC_READ,
    nullptr,
    IID_PPV_ARGS(upload_heap.GetAddressOf()));

  if (FAILED(hr)) {
    std::cerr << "[Texture] Failed to create upload heap" << '\n';
    return false;
  }

  // Upload texture data
  UpdateSubresources(command_list, resource.Get(), upload_heap.Get(), 0, 0, 1, &subresource_data);
  upload_heap_ = upload_heap;

  // Transition to shader resource
  CD3DX12_RESOURCE_BARRIER barrier =
    CD3DX12_RESOURCE_BARRIER::Transition(resource.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

  command_list->ResourceBarrier(1, &barrier);

  SetResource(resource, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

  // Create SRV
  if (!CreateSRV(device, srv_allocator)) {
    return false;
  }

  return true;
}

bool Texture::LoadFromMemory(ID3D12Device* device,
  ID3D12GraphicsCommandList* command_list,
  const void* pixel_data,
  UINT width,
  UINT height,
  DXGI_FORMAT format,
  DescriptorHeapAllocator& srv_allocator,
  UINT row_pitch) {
  assert(device != nullptr);
  assert(command_list != nullptr);
  assert(pixel_data != nullptr);
  assert(width > 0 && height > 0);

  width_ = width;
  height_ = height;
  format_ = format;
  mip_levels_ = 1;
  array_size_ = 1;

  // Calculate row pitch if not provided
  if (row_pitch == 0) {
    UINT bits_per_pixel = 0;
    switch (format) {
      case DXGI_FORMAT_R8G8B8A8_UNORM:
      case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
      case DXGI_FORMAT_B8G8R8A8_UNORM:
      case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB:
        bits_per_pixel = 32;
        break;
      case DXGI_FORMAT_R16G16B16A16_FLOAT:
        bits_per_pixel = 64;
        break;
      default:
        std::cerr << "[Texture] Unsupported format for automatic row pitch calculation" << '\n';
        return false;
    }
    row_pitch = (width * bits_per_pixel + 7) / 8;
  }

  // Create texture resource
  CD3DX12_RESOURCE_DESC texture_desc = CD3DX12_RESOURCE_DESC::Tex2D(format, width, height);
  CD3DX12_HEAP_PROPERTIES heap_props(D3D12_HEAP_TYPE_DEFAULT);

  ComPtr<ID3D12Resource> resource;
  HRESULT hr = device->CreateCommittedResource(
    &heap_props, D3D12_HEAP_FLAG_NONE, &texture_desc, D3D12_RESOURCE_STATE_COPY_DEST, nullptr, IID_PPV_ARGS(resource.GetAddressOf()));

  if (FAILED(hr)) {
    std::cerr << "[Texture] Failed to create texture resource" << '\n';
    return false;
  }

  // Create upload heap
  const UINT64 upload_buffer_size = GetRequiredIntermediateSize(resource.Get(), 0, 1);

  CD3DX12_HEAP_PROPERTIES upload_heap_props(D3D12_HEAP_TYPE_UPLOAD);
  CD3DX12_RESOURCE_DESC upload_buffer_desc = CD3DX12_RESOURCE_DESC::Buffer(upload_buffer_size);

  ComPtr<ID3D12Resource> upload_heap;
  hr = device->CreateCommittedResource(&upload_heap_props,
    D3D12_HEAP_FLAG_NONE,
    &upload_buffer_desc,
    D3D12_RESOURCE_STATE_GENERIC_READ,
    nullptr,
    IID_PPV_ARGS(upload_heap.GetAddressOf()));

  if (FAILED(hr)) {
    std::cerr << "[Texture] Failed to create upload heap" << '\n';
    return false;
  }

  // Prepare subresource data
  D3D12_SUBRESOURCE_DATA subresource_data = {};
  subresource_data.pData = pixel_data;
  subresource_data.RowPitch = row_pitch;
  subresource_data.SlicePitch = row_pitch * height;

  // Upload texture data
  UpdateSubresources(command_list, resource.Get(), upload_heap.Get(), 0, 0, 1, &subresource_data);
  upload_heap_ = upload_heap;

  // Transition to shader resource
  CD3DX12_RESOURCE_BARRIER barrier =
    CD3DX12_RESOURCE_BARRIER::Transition(resource.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

  command_list->ResourceBarrier(1, &barrier);

  SetResource(resource, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

  // Create SRV
  if (!CreateSRV(device, srv_allocator)) {
    return false;
  }

  return true;
}

bool Texture::CreateSRV(ID3D12Device* device, DescriptorHeapAllocator& srv_allocator) {
  assert(device != nullptr);
  assert(resource_ != nullptr);

  // Allocate descriptor
  srv_allocation_ = srv_allocator.Allocate(1);
  if (!srv_allocation_.IsValid()) {
    std::cerr << "[Texture] Failed to allocate SRV descriptor" << '\n';
    return false;
  }

  // Create SRV
  D3D12_SHADER_RESOURCE_VIEW_DESC srv_desc = {};
  srv_desc.Format = format_;
  srv_desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
  srv_desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
  srv_desc.Texture2D.MostDetailedMip = 0;
  srv_desc.Texture2D.MipLevels = mip_levels_;
  srv_desc.Texture2D.PlaneSlice = 0;
  srv_desc.Texture2D.ResourceMinLODClamp = 0.0f;

  device->CreateShaderResourceView(resource_.Get(), &srv_desc, srv_allocation_.cpu);

  return true;
}

void Texture::ReleaseUploadHeap() {
  upload_heap_.Reset();
}
