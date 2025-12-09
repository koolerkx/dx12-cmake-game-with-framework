#pragma once

#include <DirectXMath.h>

#include "component.h"

using namespace DirectX;

// TransformComponent: Manages position, rotation, scale and world matrix
class TransformComponent : public Component {
 public:
  TransformComponent() = default;
  ~TransformComponent() override = default;

  // Position
  void SetPosition(const XMFLOAT3& position) {
    position_ = position;
    dirty_ = true;
  }

  void SetPosition(float x, float y, float z) {
    position_ = XMFLOAT3(x, y, z);
    dirty_ = true;
  }

  const XMFLOAT3& GetPosition() const {
    return position_;
  }

  // Rotation (Euler angles in radians)
  void SetRotation(const XMFLOAT3& rotation) {
    rotation_ = rotation;
    dirty_ = true;
  }

  void SetRotation(float x, float y, float z) {
    rotation_ = XMFLOAT3(x, y, z);
    dirty_ = true;
  }

  const XMFLOAT3& GetRotation() const {
    return rotation_;
  }

  // Scale
  void SetScale(const XMFLOAT3& scale) {
    scale_ = scale;
    dirty_ = true;
  }

  void SetScale(float x, float y, float z) {
    scale_ = XMFLOAT3(x, y, z);
    dirty_ = true;
  }

  void SetScale(float uniform_scale) {
    scale_ = XMFLOAT3(uniform_scale, uniform_scale, uniform_scale);
    dirty_ = true;
  }

  const XMFLOAT3& GetScale() const {
    return scale_;
  }

  // World matrix
  XMMATRIX GetWorldMatrix();

 private:
  XMFLOAT3 position_ = {0.0f, 0.0f, 0.0f};
  XMFLOAT3 rotation_ = {0.0f, 0.0f, 0.0f};  // Euler angles in radians
  XMFLOAT3 scale_ = {1.0f, 1.0f, 1.0f};

  XMMATRIX cached_world_matrix_ = XMMatrixIdentity();
  bool dirty_ = true;

  void UpdateWorldMatrix();
};
