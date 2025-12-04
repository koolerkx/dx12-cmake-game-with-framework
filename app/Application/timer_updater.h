#pragma once

#include <algorithm>
#include <cassert>
#include <chrono>
#include <thread>

constexpr float DEFAULT_FIXED_HZ = 60.0f;
constexpr float DEFAULT_MAX_CLAMP = 0.25f;
constexpr int DEFAULT_MAX_STEPS = 1;
constexpr float DEFAULT_MAX_FRAME_CLAMP_SECONDS = 0.25f;
constexpr int DEFAULT_MAX_SUB_STEPS = 8;

/**
 * @brief High-precision game loop timer supporting both variable and fixed time steps.
 *
 * Typical usage:
 * @code
 * TimerUpdate timer(60.0f); // FixedUpdate at 60Hz
 * do {
 *   timer.tick(
 *     [&](float dt)  { OnUpdate(dt); },
 *     [&](float fdt) { OnFixedUpdate(fdt); }
 *   );
 *   OnRender(timer.alpha());
 * } while (running);
 * @endcode
 */
class TimerUpdater {
 public:
  using clock = std::chrono::steady_clock;
  using secondsf = std::chrono::duration<float>;

  /**
   * @brief Constructor
   * @param fixedHz target frequency for FixedUpdate (must be > 0).
   * @param maxClamp max frame time in seconds
   * @param maxSteps max tick in per frame
   */
  explicit TimerUpdater(float fixedHz = DEFAULT_FIXED_HZ, float maxClamp = DEFAULT_MAX_CLAMP, int maxSteps = DEFAULT_MAX_STEPS);

  /**
   * @brief reset timer
   */
  void reset();

  /**
   * @brief Set fixed update frequency in Hz
   * @param hz Frequency (> 0)
   */
  void SetFixedHz(float hz = DEFAULT_FIXED_HZ);

  /**
   * @brief Set timescale
   * @param s Time scale factor
   */
  void SetTimeScale(float s = 1.0f);

  /**
   * @brief Set max frame time
   * @param seconds Maximum frame time in seconds
   */
  void SetMaxFrameClamp(float seconds);

  /**
   * @brief Set maximum allowed FixedUpdate steps per tick
   * @param n Step count (minimum 1)
   */
  void SetMaxSubSteps(int n = 1);

  /**
   * @brief Pause or resume the timer.
   * @param p True to pause, false to resume.
   */
  void SetPaused(bool p = false);

  /**
   * @brief Advance the timer by one frame.
   *
   * Calls Update (variable timestep) exactly once, and FixedUpdate (fixed timestep)
   * as many times as needed, up to the configured maximum sub-steps.
   *
   * @tparam UpdateFn Callable with signature void(float dt) for variable update.
   * @tparam FixedFn Callable with signature void(float fixedDt) for fixed update.
   * @param update Variable timestep update function.
   * @param fixedUpdate Fixed timestep update function.
   */
  template <class UpdateFn, class FixedFn>
  void tick(UpdateFn update, FixedFn fixedUpdate) {
    // Single-thread enforcement
    if (std::this_thread::get_id() != tick_thread_id_) {
      assert(false && "tick() must be called from the same thread");
      return;  // release: ignore (or replace with logging/exception)
    }

    auto now = clock::now();
    secondsf raw = now - prev_;
    prev_ = now;

    // Treat very long stalls as suspend/minimize: drop accumulated work
    constexpr float STALL_THRESHOLD_SECONDS = 5.0f;
    if (raw > secondsf(STALL_THRESHOLD_SECONDS)) {
      raw = secondsf::zero();
      accumulator_ = secondsf::zero();
    }

    // Apply pause/time-scale and clamp extreme frame time
    secondsf frame = paused_ ? secondsf::zero() : (std::min)(secondsf(max_frame_clamp_), raw * time_scale_);

    // Cache totals (O(1) getters)
    total_unscaled_ += raw;
    total_scaled_ += frame;

    // Variable update (exactly once)
    update(frame.count());

    // Fixed-step accumulator
    accumulator_ += frame;

    int steps = 0;
    while (accumulator_ >= fixed_dt_ && steps < max_sub_steps_) {
      fixedUpdate(fixed_dt_.count());
      accumulator_ -= fixed_dt_;
      ++steps;
    }

    // Spiral-of-death guard: keep at most half step to soften pacing
    if (steps == max_sub_steps_) {
      constexpr float ACCUMULATOR_SPIRAL_CLAMP = 0.5f;
      accumulator_ = (std::min)(accumulator_, fixed_dt_ * ACCUMULATOR_SPIRAL_CLAMP);
    }

    // Interpolation alpha in [0,1]
    const auto t = accumulator_ / fixed_dt_;
    alpha_ = (std::max)(0.0f, (std::min)(1.0f, t));
  }

  /** @return Interpolation alpha (0..1) between last and next fixed update. */
  [[nodiscard]] float alpha() const noexcept {
    return alpha_;
  }

  /** @return Total unscaled elapsed time in seconds. */
  [[nodiscard]] float totalSeconds() const noexcept {
    return total_unscaled_.count();
  }

  /** @return Total scaled elapsed time in seconds. */
  [[nodiscard]] float totalScaledSeconds() const noexcept {
    return total_scaled_.count();
  }

  /** @return Current fixed update frequency in Hz. */
  [[nodiscard]] float fixedHz() const noexcept {
    return 1.0f / fixed_dt_.count();
  }

  /** @return Fixed timestep duration in seconds. */
  [[nodiscard]] float fixedDt() const noexcept {
    return fixed_dt_.count();
  }

  /** @return Current time scale factor. */
  [[nodiscard]] float timeScale() const noexcept {
    return time_scale_;
  }

  /** @return Maximum allowed frame time for clamping. */
  [[nodiscard]] float maxFrameClamp() const noexcept {
    return max_frame_clamp_;
  }

  /** @return Maximum allowed FixedUpdate steps per tick. */
  [[nodiscard]] int maxSubSteps() const noexcept {
    return max_sub_steps_;
  }

  /** @return True if timer is paused, false otherwise. */
  [[nodiscard]] bool paused() const noexcept {
    return paused_;
  }

  /**
   * @brief Rebind tick() to the current thread ID.
   * Use with caution when moving the game loop to another thread.
   */
  void BindTickThreadToCurrent();

 private:
  // Timekeeping
  clock::time_point prev_{};
  secondsf accumulator_{};
  secondsf fixed_dt_{};
  float alpha_{0.0f};

  // Parameters
  float time_scale_{1.0f};
  float max_frame_clamp_{DEFAULT_MAX_FRAME_CLAMP_SECONDS};
  int max_sub_steps_{DEFAULT_MAX_SUB_STEPS};
  bool paused_{false};

  // Cached totals
  secondsf total_unscaled_{};
  secondsf total_scaled_{};

  // Thread that owns tick()
  std::thread::id tick_thread_id_{};
};
