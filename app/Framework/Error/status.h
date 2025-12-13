#pragma once

#include <winerror.h>

#include <cstdint>
#include <expected>
#include <optional>

#include "Framework/Error/framework_error.h"

enum class StatusCategory : uint8_t {
  Ok = 0,
  Framework = 1,
  HResult = 2,
};

struct StatusPayload {
  std::optional<HRESULT> hr{};
  std::optional<uint64_t> aux{};
};

class Status final {
 public:
  constexpr Status() = default;

  [[nodiscard]] constexpr bool Ok() const noexcept {
    return category_ == StatusCategory::Ok;
  }
  [[nodiscard]] constexpr StatusCategory Category() const noexcept {
    return category_;
  }
  [[nodiscard]] constexpr FrameworkErrorDomain Domain() const noexcept {
    return domain_;
  }
  [[nodiscard]] constexpr FrameworkErrorCode Code() const noexcept {
    return code_;
  }
  [[nodiscard]] constexpr const StatusPayload& Payload() const noexcept {
    return payload_;
  }

  [[nodiscard]] static constexpr Status OkStatus() noexcept {
    return Status{};
  }

  [[nodiscard]] static constexpr Status Framework(
    FrameworkErrorDomain domain, FrameworkErrorCode code, StatusPayload payload = {}) noexcept {
    Status s;
    s.category_ = StatusCategory::Framework;
    s.domain_ = domain;
    s.code_ = code;
    s.payload_ = payload;
    return s;
  }

  [[nodiscard]] static constexpr Status FromHresult(
    HRESULT hr, FrameworkErrorDomain domain, FrameworkErrorCode code, StatusPayload payload = {}) noexcept {
    Status s;
    s.category_ = StatusCategory::HResult;
    s.domain_ = domain;
    s.code_ = code;
    s.payload_ = payload;
    s.payload_.hr = hr;
    return s;
  }

 private:
  StatusCategory category_ = StatusCategory::Ok;
  FrameworkErrorDomain domain_ = FrameworkErrorDomain::Core;
  FrameworkErrorCode code_ = FrameworkErrorCode::Unknown;
  StatusPayload payload_{};
};

template <class T>
using Result = std::expected<T, Status>;

using StatusResult = std::expected<void, Status>;
