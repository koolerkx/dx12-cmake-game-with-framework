#include "Framework/Utils/path_utils.h"

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include <vector>

#include "Framework/utils.h"

std::optional<std::filesystem::path> GetExeDir() noexcept {
  std::vector<wchar_t> buffer(MAX_PATH);

  while (true) {
    const DWORD len = GetModuleFileNameW(nullptr, buffer.data(), static_cast<DWORD>(buffer.size()));
    if (len == 0) {
      return std::nullopt;
    }

    if (len < buffer.size()) {
      break;
    }

    const size_t next_size = buffer.size() * 2;
    if (next_size > 32768) {
      return std::nullopt;
    }
    buffer.resize(next_size);
  }

  std::filesystem::path exe_path(buffer.data());
  return exe_path.parent_path();
}

std::string GetAppNameUtf8(std::string_view fallback) noexcept {
  std::vector<wchar_t> buffer(MAX_PATH);
  while (true) {
    const DWORD len = GetModuleFileNameW(nullptr, buffer.data(), static_cast<DWORD>(buffer.size()));
    if (len == 0) {
      return std::string(fallback);
    }

    if (len < buffer.size()) {
      break;
    }

    const size_t next_size = buffer.size() * 2;
    if (next_size > 32768) {
      return std::string(fallback);
    }
    buffer.resize(next_size);
  }

  const std::filesystem::path exe_path(buffer.data());
  const std::wstring stem = exe_path.stem().wstring();
  const std::string stem_utf8 = WstringToUtf8NoThrow(stem);
  if (stem_utf8.empty()) {
    return std::string(fallback);
  }
  return stem_utf8;
}

std::optional<std::filesystem::path> FindProjectRoot() noexcept {
  const auto exe_dir = GetExeDir();
  if (!exe_dir.has_value()) {
    return std::nullopt;
  }

  std::filesystem::path current = *exe_dir;
  constexpr int kMaxDepth = 10;
  for (int depth = 0; depth < kMaxDepth; ++depth) {
    const std::filesystem::path cmake_file = current / "CMakeLists.txt";
    std::error_code ec;
    if (std::filesystem::exists(cmake_file, ec) && !ec) {
      return current;
    }

    const std::filesystem::path parent = current.parent_path();
    if (parent == current) {
      break;
    }
    current = parent;
  }

  return std::nullopt;
}
