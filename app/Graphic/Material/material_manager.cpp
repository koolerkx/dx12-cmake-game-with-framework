#include "material_manager.h"

#include <cassert>

#include "Framework/Logging/logger.h"

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
    Logger::Logf(LogLevel::Warn,
      LogCategory::Validation,
      Logger::Here(),
      "[MaterialManager] Template '{}' already exists",
      name);
    return GetTemplate(name);
  }

  // Create new template
  auto material_template = std::make_unique<MaterialTemplate>();
  if (!material_template->Initialize(pso, root_signature, name, texture_slots, constant_buffers)) {
    Logger::Logf(LogLevel::Error,
      LogCategory::Graphic,
      Logger::Here(),
      "[MaterialManager] Failed to initialize template '{}'",
      name);
    return nullptr;
  }

  MaterialTemplate* ptr = material_template.get();
  templates_[name] = std::move(material_template);

  Logger::Logf(LogLevel::Info, LogCategory::Graphic, Logger::Here(), "[MaterialManager] Created template: {}", name);

  return ptr;
}

MaterialInstance* MaterialManager::CreateInstance(const std::string& name, MaterialTemplate* material_template) {
  assert(material_template != nullptr);

  if (HasInstance(name)) {
    Logger::Logf(LogLevel::Warn,
      LogCategory::Validation,
      Logger::Here(),
      "[MaterialManager] Instance '{}' already exists",
      name);
    return GetInstance(name);
  }

  auto instance = std::make_unique<MaterialInstance>();
  if (!instance->Initialize(material_template)) {
    Logger::Logf(LogLevel::Error,
      LogCategory::Graphic,
      Logger::Here(),
      "[MaterialManager] Failed to initialize instance '{}'",
      name);
    return nullptr;
  }

  MaterialInstance* ptr = instance.get();
  instances_[name] = std::move(instance);

  Logger::Logf(LogLevel::Info, LogCategory::Graphic, Logger::Here(), "[MaterialManager] Created instance: {}", name);

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
    Logger::Logf(LogLevel::Info, LogCategory::Graphic, Logger::Here(), "[MaterialManager] Removed template: {}", name);
    templates_.erase(it);
  }
}

void MaterialManager::RemoveInstance(const std::string& name) {
  auto it = instances_.find(name);
  if (it != instances_.end()) {
    Logger::Logf(LogLevel::Info, LogCategory::Graphic, Logger::Here(), "[MaterialManager] Removed instance: {}", name);
    instances_.erase(it);
  }
}

void MaterialManager::Clear() {
  Logger::Logf(LogLevel::Info,
    LogCategory::Graphic,
    Logger::Here(),
    "[MaterialManager] Clearing {} templates and {} instances",
    templates_.size(),
    instances_.size());
  instances_.clear();
  templates_.clear();
}

void MaterialManager::PrintStats() const {
  Logger::Log(LogLevel::Info, LogCategory::Graphic, "=== Material Manager Statistics ===");
  Logger::Logf(LogLevel::Info, LogCategory::Graphic, Logger::Here(), "Total Templates: {}", templates_.size());
  Logger::Logf(LogLevel::Info, LogCategory::Graphic, Logger::Here(), "Total Instances: {}", instances_.size());

  Logger::Log(LogLevel::Info, LogCategory::Graphic, "Registered Templates:");
  for (const auto& [name, template_ptr] : templates_) {
    Logger::Logf(LogLevel::Info,
      LogCategory::Graphic,
      Logger::Here(),
      "  - {} (Textures: {}, CBs: {})",
      name,
      template_ptr ? template_ptr->GetTextureSlotCount() : 0,
      template_ptr ? template_ptr->GetConstantBufferCount() : 0);
  }

  Logger::Log(LogLevel::Info, LogCategory::Graphic, "Registered Instances:");
  for (const auto& [name, instance_ptr] : instances_) {
    if (instance_ptr && instance_ptr->GetTemplate()) {
      Logger::Logf(LogLevel::Info,
        LogCategory::Graphic,
        Logger::Here(),
        "  - {} (Template: {})",
        name,
        instance_ptr->GetTemplate()->GetName());
    } else {
      Logger::Logf(LogLevel::Info, LogCategory::Graphic, Logger::Here(), "  - {}", name);
    }
  }

  Logger::Log(LogLevel::Info, LogCategory::Graphic, "===================================");
}
