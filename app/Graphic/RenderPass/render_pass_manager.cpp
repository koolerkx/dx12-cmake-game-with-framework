#include "render_pass_manager.h"

#include <cassert>

#include "Framework/Logging/logger.h"
#include "RenderPass/render_pass.h"
#include "render_target.h"
#include "depth_buffer.h"
#include "graphic.h"

bool RenderPassManager::Initialize(ID3D12Device* device, uint32_t frame_count, UploadContext& upload_context) {
  assert(device != nullptr);

  // Initialize shared scene renderer
  if (!scene_renderer_.Initialize(device, frame_count)) {
    Logger::Log(LogLevel::Error, LogCategory::Graphic, "[RenderPassManager] Failed to initialize scene renderer.");
    return false;
  }

  // Initialize fullscreen pass helper
  if (!fullscreen_helper_.Initialize(device, upload_context)) {
    Logger::Log(LogLevel::Error, LogCategory::Graphic, "[RenderPassManager] Failed to initialize fullscreen pass helper.");
    return false;
  }

  Logger::Log(LogLevel::Info, LogCategory::Graphic, "[RenderPassManager] Initialized.");
  return true;
}

void RenderPassManager::RegisterPass(const std::string& name, std::unique_ptr<RenderPass> pass) {
  assert(pass != nullptr);

  RenderPass* pass_ptr = pass.get();
  passes_.push_back(std::move(pass));
  pass_map_[name] = pass_ptr;

  Logger::Logf(LogLevel::Info, LogCategory::Graphic, Logger::Here(), "[RenderPassManager] Registered pass: {}", name);
}

RenderPass* RenderPassManager::GetPass(const std::string& name) {
  auto it = pass_map_.find(name);
  if (it != pass_map_.end()) {
    return it->second;
  }
  return nullptr;
}

void RenderPassManager::SubmitPacket(const RenderPacket& packet) {
  if (!packet.IsValid()) {
    Logger::Log(LogLevel::Warn, LogCategory::Validation, "[RenderPassManager] Invalid render packet submitted.");
    return;
  }

  render_queue_.push_back(packet);
}

void RenderPassManager::SubmitToPass(const std::string& name, const RenderPacket& packet) {
  if (!packet.IsValid()) {
    Logger::Logf(
      LogLevel::Warn, LogCategory::Validation, Logger::Here(), "[RenderPassManager] Invalid render packet submitted to pass '{}'.", name);
    return;
  }

  RenderPass* pass = GetPass(name);
  if (!pass) {
    Logger::Logf(LogLevel::Warn,
      LogCategory::Validation,
      Logger::Here(),
      "[RenderPassManager] Pass '{}' not found; falling back to unified queue.",
      name);
    render_queue_.push_back(packet);
    return;
  }

  pass_queues_[pass].push_back(packet);
}

void RenderPassManager::RenderFrame(Graphic& graphic, TextureManager& texture_manager) {
  ID3D12GraphicsCommandList* command_list = graphic.GetCommandList();
  assert(command_list != nullptr);

  if (render_queue_.empty()) {
    // Even if unified queue is empty, per-pass queues may have items
    bool any_direct_packets = false;
    for (const auto& pair : pass_queues_) {
      if (!pair.second.empty()) {
        any_direct_packets = true;
        break;
      }
    }
    if (!any_direct_packets) {
      return;
    }
  }

  // Execute each enabled pass in order
  for (const auto& pass : passes_) {
    if (!pass->IsEnabled()) {
      continue;
    }

    // Clear scene renderer and feed packets from unified queue or pass-specific queue
    scene_renderer_.Clear();

    // Unified queue flows through all passes
    for (const auto& packet : render_queue_) {
      scene_renderer_.Submit(packet);
    }
    // Pass-specific queue applies only to this pass
    auto pass_queue_it = pass_queues_.find(pass.get());
    if (pass_queue_it != pass_queues_.end()) {
      for (const auto& packet : pass_queue_it->second) {
        scene_renderer_.Submit(packet);
      }
    }

      // Apply IO (transitions, clears, binding) for this pass. If ApplyPassIO
      // returns false it indicates a fatal IO issue (e.g., invalid custom
      // target) and the pass will be skipped.
      bool ok = ApplyPassIO(pass.get(), graphic);
      if (!ok) {
        continue; // skip this pass
      }

      // Begin pass (pass-level setup should not bind RT/DSV anymore)
      pass->Begin(command_list);

      // Render pass
      pass->Render(command_list, scene_renderer_, texture_manager);

      // End pass
      pass->End(command_list);
  }
}

bool RenderPassManager::ApplyPassIO(RenderPass* pass, Graphic& graphic) {
  assert(pass != nullptr);

  PassIODesc io = pass->GetPassIO();
  ID3D12GraphicsCommandList* command_list = graphic.GetCommandList();
  assert(command_list != nullptr);

  // Resolve color target (backbuffer or custom)
  RenderTarget* color_rt = nullptr;
  if (io.color.enabled) {
    if (io.color.kind == ColorAttachmentIO::Kind::Backbuffer) {
      color_rt = graphic.GetBackBufferRenderTarget();
    } else { // Custom
      color_rt = io.color.target;
    }

    if (color_rt != nullptr && color_rt->IsValid()) {
      graphic.Transition(color_rt, io.color.state);
      if (io.color.clear) {
        graphic.Clear(color_rt, color_rt->GetClearColor().data());
      }
    } else if (io.color.enabled && io.color.kind == ColorAttachmentIO::Kind::Custom) {
      // Custom target explicitly requested but pointer invalid. In debug builds
      // treat as assert; in release, warn and skip this pass to avoid silent
      // incorrect bindings.
#if defined(_DEBUG) || defined(DEBUG)
      assert(false && "RenderPass requests custom color target but RenderTarget* is null or invalid.");
      return false;
#else
      Logger::Log(LogLevel::Warn, LogCategory::Graphic, "[RenderPassManager] Pass requests custom color target but provided RenderTarget* is null or invalid. Skipping pass.");
      return false;
#endif
    }
  }

  // Resolve depth target (main or custom)
  DepthBuffer* depth = nullptr;
  if (io.depth.enabled) {
    if (io.depth.kind == DepthAttachmentIO::Kind::MainDepth) {
      depth = graphic.GetDepthBuffer();
    } else if (io.depth.kind == DepthAttachmentIO::Kind::Custom) {
      depth = io.depth.target;
    }

      if (depth != nullptr && depth->IsValid()) {
        graphic.Transition(depth, io.depth.state);
        if (io.depth.clear) {
          graphic.Clear(depth, io.depth.clear_depth, io.depth.clear_stencil);
        }
      } else if (io.depth.enabled && io.depth.kind == DepthAttachmentIO::Kind::Custom) {
        // Custom depth requested but pointer invalid — treat similarly to color.
  #if defined(_DEBUG) || defined(DEBUG)
        assert(false && "RenderPass requests custom depth target but DepthBuffer* is null or invalid.");
        return false;
  #else
        Logger::Log(LogLevel::Warn, LogCategory::Graphic, "[RenderPassManager] Pass requests custom depth target but provided DepthBuffer* is null or invalid. Skipping pass.");
        return false;
  #endif
      }
  }

  // Bind RTV/DSV according to resolved targets
  if (color_rt != nullptr && color_rt->IsValid()) {
    D3D12_CPU_DESCRIPTOR_HANDLE rtv = color_rt->GetRTV();
    if (depth != nullptr && depth->IsValid()) {
      D3D12_CPU_DESCRIPTOR_HANDLE dsv = depth->GetDSV();
      command_list->OMSetRenderTargets(1, &rtv, FALSE, &dsv);
    } else {
      command_list->OMSetRenderTargets(1, &rtv, FALSE, nullptr);
    }
  } else if ((color_rt == nullptr || !io.color.enabled) && depth != nullptr && depth->IsValid()) {
    // Depth-only pass: bind DSV with no RTV
    D3D12_CPU_DESCRIPTOR_HANDLE dsv = depth->GetDSV();
    command_list->OMSetRenderTargets(0, nullptr, FALSE, &dsv);
  }

  // Ensure viewport/scissor are set from Graphic if the pass opts in
  if (io.use_default_viewport_scissor) {
    D3D12_VIEWPORT vp = graphic.GetScreenViewport();
    D3D12_RECT sc = graphic.GetScissorRect();
    command_list->RSSetViewports(1, &vp);
    command_list->RSSetScissorRects(1, &sc);
  }

  // (Custom depth already warned above if invalid.)
  return true;
}

void RenderPassManager::Clear() {
  render_queue_.clear();
  pass_queues_.clear();
  scene_renderer_.Clear();
  scene_renderer_.ResetStats();
}

void RenderPassManager::PrintStats() const {
  std::string enabled;
  enabled.reserve(passes_.size() * 24);
  for (const auto& pass : passes_) {
    enabled += std::format("  - {}: {}\n", pass->GetName(), (pass->IsEnabled() ? "Enabled" : "Disabled"));
  }

  Logger::Logf(LogLevel::Info,
    LogCategory::Graphic,
    Logger::Here(),
    "=== Render Pass Manager Statistics ===\nTotal Packets: {}\nRegistered Passes: {}\n\nEnabled "
    "Passes:\n{}======================================",
    render_queue_.size(),
    passes_.size(),
    enabled);

  // Print scene renderer stats
  scene_renderer_.PrintStats();
}
