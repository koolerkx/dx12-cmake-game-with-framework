#include "renderer_component.h"

#include <iostream>

#include "game_object.h"
#include "transform_component.h"

void RendererComponent::OnRender(SceneRenderer& scene_renderer) {
  // Validate mesh
  if (mesh_ == nullptr) {
    if (!missing_mesh_warned_) {
      std::cerr << "[RendererComponent] Warning: No mesh assigned";
      if (owner_ != nullptr) {
        std::cerr << " (GameObject: " << owner_->GetName() << ")";
      }
      std::cerr << '\n';
      missing_mesh_warned_ = true;
    }
    return;
  }

  // Validate material
  if (material_ == nullptr) {
    if (!missing_material_warned_) {
      std::cerr << "[RendererComponent] Warning: No material assigned";
      if (owner_ != nullptr) {
        std::cerr << " (GameObject: " << owner_->GetName() << ")";
      }
      std::cerr << '\n';
      missing_material_warned_ = true;
    }
    return;
  }

  // Get transform
  TransformComponent* transform = owner_->GetComponent<TransformComponent>();
  if (transform == nullptr) {
    if (!missing_transform_warned_) {
      std::cerr << "[RendererComponent] Warning: No TransformComponent found";
      if (owner_ != nullptr) {
        std::cerr << " (GameObject: " << owner_->GetName() << ")";
      }
      std::cerr << '\n';
      missing_transform_warned_ = true;
    }
    return;
  }

  // Create and submit render packet
  RenderPacket packet;
  packet.mesh = mesh_;
  packet.material = material_;
  packet.transform = transform->GetWorldMatrix();

  // Get layer/tag from owner GameObject
  if (owner_ != nullptr) {
    packet.layer = owner_->GetRenderLayer();
    packet.tag = owner_->GetRenderTag();
  }

  scene_renderer.Submit(packet);
}
