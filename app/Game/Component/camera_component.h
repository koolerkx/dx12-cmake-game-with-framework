#pragma once
#include <DirectXMath.h>

#include "component.h"
#include "game_object.h"

class CameraComponent : public Component {
 public:
  enum class ProjectionType { Perspective, Orthographic, OrthographicOffCenter };

  // Standard Perspective (3D Games)
  void SetPerspective(float fovY, float aspect, float nearZ, float farZ);

  // Standard Orthographic (UI / 2D)
  void SetOrthographic(float viewWidth, float viewHeight, float nearZ, float farZ);

  // Off-Center Orthographic (e.g., Cascaded Shadows)
  void SetOrthographicOffCenter(float l, float r, float b, float t, float nearZ, float farZ);

  DirectX::XMMATRIX GetViewMatrix() const;

  // 2. Get Projection Matrix (Cached)
  DirectX::XMMATRIX GetProjectionMatrix();

 private:
  void UpdateProjectionMatrix();

  ProjectionType proj_type_ = ProjectionType::Perspective;
  DirectX::XMMATRIX cached_proj_matrix_;
  bool is_dirty_ = true;

  // Union or separate vars for params
  float fov_y_, aspect_;
  float width_, height_;
  float min_x_, max_x_, min_y_, max_y_;
  float near_z_, far_z_;
};
