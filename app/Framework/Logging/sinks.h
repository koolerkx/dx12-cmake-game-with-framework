/**
@filename sinks.h
@brief Defines the ILogSink interface and built-in sink implementations (DebugSink, ConsoleSink) for log output destinations.
@author Kooler Fan
**/
#pragma once

struct LogEntry;

struct ILogSink {
  virtual ~ILogSink() = default;
  virtual void OnLog(const LogEntry& entry) = 0;
  virtual void Flush() {
  }
};

class DebugSink final : public ILogSink {
 public:
  void OnLog(const LogEntry& entry) override;
  void Flush() override;
};

class ConsoleSink final : public ILogSink {
 public:
  ConsoleSink();
  void OnLog(const LogEntry& entry) override;
  void Flush() override;

  [[nodiscard]] bool IsAttached() const;

 private:
  void AttachToParentConsoleIfPossible();

  void* output_handle_ = nullptr;
  bool attached_ = false;
};
