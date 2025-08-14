#ifndef __FLASH_H__
#define __FLASH_H__

#include "flexspi/flexspi.h"
#include "protocol/protocol.h"
#include "ExternalFlashMap.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

AppConfigData_t appConfigData;

typedef struct fw_frame {
    uint32_t address;
    uint16_t length;    
    uint8_t payload[FLASH_PAGE_SIZE];
} fw_frame_t;

void flash_get_calibration(uint8_t index, uint16_t *value);
void flash_set_calibration(uint8_t index, uint16_t value);
void flash_save_config(void);
void flash_load_config(void);
void flash_firmware_message(api_message_t *msg);
void flash_load_firmware_config(void);
void init_flash(void);

#ifdef __cplusplus
}
#endif

#endif /* __FLASH_H__ */
