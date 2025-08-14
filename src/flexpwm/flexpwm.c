#include "flexpwm/flexpwm.h"
#include "logger/logger.h"
#include "fsl_pwm.h"
#include "fsl_xbara.h"

#define PWM_SRC_CLK_FREQ          CLOCK_GetRootClockFreq(kCLOCK_Root_Bus)
#define APP_DEFAULT_PWM_FREQUENCY (1000UL)

static void PWM_DRV_Init3PhPwm(void) {
    uint16_t deadTimeVal;
    pwm_signal_param_t pwmSignal[2];
    uint32_t pwmSourceClockInHz = PWM_SRC_CLK_FREQ;
    uint32_t pwmFrequencyInHz   = APP_DEFAULT_PWM_FREQUENCY;

    /* Set deadtime count, we set this to about 650ns */
    deadTimeVal = ((uint64_t)pwmSourceClockInHz * 650) / 1000000000;

    pwmSignal[0].pwmChannel       = kPWM_PwmA;
    pwmSignal[0].level            = kPWM_HighTrue;
    pwmSignal[0].dutyCyclePercent = 0;
    pwmSignal[0].deadtimeValue    = deadTimeVal;
    pwmSignal[0].faultState       = kPWM_PwmFaultState0;
    pwmSignal[0].pwmchannelenable = true;

    pwmSignal[1].pwmChannel       = kPWM_PwmB;
    pwmSignal[1].level            = kPWM_HighTrue;
    pwmSignal[1].dutyCyclePercent = 0;
    pwmSignal[1].deadtimeValue    = deadTimeVal;
    pwmSignal[1].faultState       = kPWM_PwmFaultState0;
    pwmSignal[1].pwmchannelenable = true;

    PWM_SetupPwm(PWM1, kPWM_Module_0, pwmSignal, 2, kPWM_SignedCenterAligned, pwmFrequencyInHz, pwmSourceClockInHz);
    PWM_SetupPwm(PWM1, kPWM_Module_1, pwmSignal, 2, kPWM_SignedCenterAligned, pwmFrequencyInHz, pwmSourceClockInHz);
    PWM_SetupPwm(PWM1, kPWM_Module_2, pwmSignal, 2, kPWM_SignedCenterAligned, pwmFrequencyInHz, pwmSourceClockInHz);
    PWM_SetupPwm(PWM2, kPWM_Module_0, pwmSignal, 2, kPWM_SignedCenterAligned, pwmFrequencyInHz, pwmSourceClockInHz);
    PWM_SetupPwm(PWM2, kPWM_Module_1, pwmSignal, 2, kPWM_SignedCenterAligned, pwmFrequencyInHz, pwmSourceClockInHz);
    PWM_SetupPwm(PWM2, kPWM_Module_2, pwmSignal, 2, kPWM_SignedCenterAligned, pwmFrequencyInHz, pwmSourceClockInHz);
}

int init_pwms(void) {
    pwm_config_t pwmConfig;
    pwm_fault_param_t faultConfig;

    XBARA_Init(XBARA1);
    XBARA_SetSignalsConnection(XBARA1, kXBARA1_InputLogicHigh, kXBARA1_OutputFlexpwm1Fault0);
    XBARA_SetSignalsConnection(XBARA1, kXBARA1_InputLogicHigh, kXBARA1_OutputFlexpwm1Fault1);
    XBARA_SetSignalsConnection(XBARA1, kXBARA1_InputLogicHigh, kXBARA1_OutputFlexpwm2Fault0);
    XBARA_SetSignalsConnection(XBARA1, kXBARA1_InputLogicHigh, kXBARA1_OutputFlexpwm2Fault1);
    XBARA_SetSignalsConnection(XBARA1, kXBARA1_InputLogicHigh, kXBARA1_OutputFlexpwm1234Fault2);
    XBARA_SetSignalsConnection(XBARA1, kXBARA1_InputLogicHigh, kXBARA1_OutputFlexpwm1234Fault3);

    PWM_GetDefaultConfig(&pwmConfig);
    PWM_FaultDefaultConfig(&faultConfig);

    pwmConfig.reloadLogic     = kPWM_ReloadImmediate;
    pwmConfig.pairOperation   = kPWM_Independent;
    pwmConfig.enableDebugMode = true;

    const uint16_t faults_disable = kPWM_FaultDisable_0 | kPWM_FaultDisable_1 | kPWM_FaultDisable_2 | kPWM_FaultDisable_3;

#define PWM_MODULE_INIT(base, module)                                                       \
    if (PWM_Init(base, module, &pwmConfig) != kStatus_Success)                              \
        LOGF("PWM" #base " Module " #module " Init failed");                            \
    PWM_SetupFaultDisableMap(base, module, kPWM_PwmA, kPWM_faultchannel_0, faults_disable); \
    PWM_SetupFaultDisableMap(base, module, kPWM_PwmB, kPWM_faultchannel_0, faults_disable); \
    PWM_SetupFaultDisableMap(base, module, kPWM_PwmA, kPWM_faultchannel_1, faults_disable); \
    PWM_SetupFaultDisableMap(base, module, kPWM_PwmB, kPWM_faultchannel_1, faults_disable);

#define PWM_INIT(base)                                 \
    PWM_SetupFaults(base, kPWM_Fault_0, &faultConfig); \
    PWM_SetupFaults(base, kPWM_Fault_1, &faultConfig); \
    PWM_SetupFaults(base, kPWM_Fault_2, &faultConfig); \
    PWM_SetupFaults(base, kPWM_Fault_3, &faultConfig); \
    PWM_MODULE_INIT(base, kPWM_Module_0);              \
    PWM_MODULE_INIT(base, kPWM_Module_1);              \
    PWM_MODULE_INIT(base, kPWM_Module_2);

    PWM_INIT(PWM1);
    PWM_INIT(PWM2);

    PWM_DRV_Init3PhPwm();

#define PWM_SET_LDOK(base, module)      \
    PWM_SetPwmLdok(base, module, true); \
    PWM_StartTimer(base, module);

    PWM_SET_LDOK(PWM1, kPWM_Control_Module_0);
    PWM_SET_LDOK(PWM1, kPWM_Control_Module_1);
    PWM_SET_LDOK(PWM1, kPWM_Control_Module_2);
    PWM_SET_LDOK(PWM2, kPWM_Control_Module_0);
    PWM_SET_LDOK(PWM2, kPWM_Control_Module_1);
    PWM_SET_LDOK(PWM2, kPWM_Control_Module_2);
}

void set_pwm(const pwm_pin_t *pwm_pin, uint8_t duty_cycle) {
    PWM_UpdatePwmDutycycle(pwm_pin->base, pwm_pin->submodule, pwm_pin->channel, kPWM_SignedCenterAligned, duty_cycle);
    PWM_SetPwmLdok(pwm_pin->base, pwm_pin->module_control, true);
}
