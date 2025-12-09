#pragma once

class GameObject;

class Component {
 public:
  Component() = default;
  virtual ~Component() = default;

  Component(const Component&) = delete;
  Component& operator=(const Component&) = delete;

  virtual void OnUpdate(float) {
  }
  virtual void OnFixedUpdate(float) {
  }

  void SetOwner(GameObject* owner) {
    owner_ = owner;
  }
  GameObject* GetOwner() const {
    return owner_;
  }

 protected:
  GameObject* owner_ = nullptr;
};
