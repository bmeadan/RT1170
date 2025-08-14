#pragma once
#include "interfaces/IDriver.h"
#include "interfaces/IMotor.h"

class TvaiDriveMotor : public IMotor {

  public:
    TvaiDriveMotor(IDriver &driver)
        : driver(driver) {};

    void init() override;
    void loop(uint32_t dt_ms) override;
    void emergencyStop() override {
        driver.setDirection(false);
        driver.setPWM(0);
    }

    void setValue(int8_t value);

  private:
    IDriver &driver;

    int8_t pwm_target;
    int8_t current_required_pwm;
    int8_t last_pwm_cmd;

    bool startup_needed      = true;
    bool stop_profile_needed = false;
    bool new_direction       = true;
    bool prev_direction      = true;
};