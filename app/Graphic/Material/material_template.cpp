#include "material_template.h"

#include <cassert>
#include <iostream>

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

  std::cout << "[MaterialTemplate] Initialized: " << name_ << " with " << texture_slots_.size() << " texture slots, "
            << constant_buffers_.size() << " constant buffers" << '\n';

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
  std::cout << "\n=== MaterialTemplate: " << name_ << " ===" << '\n';
  std::cout << "PSO: " << (pso_ != nullptr ? "Valid" : "Invalid") << '\n';
  std::cout << "Root Signature: " << (root_signature_ != nullptr ? "Valid" : "Invalid") << '\n';

  std::cout << "\nTexture Slots (" << texture_slots_.size() << "):" << '\n';
  for (const auto& slot : texture_slots_) {
    std::cout << "  - " << slot.name << " (root param: " << slot.root_parameter_index << ")" << '\n';
  }

  std::cout << "\nConstant Buffers (" << constant_buffers_.size() << "):" << '\n';
  for (const auto& cb : constant_buffers_) {
    std::cout << "  - " << cb.name << " (root param: " << cb.root_parameter_index << ", size: " << cb.size_in_bytes << " bytes)" << '\n';
  }

  std::cout << "================================\n" << '\n';
}
