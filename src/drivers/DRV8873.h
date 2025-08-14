#pragma once
#include "flexpwm/flexpwm.h"
#include "fsl_common.h"
#include "interfaces/IDriver.h"

/**
 * Texas Instruments DRV8873 brushed DC motor driver
 * @link https://www.ti.com/product/DRV8873
 */
class DRV8873 : public IDriver {

  public:
    struct dir_pin_t {
        GPIO_Type *base;
        const uint32_t ph;
        const uint32_t sdo_mode = 0;
        const uint32_t disable  = 0;
        const uint32_t sleep_n  = 0;
    };

    DRV8873(const pwm_pin_t &pwm_pin, const dir_pin_t &direction_pin)
        : pwm_pin(pwm_pin), direction_pin(direction_pin) {}

    void init() override;
    void setDirection(bool direction) override;
    void setPWM(uint8_t duty_cycle) override;

  private:
    const dir_pin_t &direction_pin;
    const pwm_pin_t &pwm_pin;
};
