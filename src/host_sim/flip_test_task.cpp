// From src/test/ -> up to src/
#include "../controllers/FlipController.h"
#ifdef HOST_SIM
  #include "motors/RSBL8512.h"
#else
  #include "../motors/RSBL8512.h"
#endif

extern "C" {
#include "FreeRTOS.h"
#include "task.h"
}

// Prototypes from production
void flip_bind(RSBL8512& yaw, RSBL8512& pitch);
void flip_command();

static void flip_autotest_task(void*) {
    vTaskDelay(pdMS_TO_TICKS(1000));  // small boot delay
    flip_command();                   // trigger recovery once
    vTaskDelete(nullptr);
}

// This symbol satisfies the weak reference in flip_autostart.cpp
extern "C" void startFlipTestTask(void) {
    static RSBL8512 yaw(0);
    static RSBL8512 pitch(1);
    flip_bind(yaw, pitch);
    xTaskCreate(flip_autotest_task, "flip_autotest",
                512U, nullptr, tskIDLE_PRIORITY + 1, nullptr);
}
