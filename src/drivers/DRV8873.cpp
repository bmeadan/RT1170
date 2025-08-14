#include "DRV8873.h"
#include "fsl_gpio.h"

void DRV8873::init() {
    gpio_pin_config_t gpio_config_ph = {kGPIO_DigitalOutput, 0, kGPIO_NoIntmode};
    GPIO_PinInit(direction_pin.base, direction_pin.ph, &gpio_config_ph);

    if (direction_pin.sdo_mode != 0) {
        gpio_pin_config_t gpio_config_sdo = {kGPIO_DigitalOutput, 0, kGPIO_NoIntmode};
        GPIO_PinInit(direction_pin.base, direction_pin.sdo_mode, &gpio_config_sdo);
    }

    if (direction_pin.disable != 0) {
        gpio_pin_config_t gpio_config_disable = {kGPIO_DigitalOutput, 0, kGPIO_NoIntmode};
        GPIO_PinInit(direction_pin.base, direction_pin.disable, &gpio_config_disable);
    }

    if (direction_pin.sleep_n != 0) {
        gpio_pin_config_t gpio_config_sleep = {kGPIO_DigitalOutput, 0, kGPIO_NoIntmode};
        GPIO_PinInit(direction_pin.base, direction_pin.sleep_n, &gpio_config_sleep);
    }

    SDK_DelayAtLeastUs(10000, CLOCK_GetFreq(kCLOCK_CpuClk));

    if (direction_pin.sleep_n != 0) {
        gpio_pin_config_t gpio_config_sleep = {kGPIO_DigitalOutput, 1, kGPIO_NoIntmode};
        GPIO_PinInit(direction_pin.base, direction_pin.sleep_n, &gpio_config_sleep);
    }
}

void DRV8873::setDirection(bool direction) {
    GPIO_PinWrite(direction_pin.base, direction_pin.ph, direction ? 1U : 0U);
}

void DRV8873::setPWM(uint8_t duty_cycle) {
    set_pwm(&pwm_pin, duty_cycle);
}