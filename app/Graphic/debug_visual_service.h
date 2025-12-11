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
  TestDepth     // Respect depth buffer
};

// Category for organizing debug visuals
enum class DebugCategory { General, Gizmo, Physics, Selection, UI, Custom };

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
class DebugVisualCommandBuffer {
 public:
  void Clear() {
    lines3D.clear();
  }

  size_t GetTotalCommandCount() const {
    return lines3D.size();
  }

  // Command storage
  std::vector<DebugLine3DCommand> lines3D;
};

// 2D Command buffer for UI-space debug visuals
class DebugVisualCommandBuffer2D {
 public:
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
    DebugDepthMode mode = DebugDepthMode::IgnoreDepth,
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
  void DrawAxisGizmo(const DirectX::XMFLOAT3& origin, float length = 1.0f);
  void DrawWireBox(const DirectX::XMFLOAT3& min_point,
    const DirectX::XMFLOAT3& max_point,
    const DebugColor& color = DebugColor::White(),
    DebugDepthMode mode = DebugDepthMode::IgnoreDepth);

  // Get accumulated commands for rendering
  const DebugVisualCommandBuffer& GetCommands() const {
    return cmds_;
  }

  const DebugVisualCommandBuffer2D& Get2DCommands() const {
    return cmds2D_;
  }

  // Statistics
  size_t GetCommandCount() const {
    return cmds_.GetTotalCommandCount();
  }

  size_t Get2DCommandCount() const {
    return cmds2D_.GetTotalCommandCount();
  }

 private:
  DebugVisualCommandBuffer cmds_;
  DebugVisualCommandBuffer2D cmds2D_;
};
