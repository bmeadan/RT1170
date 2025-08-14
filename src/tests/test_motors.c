#include "platform/platform.h"
#include "motors/motors.h"
#include "tests.h"
#include "task.h"
#include <stdbool.h>

static void task_motors_test(void *pvParameters) {
    int8_t speeds[8] = {100, 0, -50, 0};
    static motors_action_t action = {0, 0, 0, 0};
    while (1) {
        for (int j = 0; j < 4; j++) {
            action.drive = speeds[j];
            action.yaw   = speeds[j];
            action.pitch = speeds[j];
            action.auto_neutral_joints = 0;
            set_motors(&action);
            vTaskDelay(pdMS_TO_TICKS(1000));
        }
    }
}

void test_motors(void) {
    xTaskCreate(task_motors_test, "motors_test", 1024U, NULL, configMAX_PRIORITIES - 5, NULL);
}
