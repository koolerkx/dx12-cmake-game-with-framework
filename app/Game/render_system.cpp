#include "render_system.h"

#include <cassert>
#include <iostream>

#include "Component/camera_component.h"
#include "Component/renderer_component.h"
#include "Component/transform_component.h"
#include "RenderPass/render_pass_manager.h"
#include "RenderPass/scene_renderer.h"
#include "graphic.h"

void RenderSystem::RenderFrame(Scene& scene, GameObject* active_camera) {
  assert(graphic_ != nullptr);

  // Begin frame - clears render targets, sets up command list
  graphic_->BeginFrame();
  const uint32_t frame_index = graphic_->GetCurrentFrameIndex();

  // Get render pass manager
  RenderPassManager& render_pass_manager = graphic_->GetRenderPassManager();
  SceneRenderer& scene_renderer = render_pass_manager.GetSceneRenderer();

  // Set camera data if available
  if (active_camera) {
    CameraComponent* camera_component = active_camera->GetComponent<CameraComponent>();
    TransformComponent* camera_transform = active_camera->GetComponent<TransformComponent>();

    if (camera_component && camera_transform) {
      SceneData scene_data;

      DirectX::XMMATRIX view = camera_component->GetViewMatrix();
      DirectX::XMMATRIX proj = camera_component->GetProjectionMatrix();
      DirectX::XMMATRIX view_proj = DirectX::XMMatrixMultiply(view, proj);

      // Store row-major matrices directly; HLSL declares them as row_major
      DirectX::XMStoreFloat4x4(&scene_data.viewMatrix, view);
      DirectX::XMStoreFloat4x4(&scene_data.projMatrix, proj);
      DirectX::XMStoreFloat4x4(&scene_data.viewProjMatrix, view_proj);

      DirectX::XMVECTOR det;
      DirectX::XMMATRIX inv_view_proj = DirectX::XMMatrixInverse(&det, view_proj);
      DirectX::XMStoreFloat4x4(&scene_data.invViewProjMatrix, inv_view_proj);

      scene_data.cameraPosition = camera_transform->GetPosition();

      // Set scene data in scene renderer
      scene_renderer.SetSceneData(scene_data);

      // Cache camera data for debug rendering
      cached_camera_data_.view_matrix = view;
      cached_camera_data_.projection_matrix = proj;
      cached_camera_data_.view_projection_matrix = view_proj;
      cached_camera_data_.camera_position = camera_transform->GetPosition();
      cached_camera_data_.is_valid = true;
    }
  } else {
    SceneData scene_data{};
    DirectX::XMStoreFloat4x4(&scene_data.viewMatrix, DirectX::XMMatrixIdentity());
    DirectX::XMStoreFloat4x4(&scene_data.projMatrix, DirectX::XMMatrixIdentity());
    DirectX::XMStoreFloat4x4(&scene_data.viewProjMatrix, DirectX::XMMatrixIdentity());
    DirectX::XMStoreFloat4x4(&scene_data.invViewProjMatrix, DirectX::XMMatrixIdentity());
    scene_data.cameraPosition = {0.0f, 0.0f, 0.0f};
    scene_renderer.SetSceneData(scene_data);
    cached_camera_data_.is_valid = false;
  }

  // Clear previous frame data
  scene_renderer.Clear();
  scene_renderer.ResetStats();

  // Submit all renderables from the scene to the appropriate queues
  Submit(scene, render_pass_manager);

  // Pass toggles for ordered rendering
  RenderPass* forward_pass = render_pass_manager.GetPass("Forward");
  RenderPass* ui_pass = render_pass_manager.GetPass("UI");

  // Phase 1: world (opaque + transparent). UI disabled.
  if (ui_pass) ui_pass->SetEnabled(false);
  if (forward_pass) forward_pass->SetEnabled(true);
  render_pass_manager.RenderFrame(graphic_->GetCommandList(), graphic_->GetTextureManager());

  // Phase 2: Debug 3D (uses depth written by world)
  RenderDebugVisuals(scene_renderer);

  // Phase 3: UI only
  if (forward_pass) forward_pass->SetEnabled(false);
  if (ui_pass) ui_pass->SetEnabled(true);
  render_pass_manager.RenderFrame(graphic_->GetCommandList(), graphic_->GetTextureManager());

  // Phase 4: Debug 2D overlay (RT only)
  RenderDebugVisuals2D(frame_index);

  // Restore pass enable states for next frame
  if (forward_pass) forward_pass->SetEnabled(true);
  if (ui_pass) ui_pass->SetEnabled(true);

  // Clear render queues for next frame before presenting
  render_pass_manager.Clear();

  // End frame - present
  graphic_->EndFrame();
}

void RenderSystem::Submit(Scene& scene, RenderPassManager& render_pass_manager) {
  const auto& game_objects = scene.GetGameObjects();

  for (const auto& game_object : game_objects) {
    if (!game_object->IsActive()) {
      continue;
    }

    auto* renderer = game_object->GetComponent<RendererComponent>();
    if (!renderer) {
      continue;
    }

    // Build a RenderPacket from the renderer component and submit to the
    // RenderPassManager unified queue. This ensures render_queue_ is
    // populated for the frame (previous code relied on direct
    // SceneRenderer submissions which left render_queue_ empty).
    RenderPacket packet;
    packet.mesh = renderer->GetMesh();
    packet.material = renderer->GetMaterial();
    packet.layer = renderer->GetLayer();
    packet.tag = renderer->GetTag();
    packet.color = renderer->GetColor();
    packet.uv_transform = renderer->GetUVTransform();
    packet.sort_order = renderer->GetSortOrder();

    // Get transform from the owning GameObject
    auto* transform = game_object->GetComponent<TransformComponent>();
    if (transform) {
      DirectX::XMStoreFloat4x4(&packet.world, transform->GetWorldMatrix());
    }

    if (!packet.IsValid()) {
      std::cerr << "[RenderSystem] Warning: Invalid render packet from GameObject: " << game_object->GetName() << '\n';
      continue;
    }

    // Route packets based on layer to keep UI isolated from world passes
    if (HasLayer(packet.layer, RenderLayer::UI)) {
      render_pass_manager.SubmitToPass("UI", packet);
    } else {
      render_pass_manager.SubmitToPass("Forward", packet);
    }
  }
}

void RenderSystem::Initialize(Graphic& graphic) {
  graphic_ = &graphic;

  // Initialize debug renderers
  debug_renderer_.Initialize(graphic);
  debug_renderer_2d_.Initialize(graphic);

  std::cout << "[RenderSystem] Initialized with debug visual support" << '\n';
}

void RenderSystem::Shutdown() {
  debug_renderer_.Shutdown();
  debug_renderer_2d_.Shutdown();
  graphic_ = nullptr;

  std::cout << "[RenderSystem] Shutdown complete" << '\n';
}

void RenderSystem::RenderDebugVisuals(SceneRenderer& scene_renderer) {
  if (!graphic_) {
    return;
  }

  if (!debug_settings_.enable_3d_debug) {
    return;
  }

  const auto& cmds3D = debug_service_.GetCommands3D();
  if (cmds3D.lines3D.empty()) {
    return;  // Nothing to render
  }

  ID3D12GraphicsCommandList* cmd_list = graphic_->GetCommandList();

  // Rebind primary render targets and viewport before debug passes
  auto rtv = graphic_->GetMainRTV();
  auto dsv = graphic_->GetMainDSV();
  cmd_list->OMSetRenderTargets(1, &rtv, FALSE, &dsv);

  auto viewport = graphic_->GetScreenViewport();
  auto scissor = graphic_->GetScissorRect();
  cmd_list->RSSetViewports(1, &viewport);
  cmd_list->RSSetScissorRects(1, &scissor);

  SceneGlobalData debug_scene_data{};
  const bool has_camera = cached_camera_data_.is_valid;
  if (has_camera) {
    debug_scene_data.view_matrix = cached_camera_data_.view_matrix;
    debug_scene_data.projection_matrix = cached_camera_data_.projection_matrix;
    debug_scene_data.view_projection_matrix = cached_camera_data_.view_projection_matrix;
    debug_scene_data.camera_position = cached_camera_data_.camera_position;
  } else {
    debug_scene_data.view_matrix = DirectX::XMMatrixIdentity();
    debug_scene_data.projection_matrix = DirectX::XMMatrixIdentity();
    debug_scene_data.view_projection_matrix = DirectX::XMMatrixIdentity();
    debug_scene_data.camera_position = {0.0f, 0.0f, 0.0f};
  }
  debug_scene_data.scene_cb_gpu_address = scene_renderer.GetFrameConstantBuffer().GetGPUAddress();

  const uint32_t frame_index = graphic_->GetCurrentFrameIndex();
  debug_renderer_.BeginFrame(frame_index);

  // Depth-tested pass then overlay pass share the same vertex buffer; allow toggling
  if (debug_settings_.enable_3d_depth_lines && has_camera) {
    debug_renderer_.RenderDepthTested(cmds3D, cmd_list, debug_scene_data, scene_renderer.GetFrameConstantBuffer(), debug_settings_);
  }
  if (debug_settings_.enable_3d_overlay_lines) {
    debug_renderer_.RenderOverlay(cmds3D, cmd_list, debug_scene_data, scene_renderer.GetFrameConstantBuffer(), debug_settings_);
  }

}

void RenderSystem::RenderDebugVisuals2D(uint32_t frame_index) {
  if (!graphic_) {
    return;
  }

  const auto& cmds2D = debug_service_.GetCommands2D();
  if (cmds2D.GetTotalCommandCount() == 0) {
    return;  // Nothing to render
  }

  ID3D12GraphicsCommandList* cmd_list = graphic_->GetCommandList();

  // 2D renderer expects no DSV bound (PSO DepthStencilFormat = UNKNOWN)
  auto rtv = graphic_->GetMainRTV();
  cmd_list->OMSetRenderTargets(1, &rtv, FALSE, nullptr);

  auto viewport = graphic_->GetScreenViewport();
  auto scissor = graphic_->GetScissorRect();
  cmd_list->RSSetViewports(1, &viewport);
  cmd_list->RSSetScissorRects(1, &scissor);

  // Create orthographic projection for UI (screen-space to NDC)
  UINT width = graphic_->GetFrameBufferWidth();
  UINT height = graphic_->GetFrameBufferHeight();

  DirectX::XMMATRIX ortho_proj = DirectX::XMMatrixOrthographicOffCenterLH(
    0.0f, static_cast<float>(width), 0.0f, static_cast<float>(height), 0.0f, 1.0f);

  UISceneData ui_scene_data;
  ui_scene_data.view_projection_matrix = ortho_proj;

  debug_renderer_2d_.BeginFrame(frame_index);
  if (debug_settings_.enable_2d_debug) {
    debug_renderer_2d_.Render(cmds2D, cmd_list, ui_scene_data, debug_settings_);
  }
}
