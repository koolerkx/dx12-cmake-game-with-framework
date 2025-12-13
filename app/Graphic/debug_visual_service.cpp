#include "debug_visual_service.h"

#include <array>
#include <cmath>
#include <span>

using namespace DirectX;

namespace {
// ============================================================================
// Unit Circle LUT (Lookup Tables)
// Precomputed (cos, sin) values for fixed segment counts to avoid sin/cos in hot path
// ============================================================================

using LUT16 = std::array<XMFLOAT2, 16>;
using LUT24 = std::array<XMFLOAT2, 24>;
using LUT32 = std::array<XMFLOAT2, 32>;

// Generate LUT for N segments covering [0, 2π)
template <size_t N>
constexpr std::array<XMFLOAT2, N> GenerateUnitCircleLUT() {
  std::array<XMFLOAT2, N> lut{};
  constexpr float TWO_PI = 6.28318530717958647692f;
  for (size_t i = 0; i < N; ++i) {
    float theta = (TWO_PI * static_cast<float>(i)) / static_cast<float>(N);
    lut[i] = XMFLOAT2(std::cos(theta), std::sin(theta));
  }
  return lut;
}

const LUT16& GetUnitCircle16() {
  static const LUT16 lut = GenerateUnitCircleLUT<16>();
  return lut;
}

const LUT24& GetUnitCircle24() {
  static const LUT24 lut = GenerateUnitCircleLUT<24>();
  return lut;
}

const LUT32& GetUnitCircle32() {
  static const LUT32 lut = GenerateUnitCircleLUT<32>();
  return lut;
}

std::span<const XMFLOAT2> GetUnitCircleLUT(DebugSegments seg) {
  switch (seg) {
    case DebugSegments::S16:
      return GetUnitCircle16();
    case DebugSegments::S24:
      return GetUnitCircle24();
    case DebugSegments::S32:
      return GetUnitCircle32();
    default:
      return GetUnitCircle16();  // Fallback
  }
}

// ============================================================================
// Helper Functions for Wire Primitive Generation
// NOTE: No sin/cos allowed in hot path. Use GetUnitCircleLUT() only.
// ============================================================================

// Get axis direction vector from enum
XMVECTOR GetAxisDirection(DebugAxis axis) {
  switch (axis) {
    case DebugAxis::X:
      return XMVectorSet(1.0f, 0.0f, 0.0f, 0.0f);
    case DebugAxis::Y:
      return XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
    case DebugAxis::Z:
      return XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f);
    default:
      return XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
  }
}

// Rotate vector by quaternion
XMVECTOR RotateByQuat(XMVECTOR v, XMVECTOR q) {
  return XMVector3Rotate(v, q);
}

}  // namespace

void DebugVisualService::DrawLine3D(
  const DirectX::XMFLOAT3& p0, const DirectX::XMFLOAT3& p1, const DebugColor& color, DebugDepthMode mode, DebugCategory category) {
  DebugLine3DCommand cmd;
  cmd.p0 = p0;
  cmd.p1 = p1;
  cmd.color = color;
  cmd.depthMode = mode;
  cmd.category = category;

  cmds_.lines3D.push_back(cmd);
}

void DebugVisualService::DrawAxisGizmo(const DirectX::XMFLOAT3& origin, float length, DebugDepthMode depthMode) {
  const float half = length * 0.5f;

  // X-axis (Red)
  DirectX::XMFLOAT3 x0 = {origin.x - half, origin.y, origin.z};
  DirectX::XMFLOAT3 x1 = {origin.x + half, origin.y, origin.z};
  DrawLine3D(x0, x1, DebugColor::Red(), depthMode, DebugCategory::Gizmo);

  // Y-axis (Green)
  DirectX::XMFLOAT3 y0 = {origin.x, origin.y - half, origin.z};
  DirectX::XMFLOAT3 y1 = {origin.x, origin.y + half, origin.z};
  DrawLine3D(y0, y1, DebugColor::Green(), depthMode, DebugCategory::Gizmo);

  // Z-axis (Blue)
  DirectX::XMFLOAT3 z0 = {origin.x, origin.y, origin.z - half};
  DirectX::XMFLOAT3 z1 = {origin.x, origin.y, origin.z + half};
  DrawLine3D(z0, z1, DebugColor::Blue(), depthMode, DebugCategory::Gizmo);
}

void DebugVisualService::DrawGrid(const XMFLOAT3& center,
  uint32_t grid_size,
  float cell_spacing,
  const DebugColor& color,
  DebugDepthMode mode,
  DebugCategory category) {
  // Calculate grid extents
  const float half_extent = static_cast<float>(grid_size) * cell_spacing;
  
  // Draw lines parallel to X axis (along Z direction)
  for (uint32_t i = 0; i <= grid_size * 2; ++i) {
    float z = center.z - half_extent + static_cast<float>(i) * cell_spacing;
    XMFLOAT3 p0 = {center.x - half_extent, center.y, z};
    XMFLOAT3 p1 = {center.x + half_extent, center.y, z};
    DrawLine3D(p0, p1, color, mode, category);
  }
  
  // Draw lines parallel to Z axis (along X direction)
  for (uint32_t i = 0; i <= grid_size * 2; ++i) {
    float x = center.x - half_extent + static_cast<float>(i) * cell_spacing;
    XMFLOAT3 p0 = {x, center.y, center.z - half_extent};
    XMFLOAT3 p1 = {x, center.y, center.z + half_extent};
    DrawLine3D(p0, p1, color, mode, category);
  }
}

void DebugVisualService::DrawWireBox(
  const DirectX::XMFLOAT3& min_point, const DirectX::XMFLOAT3& max_point, const DebugColor& color, DebugDepthMode mode) {
  // Define the 8 vertices of the box
  DirectX::XMFLOAT3 vertices[8] = {
    {min_point.x, min_point.y, min_point.z},  // 0: min corner
    {max_point.x, min_point.y, min_point.z},  // 1
    {max_point.x, max_point.y, min_point.z},  // 2
    {min_point.x, max_point.y, min_point.z},  // 3
    {min_point.x, min_point.y, max_point.z},  // 4
    {max_point.x, min_point.y, max_point.z},  // 5
    {max_point.x, max_point.y, max_point.z},  // 6: max corner
    {min_point.x, max_point.y, max_point.z}   // 7
  };

  // Draw bottom face (z = min)
  DrawLine3D(vertices[0], vertices[1], color, mode, DebugCategory::General);
  DrawLine3D(vertices[1], vertices[2], color, mode, DebugCategory::General);
  DrawLine3D(vertices[2], vertices[3], color, mode, DebugCategory::General);
  DrawLine3D(vertices[3], vertices[0], color, mode, DebugCategory::General);

  // Draw top face (z = max)
  DrawLine3D(vertices[4], vertices[5], color, mode, DebugCategory::General);
  DrawLine3D(vertices[5], vertices[6], color, mode, DebugCategory::General);
  DrawLine3D(vertices[6], vertices[7], color, mode, DebugCategory::General);
  DrawLine3D(vertices[7], vertices[4], color, mode, DebugCategory::General);

  // Draw vertical edges connecting bottom to top
  DrawLine3D(vertices[0], vertices[4], color, mode, DebugCategory::General);
  DrawLine3D(vertices[1], vertices[5], color, mode, DebugCategory::General);
  DrawLine3D(vertices[2], vertices[6], color, mode, DebugCategory::General);
  DrawLine3D(vertices[3], vertices[7], color, mode, DebugCategory::General);
}

void DebugVisualService::DrawLine2D(
  const DirectX::XMFLOAT2& p0, const DirectX::XMFLOAT2& p1, const DebugColor& color, DebugCategory2D category) {
  DebugLine2DCommand cmd;
  cmd.p0 = p0;
  cmd.p1 = p1;
  cmd.color = color;
  cmd.category = category;

  cmds2D_.lines2D.push_back(cmd);
}

void DebugVisualService::DrawRect2D(
  const DirectX::XMFLOAT2& top_left, const DirectX::XMFLOAT2& size, const DebugColor& color, DebugCategory2D category) {
  DebugRect2DCommand cmd;
  cmd.top_left = top_left;
  cmd.size = size;
  cmd.color = color;
  cmd.category = category;

  cmds2D_.rects2D.push_back(cmd);
}

// ============================================================================
// Wire Primitive Implementations
// ============================================================================

void DebugVisualService::DrawWireBox(const XMFLOAT3& center,
  const XMFLOAT4& rotation_quat,
  const XMFLOAT3& size,
  const DebugColor& color,
  DebugDepthMode mode,
  DebugCategory category) {
  // 8 local corners: (±0.5, ±0.5, ±0.5) * size
  const XMFLOAT3 local_corners[8] = {
    {-0.5f * size.x, -0.5f * size.y, -0.5f * size.z},  // 0: (-, -, -)
    {+0.5f * size.x, -0.5f * size.y, -0.5f * size.z},  // 1: (+, -, -)
    {+0.5f * size.x, +0.5f * size.y, -0.5f * size.z},  // 2: (+, +, -)
    {-0.5f * size.x, +0.5f * size.y, -0.5f * size.z},  // 3: (-, +, -)
    {-0.5f * size.x, -0.5f * size.y, +0.5f * size.z},  // 4: (-, -, +)
    {+0.5f * size.x, -0.5f * size.y, +0.5f * size.z},  // 5: (+, -, +)
    {+0.5f * size.x, +0.5f * size.y, +0.5f * size.z},  // 6: (+, +, +)
    {-0.5f * size.x, +0.5f * size.y, +0.5f * size.z}   // 7: (-, +, +)
  };

  // Transform corners to world space
  XMVECTOR quat = XMLoadFloat4(&rotation_quat);
  XMVECTOR centerV = XMLoadFloat3(&center);
  XMFLOAT3 world_corners[8];

  for (int i = 0; i < 8; ++i) {
    XMVECTOR localV = XMLoadFloat3(&local_corners[i]);
    XMVECTOR rotated = RotateByQuat(localV, quat);
    XMVECTOR world = XMVectorAdd(rotated, centerV);
    XMStoreFloat3(&world_corners[i], world);
  }

  // 12 edges (fixed, not dependent on segments)
  // Bottom face (z-)
  DrawLine3D(world_corners[0], world_corners[1], color, mode, category);
  DrawLine3D(world_corners[1], world_corners[2], color, mode, category);
  DrawLine3D(world_corners[2], world_corners[3], color, mode, category);
  DrawLine3D(world_corners[3], world_corners[0], color, mode, category);

  // Top face (z+)
  DrawLine3D(world_corners[4], world_corners[5], color, mode, category);
  DrawLine3D(world_corners[5], world_corners[6], color, mode, category);
  DrawLine3D(world_corners[6], world_corners[7], color, mode, category);
  DrawLine3D(world_corners[7], world_corners[4], color, mode, category);

  // Vertical edges
  DrawLine3D(world_corners[0], world_corners[4], color, mode, category);
  DrawLine3D(world_corners[1], world_corners[5], color, mode, category);
  DrawLine3D(world_corners[2], world_corners[6], color, mode, category);
  DrawLine3D(world_corners[3], world_corners[7], color, mode, category);
}

void DebugVisualService::DrawWireSphere(
  const XMFLOAT3& center, float radius, DebugSegments segments, const DebugColor& color, DebugDepthMode mode, DebugCategory category) {
  auto lut = GetUnitCircleLUT(segments);
  const size_t N = lut.size();

  XMVECTOR centerV = XMLoadFloat3(&center);

  // Helper lambda to draw a ring given two orthogonal axes
  auto DrawRing = [&](XMVECTOR axisU, XMVECTOR axisV) {
    for (size_t i = 0; i < N; ++i) {
      const auto& p0 = lut[i];
      const auto& p1 = lut[(i + 1) % N];

      XMVECTOR pos0 = XMVectorAdd(centerV, XMVectorScale(XMVectorAdd(XMVectorScale(axisU, p0.x), XMVectorScale(axisV, p0.y)), radius));
      XMVECTOR pos1 = XMVectorAdd(centerV, XMVectorScale(XMVectorAdd(XMVectorScale(axisU, p1.x), XMVectorScale(axisV, p1.y)), radius));

      XMFLOAT3 a, b;
      XMStoreFloat3(&a, pos0);
      XMStoreFloat3(&b, pos1);
      DrawLine3D(a, b, color, mode, category);
    }
  };

  // 3 great circles: XY, XZ, YZ planes
  XMVECTOR X = XMVectorSet(1.0f, 0.0f, 0.0f, 0.0f);
  XMVECTOR Y = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
  XMVECTOR Z = XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f);

  DrawRing(X, Y);  // XY plane (Z = 0)
  DrawRing(X, Z);  // XZ plane (Y = 0)
  DrawRing(Y, Z);  // YZ plane (X = 0)
}

void DebugVisualService::DrawWireCylinder(const XMFLOAT3& position,
  const XMFLOAT4& rotation_quat,
  float radius,
  float height,
  DebugAxis axis,
  DebugSegments segments,
  const DebugColor& color,
  DebugDepthMode mode,
  DebugCategory category) {
  auto lut = GetUnitCircleLUT(segments);
  const size_t N = lut.size();

  XMVECTOR quat = XMLoadFloat4(&rotation_quat);
  XMVECTOR posV = XMLoadFloat3(&position);

  // Get local axis direction and construct orthogonal basis
  XMVECTOR localAxisDir = GetAxisDirection(axis);
  XMVECTOR localRight, localForward;

  switch (axis) {
    case DebugAxis::Y:
      localRight = XMVectorSet(1.0f, 0.0f, 0.0f, 0.0f);    // X
      localForward = XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f);  // Z
      break;
    case DebugAxis::X:
      localRight = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);    // Y
      localForward = XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f);  // Z
      break;
    case DebugAxis::Z:
      localRight = XMVectorSet(1.0f, 0.0f, 0.0f, 0.0f);    // X
      localForward = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);  // Y
      break;
    default:
      localRight = XMVectorSet(1.0f, 0.0f, 0.0f, 0.0f);
      localForward = XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f);
      break;
  }

  // Rotate basis to world space
  XMVECTOR axisDir = RotateByQuat(localAxisDir, quat);
  XMVECTOR right = RotateByQuat(localRight, quat);
  XMVECTOR forward = RotateByQuat(localForward, quat);

  float halfHeight = height * 0.5f;
  XMVECTOR topCenter = XMVectorAdd(posV, XMVectorScale(axisDir, halfHeight));
  XMVECTOR botCenter = XMVectorAdd(posV, XMVectorScale(axisDir, -halfHeight));

  // Helper lambda to draw a ring
  auto DrawRing = [&](XMVECTOR center, XMVECTOR u, XMVECTOR v) {
    for (size_t i = 0; i < N; ++i) {
      const auto& p0 = lut[i];
      const auto& p1 = lut[(i + 1) % N];

      XMVECTOR pos0 = XMVectorAdd(center, XMVectorScale(XMVectorAdd(XMVectorScale(u, p0.x), XMVectorScale(v, p0.y)), radius));
      XMVECTOR pos1 = XMVectorAdd(center, XMVectorScale(XMVectorAdd(XMVectorScale(u, p1.x), XMVectorScale(v, p1.y)), radius));

      XMFLOAT3 a, b;
      XMStoreFloat3(&a, pos0);
      XMStoreFloat3(&b, pos1);
      DrawLine3D(a, b, color, mode, category);
    }
  };

  // Draw top and bottom rings
  DrawRing(topCenter, right, forward);
  DrawRing(botCenter, right, forward);

  // Draw 4 side lines at 0°, 90°, 180°, 270°
  const size_t indices[4] = {0, N / 4, N / 2, 3 * N / 4};
  for (size_t idx : indices) {
    const auto& p = lut[idx];
    XMVECTOR offset = XMVectorScale(XMVectorAdd(XMVectorScale(right, p.x), XMVectorScale(forward, p.y)), radius);
    XMVECTOR topPt = XMVectorAdd(topCenter, offset);
    XMVECTOR botPt = XMVectorAdd(botCenter, offset);

    XMFLOAT3 a, b;
    XMStoreFloat3(&a, topPt);
    XMStoreFloat3(&b, botPt);
    DrawLine3D(a, b, color, mode, category);
  }
}

void DebugVisualService::DrawWireCapsule(const XMFLOAT3& position,
  const XMFLOAT4& rotation_quat,
  float radius,
  float height,
  DebugAxis axis,
  DebugSegments segments,
  const DebugColor& color,
  DebugDepthMode mode,
  DebugCategory category) {
  // Degenerate case: fall back to sphere if height <= 2*radius
  if (height <= 2.0f * radius) {
    DrawWireSphere(position, radius, segments, color, mode, category);
    return;
  }

  auto lut = GetUnitCircleLUT(segments);
  const size_t N = lut.size();

  XMVECTOR quat = XMLoadFloat4(&rotation_quat);
  XMVECTOR posV = XMLoadFloat3(&position);

  // Get local axis direction and construct orthogonal basis
  XMVECTOR localAxisDir = GetAxisDirection(axis);
  XMVECTOR localRight, localForward;

  switch (axis) {
    case DebugAxis::Y:
      localRight = XMVectorSet(1.0f, 0.0f, 0.0f, 0.0f);    // X
      localForward = XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f);  // Z
      break;
    case DebugAxis::X:
      localRight = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);    // Y
      localForward = XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f);  // Z
      break;
    case DebugAxis::Z:
      localRight = XMVectorSet(1.0f, 0.0f, 0.0f, 0.0f);    // X
      localForward = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);  // Y
      break;
    default:
      localRight = XMVectorSet(1.0f, 0.0f, 0.0f, 0.0f);
      localForward = XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f);
      break;
  }

  // Rotate basis to world space
  XMVECTOR axisDir = RotateByQuat(localAxisDir, quat);
  XMVECTOR right = RotateByQuat(localRight, quat);
  XMVECTOR forward = RotateByQuat(localForward, quat);

  // Calculate cylinder half height (height of cylindrical portion)
  float halfCyl = height * 0.5f - radius;
  XMVECTOR topCenter = XMVectorAdd(posV, XMVectorScale(axisDir, halfCyl));
  XMVECTOR botCenter = XMVectorAdd(posV, XMVectorScale(axisDir, -halfCyl));

  // Helper lambda to draw a ring
  auto DrawRing = [&](XMVECTOR center, XMVECTOR u, XMVECTOR v) {
    for (size_t i = 0; i < N; ++i) {
      const auto& p0 = lut[i];
      const auto& p1 = lut[(i + 1) % N];

      XMVECTOR pos0 = XMVectorAdd(center, XMVectorScale(XMVectorAdd(XMVectorScale(u, p0.x), XMVectorScale(v, p0.y)), radius));
      XMVECTOR pos1 = XMVectorAdd(center, XMVectorScale(XMVectorAdd(XMVectorScale(u, p1.x), XMVectorScale(v, p1.y)), radius));

      XMFLOAT3 a, b;
      XMStoreFloat3(&a, pos0);
      XMStoreFloat3(&b, pos1);
      DrawLine3D(a, b, color, mode, category);
    }
  };

  // Helper lambda to draw a half-arc (silhouette)
  auto DrawHalfArc = [&](XMVECTOR center, XMVECTOR u, XMVECTOR v) {
    const size_t halfN = N / 2;
    for (size_t i = 0; i < halfN; ++i) {
      const auto& p0 = lut[i];
      const auto& p1 = lut[i + 1];

      XMVECTOR pos0 = XMVectorAdd(center, XMVectorScale(XMVectorAdd(XMVectorScale(u, p0.x), XMVectorScale(v, p0.y)), radius));
      XMVECTOR pos1 = XMVectorAdd(center, XMVectorScale(XMVectorAdd(XMVectorScale(u, p1.x), XMVectorScale(v, p1.y)), radius));

      XMFLOAT3 a, b;
      XMStoreFloat3(&a, pos0);
      XMStoreFloat3(&b, pos1);
      DrawLine3D(a, b, color, mode, category);
    }
  };

  // Draw top and bottom rings (at cylinder junction with hemispheres)
  DrawRing(topCenter, right, forward);
  DrawRing(botCenter, right, forward);

  // Draw 4 cylinder side lines at 0°, 90°, 180°, 270°
  const size_t indices[4] = {0, N / 4, N / 2, 3 * N / 4};
  for (size_t idx : indices) {
    const auto& p = lut[idx];
    XMVECTOR offset = XMVectorScale(XMVectorAdd(XMVectorScale(right, p.x), XMVectorScale(forward, p.y)), radius);
    XMVECTOR topPt = XMVectorAdd(topCenter, offset);
    XMVECTOR botPt = XMVectorAdd(botCenter, offset);

    XMFLOAT3 a, b;
    XMStoreFloat3(&a, topPt);
    XMStoreFloat3(&b, botPt);
    DrawLine3D(a, b, color, mode, category);
  }

  // Draw 4 silhouette half-arcs (hemisphere outlines)
  // In (axis, right) plane: top and bottom hemispheres
  DrawHalfArc(topCenter, right, axisDir);                  // Top hemisphere arc
  DrawHalfArc(botCenter, right, XMVectorNegate(axisDir));  // Bottom hemisphere arc (inverted axis)

  // In (axis, forward) plane: top and bottom hemispheres
  DrawHalfArc(topCenter, forward, axisDir);                  // Top hemisphere arc
  DrawHalfArc(botCenter, forward, XMVectorNegate(axisDir));  // Bottom hemisphere arc (inverted axis)
}
