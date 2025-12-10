#pragma once

#include "RenderPass/render_layer.h"
#include "RenderPass/scene_renderer.h"
#include "component.h"
#include "material_instance.h"
#include "mesh.h"

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

  void SetLayer(RenderLayer layer) {
    layer_ = layer;
  }

  void SetTag(RenderTag tag) {
    tag_ = tag;
  }

  void SetColor(const DirectX::XMFLOAT4& color) {
    color_ = color;
  }

  const DirectX::XMFLOAT4& GetColor() const {
    return color_;
  }

  void SetUVTransform(const DirectX::XMFLOAT4& uv_transform) {
    uv_transform_ = uv_transform;
  }

  const DirectX::XMFLOAT4& GetUVTransform() const {
    return uv_transform_;
  }

  Mesh* GetMesh() const {
    return mesh_;
  }

  MaterialInstance* GetMaterial() const {
    return material_;
  }

  RenderLayer GetLayer() const {
    return layer_;
  }

  RenderTag GetTag() const {
    return tag_;
  }

  // Rendering
  void OnRender(SceneRenderer& scene_renderer);

 private:
  Mesh* mesh_ = nullptr;
  MaterialInstance* material_ = nullptr;
  RenderLayer layer_ = RenderLayer::Opaque;
  RenderTag tag_ = RenderTag::Static;

  // Warning tracking (once-per-component)
  mutable bool missing_mesh_warned_ = false;
  mutable bool missing_material_warned_ = false;
  mutable bool missing_transform_warned_ = false;
  DirectX::XMFLOAT4 color_ = {1.0f, 1.0f, 1.0f, 1.0f};
  DirectX::XMFLOAT4 uv_transform_ = {0.0f, 0.0f, 1.0f, 1.0f};
};
