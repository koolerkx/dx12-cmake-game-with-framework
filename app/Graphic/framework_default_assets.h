#pragma once

#include <memory>

#include "texture_manager.h"

// Forward declarations for types from Graphic module
class Graphic;
class Mesh;
class MaterialInstance;
class MaterialTemplate;

class FrameworkDefaultAssets {
 public:
  FrameworkDefaultAssets() = default;
  ~FrameworkDefaultAssets() = default;

  // Initialize with a reference to the owning Graphic instance.
  // Implementation may create simple textures/meshes/materials using managers on Graphic.
  void Initialize(Graphic& graphic);
  void Shutdown();

  // Texture getters (may return invalid handles until populated)
  TextureHandle GetWhiteTexture() const;
  TextureHandle GetBlackTexture() const;
  TextureHandle GetFlatNormalTexture() const;
  TextureHandle GetErrorTexture() const;

  // Basic mesh used by 2D sprites / fullscreen quads
  std::shared_ptr<Mesh> GetRect2DMesh() const;

  // Default materials used by sprite rendering and debug lines
  MaterialInstance* GetSprite2DDefaultMaterial() const;
  MaterialTemplate* GetDebugLineTemplateOverlay() const;
  MaterialTemplate* GetDebugLineTemplateDepth() const;
  MaterialInstance* GetDebugLineMaterialOverlay() const;
  MaterialInstance* GetDebugLineMaterialDepth() const;  // Debug lines with depth testing
  // Backward-compatible helpers
  MaterialInstance* GetDebugLineMaterial() const {
    return GetDebugLineMaterialOverlay();
  }
  MaterialInstance* GetDebugLineDepthMaterial() const {
    return GetDebugLineMaterialDepth();
  }

 private:
  // Non-owning back-pointer to Graphic for creating assets
  Graphic* graphic_ = nullptr;

  // Placeholder handles/objects. Concrete initialization is done in Initialize().
  TextureHandle white_texture_{};
  TextureHandle black_texture_{};
  TextureHandle flat_normal_texture_{};
  TextureHandle error_texture_{};

  std::shared_ptr<Mesh> rect2d_mesh_ = nullptr;

  // Material templates and instances for default materials
  MaterialTemplate* sprite2d_template_ = nullptr;
  std::unique_ptr<MaterialInstance> sprite2d_default_;

  MaterialTemplate* debug_line_template_overlay_ = nullptr;
  std::unique_ptr<MaterialInstance> debug_line_overlay_default_;

  MaterialTemplate* debug_line_template_depth_ = nullptr;
  std::unique_ptr<MaterialInstance> debug_line_depth_default_;

  // Material instances are owned/managed by MaterialManager; we store raw pointers
  MaterialInstance* sprite_material_ = nullptr;
  MaterialInstance* debug_line_material_overlay_ = nullptr;
  MaterialInstance* debug_line_material_depth_ = nullptr;

  // Internal helper methods
  void CreateDefaultMaterials(Graphic& gfx);
  void CreateSprite2DMaterial(Graphic& gfx);
  void CreateDebugLineMaterials(Graphic& gfx);
};
