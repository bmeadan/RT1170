#include "VNHD7012.h"
#include "fsl_gpio.h"

void VNHD7012::init() {
    gpio_pin_config_t gpio_config = {kGPIO_DigitalOutput, 0, kGPIO_NoIntmode};
    GPIO_PinInit(pins.base, pins.in_a, &gpio_config); // INA - J10.8
    GPIO_PinInit(pins.base, pins.in_b, &gpio_config); // INB - J10.10
}

void VNHD7012::setDirection(bool direction) {
    GPIO_WritePinOutput(pins.base, pins.in_a, direction ? 1U : 0U);
    GPIO_WritePinOutput(pins.base, pins.in_b, direction ? 0U : 1U);
}

void VNHD7012::setPWM(uint8_t duty_cycle) {
    set_pwm(&pwm_pin, duty_cycle);
}
