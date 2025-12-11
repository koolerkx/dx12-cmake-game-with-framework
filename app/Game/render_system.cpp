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
    cached_camera_data_.is_valid = false;
  }

  // Clear previous frame data
  scene_renderer.Clear();
  scene_renderer.ResetStats();

  // Submit all renderables from the scene to the unified render queue
  Submit(scene, render_pass_manager);

  // Execute all render passes (Forward, UI, etc.)
  graphic_->RenderFrame();

  // Render debug visuals after main rendering
  RenderDebugVisuals(scene_renderer);

  // Render 2D debug visuals (UI overlay)
  RenderDebugVisuals2D();

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

    render_pass_manager.SubmitPacket(packet);
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
  if (!graphic_ || debug_service_.GetCommandCount() == 0) {
    return;  // Nothing to render
  }

  SceneGlobalData debug_scene_data;

  if (cached_camera_data_.is_valid) {
    // Use cached camera data
    debug_scene_data.view_matrix = cached_camera_data_.view_matrix;
    debug_scene_data.projection_matrix = cached_camera_data_.projection_matrix;
    debug_scene_data.view_projection_matrix = cached_camera_data_.view_projection_matrix;
    debug_scene_data.camera_position = cached_camera_data_.camera_position;
  } else {
    // Fallback to identity matrices
    debug_scene_data.view_matrix = DirectX::XMMatrixIdentity();
    debug_scene_data.projection_matrix = DirectX::XMMatrixIdentity();
    debug_scene_data.view_projection_matrix = DirectX::XMMatrixIdentity();
    debug_scene_data.camera_position = {0.0f, 0.0f, 0.0f};
  }

  // Get current frame index from swapchain
  UINT frame_index = graphic_->GetCurrentFrameIndex();
  debug_renderer_.BeginFrame(frame_index);

  // Render debug commands with current settings, passing scene_renderer's frame_cb_
  debug_renderer_.Render(
    debug_service_.GetCommands(), graphic_->GetCommandList(), debug_scene_data, scene_renderer.GetFrameConstantBuffer(), debug_settings_);
}

void RenderSystem::RenderDebugVisuals2D() {
  if (!graphic_ || debug_service_.Get2DCommandCount() == 0) {
    return;  // Nothing to render
  }

  // Create orthographic projection for UI (screen-space to NDC)
  UINT width = graphic_->GetFrameBufferWidth();
  UINT height = graphic_->GetFrameBufferHeight();

  // Top-left origin orthographic projection
  DirectX::XMMATRIX ortho_proj = DirectX::XMMatrixOrthographicOffCenterLH(0.0f,
    static_cast<float>(width),  // left, right
    static_cast<float>(height),
    0.0f,  // top, bottom (flipped for top-left origin)
    0.0f,
    1.0f  // near, far
  );

  UISceneData ui_scene_data;
  ui_scene_data.view_projection_matrix = ortho_proj;

  // Get current frame index
  UINT frame_index = graphic_->GetCurrentFrameIndex();
  debug_renderer_2d_.BeginFrame(frame_index);

  // Render 2D debug commands
  debug_renderer_2d_.Render(debug_service_.Get2DCommands(), graphic_->GetCommandList(), ui_scene_data, debug_settings_);
}
