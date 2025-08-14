#include "flash.h"
#include "logger/logger.h"

void init_flash(void) {
    status_t status = init_flexspi();
    if (status != kStatus_Success) {
        LOGF("FlexSPI initialization failed!");
        return;
    }

    flash_load_config();
    flash_load_firmware_config();
}