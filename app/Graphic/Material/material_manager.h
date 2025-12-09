#pragma once

#include <memory>
#include <string>
#include <unordered_map>

#include "material_template.h"

// MaterialManager: Manages MaterialTemplates
// Provides centralized creation and lookup of material templates
class MaterialManager {
 public:
  MaterialManager() = default;
  ~MaterialManager() = default;

  MaterialManager(const MaterialManager&) = delete;
  MaterialManager& operator=(const MaterialManager&) = delete;

  // Create and register a new material template
  MaterialTemplate* CreateTemplate(const std::string& name,
    ID3D12PipelineState* pso,
    ID3D12RootSignature* root_signature,
    const std::vector<TextureSlotDefinition>& texture_slots = {},
    const std::vector<ConstantBufferDefinition>& constant_buffers = {});

  // Get template by name
  MaterialTemplate* GetTemplate(const std::string& name);
  const MaterialTemplate* GetTemplate(const std::string& name) const;

  // Check if template exists
  bool HasTemplate(const std::string& name) const;

  // Remove template
  void RemoveTemplate(const std::string& name);

  // Clear all templates
  void Clear();

  // Statistics
  size_t GetTemplateCount() const {
    return templates_.size();
  }

  void PrintStats() const;

 private:
  std::unordered_map<std::string, std::unique_ptr<MaterialTemplate>> templates_;
};
