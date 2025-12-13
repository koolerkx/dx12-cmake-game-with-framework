#include "shader_manager.h"

#include <d3dcompiler.h>

#include "Framework/Logging/logger.h"

bool ShaderManager::LoadShader(const std::wstring& filepath, ShaderType type, const std::string& name) {
  // Check if already loaded
  if (shaders_.find(name) != shaders_.end()) {
    Logger::Logf(LogLevel::Warn,
      LogCategory::Validation,
      Logger::Here(),
      "[ShaderManager] Shader '{}' already loaded.",
      name);
    return false;
  }

  // Load shader blob from file
  ComPtr<ID3DBlob> blob;
  HRESULT hr = D3DReadFileToBlob(filepath.c_str(), blob.GetAddressOf());

  if (FAILED(hr)) {
    Logger::Logf(LogLevel::Error,
      LogCategory::Graphic,
      Logger::Here(),
      "[ShaderManager] Failed to load shader '{}' (hr=0x{:08X}).",
      name,
      static_cast<uint32_t>(hr));
    return false;
  }

  // Store shader
  ShaderBlob shader;
  shader.blob = blob;
  shader.type = type;

  shaders_[name] = shader;

  Logger::Logf(LogLevel::Info, LogCategory::Graphic, Logger::Here(), "[ShaderManager] Loaded shader: {}", name);
  return true;
}

const ShaderBlob* ShaderManager::GetShader(const std::string& name) const {
  auto it = shaders_.find(name);
  if (it != shaders_.end()) {
    return &it->second;
  }
  return nullptr;
}

bool ShaderManager::HasShader(const std::string& name) const {
  return shaders_.find(name) != shaders_.end();
}

void ShaderManager::Clear() {
  shaders_.clear();
}
