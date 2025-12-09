#pragma once

#include <d3d12.h>

#include <string>
#include <vector>

// Defines a texture slot in the material
struct TextureSlotDefinition {
  std::string name;                    // e.g., "albedo", "normal", "roughness"
  UINT root_parameter_index;           // Root signature parameter index
  D3D12_SHADER_VISIBILITY visibility;  // Which shader stage can see it
};

// Defines a constant buffer in the material
struct ConstantBufferDefinition {
  std::string name;           // e.g., "MaterialConstants"
  UINT root_parameter_index;  // Root signature parameter index
  UINT size_in_bytes;         // Size of the constant buffer
  D3D12_SHADER_VISIBILITY visibility;
};

class MaterialTemplate {
 public:
  MaterialTemplate() = default;
  ~MaterialTemplate() = default;

  MaterialTemplate(const MaterialTemplate&) = delete;
  MaterialTemplate& operator=(const MaterialTemplate&) = delete;

  bool Initialize(ID3D12PipelineState* pso,
    ID3D12RootSignature* root_signature,
    const std::string& name,
    const std::vector<TextureSlotDefinition>& texture_slots = {},
    const std::vector<ConstantBufferDefinition>& constant_buffers = {});

  ID3D12PipelineState* GetPSO() const {
    return pso_;
  }

  ID3D12RootSignature* GetRootSignature() const {
    return root_signature_;
  }

  const std::string& GetName() const {
    return name_;
  }

  int GetTextureSlotCount() const {
    return static_cast<int>(texture_slots_.size());
  }

  const TextureSlotDefinition* GetTextureSlot(const std::string& name) const;
  const TextureSlotDefinition* GetTextureSlotByIndex(int index) const;

  int GetConstantBufferCount() const {
    return static_cast<int>(constant_buffers_.size());
  }

  const ConstantBufferDefinition* GetConstantBuffer(const std::string& name) const;
  const ConstantBufferDefinition* GetConstantBufferByIndex(int index) const;

  bool IsValid() const {
    return pso_ != nullptr && root_signature_ != nullptr;
  }

  void PrintInfo() const;

 private:
  ID3D12PipelineState* pso_ = nullptr;
  ID3D12RootSignature* root_signature_ = nullptr;
  std::string name_;

  std::vector<TextureSlotDefinition> texture_slots_;
  std::vector<ConstantBufferDefinition> constant_buffers_;
};
