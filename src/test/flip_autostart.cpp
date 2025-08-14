#if !defined(__cplusplus)
#error "This file must be compiled as C++"
#endif

// From src/test/ -> up to src/
#include "../controllers/FlipController.h"
#include "../motors/RSBL8512.h"

extern "C" {
#include "FreeRTOS.h"
#include "task.h"
}

// Provided by production code
void flip_bind(RSBL8512& yaw, RSBL8512& pitch);
void flip_command();

// Optional autostart hook (link a definition in another TU to enable)
extern "C" void startFlipTestTask(void) __attribute__((weak));

static void _flip_autostart_ctor(void) __attribute__((constructor));
static void _flip_autostart_ctor(void) {
    if (startFlipTestTask) {
        startFlipTestTask();
    }
}
