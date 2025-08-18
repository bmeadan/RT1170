#include <cstdio>
#include "FreeRTOS.h"

#ifdef HOST_SIM
  #include "motors/RSBL8512.h"     // redirected to mock_all.h
#else
  #include "../motors/RSBL8512.h"
#endif

#include "../controllers/FlipController.h"

int main() {
  std::puts("[SIM] controller loop…");

  RSBL8512 yaw(0), pitch(1);
  FlipController fc(yaw, pitch);

  // If FlipController needs enabling, uncomment:
  // fc.setEnabled(true);

  for (int i = 0; i < 1000; ++i) {
    fc.loop();
    vTaskDelay(pdMS_TO_TICKS(10));
  }

  std::puts("[SIM] done.");
  return 0;
}
