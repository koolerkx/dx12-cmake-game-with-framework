#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <d3d12.h>
#include <dxgi1_6.h>
#include <wrl/client.h>

#include <exception>
#include <iostream>

#include "Application/Application.h"
#include "Graphic/Graphic.h"

template <typename T>
using ComPtr = Microsoft::WRL::ComPtr<T>;

ComPtr<ID3D12Device> device = nullptr;
ComPtr<IDXGIFactory6> dxgiFactory = nullptr;
ComPtr<IDXGISwapChain4> swap_chain = nullptr;

ComPtr<ID3D12CommandAllocator> command_allocator = nullptr;
ComPtr<ID3D12GraphicsCommandList> command_list = nullptr;

ComPtr<ID3D12CommandQueue> command_queue = nullptr;

constexpr int window_width = 1920;
constexpr int window_height = 1080;

int WINAPI wWinMain([[maybe_unused]] HINSTANCE hInstance,
  [[maybe_unused]] HINSTANCE hPrevInstance,
  [[maybe_unused]] PWSTR lpCmdLine,
  [[maybe_unused]] int nCmdShow) {
  SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);

  try {
    Application app(hInstance, window_width, window_height);
    Graphic graphic;

    graphic.Initalize(app.GetHwnd(), window_width, window_height);

    const std::function<void(float dt)> OnUpdate = [&]([[maybe_unused]] float dt) {
      graphic.BeginRender();
      graphic.EndRender();
    };

    const std::function<void(float fdt)> OnFixedUpdate = []([[maybe_unused]] float fdt) { std::cout << "FixedUpdate" << fdt << std::endl; };

    app.Run(OnUpdate, OnFixedUpdate);
  } catch (const std::exception& e) {
    std::cerr << "Exception: " << e.what() << std::endl;
    MessageBoxW(nullptr, L"An exception occurred", L"Error", MB_OK | MB_ICONERROR);
  }
  return 0;
}
