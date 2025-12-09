#pragma once

#include <map>
#include <string>

#include "component.h"
#include "material_instance.h"
#include "mesh.h"
#include "scene_renderer.h"

// RendererComponent: Submits render packets to scene renderer
class RendererComponent : public Component {
 public:
  RendererComponent() = default;
  ~RendererComponent() override = default;

  // Setup
  void SetMesh(Mesh* mesh) {
    mesh_ = mesh;
  }

  void SetMaterial(MaterialInstance* material) {
    material_ = material;
  }

  Mesh* GetMesh() const {
    return mesh_;
  }

  MaterialInstance* GetMaterial() const {
    return material_;
  }

  // Rendering
  void OnRender(SceneRenderer& scene_renderer);

 private:
  Mesh* mesh_ = nullptr;
  MaterialInstance* material_ = nullptr;

  // Warning tracking (once-per-component)
  mutable bool missing_mesh_warned_ = false;
  mutable bool missing_material_warned_ = false;
  mutable bool missing_transform_warned_ = false;
};
