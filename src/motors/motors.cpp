#include "motors.h"
#include "project.h"
#include "FreeRTOS.h"
#include "task.h"
#include "flash/flash.h"
#include "flexpwm/flexpwm.h"
#include "encoders/encoderRLS.h"
#include "interfaces/IMotor.h"
#include "pin_mux.h"
#include <vector>

extern std::vector<IMotor *> motors;

static void motor_timer_callback(void *data) {
    while (true) {
        for (auto motor : motors) {
            motor->loop(0);
        }
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

void init_motors(void) {
    BOARD_InitMotorPins();
    BOARD_InitEncoderPins();

    init_pwms();
    // init_encoders();

    for(auto motor : motors) {
        motor->init();
    }

#ifdef ENABLE_MOTORS
    xTaskCreate(motor_timer_callback, "motors_timer", 1024U, NULL, configMAX_PRIORITIES - 2, NULL);
#endif
}

void calibrate_motors(void){
    for (auto motor : motors) {
        motor->calibrate();
    }

#ifdef ENABLE_ENCODER_RLS
    calibrateRLSEncoder();
#endif

    flash_save_config();
}

void emergency_stop_motors(void) {
    for (auto motor : motors) {
        motor->emergencyStop();
    }
}