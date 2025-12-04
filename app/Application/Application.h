#pragma once

#include "timer_updater.h"

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include <functional>
#include <memory>
#include <string>

const std::wstring WINDOW_CLASS = L"DirectX 12 Game with Engine";
const std::wstring WINDOW_NAME = L"DirectX 12 Game with Engine";

// todo: extract
constexpr float FIXED_HZ = 60.0f;

// Extract to config
constexpr int INIT_WINDOW_WIDTH = 1920;
constexpr int INIT_WINDOW_HEIGHT = 1080;

class Application {
 public:
  explicit Application(HINSTANCE hInstance, int width = INIT_WINDOW_WIDTH, int height = INIT_WINDOW_HEIGHT, float frequency = FIXED_HZ);
  ~Application();

  int Run(const std::function<void(float dt)>& OnUpdate = nullptr, const std::function<void(float fdt)>& OnFixedUpdate = nullptr);

  void SetFrequency(float frequency) {
    frequency_ = frequency;
  }
  [[nodiscard]] float GetFrequency() const {
    return frequency_;
  }

  void SetTimeScale(const float s) const {
    timer_updater_->SetTimeScale(s);
  }

  [[nodiscard]] HWND GetHwnd() const {
    return hwnd_;
  }

 private:
  static LRESULT CALLBACK WindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

  bool InitWindow();

  HINSTANCE hInstance_;
  HWND hwnd_;
  int width_;
  int height_;
  bool running_;

  float frequency_;

  std::unique_ptr<TimerUpdater> timer_updater_ = nullptr;
};
