#include "material_manager.h"

#include <cassert>
#include <iostream>

#include "material_instance.h"

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

MaterialInstance* MaterialManager::CreateInstance(const std::string& name, MaterialTemplate* material_template) {
  assert(material_template != nullptr);

  if (HasInstance(name)) {
    std::cerr << "[MaterialManager] Instance '" << name << "' already exists" << '\n';
    return GetInstance(name);
  }

  auto instance = std::make_unique<MaterialInstance>();
  if (!instance->Initialize(material_template)) {
    std::cerr << "[MaterialManager] Failed to initialize instance '" << name << "'" << '\n';
    return nullptr;
  }

  MaterialInstance* ptr = instance.get();
  instances_[name] = std::move(instance);

  std::cout << "[MaterialManager] Created instance: " << name << '\n';

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

MaterialInstance* MaterialManager::GetInstance(const std::string& name) {
  auto it = instances_.find(name);
  if (it != instances_.end()) {
    return it->second.get();
  }
  return nullptr;
}

const MaterialInstance* MaterialManager::GetInstance(const std::string& name) const {
  auto it = instances_.find(name);
  if (it != instances_.end()) {
    return it->second.get();
  }
  return nullptr;
}

bool MaterialManager::HasTemplate(const std::string& name) const {
  return templates_.find(name) != templates_.end();
}

bool MaterialManager::HasInstance(const std::string& name) const {
  return instances_.find(name) != instances_.end();
}

void MaterialManager::RemoveTemplate(const std::string& name) {
  auto it = templates_.find(name);
  if (it != templates_.end()) {
    std::cout << "[MaterialManager] Removed template: " << name << '\n';
    templates_.erase(it);
  }
}

void MaterialManager::RemoveInstance(const std::string& name) {
  auto it = instances_.find(name);
  if (it != instances_.end()) {
    std::cout << "[MaterialManager] Removed instance: " << name << '\n';
    instances_.erase(it);
  }
}

void MaterialManager::Clear() {
  std::cout << "[MaterialManager] Clearing " << templates_.size() << " templates and " << instances_.size() << " instances" << '\n';
  instances_.clear();
  templates_.clear();
}

void MaterialManager::PrintStats() const {
  std::cout << "\n=== Material Manager Statistics ===" << '\n';
  std::cout << "Total Templates: " << templates_.size() << '\n';
  std::cout << "Total Instances: " << instances_.size() << '\n';

  std::cout << "\nRegistered Templates:" << '\n';
  for (const auto& [name, template_ptr] : templates_) {
    std::cout << "  - " << name << " (Textures: " << template_ptr->GetTextureSlotCount()
              << ", CBs: " << template_ptr->GetConstantBufferCount() << ")" << '\n';
  }

  std::cout << "\nRegistered Instances:" << '\n';
  for (const auto& [name, instance_ptr] : instances_) {
    std::cout << "  - " << name;
    if (instance_ptr && instance_ptr->GetTemplate()) {
      std::cout << " (Template: " << instance_ptr->GetTemplate()->GetName() << ")";
    }
    std::cout << '\n';
  }

  std::cout << "===================================\n" << '\n';
}
