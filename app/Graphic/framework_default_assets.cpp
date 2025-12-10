#include "framework_default_assets.h"

#include <cstdint>
#include <vector>

#include "graphic.h"
#include "primitive_geometry_2d.h"
#include "texture_manager.h"

void FrameworkDefaultAssets::Initialize(Graphic& graphic) {
  graphic_ = &graphic;

  // Create a simple rect mesh using the primitive geometry helper.
  try {
    rect2d_mesh_ = graphic.GetPrimitiveGeometry2D().CreateRect();
  } catch (...) {
    rect2d_mesh_ = nullptr;
  }

  // Create default textures in a single upload batch using the upload context
  graphic.ExecuteImmediate([&](ID3D12GraphicsCommandList* cmd) {
    auto& tex_mgr = graphic.GetTextureManager();

    // 1x1 white
    uint8_t white_px[4] = {255, 255, 255, 255};
    white_texture_ = tex_mgr.CreateTextureFromMemory(cmd, white_px, 1, 1, DXGI_FORMAT_R8G8B8A8_UNORM, "Default_White");

    // 1x1 black
    uint8_t black_px[4] = {0, 0, 0, 255};
    black_texture_ = tex_mgr.CreateTextureFromMemory(cmd, black_px, 1, 1, DXGI_FORMAT_R8G8B8A8_UNORM, "Default_Black");

    // 1x1 flat normal (128,128,255,255)
    uint8_t flat_normal_px[4] = {128, 128, 255, 255};
    flat_normal_texture_ = tex_mgr.CreateTextureFromMemory(cmd, flat_normal_px, 1, 1, DXGI_FORMAT_R8G8B8A8_UNORM, "Default_FlatNormal");

    // 8x8 error checker (pink and black)
    const int checker_w = 8;
    const int checker_h = 8;
    std::vector<uint8_t> checker_data(checker_w * checker_h * 4);
    for (int y = 0; y < checker_h; ++y) {
      for (int x = 0; x < checker_w; ++x) {
        int idx = (y * checker_w + x) * 4;
        bool pink = ((x + y) % 2) == 0;
        if (pink) {
          checker_data[idx + 0] = 255;  // R
          checker_data[idx + 1] = 0;    // G
          checker_data[idx + 2] = 255;  // B
          checker_data[idx + 3] = 255;  // A
        } else {
          checker_data[idx + 0] = 0;
          checker_data[idx + 1] = 0;
          checker_data[idx + 2] = 0;
          checker_data[idx + 3] = 255;
        }
      }
    }

    error_texture_ =
      tex_mgr.CreateTextureFromMemory(cmd, checker_data.data(), checker_w, checker_h, DXGI_FORMAT_R8G8B8A8_UNORM, "Default_ErrorChecker");
  });
}

void FrameworkDefaultAssets::Shutdown() {
  // Release textures from managers (Graphic still owns managers at this point)
  if (graphic_) {
    auto& tex_mgr = graphic_->GetTextureManager();
    if (tex_mgr.IsValid(white_texture_)) tex_mgr.ReleaseTexture(white_texture_);
    if (tex_mgr.IsValid(black_texture_)) tex_mgr.ReleaseTexture(black_texture_);
    if (tex_mgr.IsValid(flat_normal_texture_)) tex_mgr.ReleaseTexture(flat_normal_texture_);
    if (tex_mgr.IsValid(error_texture_)) tex_mgr.ReleaseTexture(error_texture_);
  }

  // Nothing else owned directly here; reset non-owning references.
  white_texture_ = INVALID_TEXTURE_HANDLE;
  black_texture_ = INVALID_TEXTURE_HANDLE;
  flat_normal_texture_ = INVALID_TEXTURE_HANDLE;
  error_texture_ = INVALID_TEXTURE_HANDLE;

  graphic_ = nullptr;
  rect2d_mesh_.reset();
  sprite_material_ = nullptr;
  debug_line_material_ = nullptr;
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
