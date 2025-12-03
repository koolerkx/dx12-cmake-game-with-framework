#include "Application.h"
// NOLINTBEGIN(cppcoreguidelines-pro-type-reinterpret-cast, performance-no-int-to-ptr)

#include <memory>
#include <stdexcept>

Application::Application(HINSTANCE__* const hInstance, const int width, const int height, const float frequency)
    : hInstance_(hInstance), hwnd_(nullptr), width_(width), height_(height), running_(true), frequency_(frequency) {
  if (!InitWindow()) {
    throw std::runtime_error("Failed to initialize window");
  }
}

Application::~Application() {
  if (hwnd_ != nullptr) {
    DestroyWindow(hwnd_);
  }
}

bool Application::InitWindow() {
  WNDCLASSEXW wc = {};

  wc.cbSize = sizeof(WNDCLASSEX);
  wc.style = CS_HREDRAW | CS_VREDRAW;
  wc.lpfnWndProc = WindowProc;
  wc.hInstance = hInstance_;
  // wc.hIcon = LoadIcon(hInstance_, MAKEINTRESOURCE(IDI_APP_ICON));
  wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
  wc.hbrBackground = GetSysColorBrush(COLOR_WINDOW);
  wc.lpszMenuName = nullptr;
  wc.lpszClassName = WINDOW_CLASS.c_str();
  wc.hIconSm = LoadIcon(hInstance_, IDI_APPLICATION);

  if (RegisterClassExW(&wc) == 0U) {
    return false;
  }

  constexpr DWORD STYLE = WS_OVERLAPPEDWINDOW & ~(WS_MAXIMIZEBOX | WS_THICKFRAME);

  RECT window_rect = {0, 0, static_cast<LONG>(width_), static_cast<LONG>(height_)};
  AdjustWindowRect(&window_rect, STYLE, FALSE);

  const int window_width = window_rect.right - window_rect.left;
  const int window_height = window_rect.bottom - window_rect.top;

  const int desktop_width = GetSystemMetrics(SM_CXSCREEN);
  const int desktop_height = GetSystemMetrics(SM_CYSCREEN);
  const int window_x = (std::max)(0, (desktop_width - window_width) / 2);
  const int window_y = (std::max)(0, (desktop_height - window_height) / 2);

  hwnd_ = CreateWindowExW(0,
    WINDOW_CLASS.c_str(),
    WINDOW_NAME.c_str(),
    STYLE,
    window_x,
    window_y,
    window_width,
    window_height,
    nullptr,
    nullptr,
    hInstance_,
    this);

  if (hwnd_ == nullptr) {
    return false;
  }

  ShowWindow(hwnd_, SW_SHOW);
  UpdateWindow(hwnd_);

  timer_updater_ = std::make_unique<TimerUpdater>(FIXED_HZ);

  return true;
}

LRESULT CALLBACK Application::WindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
  Application* app = nullptr;

  if (msg == WM_CREATE) {
    const auto* pCreate = reinterpret_cast<CREATESTRUCT*>(lParam);
    app = static_cast<Application*>(pCreate->lpCreateParams);
    SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(app));
  } else {
    app = reinterpret_cast<Application*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
  }

  switch (msg) {
    case WM_DESTROY:
      if (app != nullptr) {
        app->running_ = false;
      }
      // PostQuitMessage(0);
      return 0;

    case WM_KEYDOWN:
      if (wParam == VK_ESCAPE) {
        if (app != nullptr) {
          app->running_ = false;
        }
        DestroyWindow(hwnd);
      }
      return 0;
    default:
      break;
  }

  return DefWindowProc(hwnd, msg, wParam, lParam);
}

int Application::Run(const std::function<void(float dt)>& OnUpdate, const std::function<void(float fdt)>& OnFixedUpdate) {
  MSG msg = {};

  if (OnUpdate == nullptr || OnFixedUpdate == nullptr) {
    throw std::runtime_error("Application: OnUpdate or OnFixedUpdate is nullptr");
  }

  while (running_) {
    // Process all pending messages
    while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
      TranslateMessage(&msg);
      DispatchMessage(&msg);
    }

    timer_updater_->tick(OnUpdate, OnFixedUpdate);
  }

  return static_cast<int>(msg.wParam);
}
// NOLINTEND(cppcoreguidelines-pro-type-reinterpret-cast, performance-no-int-to-ptr)
