#include "texture_manager.h"

#include <cassert>

#include "Framework/Logging/logger.h"
#include "Framework/utils.h"

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

  Logger::Logf(LogLevel::Info,
    LogCategory::Resource,
    Logger::Here(),
    "[TextureManager] Initialized with capacity: {}.",
    max_textures_);
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
      Logger::Logf(LogLevel::Debug,
        LogCategory::Resource,
        Logger::Here(),
        "[TextureManager] Cache hit: {}",
        WstringToUtf8(params.file_path));
      return cached_handle;
    }
    // Handle is stale, remove from cache
    cache_.erase(cache_it);
  }

  ++cache_misses_;

  // Allocate new slot
  TextureHandle handle = AllocateSlot();
  if (!handle.IsValid()) {
    Logger::Log(LogLevel::Error, LogCategory::Resource, "[TextureManager] Failed to allocate slot for texture.");
    return INVALID_TEXTURE_HANDLE;
  }

  // Create texture object
  TextureSlot& slot = slots_[handle.index];
  slot.texture = std::make_unique<Texture>();

  // Load texture from file
  bool success = slot.texture->LoadFromFile(device_, command_list, params.file_path, *srv_allocator_);

  if (!success) {
    Logger::Logf(LogLevel::Error,
      LogCategory::Resource,
      Logger::Here(),
      "[TextureManager] Failed to load texture: {}",
      WstringToUtf8(params.file_path));
    FreeSlot(handle.index);
    return INVALID_TEXTURE_HANDLE;
  }

  // Set debug name
  slot.debug_name = params.file_path;
  slot.texture->SetDebugName(params.file_path);

  // Add to cache
  cache_[params] = handle;

  Logger::Logf(LogLevel::Info,
    LogCategory::Resource,
    Logger::Here(),
    "[TextureManager] Loaded texture: {} [{}:{}]",
    WstringToUtf8(params.file_path),
    handle.index,
    handle.generation);

  return handle;
}

TextureHandle TextureManager::CreateTexture(
  ID3D12GraphicsCommandList* command_list, const void* pixel_data, UINT width, UINT height, DXGI_FORMAT format, UINT row_pitch) {
  assert(command_list != nullptr);
  assert(pixel_data != nullptr);

  // Allocate new slot
  TextureHandle handle = AllocateSlot();
  if (!handle.IsValid()) {
    Logger::Log(LogLevel::Error, LogCategory::Resource, "[TextureManager] Failed to allocate slot for procedural texture.");
    return INVALID_TEXTURE_HANDLE;
  }

  // Create texture object
  TextureSlot& slot = slots_[handle.index];
  slot.texture = std::make_unique<Texture>();

  // Load texture from memory
  bool success = slot.texture->LoadFromMemory(device_, command_list, pixel_data, width, height, format, *srv_allocator_, row_pitch);

  if (!success) {
    Logger::Log(LogLevel::Error, LogCategory::Resource, "[TextureManager] Failed to create procedural texture.");
    FreeSlot(handle.index);
    return INVALID_TEXTURE_HANDLE;
  }

  // Set debug name
  slot.debug_name = L"ProceduralTexture_" + std::to_wstring(handle.index);
  slot.texture->SetDebugName(slot.debug_name);

  Logger::Logf(LogLevel::Info,
    LogCategory::Resource,
    Logger::Here(),
    "[TextureManager] Created procedural texture [{}:{}].",
    handle.index,
    handle.generation);

  return handle;
}

TextureHandle TextureManager::CreateTextureFromMemory(
    ID3D12GraphicsCommandList* command_list,
    const void* data,
    UINT width,
    UINT height,
    DXGI_FORMAT format,
    const std::string& name) {
  assert(command_list != nullptr);
  assert(data != nullptr);

  // Enforce required format
  if (format != DXGI_FORMAT_R8G8B8A8_UNORM) {
    Logger::Log(LogLevel::Error,
      LogCategory::Validation,
      "[TextureManager] CreateTextureFromMemory: unsupported format (only R8G8B8A8_UNORM supported)."
    );
    return INVALID_TEXTURE_HANDLE;
  }

  // Allocate new slot
  TextureHandle handle = AllocateSlot();
  if (!handle.IsValid()) {
    Logger::Log(LogLevel::Error, LogCategory::Resource, "[TextureManager] Failed to allocate slot for memory texture.");
    return INVALID_TEXTURE_HANDLE;
  }

  // Create texture object
  TextureSlot& slot = slots_[handle.index];
  slot.texture = std::make_unique<Texture>();

  // provided command list. It will not execute/submit the command list.
  bool success = slot.texture->LoadFromMemory(device_, command_list, data, width, height, format, *srv_allocator_);

  if (!success) {
    Logger::Log(LogLevel::Error, LogCategory::Resource, "[TextureManager] Failed to create texture from memory.");
    FreeSlot(handle.index);
    return INVALID_TEXTURE_HANDLE;
  }

  // Set debug name
  slot.debug_name = Utf8ToWstring(name);
  slot.texture->SetDebugName(slot.debug_name);

  Logger::Logf(LogLevel::Info,
    LogCategory::Resource,
    Logger::Here(),
    "[TextureManager] Created memory texture [{}:{}] {}",
    handle.index,
    handle.generation,
    name);

  return handle;
}

TextureHandle TextureManager::CreateEmptyTexture(UINT width, UINT height, DXGI_FORMAT format, D3D12_RESOURCE_FLAGS flags) {
  // Allocate new slot
  TextureHandle handle = AllocateSlot();
  if (!handle.IsValid()) {
    Logger::Log(LogLevel::Error, LogCategory::Resource, "[TextureManager] Failed to allocate slot for empty texture.");
    return INVALID_TEXTURE_HANDLE;
  }

  // Create texture object
  TextureSlot& slot = slots_[handle.index];
  slot.texture = std::make_unique<Texture>();

  // Create empty texture
  bool success = slot.texture->Create(device_, width, height, format, *srv_allocator_, flags);

  if (!success) {
    Logger::Log(LogLevel::Error, LogCategory::Resource, "[TextureManager] Failed to create empty texture.");
    FreeSlot(handle.index);
    return INVALID_TEXTURE_HANDLE;
  }

  // Set debug name
  slot.debug_name = L"EmptyTexture_" + std::to_wstring(handle.index);
  slot.texture->SetDebugName(slot.debug_name);

  Logger::Logf(LogLevel::Info,
    LogCategory::Resource,
    Logger::Here(),
    "[TextureManager] Created empty texture [{}:{}].",
    handle.index,
    handle.generation);

  return handle;
}

void TextureManager::ReleaseTexture(TextureHandle handle) {
  if (!ValidateHandle(handle)) {
    Logger::Logf(LogLevel::Warn,
      LogCategory::Validation,
      Logger::Here(),
      "[TextureManager] Attempted to release invalid texture handle [{}:{}].",
      handle.index,
      handle.generation);
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

  Logger::Logf(LogLevel::Info,
    LogCategory::Resource,
    Logger::Here(),
    "[TextureManager] Released texture [{}:{}].",
    handle.index,
    handle.generation);
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

  Logger::Log(LogLevel::Info, LogCategory::Resource, "[TextureManager] Cleared all textures.");
}

void TextureManager::PrintStats() const {
  float hit_rate = 0.0f;
  if (cache_hits_ + cache_misses_ > 0) {
    hit_rate = static_cast<float>(cache_hits_) / static_cast<float>(cache_hits_ + cache_misses_) * 100.0f;
  }

  Logger::Logf(LogLevel::Info,
    LogCategory::Resource,
    Logger::Here(),
    "=== Texture Manager Statistics ===\nActive Textures: {}/{}\nCache Hits: {}\nCache Misses: {}\nCache Hit Rate: {:.2f}%\n==================================",
    active_count_,
    max_textures_,
    cache_hits_,
    cache_misses_,
    hit_rate);
}

TextureHandle TextureManager::AllocateSlot() {
  if (free_list_.empty()) {
    Logger::Logf(LogLevel::Error,
      LogCategory::Resource,
      Logger::Here(),
      "[TextureManager] Out of texture slots (capacity={}).",
      max_textures_);
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
    Logger::Logf(LogLevel::Warn,
      LogCategory::Validation,
      Logger::Here(),
      "[TextureManager] Attempted to free slot that is not in use: {}.",
      index);
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
