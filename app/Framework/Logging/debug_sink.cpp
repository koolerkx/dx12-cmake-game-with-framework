/**
@filename debug_sink.cpp
@brief Logging Sink implementation that sends log output to the Visual Studio debugger output window using OutputDebugString.
@author Kooler Fan
**/
#include "Framework/Logging/sinks.h"

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include "Framework/Logging/logger.h"
#include "Framework/utils.h"

void DebugSink::OnLog(const LogEntry& entry) {
  std::wstring msg = Utf8ToWstringNoThrow(entry.message);
  msg.append(L"\n");
  OutputDebugStringW(msg.c_str());
}

void DebugSink::Flush() {
}
