#pragma once

#include <d3d12.h>
#include <d3dcommon.h>

#include <string>
#include <unordered_map>

#include "types.h"

// enum class ShaderType { Vertex, Pixel, Geometry, Hull, Domain, Compute };
enum class ShaderType { Vertex, Pixel };

struct ShaderBlob {
  ComPtr<ID3DBlob> blob = nullptr;
  ShaderType type = ShaderType::Vertex;

  D3D12_SHADER_BYTECODE GetBytecode() const {
    D3D12_SHADER_BYTECODE bytecode = {};
    if (blob) {
      bytecode.pShaderBytecode = blob->GetBufferPointer();
      bytecode.BytecodeLength = blob->GetBufferSize();
    }
    return bytecode;
  }

  bool IsValid() const {
    return blob != nullptr;
  }
};

class ShaderManager {
 public:
  ShaderManager() = default;
  ~ShaderManager() = default;

  ShaderManager(const ShaderManager&) = delete;
  ShaderManager& operator=(const ShaderManager&) = delete;

  // Load compiled shader from file (.cso)
  bool LoadShader(const std::wstring& filepath, ShaderType type, const std::string& name);

  // Get cached shader by name
  const ShaderBlob* GetShader(const std::string& name) const;

  // Check if shader exists
  bool HasShader(const std::string& name) const;

  // Clear all cached shaders
  void Clear();

  // Get shader count for debugging
  size_t GetShaderCount() const {
    return shaders_.size();
  }

 private:
  std::unordered_map<std::string, ShaderBlob> shaders_;
};
