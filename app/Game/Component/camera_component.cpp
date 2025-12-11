#include "camera_component.h"

#include <DirectXMath.h>

#include <iostream>

#include "component.h"
#include "game_object.h"
#include "transform_component.h"

// Standard Perspective (3D Games)
void CameraComponent::SetPerspective(float fovY, float aspect, float nearZ, float farZ) {
  proj_type_ = ProjectionType::Perspective;
  fov_y_ = fovY;
  aspect_ = aspect;
  near_z_ = nearZ;
  far_z_ = farZ;
  is_dirty_ = true;
}

// Standard Orthographic (UI / 2D)
void CameraComponent::SetOrthographic(float viewWidth, float viewHeight, float nearZ, float farZ) {
  proj_type_ = ProjectionType::Orthographic;
  width_ = viewWidth;
  height_ = viewHeight;
  near_z_ = nearZ;
  far_z_ = farZ;
  is_dirty_ = true;
}

// Off-Center Orthographic (e.g., Cascaded Shadows)
void CameraComponent::SetOrthographicOffCenter(float l, float r, float b, float t, float nearZ, float farZ) {
  proj_type_ = ProjectionType::OrthographicOffCenter;
  min_x_ = l;
  max_x_ = r;
  min_y_ = b;
  max_y_ = t;
  near_z_ = nearZ;
  far_z_ = farZ;
  is_dirty_ = true;
}

DirectX::XMMATRIX CameraComponent::GetViewMatrix() const {
  auto* transform = owner_->GetComponent<TransformComponent>();

  if (transform == nullptr) {
    std::cerr << "[CameraComponent] Warning: No TransformComponent found on GameObject: " << owner_->GetName() << '\n';
    return DirectX::XMMatrixIdentity();
  }

  DirectX::XMVECTOR eye = DirectX::XMLoadFloat3(&transform->GetPosition());

  // Negate pitch (x) to make positive x rotation look up in left-handed coordinates
  DirectX::XMFLOAT3 rot = transform->GetRotation();
  DirectX::XMVECTOR forward =
    DirectX::XMVector3Rotate(DirectX::XMVectorSet(0, 0, 1, 0), DirectX::XMQuaternionRotationRollPitchYaw(rot.x, rot.y, rot.z));

  DirectX::XMVECTOR up = DirectX::XMVectorSet(0, 1, 0, 0);

  return DirectX::XMMatrixLookToLH(eye, forward, up);
}

DirectX::XMMATRIX CameraComponent::GetProjectionMatrix() {
  if (is_dirty_) {
    UpdateProjectionMatrix();
  }
  return cached_proj_matrix_;
}

void CameraComponent::UpdateProjectionMatrix() {
  switch (proj_type_) {
    case ProjectionType::Perspective:
      // Standard 3D Projection
      cached_proj_matrix_ = DirectX::XMMatrixPerspectiveFovLH(fov_y_, aspect_, near_z_, far_z_);
      break;

    case ProjectionType::Orthographic:
      // Standard 2D Projection (Centered)
      cached_proj_matrix_ = DirectX::XMMatrixOrthographicLH(width_, height_, near_z_, far_z_);
      break;

    case ProjectionType::OrthographicOffCenter:
      // Custom bounds (User specific requirement)
      cached_proj_matrix_ = DirectX::XMMatrixOrthographicOffCenterLH(min_x_, max_x_, min_y_, max_y_, near_z_, far_z_);
      break;
  }
  is_dirty_ = false;
}
