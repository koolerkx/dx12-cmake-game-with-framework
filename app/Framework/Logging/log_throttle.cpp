/**
@filename log_throttle.cpp
@brief Implementation of unified throttling, deduplication, and one-time warning system.
@author Kooler Fan
**/
#include "Framework/Logging/log_throttle.h"

#include <chrono>
#include <mutex>
#include <shared_mutex>
#include <unordered_map>

#include "Framework/Error/framework_bootstrap_log.h"

namespace {
constexpr std::chrono::milliseconds kDedupTimeWindow{5000};
constexpr std::chrono::milliseconds kThrottleRefillInterval{1000};
constexpr uint32_t kThrottleMaxTokensGlobal = 1000;
constexpr uint32_t kThrottleMaxTokensPerCategory = 200;
constexpr uint32_t kThrottleTokensPerRefill = 100;
}  // namespace

struct LogThrottleManager::State {
  std::shared_mutex dedup_mutex;
  std::unordered_map<LogThrottleKey, DedupEntry, LogThrottleKeyHash> dedup_map;
  std::chrono::steady_clock::time_point last_cleanup{};

  std::shared_mutex throttle_mutex;
  std::unordered_map<LogCategory, ThrottleBucket> throttle_buckets;
  ThrottleBucket global_bucket{};

  std::shared_mutex warn_once_mutex;
  std::unordered_map<LogThrottleKey, std::atomic<bool>, LogThrottleKeyHash> warn_once_map;

  std::array<std::array<std::atomic<uint64_t>, 6>, 6> error_totals{};
  std::array<std::atomic<uint64_t>, 6> suppressed_totals{};
  std::atomic<uint64_t> warn_once_hits_total{0};
  std::array<std::atomic<uint64_t>, 6> throttled_totals{};
  std::array<std::atomic<uint64_t>, 6> dedup_totals{};

  std::atomic<bool> initialized{false};
};

LogThrottleManager::State& LogThrottleManager::GetState() {
  static State state;
  return state;
}

size_t LogThrottleKeyHash::operator()(const LogThrottleKey& key) const noexcept {
  size_t h = 14695981039346656037ull;
  auto mix = [&h](uint64_t v) {
    h ^= v;
    h *= 1099511628211ull;
  };

  mix(static_cast<uint64_t>(key.category));
  mix(static_cast<uint64_t>(key.severity));
  mix(static_cast<uint64_t>(key.code));
  mix(key.context_hash);
  mix(key.object_id);
  return h;
}

void LogThrottleManager::Init() {
  State& state = GetState();
  if (state.initialized.exchange(true)) {
    return;
  }

  const auto now = std::chrono::steady_clock::now();
  state.global_bucket.tokens.store(kThrottleMaxTokensGlobal, std::memory_order_relaxed);
  state.global_bucket.last_refill.store(now, std::memory_order_relaxed);

  for (int i = 0; i < 6; ++i) {
    ThrottleBucket bucket;
    bucket.tokens.store(kThrottleMaxTokensPerCategory, std::memory_order_relaxed);
    bucket.last_refill.store(now, std::memory_order_relaxed);
    state.throttle_buckets[static_cast<LogCategory>(i)] = bucket;
  }
}

void LogThrottleManager::Shutdown() {
  State& state = GetState();
  state.initialized.store(false, std::memory_order_release);
}

void LogThrottleManager::RefillBucket(ThrottleBucket& bucket, uint32_t max_tokens, std::chrono::milliseconds refill_interval) {
  const auto now = std::chrono::steady_clock::now();
  const auto last_refill = bucket.last_refill.load(std::memory_order_acquire);
  const auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - last_refill);

  if (elapsed >= refill_interval) {
    const uint32_t refills = static_cast<uint32_t>(elapsed.count() / refill_interval.count());
    const uint32_t tokens_to_add = refills * kThrottleTokensPerRefill;
    const uint32_t current = bucket.tokens.load(std::memory_order_relaxed);
    const uint32_t new_tokens = (current + tokens_to_add > max_tokens) ? max_tokens : (current + tokens_to_add);

    bucket.tokens.store(new_tokens, std::memory_order_release);
    bucket.last_refill.store(now, std::memory_order_release);
  }
}

bool LogThrottleManager::CheckThrottle(const LogThrottleKey& key) {
  State& state = GetState();
  if (!state.initialized.load(std::memory_order_acquire)) {
    return true;
  }

  RefillBucket(state.global_bucket, kThrottleMaxTokensGlobal, kThrottleRefillInterval);

  uint32_t global_tokens = state.global_bucket.tokens.load(std::memory_order_acquire);
  if (global_tokens == 0) {
    state.throttled_totals[static_cast<uint8_t>(key.category)].fetch_add(1, std::memory_order_relaxed);
    return false;
  }

  state.global_bucket.tokens.fetch_sub(1, std::memory_order_acq_rel);

  {
    std::shared_lock lock(state.throttle_mutex);
    auto it = state.throttle_buckets.find(key.category);
    if (it != state.throttle_buckets.end()) {
      ThrottleBucket& bucket = it->second;
      RefillBucket(bucket, kThrottleMaxTokensPerCategory, kThrottleRefillInterval);

      uint32_t category_tokens = bucket.tokens.load(std::memory_order_acquire);
      if (category_tokens == 0) {
        state.throttled_totals[static_cast<uint8_t>(key.category)].fetch_add(1, std::memory_order_relaxed);
        return false;
      }

      bucket.tokens.fetch_sub(1, std::memory_order_acq_rel);
    }
  }

  return true;
}

bool LogThrottleManager::CheckDedup(const LogThrottleKey& key, std::string_view message) {
  State& state = GetState();
  if (!state.initialized.load(std::memory_order_acquire)) {
    return true;
  }

  const auto now = std::chrono::steady_clock::now();
  const auto message_hash = HashStringView(message);

  LogThrottleKey full_key = key;
  full_key.context_hash ^= message_hash;

  {
    std::unique_lock lock(state.dedup_mutex);

    auto& entry = state.dedup_map[full_key];
    const auto first_seen = entry.first_seen.load(std::memory_order_acquire);
    const auto last_seen = entry.last_seen.load(std::memory_order_acquire);

    if (first_seen.time_since_epoch().count() == 0) {
      entry.first_seen.store(now, std::memory_order_release);
      entry.last_seen.store(now, std::memory_order_release);
      entry.count.store(1, std::memory_order_release);
      entry.suppressed.store(false, std::memory_order_release);
      return true;
    }

    const auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - first_seen);
    if (elapsed < kDedupTimeWindow) {
      entry.count.fetch_add(1, std::memory_order_relaxed);
      entry.last_seen.store(now, std::memory_order_release);
      entry.suppressed.store(true, std::memory_order_release);
      state.dedup_totals[static_cast<uint8_t>(key.category)].fetch_add(1, std::memory_order_relaxed);
      return false;
    }

    entry.first_seen.store(now, std::memory_order_release);
    entry.last_seen.store(now, std::memory_order_release);
    const uint64_t suppressed_count = entry.count.load(std::memory_order_acquire);
    entry.count.store(1, std::memory_order_release);
    entry.suppressed.store(false, std::memory_order_release);

    if (suppressed_count > 1) {
      state.suppressed_totals[static_cast<uint8_t>(key.category)].fetch_add(1, std::memory_order_relaxed);
    }
  }

  const auto cleanup_interval = std::chrono::milliseconds{30000};
  {
    std::shared_lock lock(state.dedup_mutex);
    const auto last_cleanup = state.last_cleanup;
    if (last_cleanup.time_since_epoch().count() == 0 || now - last_cleanup > cleanup_interval) {
      lock.unlock();
      std::unique_lock unique_lock(state.dedup_mutex, std::try_to_lock);
      if (unique_lock.owns_lock()) {
        auto it = state.dedup_map.begin();
        while (it != state.dedup_map.end()) {
          const auto entry_last_seen = it->second.last_seen.load(std::memory_order_acquire);
          const auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - entry_last_seen);
          if (elapsed > kDedupTimeWindow * 2) {
            it = state.dedup_map.erase(it);
          } else {
            ++it;
          }
        }
        state.last_cleanup = now;
      }
    }
  }

  return true;
}

bool LogThrottleManager::ShouldLog(const LogThrottleKey& key, std::string_view message) {
  State& state = GetState();
  if (!state.initialized.load(std::memory_order_acquire)) {
    return true;
  }

  if (!CheckThrottle(key)) {
    return false;
  }

  if (!CheckDedup(key, message)) {
    return false;
  }

  const uint8_t category_idx = static_cast<uint8_t>(key.category);
  const uint8_t severity_idx = static_cast<uint8_t>(key.severity);
  state.error_totals[category_idx][severity_idx].fetch_add(1, std::memory_order_relaxed);

  return true;
}

uint64_t LogThrottleManager::GetSuppressedCount(const LogThrottleKey& key) {
  State& state = GetState();
  if (!state.initialized.load(std::memory_order_acquire)) {
    return 0;
  }

  std::shared_lock lock(state.dedup_mutex);
  auto it = state.dedup_map.find(key);
  if (it != state.dedup_map.end()) {
    return it->second.count.load(std::memory_order_acquire);
  }
  return 0;
}

bool LogThrottleManager::WarnOnce(const LogThrottleKey& key, std::string_view message) {
  State& state = GetState();
  if (!state.initialized.load(std::memory_order_acquire)) {
    return true;
  }

  std::unique_lock lock(state.warn_once_mutex);
  auto& flag = state.warn_once_map[key];
  const bool was_set = flag.exchange(true, std::memory_order_acq_rel);
  lock.unlock();

  if (!was_set) {
    state.warn_once_hits_total.fetch_add(1, std::memory_order_relaxed);
    return true;
  }

  return false;
}

uint64_t LogThrottleManager::GetErrorTotal(LogCategory category, LogLevel severity) {
  const State& state = GetState();
  if (!state.initialized.load(std::memory_order_acquire)) {
    return 0;
  }

  const uint8_t category_idx = static_cast<uint8_t>(category);
  const uint8_t severity_idx = static_cast<uint8_t>(severity);
  return state.error_totals[category_idx][severity_idx].load(std::memory_order_acquire);
}

uint64_t LogThrottleManager::GetSuppressedTotal(LogCategory category) {
  const State& state = GetState();
  if (!state.initialized.load(std::memory_order_acquire)) {
    return 0;
  }

  return state.suppressed_totals[static_cast<uint8_t>(category)].load(std::memory_order_acquire);
}

uint64_t LogThrottleManager::GetWarnOnceHitsTotal() {
  const State& state = GetState();
  if (!state.initialized.load(std::memory_order_acquire)) {
    return 0;
  }

  return state.warn_once_hits_total.load(std::memory_order_acquire);
}

uint64_t LogThrottleManager::GetThrottledTotal(LogCategory category) {
  const State& state = GetState();
  if (!state.initialized.load(std::memory_order_acquire)) {
    return 0;
  }

  return state.throttled_totals[static_cast<uint8_t>(category)].load(std::memory_order_acquire);
}

uint64_t LogThrottleManager::GetDedupTotal(LogCategory category) {
  const State& state = GetState();
  if (!state.initialized.load(std::memory_order_acquire)) {
    return 0;
  }

  return state.dedup_totals[static_cast<uint8_t>(category)].load(std::memory_order_acquire);
}

void LogThrottleManager::ResetCounters() {
  State& state = GetState();
  if (!state.initialized.load(std::memory_order_acquire)) {
    return;
  }

  for (auto& category_array : state.error_totals) {
    for (auto& counter : category_array) {
      counter.store(0, std::memory_order_relaxed);
    }
  }

  for (auto& counter : state.suppressed_totals) {
    counter.store(0, std::memory_order_relaxed);
  }

  state.warn_once_hits_total.store(0, std::memory_order_relaxed);

  for (auto& counter : state.throttled_totals) {
    counter.store(0, std::memory_order_relaxed);
  }

  for (auto& counter : state.dedup_totals) {
    counter.store(0, std::memory_order_relaxed);
  }
}

