#include "FreeRTOS.h"
#include "project.h"
#include "board.h"
#include "pin_mux.h"
#include "clock_config.h"

#include "audio/audio.h"
#include "lights/lights.h"
#include "lpadc/lpadc.h"
#include "motors/motors.h"
#include "network/network.h"
#include "protocol/protocol.h"
#include "platform/platform.h"
#include "flash/flash.h"
#include "tests/tests.h"
#include "usb/usb_hub.h"
#include "imu/imu.h"
#include "logger/logger.h"
#include "encoders/encoderRLS.h"

#if 0
// Only used for debugging purposes
int main(void) {
    BOARD_ConfigMPU();
    return 0;
}
#else
int main(void) {
    BOARD_ConfigMPU();
    BOARD_InitCommonPins();
    BOARD_BootClockRUN();

    init_logger();
    init_flash();
    init_lights();
    init_adc();
   
#ifdef ENABLE_ENCODER_RLS
    init_EncoderRLS();
#endif

#ifdef ENABLE_IMU
   init_imu();
#endif

#ifdef ENABLE_MOTORS
    init_motors();
#endif

#ifdef ENABLE_USB_HUB
    init_usb_hub();
#endif

#ifdef ENABLE_NETWORK
    init_network();
#endif

#ifdef ENABLE_AUDIO
    init_audio();
#endif

    // test_lights();
    // test_motors();
    // test_checksum();

    platform_state = PLATFORM_STATE_RUNNING;
    vTaskStartScheduler();
    return 0;
}
#endif


static void error_handler(void) {
    taskDISABLE_INTERRUPTS();
    while (1) {
        set_rgb_led(LED_RED);
        SDK_DelayAtLeastUs(200000, SystemCoreClock);
        set_rgb_led(LED_OFF);
        SDK_DelayAtLeastUs(200000, SystemCoreClock);
    }
}

// void vApplicationIdleHook(void) {
// LOGF("vApplicationIdleHook");
// }

// void vApplicationTickHook(void) {
//     LOGF("vApplicationTickHook");
// }

// void vApplicationDaemonTaskStartupHook(void) {
// LOGF("vApplicationDaemonTaskStartupHook");
// }

void vApplicationStackOverflowHook(TaskHandle_t xTask, char *pcTaskName) {
    error_handler();
}

void vApplicationMallocFailedHook(void) {
    error_handler();
}

void HardFault_Handler(void) {
    error_handler();
}
