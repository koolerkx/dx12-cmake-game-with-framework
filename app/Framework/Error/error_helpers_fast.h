// The is designed for extremely performance critical code
// should not allocate memory, add lock, not predictable cost
// reduce the most dependency
#pragma once

#include <winerror.h>

#include <array>
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <source_location>
#include <string_view>

#include "Framework/Error/error_context.h"
#include "Framework/Error/framework_bootstrap_log.h"

struct FastErrorCounters {
  std::atomic<uint64_t> fail_count{0};
  std::atomic<uint64_t> warn_once_emitted{0};
};

namespace Detail {

constexpr uint64_t Fnv1a64Init() noexcept {
  return 14695981039346656037ull;
}
constexpr uint64_t Fnv1a64Prime() noexcept {
  return 1099511628211ull;
}

constexpr uint64_t Fnv1a64Update(uint64_t hash, uint64_t value) noexcept {
  for (int i = 0; i < 8; ++i) {
    const uint8_t byte = static_cast<uint8_t>((value >> (i * 8)) & 0xFFu);
    hash ^= byte;
    hash *= Fnv1a64Prime();
  }
  return hash;
}

// hash
constexpr uint64_t MakeKey(ContextId ctx, HRESULT hr, uint32_t extra) noexcept {
  uint64_t h = Fnv1a64Init();
  h = Fnv1a64Update(h, static_cast<uint64_t>(static_cast<uint16_t>(ctx)));
  h = Fnv1a64Update(h, static_cast<uint64_t>(static_cast<uint32_t>(hr)));
  h = Fnv1a64Update(h, static_cast<uint64_t>(extra));
  // 0 is reserved as "empty".
  return (h == 0ull) ? 1ull : h;
}

inline void AppendLiteral(char*& it, char* end, const char* s) noexcept {
  while (it < end && *s != '\0') {
    *it++ = *s++;
  }
}

inline void AppendChar(char*& it, char* end, char c) noexcept {
  if (it < end) {
    *it++ = c;
  }
}

inline void AppendHex32(char*& it, char* end, uint32_t value) noexcept {
  static constexpr char kHex[] = "0123456789ABCDEF";
  for (int i = 7; i >= 0; --i) {
    const uint32_t nibble = (value >> (i * 4)) & 0xFu;
    AppendChar(it, end, kHex[nibble]);
  }
}

inline void AppendU32(char*& it, char* end, uint32_t value) noexcept {
  char tmp[10];
  int n = 0;
  do {
    tmp[n++] = static_cast<char>('0' + (value % 10u));
    value /= 10u;
  } while (value != 0u && n < static_cast<int>(sizeof(tmp)));

  for (int i = n - 1; i >= 0; --i) {
    AppendChar(it, end, tmp[i]);
  }
}

// Fixed-size lock-free table for warn-once keys.
// Collisions may cause occasional missed warnings, but never allocates or locks.
static constexpr size_t kOnceTableSize = 4096;
inline std::array<std::atomic<uint64_t>, kOnceTableSize> g_warn_once_table{};

inline bool TryMarkWarnOnce(uint64_t key) noexcept {
  const size_t mask = kOnceTableSize - 1;
  size_t idx = static_cast<size_t>(key) & mask;

  for (size_t probe = 0; probe < 8; ++probe) {
    auto& slot = g_warn_once_table[(idx + probe) & mask];
    uint64_t existing = slot.load(std::memory_order_relaxed);
    if (existing == key) {
      return false;
    }
    if (existing == 0ull) {
      if (slot.compare_exchange_strong(existing, key, std::memory_order_relaxed)) {
        return true;
      }
    }
  }

  return false;
}

// Logger
inline void EmitWarnOnce(ContextId ctx, HRESULT hr, uint32_t extra, const std::source_location& loc) noexcept {
  char buffer[256];
  char* it = buffer;
  char* end = buffer + (sizeof(buffer) - 1);

  AppendLiteral(it, end, "[fastwarn] ctx=");
  AppendLiteral(it, end, ToContextString(ctx));
  AppendLiteral(it, end, " hr=0x");
  AppendHex32(it, end, static_cast<uint32_t>(hr));
  AppendLiteral(it, end, " extra=");
  AppendU32(it, end, extra);

  *it = '\0';
  FrameworkBootstrapLog(std::string_view(buffer), loc);
}

}  // namespace Detail

[[nodiscard]] inline bool ReturnIfFailedFast(HRESULT hr,
  ContextId ctx,
  uint32_t extra,
  FastErrorCounters* counters = nullptr,
  const std::source_location& loc = std::source_location::current()) noexcept {
  if (SUCCEEDED(hr)) {
    return false;
  }

  if (counters) {
    counters->fail_count.fetch_add(1, std::memory_order_relaxed);
  }

  const uint64_t key = Detail::MakeKey(ctx, hr, extra);
  if (Detail::TryMarkWarnOnce(key)) {
    if (counters) {
      counters->warn_once_emitted.fetch_add(1, std::memory_order_relaxed);
    }
    Detail::EmitWarnOnce(ctx, hr, extra, loc);
  }

  return true;
}

inline void LogWarnOnceFast(uint64_t key,
  ContextId ctx,
  HRESULT hr,
  FastErrorCounters* counters = nullptr,
  const std::source_location& loc = std::source_location::current()) noexcept {
  if (SUCCEEDED(hr)) {
    return;
  }

  const uint64_t normalized = (key == 0ull) ? 1ull : key;
  if (Detail::TryMarkWarnOnce(normalized)) {
    if (counters) {
      counters->warn_once_emitted.fetch_add(1, std::memory_order_relaxed);
    }
    Detail::EmitWarnOnce(ctx, hr, 0u, loc);
  }
}
