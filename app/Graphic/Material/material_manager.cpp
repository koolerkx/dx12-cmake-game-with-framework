#include "material_manager.h"

#include <cassert>
#include <iostream>

MaterialTemplate* MaterialManager::CreateTemplate(const std::string& name,
  ID3D12PipelineState* pso,
  ID3D12RootSignature* root_signature,
  const std::vector<TextureSlotDefinition>& texture_slots,
  const std::vector<ConstantBufferDefinition>& constant_buffers) {
  assert(pso != nullptr);
  assert(root_signature != nullptr);

  // Check if already exists
  if (HasTemplate(name)) {
    std::cerr << "[MaterialManager] Template '" << name << "' already exists" << '\n';
    return GetTemplate(name);
  }

  // Create new template
  auto material_template = std::make_unique<MaterialTemplate>();
  if (!material_template->Initialize(pso, root_signature, name, texture_slots, constant_buffers)) {
    std::cerr << "[MaterialManager] Failed to initialize template '" << name << "'" << '\n';
    return nullptr;
  }

  MaterialTemplate* ptr = material_template.get();
  templates_[name] = std::move(material_template);

  std::cout << "[MaterialManager] Created template: " << name << '\n';

  return ptr;
}

MaterialTemplate* MaterialManager::GetTemplate(const std::string& name) {
  auto it = templates_.find(name);
  if (it != templates_.end()) {
    return it->second.get();
  }
  return nullptr;
}

const MaterialTemplate* MaterialManager::GetTemplate(const std::string& name) const {
  auto it = templates_.find(name);
  if (it != templates_.end()) {
    return it->second.get();
  }
  return nullptr;
}

bool MaterialManager::HasTemplate(const std::string& name) const {
  return templates_.find(name) != templates_.end();
}

void MaterialManager::RemoveTemplate(const std::string& name) {
  auto it = templates_.find(name);
  if (it != templates_.end()) {
    std::cout << "[MaterialManager] Removed template: " << name << '\n';
    templates_.erase(it);
  }
}

void MaterialManager::Clear() {
  std::cout << "[MaterialManager] Clearing " << templates_.size() << " templates" << '\n';
  templates_.clear();
}

void MaterialManager::PrintStats() const {
  std::cout << "\n=== Material Manager Statistics ===" << '\n';
  std::cout << "Total Templates: " << templates_.size() << '\n';

  std::cout << "\nRegistered Templates:" << '\n';
  for (const auto& [name, template_ptr] : templates_) {
    std::cout << "  - " << name << " (Textures: " << template_ptr->GetTextureSlotCount()
              << ", CBs: " << template_ptr->GetConstantBufferCount() << ")" << '\n';
  }

  std::cout << "===================================\n" << '\n';
}
