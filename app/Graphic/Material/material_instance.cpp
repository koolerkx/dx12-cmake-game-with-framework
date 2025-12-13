#include "material_instance.h"

#include <cassert>
#include <cstring>

#include "Framework/Logging/logger.h"

bool MaterialInstance::Initialize(MaterialTemplate* material_template) {
  assert(material_template != nullptr);

  if (!material_template->IsValid()) {
    Logger::Log(LogLevel::Error, LogCategory::Validation, "[MaterialInstance] Cannot initialize with invalid template");
    return false;
  }

  template_ = material_template;

  // Pre-allocate constant buffer storage based on template definitions
  for (int i = 0; i < template_->GetConstantBufferCount(); ++i) {
    const ConstantBufferDefinition* cb_def = template_->GetConstantBufferByIndex(i);
    if (cb_def != nullptr) {
      constant_buffers_[cb_def->name].resize(cb_def->size_in_bytes, 0);
    }
  }

  Logger::Logf(LogLevel::Info,
    LogCategory::Graphic,
    Logger::Here(),
    "[MaterialInstance] Initialized with template: {}",
    template_->GetName());

  return true;
}

void MaterialInstance::SetTexture(const std::string& slot_name, TextureHandle handle) {
  // Validate slot exists in template
  const TextureSlotDefinition* slot = template_->GetTextureSlot(slot_name);
  if (slot == nullptr) {
    if (warning_logged_[slot_name] == false) {
      Logger::Logf(LogLevel::Warn,
        LogCategory::Validation,
        Logger::Here(),
        "[MaterialInstance] Texture slot '{}' not defined in template '{}'",
        slot_name,
        template_->GetName());
      warning_logged_[slot_name] = true;
    }
    return;
  }

  textures_[slot_name] = handle;
}

TextureHandle MaterialInstance::GetTexture(const std::string& slot_name) const {
  auto it = textures_.find(slot_name);
  if (it != textures_.end()) {
    return it->second;
  }
  return INVALID_TEXTURE_HANDLE;
}

bool MaterialInstance::HasTexture(const std::string& slot_name) const {
  auto it = textures_.find(slot_name);
  return it != textures_.end() && it->second.IsValid();
}

void MaterialInstance::SetConstantBufferData(const std::string& cb_name, const void* data, size_t size) {
  assert(data != nullptr);

  // Validate CB exists in template
  const ConstantBufferDefinition* cb_def = template_->GetConstantBuffer(cb_name);
  if (cb_def == nullptr) {
    if (warning_logged_[cb_name] == false) {
      Logger::Logf(LogLevel::Warn,
        LogCategory::Validation,
        Logger::Here(),
        "[MaterialInstance] Constant buffer '{}' not defined in template '{}'",
        cb_name,
        template_->GetName());
      warning_logged_[cb_name] = true;
    }
    return;
  }

  if (size != cb_def->size_in_bytes) {
    Logger::Logf(LogLevel::Error,
      LogCategory::Validation,
      Logger::Here(),
      "[MaterialInstance] Constant buffer '{}' size mismatch. Expected {} bytes, got {} bytes",
      cb_name,
      cb_def->size_in_bytes,
      size);
    return;
  }

  // Copy data
  auto& buffer = constant_buffers_[cb_name];
  buffer.resize(size);
  std::memcpy(buffer.data(), data, size);
}

/// @pre the pipeline state and rootsignature should be set before bind the buffer
void MaterialInstance::Bind(ID3D12GraphicsCommandList* command_list, TextureManager& texture_manager) const {
  assert(command_list != nullptr);

  if (!IsValid()) {
    Logger::Log(LogLevel::Error, LogCategory::Validation, "[MaterialInstance] Cannot bind invalid material instance");
    return;
  }

  // Bind textures
  for (int i = 0; i < template_->GetTextureSlotCount(); ++i) {
    const TextureSlotDefinition* slot_def = template_->GetTextureSlotByIndex(i);
    if (slot_def == nullptr) continue;

    // Get texture handle for this slot
    TextureHandle handle = GetTexture(slot_def->name);
    if (!handle.IsValid()) {
      // No texture bound for this slot - could use a default texture here
      continue;
    }

    // Get texture from manager
    const Texture* texture = texture_manager.GetTexture(handle);
    if (texture == nullptr) {
      Logger::Logf(LogLevel::Warn,
        LogCategory::Validation,
        Logger::Here(),
        "[MaterialInstance] Invalid texture handle for slot '{}'",
        slot_def->name);
      continue;
    }

    // Get SRV and bind
    auto srv = texture->GetSRV();
    if (srv.IsValid() && srv.IsShaderVisible()) {
      command_list->SetGraphicsRootDescriptorTable(slot_def->root_parameter_index, srv.gpu);
    }
  }

  // TOOD: Bind constant buffers
}

void MaterialInstance::PrintInfo() const {
  Logger::Log(LogLevel::Info, LogCategory::Graphic, "=== MaterialInstance ===");
  if (template_ != nullptr) {
    Logger::Logf(LogLevel::Info, LogCategory::Graphic, Logger::Here(), "Template: {}", template_->GetName());
  } else {
    Logger::Log(LogLevel::Info, LogCategory::Graphic, "Template: None");
  }

  Logger::Logf(LogLevel::Info, LogCategory::Graphic, Logger::Here(), "Bound Textures ({}):", textures_.size());
  for (const auto& [name, handle] : textures_) {
    Logger::Logf(LogLevel::Info,
      LogCategory::Graphic,
      Logger::Here(),
      "  - {}: [{}:{}]",
      name,
      handle.index,
      handle.generation);
  }

  Logger::Logf(LogLevel::Info, LogCategory::Graphic, Logger::Here(), "Constant Buffers ({}):", constant_buffers_.size());
  for (const auto& [name, data] : constant_buffers_) {
    Logger::Logf(LogLevel::Info, LogCategory::Graphic, Logger::Here(), "  - {}: {} bytes", name, data.size());
  }

  Logger::Log(LogLevel::Info, LogCategory::Graphic, "========================");
}
