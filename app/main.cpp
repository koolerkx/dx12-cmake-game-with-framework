#include <Windows.h>
#include <debugapi.h>

#define WIN32_LEAN_AND_MEAN

#include <exception>
#include <iostream>

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
  try {
    std::cout << "hi" << std::endl;
    MessageBoxA(nullptr, "Hi", "Error", MB_OK | MB_ICONERROR);
  } catch (const std::exception& e) {
    std::cerr << "Exception: " << e.what() << std::endl;
    MessageBoxA(nullptr, e.what(), "Error", MB_OK | MB_ICONERROR);
  }
  return 0;
}
