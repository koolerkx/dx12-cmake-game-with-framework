#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <wrl/client.h>

#include <exception>
#include <memory>
#include <string_view>
#include <vector>

#include "Application/Application.h"
#include "Framework/Error/framework_bootstrap_log.h"
#include "Framework/Error/framework_error.h"
#include "Framework/Logging/logger.h"
#include "Framework/Logging/sinks.h"
#include "Framework/utils.h"
#include "Game/game.h"
#include "Graphic/graphic.h"

constexpr int WINDOW_WIDTH = 1920;
constexpr int WINDOW_HEIGHT = 1080;

namespace {

void InitFrameworkLogger() {
  std::vector<std::unique_ptr<ILogSink>> sinks;
  sinks.push_back(std::make_unique<DebugSink>());
  sinks.push_back(std::make_unique<ConsoleSink>());
  Logger::Init(std::move(sinks));
}

}  // namespace

int WINAPI wWinMain([[maybe_unused]] HINSTANCE hInstance,
  [[maybe_unused]] HINSTANCE hPrevInstance,
  [[maybe_unused]] PWSTR lpCmdLine,
  [[maybe_unused]] int nShowCmd) try {
  SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);

  InitFrameworkLogger();

  // Initialize application and graphics
  Application app(hInstance, WINDOW_WIDTH, WINDOW_HEIGHT);
  Graphic graphic;
  graphic.Initialize(app.GetHwnd(), WINDOW_WIDTH, WINDOW_HEIGHT);

  // Initialize game
  Game game;
  game.Initialize(graphic);

  // Game loop callbacks
  std::function<void(float dt)> OnUpdate = [&](float dt) {
    game.OnUpdate(dt);
    game.OnRender(dt);
  };

  std::function<void(float fdt)> OnFixedUpdate = [&](float fdt) {
    // Fixed timestep updates
    game.OnFixedUpdate(fdt);
  };

  app.Run(OnUpdate, OnFixedUpdate);

  // Cleanup
  graphic.Shutdown();

  Logger::Shutdown();
  return 0;
} catch (const FrameworkException& e) {
  const FrameworkError& error = e.GetError();
  const std::string text = FormatErrorForLog(error);
  const auto meta = GetFrameworkErrorMetadata(error.code);

  if (Logger::IsInitialized()) {
    Logger::Log(LogLevel::Fatal, meta.log_category, text, error.loc);
    Logger::Flush();
  } else {
    FrameworkBootstrapLog(text, error.loc);
  }

  MessageBoxW(nullptr, Utf8ToWstringNoThrow(text).c_str(), L"Fatal Error", MB_OK | MB_ICONERROR);
  return -1;
} catch (const std::exception& e) {
  const std::string_view what = e.what();
  if (Logger::IsInitialized()) {
    Logger::Log(LogLevel::Fatal, LogCategory::Core, std::string(what));
    Logger::Flush();
  } else {
    FrameworkBootstrapLog(what);
  }

  MessageBoxW(nullptr, Utf8ToWstringNoThrow(what).c_str(), L"Error", MB_OK | MB_ICONERROR);

  return -1;
}
