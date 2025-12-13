/**
@filename log_throttle.h
@brief Unified throttling, deduplication, and one-time warning system for Logger.
Provides thread-safe key-based deduplication, token bucket throttling, and WarnOnce functionality.
@author Kooler Fan
**/
#pragma once

#include <atomic>
#include <chrono>
#include <cstdint>
#include <string_view>

#include "Framework/Logging/logger.h"

struct LogThrottleKey {
  LogCategory category = LogCategory::Core;
  LogLevel severity = LogLevel::Info;
  uint32_t code = 0;
  uint64_t context_hash = 0;
  uint64_t object_id = 0;

  [[nodiscard]] bool operator==(const LogThrottleKey& other) const noexcept {
    return category == other.category && severity == other.severity && code == other.code && context_hash == other.context_hash &&
           object_id == other.object_id;
  }
};

struct LogThrottleKeyHash {
  [[nodiscard]] size_t operator()(const LogThrottleKey& key) const noexcept;
};

struct DedupEntry {
  std::atomic<uint64_t> count{0};
  std::atomic<std::chrono::steady_clock::time_point> first_seen{};
  std::atomic<std::chrono::steady_clock::time_point> last_seen{};
  std::atomic<bool> suppressed{false};
};

struct ThrottleBucket {
  std::atomic<uint32_t> tokens{0};
  std::atomic<std::chrono::steady_clock::time_point> last_refill{};
};

class LogThrottleManager final {
 public:
  LogThrottleManager() = delete;

  static void Init();
  static void Shutdown();

  [[nodiscard]] static bool ShouldLog(const LogThrottleKey& key, std::string_view message);
  [[nodiscard]] static uint64_t GetSuppressedCount(const LogThrottleKey& key);
  [[nodiscard]] static bool WarnOnce(const LogThrottleKey& key, std::string_view message);

  [[nodiscard]] static uint64_t GetErrorTotal(LogCategory category, LogLevel severity);
  [[nodiscard]] static uint64_t GetSuppressedTotal(LogCategory category);
  [[nodiscard]] static uint64_t GetWarnOnceHitsTotal();
  [[nodiscard]] static uint64_t GetThrottledTotal(LogCategory category);
  [[nodiscard]] static uint64_t GetDedupTotal(LogCategory category);

  static void ResetCounters();

 private:
  struct State;
  static State& GetState();

  [[nodiscard]] static bool CheckDedup(const LogThrottleKey& key, std::string_view message);
  [[nodiscard]] static bool CheckThrottle(const LogThrottleKey& key);
  static void RefillBucket(ThrottleBucket& bucket, uint32_t max_tokens, std::chrono::milliseconds refill_interval);
};

[[nodiscard]] inline uint64_t HashStringView(std::string_view str) noexcept {
  uint64_t hash = 14695981039346656037ull;
  for (char c : str) {
    hash ^= static_cast<uint8_t>(c);
    hash *= 1099511628211ull;
  }
  return hash;
}

[[nodiscard]] inline LogThrottleKey MakeThrottleKey(LogCategory category,
  LogLevel severity,
  uint32_t code,
  std::string_view context,
  uint64_t object_id = 0) noexcept {
  return {.category = category,
    .severity = severity,
    .code = code,
    .context_hash = HashStringView(context),
    .object_id = object_id};
}

