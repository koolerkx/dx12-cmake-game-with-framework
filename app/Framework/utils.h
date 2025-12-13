#pragma once

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include <stdexcept>
#include <string>
#include <string_view>

inline std::wstring Utf8ToWstring(std::string_view utf8_str) {
  if (utf8_str.empty()) return L"";

  const int size_needed = MultiByteToWideChar(CP_UTF8, 0, utf8_str.data(), static_cast<int>(utf8_str.size()), nullptr, 0);
  if (size_needed == 0) {
    throw std::runtime_error("UTF-8 to UTF-16 conversion failed");
  }

  std::wstring wstr(static_cast<size_t>(size_needed), L'\0');
  MultiByteToWideChar(CP_UTF8, 0, utf8_str.data(), static_cast<int>(utf8_str.size()), wstr.data(), size_needed);
  return wstr;
}

inline std::wstring Utf8ToWstringNoThrow(std::string_view utf8_str) noexcept {
  if (utf8_str.empty()) return L"";

  const int size_needed = MultiByteToWideChar(CP_UTF8, 0, utf8_str.data(), static_cast<int>(utf8_str.size()), nullptr, 0);
  if (size_needed == 0) {
    return L"(UTF-8 decode failed)";
  }

  std::wstring wstr(static_cast<size_t>(size_needed), L'\0');
  MultiByteToWideChar(CP_UTF8, 0, utf8_str.data(), static_cast<int>(utf8_str.size()), wstr.data(), size_needed);
  return wstr;
}

inline std::string WstringToUtf8(std::wstring_view wstr) {
  if (wstr.empty()) return std::string();

  const int size_needed = WideCharToMultiByte(CP_UTF8, 0, wstr.data(), static_cast<int>(wstr.size()), nullptr, 0, nullptr, nullptr);
  if (size_needed == 0) {
    throw std::runtime_error("UTF-16 to UTF-8 conversion failed");
  }

  std::string utf8_str(static_cast<size_t>(size_needed), '\0');
  WideCharToMultiByte(CP_UTF8, 0, wstr.data(), static_cast<int>(wstr.size()), utf8_str.data(), size_needed, nullptr, nullptr);
  return utf8_str;
}

inline std::string WstringToUtf8NoThrow(std::wstring_view wstr) noexcept {
  if (wstr.empty()) return std::string();

  const int size_needed = WideCharToMultiByte(CP_UTF8, 0, wstr.data(), static_cast<int>(wstr.size()), nullptr, 0, nullptr, nullptr);
  if (size_needed == 0) {
    return "(UTF-16 decode failed)";
  }

  std::string utf8_str(static_cast<size_t>(size_needed), '\0');
  WideCharToMultiByte(CP_UTF8, 0, wstr.data(), static_cast<int>(wstr.size()), utf8_str.data(), size_needed, nullptr, nullptr);
  return utf8_str;
}
