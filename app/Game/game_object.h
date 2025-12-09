#pragma once

#include <string>
#include <vector>

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

 private:
  std::string name_;
  bool active_ = true;
  std::vector<Component*> components_;
};
