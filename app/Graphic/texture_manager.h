#pragma once

#include <d3d12.h>
#include <dxgiformat.h>

#include <cstdint>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "descriptor_heap_allocator.h"
#include "texture.h"

// Lightweight handle for texture references
struct TextureHandle {
  uint32_t index = INVALID_INDEX;
  uint32_t generation = 0;

  static constexpr uint32_t INVALID_INDEX = UINT32_MAX;

  bool IsValid() const {
    return index != INVALID_INDEX;
  }

  bool operator==(const TextureHandle& other) const {
    return index == other.index && generation == other.generation;
  }

  bool operator!=(const TextureHandle& other) const {
    return !(*this == other);
  }
};

// Invalid handle constant
inline constexpr TextureHandle INVALID_TEXTURE_HANDLE = {TextureHandle::INVALID_INDEX, 0};

// Texture loading parameters for cache key
struct TextureLoadParams {
  std::wstring file_path;
  bool generate_mips = false;
  bool force_srgb = false;

  bool operator==(const TextureLoadParams& other) const {
    return file_path == other.file_path && generate_mips == other.generate_mips && force_srgb == other.force_srgb;
  }
};

// Hash function for TextureLoadParams (used in cache map)
namespace std {
template <>
struct hash<TextureLoadParams> {
  size_t operator()(const TextureLoadParams& params) const {
    size_t h1 = hash<wstring>()(params.file_path);
    size_t h2 = hash<bool>()(params.generate_mips);
    size_t h3 = hash<bool>()(params.force_srgb);
    return h1 ^ (h2 << 1) ^ (h3 << 2);
  }
};
}  // namespace std

class TextureManager {
 public:
  TextureManager() = default;
  ~TextureManager() = default;

  TextureManager(const TextureManager&) = delete;
  TextureManager& operator=(const TextureManager&) = delete;

  // Initialize with device and descriptor allocator
  bool Initialize(ID3D12Device* device, DescriptorHeapAllocator* srv_allocator, uint32_t max_textures = 1024);

  // Load texture from file with caching
  // Returns existing handle if already loaded with same parameters
  TextureHandle LoadTexture(ID3D12GraphicsCommandList* command_list, const TextureLoadParams& params);

  // Create procedural texture (non-cached)
  TextureHandle CreateTexture(
    ID3D12GraphicsCommandList* command_list, const void* pixel_data, UINT width, UINT height, DXGI_FORMAT format, UINT row_pitch = 0);

  // Create empty texture with specified parameters (non-cached)
  TextureHandle CreateEmptyTexture(UINT width, UINT height, DXGI_FORMAT format, D3D12_RESOURCE_FLAGS flags = D3D12_RESOURCE_FLAG_NONE);

  // Release texture by handle
  void ReleaseTexture(TextureHandle handle);

  // Get texture pointer (nullptr if invalid)
  Texture* GetTexture(TextureHandle handle);
  const Texture* GetTexture(TextureHandle handle) const;

  // Validate handle
  bool IsValid(TextureHandle handle) const;

  // Clear all textures
  void Clear();

  // Statistics
  uint32_t GetTextureCount() const {
    return active_count_;
  }
  uint32_t GetCapacity() const {
    return max_textures_;
  }
  uint32_t GetCacheHits() const {
    return cache_hits_;
  }
  uint32_t GetCacheMisses() const {
    return cache_misses_;
  }

  void PrintStats() const;

 private:
  struct TextureSlot {
    std::unique_ptr<Texture> texture = nullptr;
    uint32_t generation = 0;
    bool in_use = false;
    std::wstring debug_name;
  };

  ID3D12Device* device_ = nullptr;
  DescriptorHeapAllocator* srv_allocator_ = nullptr;
  uint32_t max_textures_ = 0;

  std::vector<TextureSlot> slots_;
  std::vector<uint32_t> free_list_;

  // Cache: maps loading parameters to handle
  std::unordered_map<TextureLoadParams, TextureHandle> cache_;

  uint32_t active_count_ = 0;
  uint32_t cache_hits_ = 0;
  uint32_t cache_misses_ = 0;

  // Internal allocation
  TextureHandle AllocateSlot();
  void FreeSlot(uint32_t index);

  // Validation
  bool ValidateHandle(TextureHandle handle) const;
};
