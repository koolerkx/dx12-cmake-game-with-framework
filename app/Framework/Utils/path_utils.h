#pragma once

#include <filesystem>
#include <optional>
#include <string>
#include <string_view>

std::optional<std::filesystem::path> GetExeDir() noexcept;

std::string GetAppNameUtf8(std::string_view fallback) noexcept;

std::optional<std::filesystem::path> FindProjectRoot() noexcept;
