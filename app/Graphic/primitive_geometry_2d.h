#pragma once

#include <d3d12.h>

#include <memory>

#include "mesh.h"

class UploadContext;

class PrimitiveGeometry2D {
 public:
  explicit PrimitiveGeometry2D(ID3D12Device* device);

  // Create a unit quad in NDC/world space for 2D sprites
  std::shared_ptr<Mesh> CreateRect(UploadContext& upload_context);

 private:
  ID3D12Device* device_;
};
