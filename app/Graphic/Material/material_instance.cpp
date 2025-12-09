#include "material_instance.h"

#include <cassert>
#include <cstring>
#include <iostream>

bool MaterialInstance::Initialize(MaterialTemplate* material_template) {
  assert(material_template != nullptr);

  if (!material_template->IsValid()) {
    std::cerr << "[MaterialInstance] Cannot initialize with invalid template" << '\n';
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

  std::cout << "[MaterialInstance] Initialized with template: " << template_->GetName() << '\n';

  return true;
}

void MaterialInstance::SetTexture(const std::string& slot_name, TextureHandle handle) {
  // Validate slot exists in template
  const TextureSlotDefinition* slot = template_->GetTextureSlot(slot_name);
  if (slot == nullptr) {
    if (warning_logged_[slot_name] == false) {
      std::cerr << "[MaterialInstance] Warning: Texture slot '" << slot_name << "' not defined in template '" << template_->GetName() << "'"
                << '\n';
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
      std::cerr << "[MaterialInstance] Warning: Constant buffer '" << cb_name << "' not defined in template '" << template_->GetName()
                << "'" << '\n';
      warning_logged_[cb_name] = true;
    }
    return;
  }

  if (size != cb_def->size_in_bytes) {
    std::cerr << "[MaterialInstance] Error: Constant buffer '" << cb_name << "' size mismatch. Expected " << cb_def->size_in_bytes
              << " bytes, got " << size << " bytes" << '\n';
    return;
  }

  // Copy data
  auto& buffer = constant_buffers_[cb_name];
  buffer.resize(size);
  std::memcpy(buffer.data(), data, size);
}

void MaterialInstance::Bind(ID3D12GraphicsCommandList* command_list, TextureManager& texture_manager) const {
  assert(command_list != nullptr);

  if (!IsValid()) {
    std::cerr << "[MaterialInstance] Cannot bind invalid material instance" << '\n';
    return;
  }

  // Set PSO and root signature
  command_list->SetPipelineState(template_->GetPSO());
  command_list->SetGraphicsRootSignature(template_->GetRootSignature());

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
      std::cerr << "[MaterialInstance] Warning: Invalid texture handle for slot '" << slot_def->name << "'" << '\n';
      continue;
    }

    // Get SRV and bind
    auto srv = texture->GetSRV();
    if (srv.IsValid() && srv.IsShaderVisible()) {
      command_list->SetGraphicsRootDescriptorTable(slot_def->root_parameter_index, srv.gpu);
    }
  }

  // Bind constant buffers
  // Note: For now, we're not implementing GPU upload of CB data
  // This would require upload buffers and proper synchronization
  // For Step 2, we'll leave this as a placeholder
  // Full implementation would be in Step 3 or later
}

void MaterialInstance::PrintInfo() const {
  std::cout << "\n=== MaterialInstance ===" << '\n';
  if (template_ != nullptr) {
    std::cout << "Template: " << template_->GetName() << '\n';
  } else {
    std::cout << "Template: None" << '\n';
  }

  std::cout << "\nBound Textures (" << textures_.size() << "):" << '\n';
  for (const auto& [name, handle] : textures_) {
    std::cout << "  - " << name << ": [" << handle.index << ":" << handle.generation << "]" << '\n';
  }

  std::cout << "\nConstant Buffers (" << constant_buffers_.size() << "):" << '\n';
  for (const auto& [name, data] : constant_buffers_) {
    std::cout << "  - " << name << ": " << data.size() << " bytes" << '\n';
  }

  std::cout << "========================\n" << '\n';
}
