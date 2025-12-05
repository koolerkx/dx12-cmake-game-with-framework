#include "game.h"

void Game::OnUpdate([[maybe_unused]] float dt) {
  graphic_.BeginRender();
  graphic_.DrawTestQuad();
  graphic_.EndRender();
}

void Game::OnFixedUpdate([[maybe_unused]] float dt) {
}
