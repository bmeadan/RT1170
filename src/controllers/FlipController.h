#pragma once
#include <stdint.h>
#include <math.h>
#include "imu/imu.h"

extern "C" {
#include "FreeRTOS.h"
#include "task.h"
}
#ifdef HOST_SIM
  #include "motors/RSBL8512.h"    
#else
  #include "../motors/RSBL8512.h"  // real header on device
#endif

class FlipController {
public:
  explicit FlipController(RSBL8512& yawMotor, RSBL8512& pitchMotor)
    : yaw(yawMotor), pitch(pitchMotor) {}

  void setEnabled(void);
  void setDisabled(void);
  void loop(void);               // call at ~100 Hz (dt = 0.01s)
  void checkPosition(void);      // updates curYaw/curPitch/curRoll from IMU

  void triggerRecovery();        // call on L2 press
  void triggerRecoveryFromISR(); // ISR-safe variant

  volatile bool active = false;

  enum Orientation : uint8_t {
    ORIENT_UPRIGHT = 0,
    ORIENT_LEFT,
    ORIENT_RIGHT,
    ORIENT_UPSIDE_DOWN
  };


private:
  // --------- Types ---------
  enum Phase : uint8_t {
    PH_IDLE = 0,
    PH_ALIGN_YAW,
    PH_FLIP_PITCH,
    PH_RECOVER
  };

  static Orientation classifyOrientation(float pitchDeg);

  // --------- Members ---------
  RSBL8512& yaw;
  RSBL8512& pitch;

  Phase  phase     = PH_IDLE;

  float  curYaw    = 0.0f;   // degrees [-180,180]
  float  curPitch  = 0.0f;   // degrees [-180,180]

  float  tgtYaw    = 0.0f;
  float  tgtPitch  = 0.0f;

  // rates (deg/s)
  const float yawRateDegPerSec   = 90.0f;
  const float pitchRateDegPerSec = 120.0f;

  // epsilons for “close enough”
  const float yawEps   = 1.0f;
  const float pitchEps = 1.0f;

  // loop period (s)
  const float dt = 0.010f;

  bool   flipInProgress   = false;
  bool   recoverRequested = false;

  // --------- Helpers ---------
  static inline float normalizeDeg(float d) {
    while (d > 180.0f) d -= 360.0f;
    while (d < -180.0f) d += 360.0f;
    return d;
  }

  static inline float shortestDelta(float fromDeg, float toDeg) {
    return normalizeDeg(toDeg - fromDeg);
  }

  static inline float stepTowards(float cur, float tgt, float maxStep) {
    float d = shortestDelta(cur, tgt);
    if (fabsf(d) <= maxStep) return tgt;
    return normalizeDeg(cur + ((d > 0.f) ? maxStep : -maxStep));
  }

  static inline int8_t p_cmd(float err, float kp, int8_t maxPwm, float eps) {
    if (fabsf(err) <= eps) return 0;
    float u = kp * err;
    if (u >  maxPwm) u =  maxPwm;
    if (u < -maxPwm) u = -maxPwm;
    return (int8_t)u; 
  }

  void sendCmd(int8_t yawPwm, int8_t pitchPwm);

  // Gains and limits (tune as needed)
  static constexpr float KP_YAW        = 1.0f;
  static constexpr float KP_PITCH      = 1.0f;
  static constexpr int8_t MAX_PWM_YAW  = 100;
  static constexpr int8_t MAX_PWM_PITCH= 100;

  static constexpr float SCORPION_DEG  = 45.0f; // pitch impulse toward level
};
