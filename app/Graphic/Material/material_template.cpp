#include "material_template.h"

#include <cassert>

#include "Framework/Logging/logger.h"

// TODO: validate the param index doesn't violate the engine convention (fixed root signature buffer)
bool MaterialTemplate::Initialize(ID3D12PipelineState* pso,
  ID3D12RootSignature* root_signature,
  const std::string& name,
  const std::vector<TextureSlotDefinition>& texture_slots,
  const std::vector<ConstantBufferDefinition>& constant_buffers) {
  assert(pso != nullptr);
  assert(root_signature != nullptr);

  // Store COM pointers to keep lifetime ( caller may pass temporary ComPtr.Get() )
  pso_ = pso;
  root_signature_ = root_signature;
  name_ = name;
  texture_slots_ = texture_slots;
  constant_buffers_ = constant_buffers;

  Logger::Logf(LogLevel::Info,
    LogCategory::Graphic,
    Logger::Here(),
    "[MaterialTemplate] Initialized: {} with {} texture slots, {} constant buffers",
    name_,
    texture_slots_.size(),
    constant_buffers_.size());

  return true;
}

const TextureSlotDefinition* MaterialTemplate::GetTextureSlot(const std::string& name) const {
  for (const auto& slot : texture_slots_) {
    if (slot.name == name) {
      return &slot;
    }
  }
  return nullptr;
}

const TextureSlotDefinition* MaterialTemplate::GetTextureSlotByIndex(int index) const {
  if (index < 0 || index >= static_cast<int>(texture_slots_.size())) {
    return nullptr;
  }
  return &texture_slots_[index];
}

const ConstantBufferDefinition* MaterialTemplate::GetConstantBuffer(const std::string& name) const {
  for (const auto& cb : constant_buffers_) {
    if (cb.name == name) {
      return &cb;
    }
  }
  return nullptr;
}

const ConstantBufferDefinition* MaterialTemplate::GetConstantBufferByIndex(int index) const {
  if (index < 0 || index >= static_cast<int>(constant_buffers_.size())) {
    return nullptr;
  }
  return &constant_buffers_[index];
}

void MaterialTemplate::PrintInfo() const {
  Logger::Logf(LogLevel::Info, LogCategory::Graphic, Logger::Here(), "=== MaterialTemplate: {} ===", name_);
  Logger::Logf(LogLevel::Info, LogCategory::Graphic, Logger::Here(), "PSO: {}", (pso_ != nullptr ? "Valid" : "Invalid"));
  Logger::Logf(LogLevel::Info, LogCategory::Graphic, Logger::Here(), "Root Signature: {}", (root_signature_ != nullptr ? "Valid" : "Invalid"));

  Logger::Logf(LogLevel::Info, LogCategory::Graphic, Logger::Here(), "Texture Slots ({}):", texture_slots_.size());
  for (const auto& slot : texture_slots_) {
    Logger::Logf(LogLevel::Info, LogCategory::Graphic, Logger::Here(), "  - {} (root param: {})", slot.name, slot.root_parameter_index);
  }

  Logger::Logf(LogLevel::Info, LogCategory::Graphic, Logger::Here(), "Constant Buffers ({}):", constant_buffers_.size());
  for (const auto& cb : constant_buffers_) {
    Logger::Logf(LogLevel::Info,
      LogCategory::Graphic,
      Logger::Here(),
      "  - {} (root param: {}, size: {} bytes)",
      cb.name,
      cb.root_parameter_index,
      cb.size_in_bytes);
  }

  Logger::Log(LogLevel::Info, LogCategory::Graphic, "================================");
}
