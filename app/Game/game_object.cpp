#include "game_object.h"

#include "Component/component.h"

void GameObject::AddComponent(Component* component) {
  if (component == nullptr) {
    return;
  }

  component->SetOwner(this);
  components_.push_back(component);
}

void GameObject::Update(float dt) {
  if (!active_) {
    return;
  }

  for (auto* component : components_) {
    component->OnUpdate(dt);
  }
}

void GameObject::FixedUpdate(float dt) {
  if (!active_) {
    return;
  }

  for (auto* component : components_) {
    component->OnFixedUpdate(dt);
  }
}
