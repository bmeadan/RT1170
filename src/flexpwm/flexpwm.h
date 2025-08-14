#pragma once
#include "fsl_pwm.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct pwm_pin {
    PWM_Type *base;
    pwm_submodule_t submodule;
    pwm_module_control_t module_control;
    pwm_channels_t channel;
} pwm_pin_t;

int init_pwms(void);
void set_pwm(const pwm_pin_t *pwm_pin, uint8_t duty_cycle);

#ifdef __cplusplus
}
#endif