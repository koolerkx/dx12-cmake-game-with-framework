#pragma once

#include <cstdint>

// Context identifiers for hot-path (Draw/Frame) error helpers.
// Naming convention: Subsystem::Action::Detail
enum class ContextId : uint16_t {
  Unknown = 0,

  Graphic_BeginFrame_ResetCommandAllocator,
  Graphic_BeginFrame_ResetCommandList,
  Graphic_EndFrame_CloseCommandList,
  Graphic_Present_SwapChainPresent,
};

[[nodiscard]] constexpr const char* ToContextString(ContextId id) noexcept {
  switch (id) {
    case ContextId::Graphic_BeginFrame_ResetCommandAllocator:
      return "Graphic::BeginFrame::ResetCommandAllocator";
    case ContextId::Graphic_BeginFrame_ResetCommandList:
      return "Graphic::BeginFrame::ResetCommandList";
    case ContextId::Graphic_EndFrame_CloseCommandList:
      return "Graphic::EndFrame::CloseCommandList";
    case ContextId::Graphic_Present_SwapChainPresent:
      return "Graphic::Present::SwapChainPresent";
    case ContextId::Unknown:
    default:
      return "Unknown::Unknown::Unknown";
  }
}
