#include "FreeRTOS.h"
#include "task.h"
#include <math.h>
#include "FlipController.h"
#include "motors/motors.h"
#include "motors/RSBL8512.h"
#include "motors/TvaiDriveMotor.h"
#include "motors/AlonDriveMotor.h"
#include "imu/imu.h"

// ------- SIMULATION TEST ------------ -------
#define FLIP_SIM_AUTOTEST 1

static void start_flip_autotest();

static FlipController* gFlip = nullptr;

// Local task handle since the header no longer exposes one
static TaskHandle_t sFlipTaskHandle = nullptr;
// Local yaw sign used during side flips
static int8_t sYawSign = 0;

void flip_bind(RSBL8512& yaw, RSBL8512& pitch) {
    static FlipController controller(yaw, pitch);
    gFlip = &controller;
    gFlip->setEnabled();
}

void flip_command() {
    if (gFlip) {
        gFlip->triggerRecovery();
    }
}

static inline float normalize_deg(float a) {
    while (a <= -180.0f) a += 360.0f;
    while (a >   180.0f) a -= 360.0f;
    return a;
}
static inline float shortest_delta_deg(float from, float to) {
    return normalize_deg(to - from);
}
static inline float step_towards(float current, float target, float max_step) {
    float d = shortest_delta_deg(current, target);
    if (d >  max_step) return normalize_deg(current + max_step);
    if (d < -max_step) return normalize_deg(current - max_step);
    return normalize_deg(target);
}

static inline int8_t clamp_i8(int v, int lo, int hi) {
    if (v < lo) return (int8_t)lo;
    if (v > hi) return (int8_t)hi;
    return (int8_t)v;
}
static inline int8_t p_cmd(float err_deg, float kp, int8_t max_pwm, float deadband_deg) {
    if (fabsf(err_deg) <= deadband_deg) return 0;
    float u = kp * err_deg;
    return clamp_i8((int)lroundf(u), -max_pwm, max_pwm);
}

static void send_yaw_pitch_cmd(int8_t yaw_pwm, int8_t pitch_pwm) {
    motors_action_t act{};
    act.drive = 0;
    act.yaw = yaw_pwm;
    act.pitch = pitch_pwm;
    act.auto_neutral_joints = 0;
    act.drive_pulse = 0;
    act.flip_mode = 0;
    set_motors(&act);
}

// ----------------- Recovery params  -----------------
static constexpr float LEVEL_EPS       = 12.0f;
static constexpr float SIDE_ROLL_TH    = 45.0f;
static constexpr float BACK_PITCH_TH   = 100.0f;

static constexpr float YAW_PREP_DEG    = 35.0f;
static constexpr float SCORPION_DEG    = 75.0f;

static constexpr float YAW_RATE_DPS    = 90.0f;
static constexpr float PITCH_RATE_DPS  = 120.0f;

static constexpr float KP_YAW          = 1.6f;
static constexpr float KP_PITCH        = 1.6f;
static constexpr int8_t MAX_PWM_YAW    = 60;
static constexpr int8_t MAX_PWM_PITCH  = 60;

static constexpr float YAW_EPS_DEG     = 3.0f;
static constexpr float PITCH_EPS_DEG   = 3.0f;

static constexpr float DT_SEC          = 0.010f;

static void task_flip_controller(void *pvParameters) {
    auto *ctrl = static_cast<FlipController*>(pvParameters);
    while (ctrl->active) {
        ctrl->loop();
        vTaskDelay(pdMS_TO_TICKS(10));
    }
    send_yaw_pitch_cmd(0, 0);
    vTaskDelete(NULL);
}

FlipController::Orientation
FlipController::classifyOrientation(float pitchDeg, float rollDeg) {
    const float ap = fabsf(pitchDeg);
    const float ar = fabsf(rollDeg);

    // Map to new enum names from the header
    if (ap < LEVEL_EPS && ar < LEVEL_EPS) return ORIENT_UPRIGHT;
    if (ap > BACK_PITCH_TH)               return ORIENT_UPSIDE_DOWN;
    if (rollDeg <= -SIDE_ROLL_TH)         return ORIENT_LEFT;
    if (rollDeg >=  SIDE_ROLL_TH)         return ORIENT_RIGHT;
    return (rollDeg < 0) ? ORIENT_LEFT : ORIENT_RIGHT;
}

void FlipController::checkPosition(void) {
    imu_data_t imu = get_imu_data();
    curYaw   = normalize_deg((float)imu.yaw   / 100.0f);
    curPitch = normalize_deg((float)imu.pitch / 100.0f);
    curRoll  = normalize_deg((float)imu.roll  / 100.0f);
}

void FlipController::loop(void) {
    checkPosition();

    // Pickup ISR/user trigger
    if (ulTaskNotifyTake(pdTRUE, 0) > 0) {
        recoverRequested = true;
    }

    if (recoverRequested && phase == PH_IDLE) {
        Orientation o = classifyOrientation(curPitch, curRoll);
        recoverRequested = false;

        if (o == ORIENT_UPRIGHT) {
            send_yaw_pitch_cmd(0, 0);
            return;
        }

        // Plan the maneuver
        if (o == ORIENT_LEFT) {
            sYawSign = -1;
            tgtYaw   = normalize_deg(curYaw + sYawSign * YAW_PREP_DEG);
            tgtPitch = curPitch; // scorpion planned after yaw align
            phase    = PH_ALIGN_YAW;
        } else if (o == ORIENT_RIGHT) {
            sYawSign = +1;
            tgtYaw   = normalize_deg(curYaw + sYawSign * YAW_PREP_DEG);
            tgtPitch = curPitch;
            phase    = PH_ALIGN_YAW;
        } else { // ORIENT_UPSIDE_DOWN
            // No yaw align, immediate scorpion
            sYawSign = 0;
            tgtYaw   = curYaw; // hold current yaw
            tgtPitch = normalize_deg(curPitch + ((curPitch > 0) ? -SCORPION_DEG : SCORPION_DEG));
            phase    = PH_FLIP_PITCH;
        }

        flipInProgress = true;
    }

    if (!flipInProgress) {
        send_yaw_pitch_cmd(0, 0);
        return;
    }

    const float yawStepMax   = YAW_RATE_DPS   * DT_SEC;
    const float pitchStepMax = PITCH_RATE_DPS * DT_SEC;

    switch (phase) {
    case PH_IDLE:
        send_yaw_pitch_cmd(0, 0);
        break;

    case PH_ALIGN_YAW: {
        float nextYawTarget = step_towards(curYaw, tgtYaw, yawStepMax);
        float yawErr = shortest_delta_deg(curYaw, nextYawTarget);
        int8_t yawCmd = p_cmd(yawErr, KP_YAW, MAX_PWM_YAW, YAW_EPS_DEG);

        send_yaw_pitch_cmd(yawCmd, 0);

        bool yawAligned = fabsf(shortest_delta_deg(curYaw, tgtYaw)) <= YAW_EPS_DEG;
        if (yawAligned) {
            // עקרב: pitch up/down toward level
            tgtPitch = normalize_deg(curPitch + ((curPitch > 0) ? -SCORPION_DEG : SCORPION_DEG));
            phase = PH_FLIP_PITCH;
        }
        break;
    }

    case PH_FLIP_PITCH: {
        float nextPitchTarget = step_towards(curPitch, tgtPitch, pitchStepMax);
        float pitchErr = shortest_delta_deg(curPitch, nextPitchTarget);
        int8_t pitchCmd = p_cmd(pitchErr, KP_PITCH, MAX_PWM_PITCH, PITCH_EPS_DEG);

        float yawErr = shortest_delta_deg(curYaw, tgtYaw);
        int8_t yawCmd = p_cmd(yawErr, KP_YAW, MAX_PWM_YAW, YAW_EPS_DEG);

        send_yaw_pitch_cmd(yawCmd, pitchCmd);

        bool pitchReached = fabsf(shortest_delta_deg(curPitch, tgtPitch)) <= PITCH_EPS_DEG;
        if (pitchReached) {
            tgtPitch = 0.0f;    // settle to level
            phase = PH_RECOVER;
        }
        break;
    }

    case PH_RECOVER: {
        float pitchErr = shortest_delta_deg(curPitch, tgtPitch); // target 0
        int8_t pitchCmd = p_cmd(pitchErr, KP_PITCH, MAX_PWM_PITCH, PITCH_EPS_DEG);

        float yawErr = shortest_delta_deg(curYaw, tgtYaw);       // hold planned yaw
        int8_t yawCmd = p_cmd(yawErr, KP_YAW, MAX_PWM_YAW, YAW_EPS_DEG);

        send_yaw_pitch_cmd(yawCmd, pitchCmd);

        const bool leveled = fabsf(shortest_delta_deg(curPitch, 0.0f)) <= PITCH_EPS_DEG;
        if (leveled) {
            send_yaw_pitch_cmd(0, 0);
            flipInProgress = false;
            sYawSign = 0;
            phase = PH_IDLE;
        }
        break;
    }
    }
}

void FlipController::setEnabled(void) {
    if (!active) {
        active = true;
        phase = PH_IDLE;
        flipInProgress = false;
        recoverRequested = false;
        sYawSign = 0;

        xTaskCreate(
            task_flip_controller,
            "flip_controller",
            1024U,
            this,
            configMAX_PRIORITIES - 2,
            &sFlipTaskHandle
        );
    }
}

void FlipController::setDisabled(void) {
    active = false;
    send_yaw_pitch_cmd(0, 0);
}

void FlipController::triggerRecovery() {
    if (sFlipTaskHandle) {
        xTaskNotifyGive(sFlipTaskHandle);
    } else {
        recoverRequested = true;
    }
}

void FlipController::triggerRecoveryFromISR() {
    BaseType_t woken = pdFALSE;
    if (sFlipTaskHandle) {
        vTaskNotifyGiveFromISR(sFlipTaskHandle, &woken);
        portYIELD_FROM_ISR(woken);
    } else {
        recoverRequested = true;
    }
}
