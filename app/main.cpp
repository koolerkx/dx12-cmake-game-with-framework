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

  Application app(hInstance, WINDOW_WIDTH, WINDOW_HEIGHT);
  Graphic graphic;
  graphic.Initalize(app.GetHwnd(), WINDOW_WIDTH, WINDOW_HEIGHT);

  Game game(graphic);

  std::function<void(float dt)> OnUpdate = [&]([[maybe_unused]] float dt) { game.OnUpdate(dt); };

  std::function<void(float fdt)> OnFixedUpdate = [&]([[maybe_unused]] float fdt) { game.OnFixedUpdate(fdt); };

  app.Run(OnUpdate, OnFixedUpdate);

  graphic.Shutdown();

  return 0;
} catch (const std::exception& e) {
  std::cerr << "Exception: " << e.what() << "\n";
  MessageBoxW(nullptr, utils::Utf8ToWstring(e.what()).c_str(), L"Error", MB_OK | MB_ICONERROR);

  return -1;
}
