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
  MaterialInstance* GetSprite2DDefaultMaterial() const;  // Alias to UI by default
  MaterialInstance* GetSpriteWorldOpaqueMaterial() const;
  MaterialInstance* GetSpriteWorldTransparentMaterial() const;
  MaterialInstance* GetSpriteUIMaterial() const;
  MaterialTemplate* GetDebugLineTemplateOverlay() const;
  MaterialTemplate* GetDebugLineTemplateDepth() const;
  MaterialInstance* GetDebugLineMaterialOverlay() const;
  MaterialInstance* GetDebugLineMaterialDepth() const;  // Debug lines with depth testing

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
  MaterialTemplate* sprite_world_opaque_template_ = nullptr;
  MaterialTemplate* sprite_world_transparent_template_ = nullptr;
  MaterialTemplate* sprite_ui_template_ = nullptr;

  MaterialTemplate* debug_line_template_overlay_ = nullptr;
  MaterialTemplate* debug_line_template_depth_ = nullptr;

  // Material instances are owned/managed by MaterialManager; we store raw pointers
  MaterialInstance* sprite_world_opaque_material_ = nullptr;
  MaterialInstance* sprite_world_transparent_material_ = nullptr;
  MaterialInstance* sprite_ui_material_ = nullptr;
  MaterialInstance* debug_line_material_overlay_ = nullptr;
  MaterialInstance* debug_line_material_depth_ = nullptr;

  // Internal helper methods
  void CreateDefaultMaterials(Graphic& gfx);
  void CreateSpriteMaterials(Graphic& gfx);
  void CreateDebugLineMaterials(Graphic& gfx);
};
