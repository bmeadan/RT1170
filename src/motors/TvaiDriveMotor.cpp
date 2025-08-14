#include "TvaiDriveMotor.h"
#include "motors.h"
#include <stdlib.h>

#define STARTUP_PROFILE_FINAL_PWM 10 // 20
#define STARTUP_PROFILE_INCREMENT 1
#define MAX_PWM_DIFF              10

void TvaiDriveMotor::init() {
    driver.init();
}

void TvaiDriveMotor::loop(uint32_t dt_ms) {
    if (startup_needed) {
        if (current_required_pwm < pwm_target) {
            current_required_pwm += STARTUP_PROFILE_INCREMENT;
        } else {
            current_required_pwm = pwm_target;
            startup_needed       = false;
        }
    } else if (stop_profile_needed) {
        if (current_required_pwm > 0) {
            current_required_pwm -= STARTUP_PROFILE_INCREMENT;
        } else {
            current_required_pwm = 0;
            stop_profile_needed  = false;
        }
    } else {
        current_required_pwm = pwm_target;
    }
}

void TvaiDriveMotor::setValue(int8_t value) {
    // Update PWM & direction targets
    pwm_target    = abs(value);
    new_direction = (value > 0);

    // Check if startup profile needed
    if ((last_pwm_cmd == 0) && (value != 0)) {
        startup_needed = true;
    }

    // Check if stop profile needed
    if ((last_pwm_cmd > 16) && (value == 0)) {
        stop_profile_needed = true;
    }
}
