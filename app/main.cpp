#include "Application/Application.h"
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include <exception>
#include <iostream>

#include "Application/Application.h"

int WINAPI wWinMain([[maybe_unused]] HINSTANCE hInstance,
  [[maybe_unused]] HINSTANCE hPrevInstance,
  [[maybe_unused]] PWSTR lpCmdLine,
  [[maybe_unused]] int nCmdShow) {
  SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);

  try {
    Application app(hInstance, 1920, 1080);

    std::function<void(float dt)> OnUpdate = []([[maybe_unused]] float dt) { std::cout << "Update" << dt << std::endl; };
    std::function<void(float fdt)> OnFixedUpdate = []([[maybe_unused]] float fdt) { std::cout << "FixedUpdate" << fdt << std::endl; };

    app.Run(OnUpdate, OnFixedUpdate);
  } catch (const std::exception& e) {
    std::cerr << "Exception: " << e.what() << std::endl;
    MessageBoxW(nullptr, L"An exception occurred", L"Error", MB_OK | MB_ICONERROR);
  }
  return 0;
}
