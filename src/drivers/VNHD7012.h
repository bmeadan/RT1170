#pragma once
#include "flexpwm/flexpwm.h"
#include "interfaces/IDriver.h"

/**
 * STMicroelectronics VNHD7012AY high side driver
 * @link https://www.st.com/resource/en/datasheet/vnhd7012ay.pdf
 */
class VNHD7012 : public IDriver {

  public:
    struct pins_t {
        GPIO_Type *base;
        uint32_t in_a;
        uint32_t in_b;
    };

    VNHD7012(const pwm_pin_t &pwm_pin, const pins_t &pins)
        : pwm_pin(pwm_pin), pins(pins) {}

    void init() override;
    void setDirection(bool direction) override;
    void setPWM(uint8_t duty_cycle) override;

  private:
    const pins_t &pins;
    const pwm_pin_t &pwm_pin;
};
