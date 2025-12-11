// Debug Visual Service
// Immediate-mode API for debug drawing.
// - 3D: DrawLine3D, DrawWireBox, DrawAxisGizmo (supports depth test or overlay).
// - 2D: DrawLine2D, DrawRect2D (screen-space overlay).
// - Filter with DebugCategory and DebugCategory2D.
#pragma once

#include <DirectXMath.h>

#include <vector>

// Color structure for debug visuals
struct DebugColor {
  float r, g, b, a;

  constexpr DebugColor() : r(1.0f), g(1.0f), b(1.0f), a(1.0f) {
  }
  constexpr DebugColor(float red, float green, float blue, float alpha = 1.0f) : r(red), g(green), b(blue), a(alpha) {
  }

  // Common colors
  static constexpr DebugColor White() {
    return {1.0f, 1.0f, 1.0f, 1.0f};
  }
  static constexpr DebugColor Red() {
    return {1.0f, 0.0f, 0.0f, 1.0f};
  }
  static constexpr DebugColor Green() {
    return {0.0f, 1.0f, 0.0f, 1.0f};
  }
  static constexpr DebugColor Blue() {
    return {0.0f, 0.0f, 1.0f, 1.0f};
  }
  static constexpr DebugColor Yellow() {
    return {1.0f, 1.0f, 0.0f, 1.0f};
  }
  static constexpr DebugColor Cyan() {
    return {0.0f, 1.0f, 1.0f, 1.0f};
  }
  static constexpr DebugColor Magenta() {
    return {1.0f, 0.0f, 1.0f, 1.0f};
  }
  static constexpr DebugColor Black() {
    return {0.0f, 0.0f, 0.0f, 1.0f};
  }

  // Convert to packed RGBA8
  uint32_t ToRGBA8() const {
    uint32_t r8 = static_cast<uint32_t>(r * 255.0f) & 0xFF;
    uint32_t g8 = static_cast<uint32_t>(g * 255.0f) & 0xFF;
    uint32_t b8 = static_cast<uint32_t>(b * 255.0f) & 0xFF;
    uint32_t a8 = static_cast<uint32_t>(a * 255.0f) & 0xFF;
    return (a8 << 24) | (b8 << 16) | (g8 << 8) | r8;
  }
};

// Depth testing mode for debug visuals
enum class DebugDepthMode {
  IgnoreDepth,  // Always visible (no depth test)
  TestDepth     // Respect depth buffer (read-only depth testing)
};

// Category for organizing debug visuals (3D)
enum class DebugCategory {
  General,
  Gizmo,
  Physics,
  Selection,
};

// Category for 2D UI debug visuals
enum class DebugCategory2D { General, Layout, Guides, Selection, All };

// 3D Line command
struct DebugLine3DCommand {
  DirectX::XMFLOAT3 p0, p1;
  DebugColor color;
  DebugDepthMode depthMode;
  DebugCategory category;
};

// 2D Line command (screen-space coordinates)
struct DebugLine2DCommand {
  DirectX::XMFLOAT2 p0, p1;
  DebugColor color;
  DebugCategory2D category;
};

// 2D Rectangle command (screen-space coordinates)
struct DebugRect2DCommand {
  DirectX::XMFLOAT2 top_left;
  DirectX::XMFLOAT2 size;
  DebugColor color;
  DebugCategory2D category;
};

// Command buffer that stores all debug draw commands for the current frame
struct DebugVisualCommandBuffer {
  void Clear() {
    lines3D.clear();
  }

  size_t GetTotalCommandCount() const {
    return lines3D.size();
  }

  // Command storage (future shapes will expand into lines3D)
  std::vector<DebugLine3DCommand> lines3D;
};

// 2D Command buffer for UI-space debug visuals
struct DebugVisualCommandBuffer2D {
  void Clear() {
    lines2D.clear();
    rects2D.clear();
  }

  size_t GetTotalCommandCount() const {
    return lines2D.size() + rects2D.size();
  }

  // Command storage
  std::vector<DebugLine2DCommand> lines2D;
  std::vector<DebugRect2DCommand> rects2D;
};

// Settings for debug visual rendering
struct DebugVisualSettings {
  // 3D category filters (bitwise flags)
  bool enable_3d_general = true;
  bool enable_3d_gizmo = true;
  bool enable_3d_physics = true;
  bool enable_3d_selection = true;

  // 2D category filters
  bool enable_2d_general = true;
  bool enable_2d_layout = true;
  bool enable_2d_guides = true;
  bool enable_2d_selection = true;

  // Global toggles
  bool enable_3d_debug = true;
  bool enable_2d_debug = true;

  // Check if a 3D category is enabled
  bool IsCategoryEnabled(DebugCategory category) const {
    if (!enable_3d_debug) return false;
    switch (category) {
      case DebugCategory::General:
        return enable_3d_general;
      case DebugCategory::Gizmo:
        return enable_3d_gizmo;
      case DebugCategory::Physics:
        return enable_3d_physics;
      case DebugCategory::Selection:
        return enable_3d_selection;
      default:
        return true;
    }
  }

  // Check if a 2D category is enabled
  bool IsCategoryEnabled(DebugCategory2D category) const {
    if (!enable_2d_debug) return false;
    if (category == DebugCategory2D::All) return true;
    switch (category) {
      case DebugCategory2D::General:
        return enable_2d_general;
      case DebugCategory2D::Layout:
        return enable_2d_layout;
      case DebugCategory2D::Guides:
        return enable_2d_guides;
      case DebugCategory2D::Selection:
        return enable_2d_selection;
      default:
        return true;
    }
  }
};

// High-level debug visual service
// Provides immediate-mode style API for debug drawing
// All commands are accumulated in a command buffer for rendering later
class DebugVisualService {
 public:
  DebugVisualService() = default;
  ~DebugVisualService() = default;

  // Call this at the beginning of each frame to clear previous commands
  void BeginFrame() {
    cmds_.Clear();
    cmds2D_.Clear();
  }

  // 3D Line drawing
  void DrawLine3D(const DirectX::XMFLOAT3& p0,
    const DirectX::XMFLOAT3& p1,
    const DebugColor& color = DebugColor::White(),
    DebugDepthMode mode = DebugDepthMode::TestDepth,
    DebugCategory category = DebugCategory::General);

  // 2D Line drawing (screen-space pixels)
  void DrawLine2D(const DirectX::XMFLOAT2& p0,
    const DirectX::XMFLOAT2& p1,
    const DebugColor& color = DebugColor::White(),
    DebugCategory2D category = DebugCategory2D::General);

  // 2D Rectangle drawing (screen-space pixels)
  void DrawRect2D(const DirectX::XMFLOAT2& top_left,
    const DirectX::XMFLOAT2& size,
    const DebugColor& color = DebugColor::White(),
    DebugCategory2D category = DebugCategory2D::General);

  // Convenience overloads for common line types
  void DrawAxisGizmo(const DirectX::XMFLOAT3& origin, float length = 1.0f, DebugDepthMode depthMode = DebugDepthMode::TestDepth);
  void DrawWireBox(const DirectX::XMFLOAT3& min_point,
    const DirectX::XMFLOAT3& max_point,
    const DebugColor& color = DebugColor::White(),
    DebugDepthMode mode = DebugDepthMode::TestDepth);

  // Get accumulated commands for rendering
  const DebugVisualCommandBuffer& GetCommands3D() const {
    return cmds_;
  }

  const DebugVisualCommandBuffer2D& GetCommands2D() const {
    return cmds2D_;
  }

  // Statistics
  size_t GetCommandCount() const {
    return cmds_.GetTotalCommandCount();
  }

  size_t Get2DCommandCount() const {
    return cmds2D_.GetTotalCommandCount();
  }

  // Settings access
  DebugVisualSettings& GetSettings() {
    return settings_;
  }

  const DebugVisualSettings& GetSettings() const {
    return settings_;
  }

 private:
  DebugVisualCommandBuffer cmds_;
  DebugVisualCommandBuffer2D cmds2D_;
  DebugVisualSettings settings_;
};
