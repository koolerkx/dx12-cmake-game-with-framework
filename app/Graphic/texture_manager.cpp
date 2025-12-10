#include "texture_manager.h"

#include <cassert>
#include <iostream>

bool TextureManager::Initialize(ID3D12Device* device, DescriptorHeapAllocator* srv_allocator, uint32_t max_textures) {
  assert(device != nullptr);
  assert(srv_allocator != nullptr);
  assert(max_textures > 0);

  device_ = device;
  srv_allocator_ = srv_allocator;
  max_textures_ = max_textures;

  // Pre-allocate slots
  slots_.resize(max_textures_);

  // Initialize free list
  free_list_.reserve(max_textures_);
  for (uint32_t i = 0; i < max_textures_; ++i) {
    free_list_.push_back(i);
  }

  active_count_ = 0;
  cache_hits_ = 0;
  cache_misses_ = 0;

  std::cout << "[TextureManager] Initialized with capacity: " << max_textures_ << '\n';
  return true;
}

TextureHandle TextureManager::LoadTexture(ID3D12GraphicsCommandList* command_list, const TextureLoadParams& params) {
  assert(command_list != nullptr);

  // Check cache first
  auto cache_it = cache_.find(params);
  if (cache_it != cache_.end()) {
    TextureHandle cached_handle = cache_it->second;
    if (ValidateHandle(cached_handle)) {
      ++cache_hits_;
      std::wcout << L"[TextureManager] Cache hit: " << params.file_path << '\n';
      return cached_handle;
    }
    // Handle is stale, remove from cache
    cache_.erase(cache_it);
  }

  ++cache_misses_;

  // Allocate new slot
  TextureHandle handle = AllocateSlot();
  if (!handle.IsValid()) {
    std::cerr << "[TextureManager] Failed to allocate slot for texture" << '\n';
    return INVALID_TEXTURE_HANDLE;
  }

  // Create texture object
  TextureSlot& slot = slots_[handle.index];
  slot.texture = std::make_unique<Texture>();

  // Load texture from file
  bool success = slot.texture->LoadFromFile(device_, command_list, params.file_path, *srv_allocator_);

  if (!success) {
    std::wcerr << L"[TextureManager] Failed to load texture: " << params.file_path << '\n';
    FreeSlot(handle.index);
    return INVALID_TEXTURE_HANDLE;
  }

  // Set debug name
  slot.debug_name = params.file_path;
  slot.texture->SetDebugName(params.file_path);

  // Add to cache
  cache_[params] = handle;

  std::wcout << L"[TextureManager] Loaded texture: " << params.file_path << L" [" << handle.index << L":" << handle.generation << L"]"
             << '\n';

  return handle;
}

TextureHandle TextureManager::CreateTexture(
  ID3D12GraphicsCommandList* command_list, const void* pixel_data, UINT width, UINT height, DXGI_FORMAT format, UINT row_pitch) {
  assert(command_list != nullptr);
  assert(pixel_data != nullptr);

  // Allocate new slot
  TextureHandle handle = AllocateSlot();
  if (!handle.IsValid()) {
    std::cerr << "[TextureManager] Failed to allocate slot for procedural texture" << '\n';
    return INVALID_TEXTURE_HANDLE;
  }

  // Create texture object
  TextureSlot& slot = slots_[handle.index];
  slot.texture = std::make_unique<Texture>();

  // Load texture from memory
  bool success = slot.texture->LoadFromMemory(device_, command_list, pixel_data, width, height, format, *srv_allocator_, row_pitch);

  if (!success) {
    std::cerr << "[TextureManager] Failed to create procedural texture" << '\n';
    FreeSlot(handle.index);
    return INVALID_TEXTURE_HANDLE;
  }

  // Set debug name
  slot.debug_name = L"ProceduralTexture_" + std::to_wstring(handle.index);
  slot.texture->SetDebugName(slot.debug_name);

  std::cout << "[TextureManager] Created procedural texture [" << handle.index << ":" << handle.generation << "]" << '\n';

  return handle;
}

TextureHandle TextureManager::CreateEmptyTexture(UINT width, UINT height, DXGI_FORMAT format, D3D12_RESOURCE_FLAGS flags) {
  // Allocate new slot
  TextureHandle handle = AllocateSlot();
  if (!handle.IsValid()) {
    std::cerr << "[TextureManager] Failed to allocate slot for empty texture" << '\n';
    return INVALID_TEXTURE_HANDLE;
  }

  // Create texture object
  TextureSlot& slot = slots_[handle.index];
  slot.texture = std::make_unique<Texture>();

  // Create empty texture
  bool success = slot.texture->Create(device_, width, height, format, *srv_allocator_, flags);

  if (!success) {
    std::cerr << "[TextureManager] Failed to create empty texture" << '\n';
    FreeSlot(handle.index);
    return INVALID_TEXTURE_HANDLE;
  }

  // Set debug name
  slot.debug_name = L"EmptyTexture_" + std::to_wstring(handle.index);
  slot.texture->SetDebugName(slot.debug_name);

  std::cout << "[TextureManager] Created empty texture [" << handle.index << ":" << handle.generation << "]" << '\n';

  return handle;
}

void TextureManager::ReleaseTexture(TextureHandle handle) {
  if (!ValidateHandle(handle)) {
    std::cerr << "[TextureManager] Attempted to release invalid texture handle [" << handle.index << ":" << handle.generation << "]"
              << '\n';
    return;
  }

  // Remove from cache if present
  for (auto it = cache_.begin(); it != cache_.end();) {
    if (it->second == handle) {
      it = cache_.erase(it);
    } else {
      ++it;
    }
  }

  // Free the slot
  FreeSlot(handle.index);

  std::cout << "[TextureManager] Released texture [" << handle.index << ":" << handle.generation << "]" << '\n';
}

Texture* TextureManager::GetTexture(TextureHandle handle) {
  if (!ValidateHandle(handle)) {
    return nullptr;
  }
  return slots_[handle.index].texture.get();
}

const Texture* TextureManager::GetTexture(TextureHandle handle) const {
  if (!ValidateHandle(handle)) {
    return nullptr;
  }
  return slots_[handle.index].texture.get();
}

bool TextureManager::IsValid(TextureHandle handle) const {
  return ValidateHandle(handle);
}

void TextureManager::Clear() {
  // Clear cache
  cache_.clear();

  // Reset all slots
  for (auto& slot : slots_) {
    slot.texture.reset();
    slot.in_use = false;
    ++slot.generation;
  }

  // Reset free list
  free_list_.clear();
  free_list_.reserve(max_textures_);
  for (uint32_t i = 0; i < max_textures_; ++i) {
    free_list_.push_back(i);
  }

  active_count_ = 0;

  std::cout << "[TextureManager] Cleared all textures" << '\n';
}

void TextureManager::PrintStats() const {
  std::cout << "\n=== Texture Manager Statistics ===" << '\n';
  std::cout << "Active Textures: " << active_count_ << "/" << max_textures_ << '\n';
  std::cout << "Cache Hits: " << cache_hits_ << '\n';
  std::cout << "Cache Misses: " << cache_misses_ << '\n';

  if (cache_hits_ + cache_misses_ > 0) {
    float hit_rate = static_cast<float>(cache_hits_) / static_cast<float>(cache_hits_ + cache_misses_) * 100.0f;
    std::cout << "Cache Hit Rate: " << hit_rate << "%" << '\n';
  }

  std::cout << "==================================\n" << '\n';
}

TextureHandle TextureManager::AllocateSlot() {
  if (free_list_.empty()) {
    std::cerr << "[TextureManager] Out of texture slots! Capacity: " << max_textures_ << '\n';
    return INVALID_TEXTURE_HANDLE;
  }

  // Get slot from free list
  uint32_t index = free_list_.back();
  free_list_.pop_back();

  // Initialize slot
  TextureSlot& slot = slots_[index];
  slot.in_use = true;
  // Generation stays the same until freed

  ++active_count_;

  // Create handle
  TextureHandle handle;
  handle.index = index;
  handle.generation = slot.generation;

  return handle;
}

void TextureManager::FreeSlot(uint32_t index) {
  assert(index < max_textures_);

  TextureSlot& slot = slots_[index];

  if (!slot.in_use) {
    std::cerr << "[TextureManager] Attempted to free slot that is not in use: " << index << '\n';
    return;
  }

  // Destroy texture
  slot.texture.reset();
  slot.in_use = false;
  slot.debug_name.clear();

  // Increment generation to invalidate existing handles
  ++slot.generation;

  // Return to free list
  free_list_.push_back(index);

  --active_count_;
}

bool TextureManager::ValidateHandle(TextureHandle handle) const {
  if (!handle.IsValid()) {
    return false;
  }

  if (handle.index >= max_textures_) {
    return false;
  }

  const TextureSlot& slot = slots_[handle.index];

  if (!slot.in_use) {
    return false;
  }

  if (slot.generation != handle.generation) {
    return false;
  }

  return true;
}
