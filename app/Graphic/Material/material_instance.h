#pragma once

#include <d3d12.h>

#include <cstdint>
#include <map>
#include <string>
#include <vector>

#include "material_template.h"
#include "texture_manager.h"

// MaterialInstance: Holds per-instance data (textures, constants)
// References a shared MaterialTemplate for PSO and root signature
class MaterialInstance {
 public:
  MaterialInstance() = default;
  ~MaterialInstance() = default;

  MaterialInstance(const MaterialInstance&) = delete;
  MaterialInstance& operator=(const MaterialInstance&) = delete;

  // Initialize with a template
  bool Initialize(MaterialTemplate* material_template);

  // Texture management
  void SetTexture(const std::string& slot_name, TextureHandle handle);
  TextureHandle GetTexture(const std::string& slot_name) const;
  bool HasTexture(const std::string& slot_name) const;

  // Constant buffer data management
  // Sets raw data for a constant buffer
  void SetConstantBufferData(const std::string& cb_name, const void* data, size_t size);

  // Template-based constant buffer helpers
  template <typename T>
  void SetConstantBuffer(const std::string& cb_name, const T& data) {
    SetConstantBufferData(cb_name, &data, sizeof(T));
  }

  template <typename T>
  bool GetConstantBuffer(const std::string& cb_name, T& out_data) const {
    auto it = constant_buffers_.find(cb_name);
    if (it == constant_buffers_.end() || it->second.size() != sizeof(T)) {
      return false;
    }
    std::memcpy(&out_data, it->second.data(), sizeof(T));
    return true;
  }

  // Bind this material instance for rendering
  // Sets PSO, root signature, textures, and constant buffers
  void Bind(ID3D12GraphicsCommandList* command_list, TextureManager& texture_manager) const;

  // Getters
  MaterialTemplate* GetTemplate() const {
    return template_;
  }

  bool IsValid() const {
    return template_ != nullptr && template_->IsValid();
  }

  void PrintInfo() const;

 private:
  MaterialTemplate* template_ = nullptr;

  // Texture assignments: slot_name -> TextureHandle
  std::map<std::string, TextureHandle> textures_;

  // Constant buffer data: cb_name -> raw byte data
  std::map<std::string, std::vector<uint8_t>> constant_buffers_;

  // Warning tracking to avoid spam
  mutable std::map<std::string, bool> warning_logged_;
};
