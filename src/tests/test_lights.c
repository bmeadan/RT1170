#include "lights/lights.h"
#include "tests.h"
#include "task.h"

static void task_lights_test(void *pvParameters) {
    int state = 0;
    while (1) {
        set_red_led(state % 2 == 0);
        set_green_led(state % 2 == 1);
        set_rgb_led(state % 8);
        set_flashlights(state % 8);

        state++;
        state %= 8;
        vTaskDelay(pdMS_TO_TICKS(200));
    }
    vTaskDelete(NULL);
}

void test_lights(void) {
    xTaskCreate(task_lights_test, "lights_test", 1024U, NULL, configMAX_PRIORITIES - 17, NULL);
}