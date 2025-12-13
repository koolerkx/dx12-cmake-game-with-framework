# Framework Module

## Overview

The Framework module provides core infrastructure for logging and error handling in the game engine. It consists of two primary subsystems:

- **Logging System**: Asynchronous message recording with configurable output destinations
- **Error Handling System**: Structured error management with performance-optimized variants for different execution contexts

also provided the utils type and common header in this Framework module.

## Module Structure

### Framework/Logging

**Purpose**: Asynchronous message capture, formatting, and output routing.

**Components**:
- `logger.h/cpp`: Core logging engine managing worker threads and message queues
- `sinks.h`: Output destination interfaces (Console, Debug Window, File)
- `log_throttle.h`: Rate limiting for duplicate messages

### Framework/Error

**Purpose**: Error classification, reporting, and failure response strategies.

**Components**:
- `framework_error.h`: Error code definitions and domain categorization
- `error_helpers.h`: Standard error handling macros for initialization code
- `error_helpers_fast.h`: Zero-allocation error handling for performance-critical paths
- `status.h`: Result type for error propagation without exceptions

### Framework/Utils

**Purpose**: Common utilities supporting framework components.

**Components**:
- `path_utils.h`: Executable path resolution for log file placement

## API Reference

### Logging

The logging system operates asynchronously using a producer-consumer pattern. Messages are queued by the caller thread and written by a dedicated worker thread.

#### Basic Usage

```cpp
#include "Framework/Logging/logger.h"

// Formatted logging
Logger::Logf(LogLevel::Info, LogCategory::Core, "Loaded {} textures", count);

// Simple logging
Logger::Log(LogLevel::Warn, LogCategory::Rendering, "Texture cache full");
```

#### Log Levels

- `Trace`: Verbose diagnostic information (development only)
- `Debug`: Detailed debugging information
- `Info`: Significant state transitions
- `Warn`: Recoverable issues requiring attention
- `Error`: Functional failures affecting features

#### Initialization

```cpp
#include "Framework/Logging/logger.h"
#include "Framework/Logging/sinks.h"

void InitializeLogging() {
    std::vector<std::unique_ptr<ILogSink>> sinks;
    sinks.push_back(std::make_unique<DebugSink>());
    sinks.push_back(std::make_unique<ConsoleSink>());
    
    LoggerConfig config;
    sinks.push_back(std::make_unique<FileSink>(config));
    
    Logger::Init(config, std::move(sinks));
}
```

#### Throttling

Prevents log flooding from high-frequency warnings:

```cpp
#include "Framework/Logging/log_throttle.h"

void UpdateSystem() {
    if (errorCondition) {
        auto key = MakeThrottleKey(LogCategory::Physics, LogLevel::Warn, 
                                   101, "CollisionOverflow");
        
        if (LogThrottleManager::ShouldLog(key, "Collision buffer overflow")) {
            Logger::Log(LogLevel::Warn, LogCategory::Physics, 
                       "Collision buffer overflow");
        }
    }
}
```

### Error Handling

#### Cold Path (Initialization/Loading)

Use detailed error reporting with stack traces for non-performance-critical code:

```cpp
#include "Framework/Error/error_helpers.h"

void InitializeDevice() {
    HRESULT hr = D3D12CreateDevice(/* ... */);
    ThrowIfFailed(hr, FrameworkErrorCode::D3d12DeviceCreateFailed, 
                  "Failed to create D3D12 device");
}

bool LoadResource() {
    HRESULT hr = LoadTextureFromFile(path);
    if (ReturnIfFailed(hr, FrameworkErrorCode::ResourceLoadFailed, 
                       "Texture load failed")) {
        return false;
    }
    return true;
}
```

#### Hot Path (Render Loop/Update)

Use zero-allocation error handling for performance-critical sections:

```cpp
#include "Framework/Error/error_helpers_fast.h"

void RenderFrame() {
    HRESULT hr = commandList->Close();
    
    if (ReturnIfFailedFast(hr, ContextId::Graphic_EndFrame_CloseCommandList, 0)) {
        return; // Skip frame on failure
    }
    
    // Continue rendering
}
```

#### Panic Termination

For unrecoverable errors requiring immediate termination:

```cpp
#include "Framework/Error/framework_error.h"

void CriticalSystemCheck() {
    if (memoryCorrupted) {
        FrameworkFail::Panic("Memory corruption detected");
        // Application terminates immediately
    }
}
```

## Best Practices

### Logging Guidelines

- Use `Trace`/`Debug` for verbose diagnostics (filtered in release builds)
- Use `Info` for milestone events (level load, mode transitions)
- Use `Warn` for degraded functionality with fallbacks
- Use `Error` for feature-breaking failures
- Call `Logger::Init()` before any logging operations
- Consider throttling for high-frequency warnings

### Error Handling Strategy

#### Cold Path Context
- Initialization code
- Resource loading
- Configuration parsing
- Use `ThrowIfFailed` or `ReturnIfFailed`
- Detailed error messages with context

#### Hot Path Context
- Render loop
- Update tick
- Input processing
- Use `ReturnIfFailedFast`
- Avoid string formatting
- Deduplication via context IDs

## Implementation Notes

### Logging System

**Queue Behavior**: When the message queue reaches capacity, the system prioritizes by dropping `Trace` and `Debug` level messages to maintain performance.

**File Naming**: Log files use the pattern `{AppName}_{Timestamp}_{PID}.log` for unique identification across runs.

**Thread Safety**: All logging operations are thread-safe via lock-free queue implementation.

### Error System

**Hot Path Optimization**: `ReturnIfFailedFast` uses hash-based deduplication to log each unique error only once, eliminating allocation overhead.

**Error Domains**: Errors are categorized by domain (Core, Graphics, Audio, etc.) for filtering and routing.

**HRESULT Integration**: Automatic translation of Windows error codes to human-readable descriptions.

## Migration Guide

### From Legacy Logging

```cpp
// Old
printf("Error: %s\n", message);

// New
Logger::Logf(LogLevel::Error, LogCategory::Core, "Error: {}", message);
```

### From Exception-Only Error Handling

```cpp
// Old
try {
    HRESULT hr = Operation();
    if (FAILED(hr)) throw std::runtime_error("Failed");
} catch (...) { /* handle */ }

// New - Cold Path
ThrowIfFailed(hr, FrameworkErrorCode::OperationFailed, "Operation failed");

// New - Hot Path
if (ReturnIfFailedFast(hr, ContextId::HotPath_Operation, 0)) {
    return;
}
```

## Performance Characteristics

- Logging: Lock-free queue, ~100ns per log call
- Hot path error handling: Zero allocation, single branch
- Cold path error handling: Full diagnostics, exception-based
- Throttling: O(1) hash lookup per check