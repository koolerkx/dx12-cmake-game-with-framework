#include "framework_default_assets.h"

#include "graphic.h"
#include "primitive_geometry_2d.h"
#include "texture_manager.h"

void FrameworkDefaultAssets::Initialize(Graphic& graphic) {
  graphic_ = &graphic;

  // Create a simple rect mesh using the primitive geometry helper.
  // Concrete textures and materials are left as placeholders for now.
  try {
    rect2d_mesh_ = graphic.GetPrimitiveGeometry2D().CreateSpriteQuad();
  } catch (...) {
    rect2d_mesh_ = nullptr;
  }
}

void FrameworkDefaultAssets::Shutdown() {
  // Nothing owned directly here; reset non-owning references.
  graphic_ = nullptr;
  rect2d_mesh_.reset();
  sprite_material_ = nullptr;
  debug_line_material_ = nullptr;
  // TextureHandles remain as-is; managers will clean up on full Graphic shutdown.
}

TextureHandle FrameworkDefaultAssets::GetWhiteTexture() const {
  return white_texture_;
}

TextureHandle FrameworkDefaultAssets::GetBlackTexture() const {
  return black_texture_;
}

TextureHandle FrameworkDefaultAssets::GetFlatNormalTexture() const {
  return flat_normal_texture_;
}

TextureHandle FrameworkDefaultAssets::GetErrorTexture() const {
  return error_texture_;
}

std::shared_ptr<Mesh> FrameworkDefaultAssets::GetRect2DMesh() const {
  return rect2d_mesh_;
}

MaterialInstance* FrameworkDefaultAssets::GetSprite2DDefaultMaterial() const {
  return sprite_material_;
}

MaterialInstance* FrameworkDefaultAssets::GetDebugLineMaterial() const {
  return debug_line_material_;
}
