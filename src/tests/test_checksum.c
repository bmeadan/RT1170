#include "platform/platform.h"
#include "protocol/protocol.h"
#include "logger/logger.h"
#include "tests.h"
#include "task.h"

static void task_test_checksum(void *pvParameters) {
    uint8_t data[PROTOCOL_MAX_MTU] = {0};
    for (int i = 0; i < PROTOCOL_MAX_MTU; i++) {
        data[i] = 100;
    }

    uint8_t slot      = 0;
    uint16_t fragment = 0;
    while (1) {
        send_video_frame(slot, fragment, data, PROTOCOL_MAX_MTU);
        LOGF("Sent message");
        vTaskDelay(pdMS_TO_TICKS(1000));
    }

    vTaskDelete(NULL);
}

void test_checksum(void) {
    xTaskCreate(task_test_checksum, "test_checksum", 2048U, NULL, configMAX_PRIORITIES - 5, NULL);
}