#pragma once
#include <stdint.h>
#include "imu/imu.h"
 
extern "C" {
#include "FreeRTOS.h"
#include "task.h"
}
#include "../motors/RSBL8512.h"

class FlipController {
public:
  explicit FlipController(RSBL8512& yawMotor, RSBL8512& pitchMotor)
    : yaw(yawMotor), pitch(pitchMotor) {}

  void setEnabled(void);
  void setDisabled(void);
  void loop(void);
  void checkPosition(void);

  void triggerRecovery();           
  void triggerRecoveryFromISR();   

  volatile bool active = false;

private:
  RSBL8512& yaw;
  RSBL8512& pitch;

  enum Orientation : uint8_t { ORIENT_LEVEL, ORIENT_LEFT, ORIENT_RIGHT, ORIENT_BACK };
  static Orientation classifyOrientation(float pitchDeg, float rollDeg);  

  enum Phase : uint8_t { PH_IDLE, PH_ALIGN_YAW, PH_FLIP_PITCH, PH_ALIGN_FINAL, PH_DONE } phase = PH_IDLE;

  float curYaw   = 0.0f;
  float curPitch = 0.0f;
  float curRoll  = 0.0f;   

  float tgtYaw   = 0.0f;
  float tgtPitch = 0.0f;
  const float yawRateDegPerSec   = 90.0f;
  const float pitchRateDegPerSec = 120.0f;
  const float yawEps   = 1.0f;
  const float pitchEps = 1.0f;
  const float dt = 0.010f;

  bool  flipInProgress = false;
  bool  recoverRequested = false;
  float startYaw = 0.0f;
  int8_t yawSign = 0;

  TaskHandle_t taskHandle = nullptr;
}; 