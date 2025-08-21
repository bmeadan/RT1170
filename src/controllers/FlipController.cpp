#include "FreeRTOS.h"
#include "task.h"
#include <math.h>
#include "FlipController.h"

#include "motors/motors.h"
#ifdef HOST_SIM
#include "mock_all.h"
#include "motors/RSBL8512.h"
#endif
#include "motors/TvaiDriveMotor.h"
#include "motors/AlonDriveMotor.h"
#include "imu/imu.h"
#include <initializer_list>
#include <cstdio>

// ------- SIMULATION TEST ------------ -------
#define FLIP_SIM_AUTOTEST 1

#ifdef HOST_SIM
    static void integrate_pose_from_pwm();
#endif

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
    while (a > 180.0f) a -= 360.0f;
    return a;
}

static inline float shortest_delta_deg(float from, float to) {
    float diff = to - from;
    while (diff < -180.0f) diff += 360.0f;
    while (diff > 180.0f) diff -= 360.0f;
    return diff;
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

static motors_action_t last_motor_cmd{};

static void send_yaw_pitch_cmd(int8_t yaw_pwm, int8_t pitch_pwm) {
    motors_action_t act{};
    act.drive = 0;
    act.yaw = yaw_pwm;
    act.pitch = pitch_pwm;
    act.auto_neutral_joints = 0;
    act.drive_pulse = 0;
    act.flip_mode = 0;

    last_motor_cmd = act; // store for simulation use
    set_motors(&act);
}

// ----------------- Recovery params  -----------------
static constexpr float LEVEL_EPS       = 12.0f;
static constexpr float BACK_PITCH_TH   = 100.0f;

static constexpr float YAW_PREP_DEG    = 35.0f;
static constexpr float SCORPION_DEG    = 75.0f;

static constexpr float YAW_RATE_DPS    = 90.0f;
static constexpr float PITCH_RATE_DPS  = 120.0f;

static constexpr float KP_YAW          = 1.6f;
static constexpr float KP_PITCH        = 1.6f;
static constexpr int8_t MAX_PWM_YAW    = 100;
static constexpr int8_t MAX_PWM_PITCH  = 60;

static constexpr float YAW_EPS_DEG     = 1.0f;
static constexpr float PITCH_EPS_DEG   = 1.0f;

static constexpr float DT_SEC          = 0.010f;

static void task_flip_controller(void *pvParameters) {
    auto *ctrl = static_cast<FlipController*>(pvParameters);
    while (ctrl->active) {
#ifdef HOST_SIM
    integrate_pose_from_pwm();
#endif
        ctrl->loop();
        vTaskDelay(pdMS_TO_TICKS(10));
    }
    send_yaw_pitch_cmd(0, 0);
    vTaskDelete(NULL);
}

FlipController::Orientation
FlipController::classifyOrientation(float pitchDeg) {
    if (pitchDeg > 90.0f) return ORIENT_UPSIDE_DOWN;
    if (pitchDeg < -45.0f) return ORIENT_LEFT;
    if (pitchDeg > 45.0f) return ORIENT_RIGHT;
    return ORIENT_UPRIGHT;
}

void FlipController::checkPosition(void) {
    imu_data_t imu = get_imu_data();
    curYaw   = normalize_deg((float)imu.yaw   / 100.0f);
    curPitch = normalize_deg((float)imu.pitch / 100.0f);
}

void FlipController::loop(void) {
    #ifdef HOST_SIM
        integrate_pose_from_pwm();
    #endif
    checkPosition();

    if (ulTaskNotifyTake(pdTRUE, 0) > 0) {
        recoverRequested = true;
    }
    if (recoverRequested && phase == PH_IDLE) {
    Orientation o = classifyOrientation(curPitch);
    std::printf("[SIM] Detected orientation: %d (pitch=%.1f)\n", (int)o, curPitch);

    recoverRequested = false;

    if (o == ORIENT_UPRIGHT) {
        send_yaw_pitch_cmd(0, 0);
        return;
    }

    flipInProgress = true;

    if (o == ORIENT_UPSIDE_DOWN) {
        tgtPitch = -90.0f;
        startSequence({PH_PITCH_DOWN});
    }
    else if (o == ORIENT_LEFT) {
        sYawSign = 1;
        tgtYaw = normalize_deg(curYaw + 90.0f);
        tgtPitch = 0.0f;
        startSequence({PH_YAW_TURN1, PH_PITCH_UP, PH_PITCH_DOWN, PH_YAW_TURN2});
    }
    else if (o == ORIENT_RIGHT) {
        sYawSign = -1;
        tgtYaw = normalize_deg(curYaw - 90.0f);
        tgtPitch = 0.0f;
        startSequence({PH_YAW_TURN1, PH_PITCH_UP, PH_PITCH_DOWN, PH_YAW_TURN2});
    }

    }

    if (!flipInProgress) {
        send_yaw_pitch_cmd(0, 0);
        return;
    }

    const float yawStepMax   = YAW_RATE_DPS   * DT_SEC;
    const float pitchStepMax = PITCH_RATE_DPS * DT_SEC;

    if (!flipInProgress || currentStepIndex >= sequenceLength) {
        send_yaw_pitch_cmd(0, 0);
        return;
    }

    phase = phaseSequence[currentStepIndex];
    switch (phase) {
    case PH_IDLE:
        send_yaw_pitch_cmd(0, 0);
        #ifdef HOST_SIM
        std::printf("[SIM] Idle: curPitch=%.1f curYaw=%.1f (no recovery in progress)\n", curPitch, curYaw);
        #endif
        currentStepIndex++;
        break;

    case PH_YAW_TURN1: {
        float yawErr = shortest_delta_deg(curYaw, tgtYaw);
        int8_t yawCmd = p_cmd(yawErr, KP_YAW, MAX_PWM_YAW, YAW_EPS_DEG);
        #ifdef HOST_SIM
        std::printf("[SIM] YAW_TURN1: cur=%.1f tgt=%.1f err=%.1f cmd=%d\n", curYaw, tgtYaw, yawErr, yawCmd);
        #endif
        send_yaw_pitch_cmd(yawCmd, 0);

        if (fabsf(yawErr) <= YAW_EPS_DEG) {
            send_yaw_pitch_cmd(0, 0);
            tgtPitch = 0.0f;
            currentStepIndex++;  
        }
        break;
    }



    case PH_PITCH_UP: {
        float pitchErr = shortest_delta_deg(curPitch, tgtPitch);
        int8_t pitchCmd = p_cmd(pitchErr, KP_PITCH, MAX_PWM_PITCH, PITCH_EPS_DEG);
        #ifdef HOST_SIM
        std::printf("[SIM] PITCH_UP: cur=%.1f tgt=%.1f err=%.1f cmd=%d\n", curPitch, tgtPitch, pitchErr, pitchCmd);
        #endif
        send_yaw_pitch_cmd(0, pitchCmd);

        if (fabsf(pitchErr) <= PITCH_EPS_DEG) {
            send_yaw_pitch_cmd(0, 0);
            tgtPitch = -90.0f;
            currentStepIndex++;  
        }
        break;
    }

    case PH_PITCH_DOWN: {
        float nextPitch = step_towards(curPitch, tgtPitch, pitchStepMax);
        float pitchErr = shortest_delta_deg(curPitch, nextPitch);
        int8_t pitchCmd = p_cmd(pitchErr, KP_PITCH, MAX_PWM_PITCH, PITCH_EPS_DEG);
        send_yaw_pitch_cmd(0, pitchCmd);

        if (fabsf(shortest_delta_deg(curPitch, tgtPitch)) <= PITCH_EPS_DEG) {
            tgtYaw = normalize_deg(curYaw + sYawSign * 90.0f);
            #ifdef HOST_SIM
            std::printf("[SIM] Planning yaw: curYaw=%.1f → tgtYaw=%.1f\n", curYaw, tgtYaw);
            #endif
            phase = PH_YAW_TURN2;
        }
        currentStepIndex++;
        break;
    }

case PH_YAW_TURN2:
  {
    imu_data_t imu = get_imu_data();
    //float curYaw = normalize_deg((float)imu.yaw / 100.0f);
    float yawErr = shortest_delta_deg(curYaw, tgtYaw);
    int yawCmd = p_cmd(yawErr, KP_YAW, MAX_PWM_YAW, YAW_EPS_DEG);
    #ifdef HOST_SIM
        std::printf("[SIM] YAW_TURN2: cur=%.1f tgt=%.1f err=%.1f cmd=%d\n",
                    curYaw, tgtYaw, yawErr, yawCmd);
    #endif
    motors_action_t act = {};
    act.yaw = yawCmd;
    act.flip_mode = 0;
    set_motors(&act);

    #ifdef HOST_SIM
    g_imu.yaw_deg = normalize_deg(g_imu.yaw_deg + (float)yawCmd * DT_SEC);
    #endif

    if (fabsf(yawErr) <= YAW_EPS_DEG || yawCmd == 0) {
        motors_action_t stop = {};
        set_motors(&stop);
        #ifdef HOST_SIM
        std::printf("[SIM] YAW_TURN2 complete, switching to IDLE\n");
        #endif
        flipInProgress = false;
        phase = PH_IDLE;  
    }
    currentStepIndex++;
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

#ifdef HOST_SIM
static motors_action_t get_last_motor_cmd() {
    return last_motor_cmd;
}

static void integrate_pose_from_pwm() {
    motors_action_t act = get_last_motor_cmd();

    float pitch_change = (float)act.pitch * DT_SEC * PITCH_RATE_DPS;
    float yaw_change   = (float)act.yaw   * DT_SEC * YAW_RATE_DPS;

    g_imu.pitch_deg = normalize_deg(g_imu.pitch_deg + pitch_change);
    g_imu.yaw_deg   = normalize_deg(g_imu.yaw_deg + yaw_change);
}
#endif


void FlipController::startSequence(std::initializer_list<phase_t> seq) {
    sequenceLength = seq.size();
    std::copy(seq.begin(), seq.end(), phaseSequence);
    currentStepIndex = 0;
    flipInProgress = true;
}