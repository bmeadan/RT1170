#include "platform.h"
#include "FreeRTOS.h"
#include "task.h"
#include "fsl_silicon_id.h"

volatile platform_state_t platform_state = PLATFORM_STATE_INIT;

uint32_t get_time() {
    return (xTaskGetTickCount() * portTICK_PERIOD_MS);
}

uint8_t get_chip_id() {
    uint8_t siliconId[SILICONID_MAX_LENGTH];
    uint32_t idLen = sizeof(siliconId);

    status_t result = SILICONID_GetID(&siliconId[0], &idLen);
    if (result != kStatus_Success) {
        return 0x11;
    }

    return siliconId[0];
}

static void task_reboot(void *pvParameters) {
    uint32_t timeout_ms = (uint32_t)pvParameters;
    vTaskDelay(pdMS_TO_TICKS(timeout_ms));
    taskDISABLE_INTERRUPTS();
    NVIC_SystemReset();
    vTaskDelete(NULL);
}

void platform_reboot(uint32_t timeout_ms) {
    platform_state_t state = platform_state;
    if (state == PLATFORM_STATE_SHUTDOWN) {
        // Prevent reboot if already upgrading
        return;
    }

    platform_state = PLATFORM_STATE_SHUTDOWN;
    if (xTaskCreate(task_reboot, "platform_reboot", 1024, (void*)timeout_ms, configMAX_PRIORITIES - 1, NULL) != pdPASS) {
        platform_state = state; // Restore previous state if task creation fails
    }
}
