#define WIN32_LEAN_AND_MEAN
#include <wrl/client.h>

#include <exception>
#include <iostream>

#include "Application/Application.h"
#include "Game/game.h"
#include "Graphic/graphic.h"
#include "utils.h"

template <typename T>
using ComPtr = Microsoft::WRL::ComPtr<T>;

constexpr int WINDOW_WIDTH = 1920;
constexpr int WINDOW_HEIGHT = 1080;

int WINAPI wWinMain([[maybe_unused]] HINSTANCE hInstance,
  [[maybe_unused]] HINSTANCE hPrevInstance,
  [[maybe_unused]] PWSTR lpCmdLine,
  [[maybe_unused]] int nShowCmd) try {
  SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);

  // Initialize application and graphics
  Application app(hInstance, WINDOW_WIDTH, WINDOW_HEIGHT);
  Graphic graphic;
  graphic.Initalize(app.GetHwnd(), WINDOW_WIDTH, WINDOW_HEIGHT);

  // Initialize game
  Game game;
  game.Initialize(graphic);

  // Step 5: Pull-Based Game Loop
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

  return 0;
} catch (const std::exception& e) {
  std::cerr << "Exception: " << e.what() << "\n";
  MessageBoxW(nullptr, utils::Utf8ToWstring(e.what()).c_str(), L"Error", MB_OK | MB_ICONERROR);

  return -1;
}
