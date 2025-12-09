#include "transform_component.h"

XMMATRIX TransformComponent::GetWorldMatrix() {
  if (dirty_) {
    UpdateWorldMatrix();
    dirty_ = false;
  }
  return cached_world_matrix_;
}

void TransformComponent::UpdateWorldMatrix() {
  XMMATRIX scale_matrix = XMMatrixScaling(scale_.x, scale_.y, scale_.z);
  XMMATRIX rotation_matrix = XMMatrixRotationRollPitchYaw(rotation_.x, rotation_.y, rotation_.z);
  XMMATRIX translation_matrix = XMMatrixTranslation(position_.x, position_.y, position_.z);

  cached_world_matrix_ = scale_matrix * rotation_matrix * translation_matrix;
}
