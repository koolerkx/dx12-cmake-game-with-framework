#include "render_system.h"

#include <cassert>

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

      DirectX::XMStoreFloat4x4(&scene_data.viewMatrix, DirectX::XMMatrixTranspose(view));
      DirectX::XMStoreFloat4x4(&scene_data.projMatrix, DirectX::XMMatrixTranspose(proj));
      DirectX::XMStoreFloat4x4(&scene_data.viewProjMatrix, DirectX::XMMatrixTranspose(view_proj));

      DirectX::XMVECTOR det;
      DirectX::XMMATRIX inv_view_proj = DirectX::XMMatrixInverse(&det, view_proj);
      DirectX::XMStoreFloat4x4(&scene_data.invViewProjMatrix, DirectX::XMMatrixTranspose(inv_view_proj));

      scene_data.cameraPosition = camera_transform->GetPosition();

      // Set scene data in scene renderer
      scene_renderer.SetSceneData(scene_data);
    }
  }

  // Clear previous frame data
  scene_renderer.Clear();
  scene_renderer.ResetStats();

  // Submit all renderables from the scene to the unified render queue
  Submit(scene, render_pass_manager);

  // Execute all render passes (Forward, UI, etc.)
  graphic_->RenderFrame();

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
