/**
@filename framework_error.cpp
@brief Implementation of the engine-wide error framework. Provides structured error types, DirectX HRESULT handling helpers, throttled
logging, and fatal failure handling.
@author Kooler Fan
**/
#include "Framework/Error/framework_error.h"

#include "Framework/Error/framework_bootstrap_log.h"

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include <format>
#include <mutex>
#include <unordered_map>
#include <utility>

namespace {

std::string WideToUtf8(std::wstring_view wstr) {
  if (wstr.empty()) {
    return {};
  }

  const int required = WideCharToMultiByte(CP_UTF8, 0, wstr.data(), static_cast<int>(wstr.size()), nullptr, 0, nullptr, nullptr);
  if (required <= 0) {
    return "[framework] UTF-16->UTF-8 conversion failed";
  }

  std::string out(static_cast<size_t>(required), '\0');
  WideCharToMultiByte(CP_UTF8, 0, wstr.data(), static_cast<int>(wstr.size()), out.data(), required, nullptr, nullptr);
  return out;
}

std::wstring FormatMessageForHresult(HRESULT hr) {
  wchar_t* buffer = nullptr;

  const DWORD flags = FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS;
  const DWORD lang_id = MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT);

  const DWORD written = FormatMessageW(flags, nullptr, static_cast<DWORD>(hr), lang_id, reinterpret_cast<wchar_t*>(&buffer), 0, nullptr);
  if (written == 0 || buffer == nullptr) {
    return {};
  }

  std::wstring message(buffer, buffer + written);
  LocalFree(buffer);

  while (!message.empty() && (message.back() == L'\r' || message.back() == L'\n')) {
    message.pop_back();
  }
  return message;
}

}  // namespace

FrameworkException::FrameworkException(FrameworkError error) : std::runtime_error(std::string(error.message)), error_(std::move(error)) {
}

const FrameworkError& FrameworkException::GetError() const {
  return error_;
}

FrameworkErrorMetadata GetFrameworkErrorMetadata(FrameworkErrorCode code) {
  switch (code) {
    case FrameworkErrorCode::Unknown:
      return {
        .meaning = "Unclassified failure.",
        .typical_causes = "Legacy/unmapped errors.",
        .handling = "Level2 throw at boundaries.",
        .log_category = LogCategory::Core,
      };
    case FrameworkErrorCode::InvalidArgument:
      return {
        .meaning = "Caller passed invalid argument(s).",
        .typical_causes = "Null pointer, invalid sizes, invalid ranges.",
        .handling = "Level3 return; Level2 throw if startup boundary.",
        .log_category = LogCategory::Validation,
      };
    case FrameworkErrorCode::InvalidState:
      return {
        .meaning = "State machine violated; call order invalid.",
        .typical_causes = "Bug in control flow or missing initialization.",
        .handling = "Debug assert break; Release terminate (panic).",
        .log_category = LogCategory::Validation,
      };
    case FrameworkErrorCode::DxgiFactoryCreateFailed:
      return {
        .meaning = "Failed to create DXGI factory.",
        .typical_causes = "DXGI not available, debug layer configuration, OS limitations.",
        .handling = "Level2 throw at startup boundary.",
        .log_category = LogCategory::Graphic,
      };
    case FrameworkErrorCode::D3d12DeviceCreateFailed:
      return {
        .meaning = "Failed to create D3D12 device.",
        .typical_causes = "Unsupported adapter/feature level, driver issues.",
        .handling = "Level2 throw at startup boundary.",
        .log_category = LogCategory::Graphic,
      };
    case FrameworkErrorCode::CommandQueueCreateFailed:
      return {
        .meaning = "Failed to create D3D12 command queue.",
        .typical_causes = "Device failure or driver problems.",
        .handling = "Level2 throw at startup boundary.",
        .log_category = LogCategory::Graphic,
      };
    case FrameworkErrorCode::CommandAllocatorCreateFailed:
      return {
        .meaning = "Failed to create per-frame command allocator(s).",
        .typical_causes = "Out of memory, device instability.",
        .handling = "Level2 throw at startup boundary.",
        .log_category = LogCategory::Graphic,
      };
    case FrameworkErrorCode::CommandListCreateFailed:
      return {
        .meaning = "Failed to create command list.",
        .typical_causes = "Out of memory, device instability.",
        .handling = "Level2 throw at startup boundary.",
        .log_category = LogCategory::Graphic,
      };
    case FrameworkErrorCode::DescriptorHeapManagerInitFailed:
      return {
        .meaning = "Descriptor heap manager initialization failed.",
        .typical_causes = "Invalid device, wrong heap sizes, allocation failure.",
        .handling = "Level2 throw at startup boundary.",
        .log_category = LogCategory::Graphic,
      };
    case FrameworkErrorCode::TextureManagerInitFailed:
      return {
        .meaning = "Texture manager initialization failed.",
        .typical_causes = "Invalid allocators, capacity too small, device issues.",
        .handling = "Level2 throw at startup boundary.",
        .log_category = LogCategory::Resource,
      };
    case FrameworkErrorCode::SwapchainInitFailed:
      return {
        .meaning = "Swapchain initialization failed.",
        .typical_causes = "Invalid HWND, unsupported swapchain parameters, DXGI failures.",
        .handling = "Level2 throw at startup boundary.",
        .log_category = LogCategory::Graphic,
      };
    case FrameworkErrorCode::DepthBufferCreateFailed:
      return {
        .meaning = "Depth buffer creation failed.",
        .typical_causes = "Unsupported format, allocation failure, descriptor allocation failure.",
        .handling = "Level2 throw at startup boundary.",
        .log_category = LogCategory::Graphic,
      };
    case FrameworkErrorCode::FenceManagerInitFailed:
      return {
        .meaning = "Fence manager initialization failed.",
        .typical_causes = "Device/driver issues, fence creation failed.",
        .handling = "Level2 throw at startup boundary.",
        .log_category = LogCategory::Graphic,
      };
    case FrameworkErrorCode::UploadContextInitFailed:
      return {
        .meaning = "Upload context initialization failed.",
        .typical_causes = "Allocator/list failures, fence issues.",
        .handling = "Level2 throw at startup boundary.",
        .log_category = LogCategory::Resource,
      };
    case FrameworkErrorCode::RenderPassManagerInitFailed:
      return {
        .meaning = "Render pass manager initialization failed.",
        .typical_causes = "Invalid device assumptions or pass graph setup failure.",
        .handling = "Level2 throw at startup boundary.",
        .log_category = LogCategory::Graphic,
      };
    case FrameworkErrorCode::RenderPassInitFailed:
      return {
        .meaning = "Render pass initialization failed.",
        .typical_causes = "PSO/root signature creation failure.",
        .handling = "Level2 throw at startup boundary.",
        .log_category = LogCategory::Graphic,
      };
  }

  return GetFrameworkErrorMetadata(FrameworkErrorCode::Unknown);
}

std::string_view ToString(FrameworkErrorDomain domain) {
  switch (domain) {
    case FrameworkErrorDomain::Core:
      return "Core";
    case FrameworkErrorDomain::Graphic:
      return "Graphic";
    case FrameworkErrorDomain::Resource:
      return "Resource";
    case FrameworkErrorDomain::Game:
      return "Game";
    case FrameworkErrorDomain::UI:
      return "UI";
    case FrameworkErrorDomain::Validation:
      return "Validation";
  }
  return "Core";
}

std::string_view ToString(FrameworkErrorCode code) {
  switch (code) {
    case FrameworkErrorCode::Unknown:
      return "Unknown";
    case FrameworkErrorCode::InvalidArgument:
      return "InvalidArgument";
    case FrameworkErrorCode::InvalidState:
      return "InvalidState";
    case FrameworkErrorCode::DxgiFactoryCreateFailed:
      return "DxgiFactoryCreateFailed";
    case FrameworkErrorCode::D3d12DeviceCreateFailed:
      return "D3d12DeviceCreateFailed";
    case FrameworkErrorCode::CommandQueueCreateFailed:
      return "CommandQueueCreateFailed";
    case FrameworkErrorCode::CommandAllocatorCreateFailed:
      return "CommandAllocatorCreateFailed";
    case FrameworkErrorCode::CommandListCreateFailed:
      return "CommandListCreateFailed";
    case FrameworkErrorCode::DescriptorHeapManagerInitFailed:
      return "DescriptorHeapManagerInitFailed";
    case FrameworkErrorCode::TextureManagerInitFailed:
      return "TextureManagerInitFailed";
    case FrameworkErrorCode::SwapchainInitFailed:
      return "SwapchainInitFailed";
    case FrameworkErrorCode::DepthBufferCreateFailed:
      return "DepthBufferCreateFailed";
    case FrameworkErrorCode::FenceManagerInitFailed:
      return "FenceManagerInitFailed";
    case FrameworkErrorCode::UploadContextInitFailed:
      return "UploadContextInitFailed";
    case FrameworkErrorCode::RenderPassManagerInitFailed:
      return "RenderPassManagerInitFailed";
    case FrameworkErrorCode::RenderPassInitFailed:
      return "RenderPassInitFailed";
  }
  return "Unknown";
}

std::string DescribeHresult(HRESULT hr) {
  const std::wstring message = FormatMessageForHresult(hr);
  if (message.empty()) {
    return std::format("hr=0x{:08X}", static_cast<uint32_t>(hr));
  }
  return std::format("hr=0x{:08X} ({})", static_cast<uint32_t>(hr), WideToUtf8(message));
}

std::string FormatErrorForLog(const FrameworkError& error) {
  const auto meta = GetFrameworkErrorMetadata(error.code);

  std::string out;
  out.reserve(256);

  out.append(error.message);

  if (error.hr.has_value()) {
    out.append(" | ");
    out.append(DescribeHresult(*error.hr));
  }

  out.append(" | domain=");
  out.append(ToString(error.domain));

  out.append(" | code=");
  out.append(ToString(error.code));

  out.append(" | handling=");
  out.append(meta.handling);

  out.append(" | at ");
  out.append(error.loc.file_name());
  out.push_back(':');
  out.append(std::to_string(error.loc.line()));

  return out;
}

std::atomic<uint64_t> FrameworkDx::fast_fail_count_{0};

size_t FrameworkDx::ThrottleKeyHash::operator()(const ThrottleKey& k) const noexcept {
  size_t h = 1469598103934665603ull;
  auto mix = [&h](size_t v) {
    h ^= v;
    h *= 1099511628211ull;
  };

  mix(static_cast<size_t>(k.code));
  mix(static_cast<size_t>(k.hr));
  mix(reinterpret_cast<size_t>(k.file));
  mix(static_cast<size_t>(k.line));
  return h;
}

bool FrameworkDx::ThrottleKeyEq::operator()(const ThrottleKey& a, const ThrottleKey& b) const noexcept {
  return a.code == b.code && a.hr == b.hr && a.file == b.file && a.line == b.line;
}

bool FrameworkDx::ShouldLogThrottled(FrameworkErrorCode code, HRESULT hr, const std::source_location& loc) {
  static std::mutex mutex;
  static std::unordered_map<ThrottleKey, uint32_t, ThrottleKeyHash, ThrottleKeyEq> counts;

  const ThrottleKey key = {.code = code, .hr = hr, .file = loc.file_name(), .line = loc.line()};

  uint32_t count = 0;
  {
    std::scoped_lock lock(mutex);
    count = ++counts[key];
  }

  return (count != 0u) && ((count & (count - 1u)) == 0u);
}

void FrameworkDx::ThrowIfFailed(HRESULT hr, FrameworkErrorCode code, std::string_view context, const std::source_location& loc) {
  if (SUCCEEDED(hr)) {
    return;
  }

  FrameworkError error;
  error.domain = FrameworkErrorDomain::Graphic;
  error.code = code;
  error.hr = hr;
  error.loc = loc;
  error.message = std::string(context);

  throw FrameworkException(std::move(error));
}

HRESULT FrameworkDx::ReturnIfFailed(HRESULT hr, FrameworkErrorCode code, std::string_view context, const std::source_location& loc) {
  if (SUCCEEDED(hr)) {
    return hr;
  }

  if (!ShouldLogThrottled(code, hr, loc)) {
    return hr;
  }

  FrameworkError error;
  error.domain = FrameworkErrorDomain::Graphic;
  error.code = code;
  error.hr = hr;
  error.loc = loc;
  error.message = std::string(context);

  const auto meta = GetFrameworkErrorMetadata(error.code);
  const std::string text = FormatErrorForLog(error);

  if (Logger::IsInitialized()) {
    Logger::Log(LogLevel::Error, meta.log_category, text, loc);
  } else {
    FrameworkBootstrapLog(text, loc);
  }

  return hr;
}

HRESULT FrameworkDx::CheckFast(HRESULT hr) noexcept {
  if (SUCCEEDED(hr)) {
    return hr;
  }
  fast_fail_count_.fetch_add(1, std::memory_order_relaxed);
  return hr;
}

void FrameworkFail::Throw(FrameworkErrorDomain domain, FrameworkErrorCode code, std::string_view message, const std::source_location& loc) {
  FrameworkError error;
  error.domain = domain;
  error.code = code;
  error.loc = loc;
  error.message = std::string(message);

  throw FrameworkException(std::move(error));
}

void FrameworkFail::Panic(
  FrameworkErrorDomain domain, FrameworkErrorCode code, std::string_view message, const std::source_location& loc) noexcept {
  Logger::EnterPanic();

  FrameworkError error;
  error.domain = domain;
  error.code = code;
  error.loc = loc;
  error.message = std::string(message);

  const auto meta = GetFrameworkErrorMetadata(error.code);

  std::string text;
  text.reserve(256);
  text.append("[panic] ");
  text.append(ToString(error.domain));
  text.append("/");
  text.append(ToString(error.code));

  if (error.hr.has_value()) {
    text.append(" hr=");
    text.append(std::format("0x{:08X}", static_cast<uint32_t>(*error.hr)));
  }

  text.append(" ");
  text.append(error.message);
  text.append(" @");
  text.append(error.loc.file_name());
  text.push_back(':');
  text.append(std::to_string(error.loc.line()));

  Logger::EmitDirectMinimal(LogLevel::Fatal, meta.log_category, text, loc);

#if defined(DEBUG) || defined(_DEBUG)
  __debugbreak();
#endif
  std::terminate();
}

void FrameworkFail::Assert(bool condition, FrameworkErrorCode code, std::string_view message, const std::source_location& loc) noexcept {
  if (condition) {
    return;
  }

#if defined(DEBUG) || defined(_DEBUG)
  FrameworkError error;
  error.domain = FrameworkErrorDomain::Validation;
  error.code = code;
  error.loc = loc;
  error.message = std::string(message);

  const auto meta = GetFrameworkErrorMetadata(error.code);
  const std::string text = FormatErrorForLog(error);

  if (Logger::IsInitialized()) {
    Logger::Log(LogLevel::Fatal, meta.log_category, text, loc);
    Logger::Flush();
  } else {
    FrameworkBootstrapLog(text, loc);
  }

  __debugbreak();
#else
  (void)code;
  (void)message;
  (void)loc;
#endif
}
