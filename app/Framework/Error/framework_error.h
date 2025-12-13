/**
@filename framework_error.h
@brief Public definitions for the engine error framework, including error domains, error codes, error metadata, and helper utilities for
failure handling.
@author Kooler Fan
**/
#pragma once

#include <winerror.h>

#include <atomic>
#include <cstdint>
#include <optional>
#include <source_location>
#include <stdexcept>
#include <string>
#include <string_view>

#include "Framework/Logging/logger.h"

enum class FrameworkErrorDomain : uint8_t {
  Core,
  // Meaning: Cross-cutting framework/runtime errors; Causes: initialization, configuration, unclassified failures; Handling: Level2 throw
  // at boundaries; LogCategory: Core.

  Graphic,
  // Meaning: D3D12/DXGI/GPU device pipeline errors; Causes: driver/device/feature mismatch; Handling: Level2 throw or Level3 return;
  // LogCategory: Graphic.

  Resource,
  // Meaning: Asset/resource IO and GPU resource lifetime issues; Causes: missing/corrupt assets, allocation failures; Handling: Level3
  // return with fallback when possible; LogCategory: Resource.

  Game,
  // Meaning: Game logic layer errors; Causes: invalid content/state; Handling: Level3 return or Level4 panic for invariant break;
  // LogCategory: Game.

  UI,
  // Meaning: UI/render overlay errors; Causes: invalid layout/state; Handling: Level3 return; LogCategory: UI.

  Validation,
  // Meaning: Contract/invariant violations; Causes: programmer error; Handling: Debug assert break, Release terminate for non-recoverable;
  // LogCategory: Validation.
};

enum class FrameworkErrorCode : uint16_t {
  // Meaning: Any unexpected failure without a more specific code; Causes: unclassified/legacy errors; Handling: Level2 throw at boundaries;
  // LogCategory: Core.
  Unknown,

  // Meaning: Public API contract violated (null/size/range); Causes: caller bug; Handling: Level3 return or Level2 throw for startup-only;
  // LogCategory: Validation.
  InvalidArgument,

  // Meaning: Internal state machine broken (e.g., called out-of-order); Causes: logic bug; Handling: Debug assert break, Release Level4
  // panic/terminate; LogCategory: Validation.
  InvalidState,

  // Meaning: DXGI factory init failure; Causes: OS/SDK mismatch, debug layer issues; Handling: Level2 throw (startup boundary);
  // LogCategory: Graphic; include HRESULT.
  DxgiFactoryCreateFailed,

  // Meaning: D3D12 device creation failure; Causes: unsupported adapter/feature level, driver issues; Handling: Level2 throw; LogCategory:
  // Graphic; include HRESULT.
  D3d12DeviceCreateFailed,

  // Meaning: D3D12 command queue creation failure; Causes: device mis-init, driver issues; Handling: Level2 throw; LogCategory: Graphic;
  // include HRESULT.
  CommandQueueCreateFailed,

  // Meaning: Per-frame command allocator creation failure; Causes: device failure/out-of-memory; Handling: Level2 throw; LogCategory:
  // Graphic; include HRESULT.
  CommandAllocatorCreateFailed,

  // Meaning: Command list creation failure; Causes: device failure/out-of-memory; Handling: Level2 throw; LogCategory: Graphic; include
  // HRESULT.
  CommandListCreateFailed,

  // Meaning: Descriptor heap manager initialization failed; Causes: invalid device/heap sizing; Handling: Level2 throw; LogCategory:
  // Graphic.
  DescriptorHeapManagerInitFailed,

  // Meaning: Texture manager initialization failed; Causes: invalid allocators/capacity; Handling: Level2 throw; LogCategory: Resource.
  TextureManagerInitFailed,

  // Meaning: Swapchain initialization failed; Causes: invalid HWND, DXGI setup, fullscreen restrictions; Handling: Level2 throw;
  // LogCategory: Graphic; include HRESULT if available.
  SwapchainInitFailed,

  // Meaning: Depth buffer resource/descriptor creation failed; Causes: allocation failure/unsupported format; Handling: Level2 throw;
  // LogCategory: Graphic; include HRESULT if available.
  DepthBufferCreateFailed,

  // Meaning: GPU fence setup failure; Causes: device/driver failures; Handling: Level2 throw; LogCategory: Graphic; include HRESULT if
  // available.
  FenceManagerInitFailed,

  // Meaning: Upload context init failure; Causes: allocator/list failures; Handling: Level2 throw; LogCategory: Resource; include HRESULT
  // if available.
  UploadContextInitFailed,

  // Meaning: RenderPassManager init failure; Causes: device incompatibility/invalid assumptions; Handling: Level2 throw; LogCategory:
  // Graphic.
  RenderPassManagerInitFailed,

  // Meaning: Render pass creation failed; Causes: PSO/root signature failures; Handling: Level2 throw; LogCategory: Graphic.
  RenderPassInitFailed,
};

struct FrameworkError {
  FrameworkErrorDomain domain = FrameworkErrorDomain::Core;
  FrameworkErrorCode code = FrameworkErrorCode::Unknown;
  std::optional<HRESULT> hr{};
  std::string message;
  std::source_location loc{};
};

class FrameworkException final : public std::runtime_error {
 public:
  explicit FrameworkException(FrameworkError error);

  [[nodiscard]] const FrameworkError& GetError() const;

 private:
  FrameworkError error_;
};

struct FrameworkErrorMetadata {
  std::string_view meaning;
  std::string_view typical_causes;
  std::string_view handling;
  LogCategory log_category = LogCategory::Core;
};

[[nodiscard]] FrameworkErrorMetadata GetFrameworkErrorMetadata(FrameworkErrorCode code);
[[nodiscard]] std::string_view ToString(FrameworkErrorDomain domain);
[[nodiscard]] std::string_view ToString(FrameworkErrorCode code);

[[nodiscard]] std::string DescribeHresult(HRESULT hr);
[[nodiscard]] std::string FormatErrorForLog(const FrameworkError& error);

class FrameworkDx final {
 public:
  FrameworkDx() = delete;

  static void ThrowIfFailed(
    HRESULT hr, FrameworkErrorCode code, std::string_view context, const std::source_location& loc = std::source_location::current());

  static HRESULT ReturnIfFailed(
    HRESULT hr, FrameworkErrorCode code, std::string_view context, const std::source_location& loc = std::source_location::current());

  static HRESULT CheckFast(HRESULT hr) noexcept;

 private:
  struct ThrottleKey {
    FrameworkErrorCode code{};
    HRESULT hr{};
    const char* file = nullptr;
    uint32_t line = 0;
  };

  struct ThrottleKeyHash {
    size_t operator()(const ThrottleKey& k) const noexcept;
  };

  struct ThrottleKeyEq {
    bool operator()(const ThrottleKey& a, const ThrottleKey& b) const noexcept;
  };

  static bool ShouldLogThrottled(FrameworkErrorCode code, HRESULT hr, const std::source_location& loc);

  static std::atomic<uint64_t> fast_fail_count_;
};

class FrameworkFail final {
 public:
  FrameworkFail() = delete;

  static void Throw(FrameworkErrorDomain domain,
    FrameworkErrorCode code,
    std::string_view message,
    const std::source_location& loc = std::source_location::current());

  static void Panic(FrameworkErrorDomain domain,
    FrameworkErrorCode code,
    std::string_view message,
    const std::source_location& loc = std::source_location::current()) noexcept;

  static void Assert(bool condition,
    FrameworkErrorCode code,
    std::string_view message,
    const std::source_location& loc = std::source_location::current()) noexcept;
};
