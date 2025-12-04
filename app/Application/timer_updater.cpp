#include "timer_updater.h"

#include <algorithm>
#include <cassert>

constexpr float MIN_TIME_SCALE = 0.0f;
constexpr float MAX_TIME_SCALE = 10.0f;

TimerUpdater::TimerUpdater(float fixedHz, float maxClamp, int maxSteps) {
  // Parameter validation (assert + runtime fallback)
  assert(fixedHz > 0.0f && "fixedHz must be > 0");
  if (fixedHz <= 0.0f) fixedHz = DEFAULT_FIXED_HZ;

  assert(maxClamp >= 0.0f && "maxClamp must be >= 0");
  if (maxClamp < 0.0f) maxClamp = DEFAULT_MAX_CLAMP;

  assert(maxSteps >= 1 && "maxSteps must be >= 1");
  if (maxSteps < 1) maxSteps = DEFAULT_MAX_STEPS;

  fixed_dt_ = secondsf(1.0f / fixedHz);
  max_frame_clamp_ = maxClamp;
  max_sub_steps_ = maxSteps;

  reset();
  tick_thread_id_ = std::this_thread::get_id();
}

void TimerUpdater::reset() {
  prev_ = clock::now();
  accumulator_ = secondsf::zero();
  alpha_ = 0.0f;
  time_scale_ = 1.0f;
  paused_ = false;
  total_unscaled_ = secondsf::zero();
  total_scaled_ = secondsf::zero();
}

void TimerUpdater::SetFixedHz(float hz) {
  assert(hz > 0.0f && "fixedHz must be > 0");
  if (hz <= 0.0f) hz = DEFAULT_FIXED_HZ;
  fixed_dt_ = secondsf(1.0f / hz);
}

void TimerUpdater::SetTimeScale(float s) {
  s = std::max(s, MIN_TIME_SCALE);
  s = std::min(s, MAX_TIME_SCALE);
  time_scale_ = s;
}

void TimerUpdater::SetMaxFrameClamp(float seconds) {
  assert(seconds >= 0.0f);
  seconds = std::max(seconds, 0.0f);
  max_frame_clamp_ = seconds;
}

void TimerUpdater::SetMaxSubSteps(int n) {
  assert(n >= 1);
  n = std::max(n, 1);
  max_sub_steps_ = n;
}

void TimerUpdater::SetPaused(bool p) {
  paused_ = p;
}

void TimerUpdater::BindTickThreadToCurrent() {
  tick_thread_id_ = std::this_thread::get_id();
}
