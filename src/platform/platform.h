#ifndef PLATFORM_H
#define PLATFORM_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum platform_state {
    PLATFORM_STATE_INIT = 0,
    PLATFORM_STATE_RUNNING,
    PLATFORM_STATE_FLASHING,
    PLATFORM_STATE_SHUTDOWN
} platform_state_t;

extern volatile platform_state_t platform_state;

uint32_t get_time();
uint8_t get_chip_id();

void platform_reboot(uint32_t timeout_ms);

#ifdef __cplusplus
}
#endif

#endif