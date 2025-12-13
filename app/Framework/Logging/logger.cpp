/**
@filename logger.cpp
@brief Core logging manager implementation. Owns global logger state, manages sinks, performs thread-safe log dispatch, filtering, and
flushing.
@author Kooler Fan
**/
#include "Framework/Logging/logger.h"

#include <atomic>
#include <mutex>
#include <utility>

struct Logger::State {
  std::mutex mutex;
  std::vector<std::unique_ptr<ILogSink>> sinks;

  std::atomic<bool> initialized{false};
  LogLevel min_level = LogLevel::Trace;
};

Logger::State& Logger::GetState() {
  static State state;
  return state;
}

void Logger::Init(std::vector<std::unique_ptr<ILogSink>> sinks) {
  State& state = GetState();
  std::scoped_lock lock(state.mutex);

  state.sinks = std::move(sinks);
  state.initialized.store(true, std::memory_order_release);
}

void Logger::Shutdown() {
  State& state = GetState();
  std::scoped_lock lock(state.mutex);

  state.initialized.store(false, std::memory_order_release);
  state.sinks.clear();
}

void Logger::Flush() {
  State& state = GetState();
  std::scoped_lock lock(state.mutex);

  for (auto& sink : state.sinks) {
    if (sink) {
      sink->Flush();
    }
  }
}

bool Logger::IsInitialized() {
  return GetState().initialized.load(std::memory_order_acquire);
}

bool Logger::IsEnabled(LogLevel level, [[maybe_unused]] LogCategory category) {
  const State& state = GetState();
  if (!state.initialized.load(std::memory_order_acquire)) {
    return false;
  }
  return static_cast<uint8_t>(level) >= static_cast<uint8_t>(state.min_level);
}

void Logger::Log(LogLevel level, LogCategory category, std::string message, const std::source_location& loc) {
  if (!IsEnabled(level, category)) {
    return;
  }

  LogEntry entry;
  entry.level = level;
  entry.category = category;
  entry.timestamp = std::chrono::system_clock::now();
  entry.loc = loc;
  entry.message = std::move(message);

  Emit(entry);

  if (level == LogLevel::Fatal) {
    Flush();
  }
}

void Logger::Logv(LogLevel level, LogCategory category, std::string_view fmt, std::format_args args, const std::source_location& loc) {
  if (!IsEnabled(level, category)) {
    return;
  }
  Log(level, category, std::vformat(fmt, args), loc);
}

void Logger::Emit(const LogEntry& entry) {
  State& state = GetState();
  std::scoped_lock lock(state.mutex);

  if (!state.initialized.load(std::memory_order_acquire)) {
    return;
  }

  for (auto& sink : state.sinks) {
    if (sink) {
      sink->OnLog(entry);
    }
  }
}
