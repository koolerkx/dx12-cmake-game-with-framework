#pragma once

#include <winerror.h>

#include <source_location>
#include <string_view>

#include "Framework/Error/framework_error.h"


// Non-hot-path helpers (Create/Init). May format, allocate, throttle, etc.
[[nodiscard]] inline HRESULT ReturnIfFailed(
  HRESULT hr, FrameworkErrorCode code, std::string_view context, const std::source_location& loc = std::source_location::current()) {
  return FrameworkDx::ReturnIfFailed(hr, code, context, loc);
}

inline void ThrowIfFailed(
  HRESULT hr, FrameworkErrorCode code, std::string_view context, const std::source_location& loc = std::source_location::current()) {
  FrameworkDx::ThrowIfFailed(hr, code, context, loc);
}
