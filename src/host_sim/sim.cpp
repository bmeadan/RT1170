// src/host_sim/sim.cpp
#include <cstdio>
#include <cstdint>
#include <cmath>
#include <thread>
#include <chrono>

#include "mock_all.h"
SimIMU g_imu;

static inline int32_t deg_to_centideg(float d) {
  return (int32_t) std::lround(d * 100.0f);
}

// --- Actuator command sink ---
static int8_t g_last_yaw_pwm   = 0;
static int8_t g_last_pitch_pwm = 0;

extern "C" void send_yaw_pitch_cmd(int8_t yaw_pwm, int8_t pitch_pwm) {
  if (yaw_pwm != g_last_yaw_pwm || pitch_pwm != g_last_pitch_pwm) {
    g_last_yaw_pwm   = yaw_pwm;
    g_last_pitch_pwm = pitch_pwm;
    std::printf("[SIM] motors: yaw=%d pitch=%d\n", yaw_pwm, pitch_pwm);
  }
}

// --- Helpers ---
static constexpr float DT_SEC = 0.010f; // 10 ms

static void print_pose() {
  std::printf("[SIM] pose: pitch=%.1f°, yaw=%.1f°\n", g_imu.pitch_deg, g_imu.yaw_deg);
}

static void set_initial_pose_from_choice(int choice) {
  switch (choice) {
    case 1: // Left side
      g_imu.pitch_deg = -90.0f;
      g_imu.yaw_deg   = 0.0f;
      break;
    case 2: // Right side
      g_imu.pitch_deg = 90.0f;
      g_imu.yaw_deg   = 0.0f;
      break;
    case 3: // Upside down
      g_imu.pitch_deg = 180.0f;
      g_imu.yaw_deg   = 0.0f;
      break;
    default: // Invalid → treat as upright
      g_imu.pitch_deg = 0.0f;
      g_imu.yaw_deg   = 0.0f;
      break;
  }
}

static bool nearly_zero(int v, int eps = 3) { return std::abs(v) <= eps; }

static bool level_again(float yaw_eps = 12.0f, float pitch_eps = 12.0f, float roll_eps = 12.0f) {
  return (std::fabs(g_imu.yaw_deg)   <= yaw_eps) &&
         (std::fabs(g_imu.pitch_deg) <= pitch_eps);
}

// Physics: PWM drives yaw & pitch; roll relaxes near level
static void integrate_pose_from_pwm(float dt) {
  static constexpr float YAW_RATE_DPS   = 90.0f;
  static constexpr float PITCH_RATE_DPS = 120.0f;

  const float yaw_rate   = (g_last_yaw_pwm   / 100.0f) * YAW_RATE_DPS;
  const float pitch_rate = (g_last_pitch_pwm / 100.0f) * PITCH_RATE_DPS;

  g_imu.yaw_deg   += yaw_rate   * dt;
  g_imu.pitch_deg += pitch_rate * dt;

  while (g_imu.yaw_deg   > 180.0f) g_imu.yaw_deg   -= 360.0f;
  while (g_imu.yaw_deg   < -180.0f) g_imu.yaw_deg   += 360.0f;
  while (g_imu.pitch_deg > 180.0f) g_imu.pitch_deg -= 360.0f;
  while (g_imu.pitch_deg < -180.0f) g_imu.pitch_deg += 360.0f;

}

// --- Controller ---
#include "../controllers/FlipController.h"

int main() {
  std::puts("[SIM] Flip simulation");
  std::puts("Choose initial position:\n  1) On Left side\n  2) On Right side\n  3) Upside down");
  std::printf("Enter 1/2/3: ");
  int choice = 0; if (std::scanf("%d", &choice) != 1) choice = 3;

  set_initial_pose_from_choice(choice);
  print_pose();

  RSBL8512 yawMotor(0);
  RSBL8512 pitchMotor(1);
  FlipController fc(yawMotor, pitchMotor);

  // IMPORTANT: do NOT call setEnabled() in Option B (no RTOS).
  // fc.setEnabled();

  // Trigger flip; with no task handle, this sets recoverRequested=true.
  fc.triggerRecovery();
  std::puts("[SIM] Triggered → flipping…");

  const int MAX_TICKS = 3000; // 30s guard
  int stable_ticks = 0;

  for (int tick = 0; tick < MAX_TICKS; ++tick) {
    // Run your actual controller logic every 10 ms (like the task would)
    fc.loop();

    // Integrate according to the motor commands it produced
    integrate_pose_from_pwm(DT_SEC);

    if (tick % 50 == 0) print_pose();

    const bool motors_idle = nearly_zero(g_last_yaw_pwm) && nearly_zero(g_last_pitch_pwm);
    if (motors_idle && level_again()) {
      ++stable_ticks;
      if (stable_ticks >= 50) {
        std::puts("[SIM] Flip complete: motors idle and device level.");
        print_pose();
        break;
      }
    } else {
      stable_ticks = 0;
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(10));
  }

  std::puts("[SIM] done.");
  return 0;
}
