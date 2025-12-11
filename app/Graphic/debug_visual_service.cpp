#include "debug_visual_service.h"

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

void DebugVisualService::DrawAxisGizmo(const DirectX::XMFLOAT3& origin, float length) {
  // X-axis (Red)
  DirectX::XMFLOAT3 x_end = {origin.x + length, origin.y, origin.z};
  DrawLine3D(origin, x_end, DebugColor::Red(), DebugDepthMode::IgnoreDepth, DebugCategory::Gizmo);

  // Y-axis (Green)
  DirectX::XMFLOAT3 y_end = {origin.x, origin.y + length, origin.z};
  DrawLine3D(origin, y_end, DebugColor::Green(), DebugDepthMode::IgnoreDepth, DebugCategory::Gizmo);

  // Z-axis (Blue)
  DirectX::XMFLOAT3 z_end = {origin.x, origin.y, origin.z + length};
  DrawLine3D(origin, z_end, DebugColor::Blue(), DebugDepthMode::IgnoreDepth, DebugCategory::Gizmo);
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
