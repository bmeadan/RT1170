// src/host_sim/sim.cpp
// Host-only flip simulation that:
// 1) Asks for initial pose (left/right/upside)
// 2) Fakes a trigger into FlipController (L2-like)
// 3) Runs a 10ms loop and logs actuator commands until done

#include <cstdio>
#include <cstdint>
#include <cmath>
#include <thread>
#include <chrono>

// ---------- IMU pose for the sim (degrees) ----------
struct SimIMU {
  float roll_deg  = 0.0f;  // +roll = right side down, -roll = left side down
  float pitch_deg = 0.0f;  // +pitch = nose up (180 = upside down)
  float yaw_deg   = 0.0f;
} g_imu;

// If your controller reads IMU via free functions, provide these symbols.
// If they’re unused, they’ll be dead-stripped by the linker.
extern "C" float imu_get_roll_deg()  { return g_imu.roll_deg;  }
extern "C" float imu_get_pitch_deg() { return g_imu.pitch_deg; }
extern "C" float imu_get_yaw_deg()   { return g_imu.yaw_deg;   }

// ---------- Actuator command sinks (capture whatever the controller calls) ----------
static int8_t g_last_yaw_pwm   = 127; // 127 means "unset"
static int8_t g_last_pitch_pwm = 127;

// common 2-axis API seen in some codebases
extern "C" void send_yaw_pitch_cmd(int8_t yaw_pwm, int8_t pitch_pwm) {
  if (yaw_pwm != g_last_yaw_pwm || pitch_pwm != g_last_pitch_pwm) {
    g_last_yaw_pwm = yaw_pwm;
    g_last_pitch_pwm = pitch_pwm;
    std::printf("[SIM] motors: yaw_pwm=%d, pitch_pwm=%d\n", (int)yaw_pwm, (int)pitch_pwm);
  }
}

// older 4-arg pattern sometimes used
extern "C" void set_motors(int8_t yaw, int8_t pitch, int8_t drive, int8_t mode) {
  (void)drive; (void)mode;
  send_yaw_pitch_cmd(yaw, pitch);
}

// ---------- L2/trigger shim (if your code polls a button) ----------
static bool g_L2 = false;
extern "C" bool controller_L2_pressed() { return g_L2; }

// ---------- Helpers ----------
static constexpr float DT_SEC = 0.010f; // 10 ms

static void print_pose() {
  std::printf("[SIM] pose: roll=%.1f°, pitch=%.1f°, yaw=%.1f°\n",
              g_imu.roll_deg, g_imu.pitch_deg, g_imu.yaw_deg);
}

static void set_initial_pose_from_choice(int choice) {
  switch (choice) {
    case 1: // On Left side
      g_imu.roll_deg = -90.0f; g_imu.pitch_deg = 0.0f; g_imu.yaw_deg = 0.0f; break;
    case 2: // On Right side
      g_imu.roll_deg = +90.0f; g_imu.pitch_deg = 0.0f; g_imu.yaw_deg = 0.0f; break;
    case 3: // Upside down
      g_imu.roll_deg = 0.0f; g_imu.pitch_deg = 180.0f; g_imu.yaw_deg = 0.0f; break;
    default:
      std::printf("[SIM] Invalid choice; defaulting to 'Upside down'.\n");
      g_imu.roll_deg = 0.0f; g_imu.pitch_deg = 180.0f; g_imu.yaw_deg = 0.0f; break;
  }
}

static bool nearly_zero(int v, int eps = 3) { return std::abs(v) <= eps; }

static bool level_again(float roll_eps = 12.0f, float pitch_eps = 12.0f) {
  return (std::fabs(g_imu.roll_deg)  <= roll_eps) &&
         (std::fabs(g_imu.pitch_deg) <= pitch_eps);
}

// crude physics so pose changes when PWM changes (just to visualize progress)
static void integrate_pose_from_pwm(float dt) {
  static constexpr float YAW_RATE_DPS   = 90.0f;   // deg/sec for 100% PWM
  static constexpr float PITCH_RATE_DPS = 120.0f;  // deg/sec for 100% PWM

  const float yaw_rate   = (g_last_yaw_pwm   == 127 ? 0.0f : (g_last_yaw_pwm   / 100.0f) * YAW_RATE_DPS);
  const float pitch_rate = (g_last_pitch_pwm == 127 ? 0.0f : (g_last_pitch_pwm / 100.0f) * PITCH_RATE_DPS);

  g_imu.yaw_deg   += yaw_rate   * dt;
  g_imu.pitch_deg += pitch_rate * dt;

  // minimal damping of roll when pitch is near level
  if (std::fabs(g_imu.pitch_deg) < 10.0f) {
    g_imu.roll_deg *= 0.98f;
    if (std::fabs(g_imu.roll_deg) < 0.2f) g_imu.roll_deg = 0.0f;
  }

  while (g_imu.yaw_deg > 180.0f) g_imu.yaw_deg -= 360.0f;
  while (g_imu.yaw_deg < -180.0f) g_imu.yaw_deg += 360.0f;
}

// ---------- Include controller AFTER stubs ----------
#include "../controllers/FlipController.h"
// IMPORTANT: do NOT include ../motors/RSBL8512.h directly here.
// The .gen/redirects path already brings in a mock RSBL8512 via fsl_common.h → mock_all.h,
// and including the real header will cause a redefinition.

// If you ever want to use the real RSBL8512 constructor with IDs, set this to 1 (e.g., via -DRSBL_CTOR_TAKES_ID=1)
#ifndef RSBL_CTOR_TAKES_ID
#define RSBL_CTOR_TAKES_ID 0
#endif

int main() {
  std::puts("[SIM] Flip simulation");
  std::puts("Choose initial position:\n  1) On Left side\n  2) On Right side\n  3) Upside down");
  std::printf("Enter 1/2/3: ");
  int choice = 0;
  if (std::scanf("%d", &choice) != 1) choice = 3;

  set_initial_pose_from_choice(choice);
  print_pose();


  RSBL8512 yawMotor(0);
  RSBL8512 pitchMotor(1);


  FlipController fc(yawMotor, pitchMotor);

  // Enable controller
  fc.setEnabled();

  // Fake the trigger (choose one that exists in your class; both included here)
  g_L2 = true;                  // if your code polls controller_L2_pressed()
  fc.triggerRecovery();         // if your class exposes this
  // fc.triggerRecoveryFromISR(); // or this one

  std::puts("[SIM] Triggered → flipping…");

  const int MAX_TICKS = 12000; // 120 seconds guard
  int stable_ticks = 0;

  for (int tick = 0; tick < MAX_TICKS; ++tick) {
    // Integrate world according to commanded PWMs
    integrate_pose_from_pwm(DT_SEC);

    if (tick % 50 == 0) print_pose();

    // Completion: motors idle + level attitude for ~0.5s
    const int yaw_cmd   = (g_last_yaw_pwm   == 127 ? 0 : g_last_yaw_pwm);
    const int pitch_cmd = (g_last_pitch_pwm == 127 ? 0 : g_last_pitch_pwm);
    const bool motors_idle = nearly_zero(yaw_cmd) && nearly_zero(pitch_cmd);

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
