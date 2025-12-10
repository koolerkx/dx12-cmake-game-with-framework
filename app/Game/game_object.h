#pragma once

#include <string>
#include <vector>

#include "RenderPass/render_layer.h"

class Component;

class GameObject {
 public:
  GameObject() = default;
  explicit GameObject(const std::string& name) : name_(name) {
  }
  ~GameObject() = default;

  GameObject(const GameObject&) = delete;
  GameObject& operator=(const GameObject&) = delete;

  // Component management
  void AddComponent(Component* component);

  template <typename T>
  T* GetComponent() {
    for (auto* comp : components_) {
      T* result = dynamic_cast<T*>(comp);
      if (result != nullptr) {
        return result;
      }
    }
    return nullptr;
  }

  // Update
  void Update(float dt);
  void FixedUpdate(float dt);

  const std::string& GetName() const {
    return name_;
  }
  void SetName(const std::string& name) {
    name_ = name;
  }

  bool IsActive() const {
    return active_;
  }
  void SetActive(bool active) {
    active_ = active;
  }

  // Render Layer & Tag system
  void SetRenderLayer(RenderLayer layer) {
    render_layer_ = layer;
  }

  RenderLayer GetRenderLayer() const {
    return render_layer_;
  }

  void SetRenderTag(RenderTag tag) {
    render_tag_ = tag;
  }

  RenderTag GetRenderTag() const {
    return render_tag_;
  }

  bool HasLayer(RenderLayer layer) const {
    return ::HasLayer(render_layer_, layer);
  }

  bool HasTag(RenderTag tag) const {
    return ::HasTag(render_tag_, tag);
  }

  bool HasAnyTag(RenderTag mask) const {
    return ::HasAnyTag(render_tag_, mask);
  }

 private:
  std::string name_;
  bool active_ = true;
  std::vector<Component*> components_;

  RenderLayer render_layer_ = RenderLayer::Opaque;
  RenderTag render_tag_ = RenderTag::None;
};
