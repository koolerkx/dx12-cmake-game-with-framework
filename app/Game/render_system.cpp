#include "render_system.h"

#include <cassert>

#include "Component/camera_component.h"
#include "Component/renderer_component.h"
#include "Component/transform_component.h"
#include "RenderPass/scene_renderer.h"
#include "graphic.h"

void RenderSystem::RenderFrame(Scene& scene, GameObject* active_camera) {
  assert(graphic_ != nullptr);

  graphic_->BeginRender();

  SceneRenderer& scene_renderer = graphic_->GetSceneRenderer();

  if (active_camera) {
    CameraComponent* camera_component = active_camera->GetComponent<CameraComponent>();
    TransformComponent* camera_transform = active_camera->GetComponent<TransformComponent>();

    SceneData sceneData;

    DirectX::XMMATRIX view = camera_component->GetViewMatrix();
    DirectX::XMMATRIX proj = camera_component->GetProjectionMatrix();
    DirectX::XMMATRIX viewProj = XMMatrixMultiply(view, proj);

    DirectX::XMStoreFloat4x4(&sceneData.viewMatrix, XMMatrixTranspose(view));
    DirectX::XMStoreFloat4x4(&sceneData.projMatrix, XMMatrixTranspose(proj));
    DirectX::XMStoreFloat4x4(&sceneData.viewProjMatrix, XMMatrixTranspose(viewProj));

    DirectX::XMVECTOR det;
    DirectX::XMMATRIX invViewProj = XMMatrixInverse(&det, viewProj);
    DirectX::XMStoreFloat4x4(&sceneData.invViewProjMatrix, XMMatrixTranspose(invViewProj));

    sceneData.cameraPosition = camera_transform->GetPosition();

    // Set Buffer
    scene_renderer.SetSceneData(sceneData);
  }

  scene_renderer.Clear();
  scene_renderer.ResetStats();

  Submit(scene);
  graphic_->FlushRenderQueue();

  graphic_->EndRender();
}

void RenderSystem::Submit(Scene& scene) {
  const auto& game_objects = scene.GetGameObjects();

  for (const auto& game_object : game_objects) {
    if (!game_object->IsActive()) {
      continue;
    }

    auto* renderer = game_object->GetComponent<RendererComponent>();
    if (!renderer) {
      continue;
    }

    renderer->OnRender(graphic_->GetSceneRenderer());
  }
}
