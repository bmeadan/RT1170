#include "flexspi.h"

status_t init_flexspi(void) {
    static bool initialized = false;
    if (initialized) {
        return kStatus_Success; // Already initialized
    }

    uint8_t vendorID = 0;
    status_t status;

    flexspi_nor_flash_init(FLASH_FLEXSPI);
    status = flexspi_nor_get_vendor_id(FLASH_FLEXSPI, &vendorID);
    if (status != kStatus_Success) {
        return status;
    }

    status = flexspi_nor_enable_quad_mode(FLASH_FLEXSPI);
    if (status != kStatus_Success) {
        return status;
    }

    initialized = true;
    return kStatus_Success;
}