#include "flash.h"
#include "network/network.h"
#include "platform/platform.h"
#include "FreeRTOS.h"
#include "semphr.h"
#include <stdint.h>

static volatile bool is_firmware_task_running = false;

static volatile uint32_t fw_image_address = 0;
static volatile uint32_t fw_image_size = 0;
static volatile uint32_t fw_image_checksum = 0;
static volatile uint32_t fw_image_index = Image_A;

static fw_frame_t fw_frame;
static BootConfigData_t bootConfigData;
static uint8_t boot_buff[FLASH_PAGE_SIZE];

static void flash_firmware_start(uint32_t image_size) {
    error_code_t err = ERROR_NONE;
    if (fw_image_address == 0) {
        err = ERROR_FLASH_INTERNAL_ERROR;
    } else {
        fw_image_size = image_size;
        fw_image_checksum = 0;

        if (platform_state == PLATFORM_STATE_SHUTDOWN) {
            err = ERROR_FLASH_INTERNAL_ERROR;
        } else {
            platform_state = PLATFORM_STATE_FLASHING; // Set the platform state to flashing
            vTaskDelay(pdMS_TO_TICKS(100)); // Allow time for USB streams to stop
        }
    }

    send_acknowledgment(err);
}

static void flash_firmware_packet(void *pvParameters) {
    status_t status;
    error_code_t err = ERROR_NONE;
    
    fw_frame_t *fw_frame = (fw_frame_t *)pvParameters;
    uint32_t address = fw_frame->address + fw_image_address - FLASH_FLEXSPI_AMBA_BASE;

    if (fw_frame->address >= fw_image_size) {
        err = ERROR_FLASH_INVALID_PACKET_ADDRESS;
    }

    if (err == ERROR_NONE && fw_frame->length > FLASH_PAGE_SIZE){
        err = ERROR_FLASH_INVALID_PACKET_LENGTH;
    }

    if (err == ERROR_NONE && fw_frame->address % SECTOR_SIZE == 0) {
        status = flexspi_nor_flash_erase_sector(FLASH_FLEXSPI, address);
        if (status != kStatus_Success) {
            err = ERROR_FLASH_ERASE_FAILURE;
        }
    }

    if (err == ERROR_NONE) {
        // Write the firmware frame to flash
        status = flexspi_nor_flash_program(FLASH_FLEXSPI, address, (void *)fw_frame->payload, fw_frame->length);
        if (status != kStatus_Success) {
            err = ERROR_FLASH_WRITE_FAILURE;
        }
    }

    if (err == ERROR_NONE) {
        status = flexspi_nor_flash_read(FLASH_FLEXSPI, address, (void *)boot_buff, fw_frame->length);
        if (status != kStatus_Success) {
            err = ERROR_FLASH_READ_FAILURE;
        }
    }

    if (err == ERROR_NONE) {
        int cmp_result = memcmp(boot_buff, fw_frame->payload, fw_frame->length);
        if (cmp_result != 0) {
            err = ERROR_FLASH_COMPARE_FAILURE;
        }
    }

    if (err == ERROR_NONE) {
        // Update the checksum
        for (uint16_t i = 0; i < fw_frame->length; i++) {
            fw_image_checksum += fw_frame->payload[i];
        }
    }

    is_firmware_task_running = false;
    send_acknowledgment(err);
    vTaskDelete(NULL);
}

static void flash_firmware_end(void *pvParameters) {
    status_t status;
    error_code_t err = ERROR_NONE;
    uint32_t checksum = (uint32_t)pvParameters;

    if (fw_image_checksum != checksum) {
        err = ERROR_FLASH_MISMATCH_CHECKSUM;
    }

    if (err == ERROR_NONE) {
        status = flexspi_nor_flash_erase_sector(FLASH_FLEXSPI, BOOT_CONFIG_S - FLASH_FLEXSPI_AMBA_BASE);
        if (status != kStatus_Success) {
            err = ERROR_FLASH_ERASE_FAILURE;
        }
    }

    if (err == ERROR_NONE) {
        bootConfigData.ImageIdx = fw_image_index;
        bootConfigData.ImageData[fw_image_index].ImageSize = fw_image_size;
        bootConfigData.ImageData[fw_image_index].ImageChecksum = fw_image_checksum;

        memcpy((void *)boot_buff, (void *)&bootConfigData, sizeof(BootConfigData_t));
        status = flexspi_nor_flash_program(FLASH_FLEXSPI, BOOT_CONFIG_S - FLASH_FLEXSPI_AMBA_BASE, (void *)boot_buff, sizeof(BootConfigData_t));
        if (status != kStatus_Success)
        {
            err = ERROR_FLASH_WRITE_FAILURE;
        }
    }

    is_firmware_task_running = false;
    send_acknowledgment(err);
    vTaskDelete(NULL);
}


void flash_firmware_message(api_message_t *msg) {    
    static uint32_t checksum = 0;
    static api_fw_frame_t api_fw_frame;

    if (is_firmware_task_running) {
        send_acknowledgment(ERROR_FLASH_BUSY_TASK);
        return;
    }

    switch (msg->opcode)
    {
    case API_FW_START:
        uint32_t image_size = *((uint32_t*)msg->payload);
        flash_firmware_start(image_size);
        break;
    case API_FW_PACKET: {
        memcpy(&api_fw_frame, msg->payload, 6);
        fw_frame.address = api_fw_frame.address;
        fw_frame.length = api_fw_frame.length;
        memcpy(fw_frame.payload, msg->payload + 6, msg->length - 6);

        is_firmware_task_running = true;
        if (xTaskCreate(flash_firmware_packet, "flash_firmware_packet", 1024, (void *)&fw_frame, configMAX_PRIORITIES - 7, NULL) != pdPASS) {
            is_firmware_task_running = false;
            send_acknowledgment(ERROR_FLASH_CREATE_TASK);
        }
        break;
    }
    case API_FW_END:
        checksum = *((uint32_t*)msg->payload);
        is_firmware_task_running = true;
        if (xTaskCreate(flash_firmware_end, "flash_firmware_end", 1024, (void *)checksum, configMAX_PRIORITIES - 7, NULL) != pdPASS) {
            is_firmware_task_running = false;
            send_acknowledgment(ERROR_FLASH_CREATE_TASK);
        }
        break;
    }
}

// static void task_log(void *pvParameters) {
//     while(1) {
//         char log_message[128];
//         snprintf(log_message, sizeof(log_message), "ImageIdx: %d, ImageSize: %d, Checksum: %d, fw_image_index: %d, fw_image_address: 0x%08X",
//                  bootConfigData.ImageIdx,
//                  bootConfigData.ImageData[bootConfigData.ImageIdx].ImageSize,
//                  bootConfigData.ImageData[bootConfigData.ImageIdx].ImageChecksum,
//                  fw_image_index,
//                  fw_image_address);
//         LOGF(log_message);
//         vTaskDelay(pdMS_TO_TICKS(1000)); // Log every second
//     }
//     vTaskDelete(NULL);
// }

void flash_load_firmware_config(void) {
    flexspi_nor_flash_read(FLASH_FLEXSPI, BOOT_CONFIG_S - FLASH_FLEXSPI_AMBA_BASE, (void *)boot_buff, sizeof(BootConfigData_t));
    memcpy((void *)&bootConfigData, (void *)boot_buff, sizeof(BootConfigData_t));

    if (bootConfigData.ImageIdx == Image_A) {
        fw_image_index = Image_B;
    } else if (bootConfigData.ImageIdx == Image_B) {
        fw_image_index = Image_A;
    } else { // Golden
        fw_image_index = Image_A; // Default to Image_A for Golden
    }
    
    // Set the firmware image address based on the current image index
    fw_image_address = GetImageBaseAddress(fw_image_index);
}