#include "flash.h"
#include "FreeRTOS.h"
#include "logger/logger.h"
#include "platform/platform.h"
#include "task.h"

static volatile bool flash_app_config_saving = false;
SDK_ALIGN(static uint8_t config_buff[FLASH_PAGE_SIZE], 4);

static uint32_t calculate_application_checksum(void) {
    return CalculateApplicationChecksum((void *)&appConfigData, sizeof(AppConfigData_t)-sizeof(appConfigData.chksum));
}

static status_t flash_write_app_config(void) {
    status_t status;
    
    // Erase the sector where the configuration data is stored
    status = flexspi_nor_flash_erase_sector(FLASH_FLEXSPI, APP_CONFIG_S - FLASH_FLEXSPI_AMBA_BASE);
    if (status != kStatus_Success)
    {
        LOGF("Erase sector failure !");
        return status;
    }

    // Prepare the configuration data buffer
    appConfigData.chksum = calculate_application_checksum();
    memcpy((void *)config_buff, (void *)&appConfigData, sizeof(AppConfigData_t));
    
    // Flash the configuration data to the external flash
    status = flexspi_nor_flash_program(FLASH_FLEXSPI, APP_CONFIG_S - FLASH_FLEXSPI_AMBA_BASE, (void *)config_buff, sizeof(AppConfigData_t));
    if (status != kStatus_Success)
    {
        LOGF("Program failure !");
        return status;
    }

    return status;    
}

static void task_save_app_config(void *pvParameters) {
    platform_state = PLATFORM_STATE_FLASHING; // Set the platform state to flashing
    vTaskDelay(pdMS_TO_TICKS(100)); // Allow time for USB streams to stop
    flash_write_app_config();
    flash_app_config_saving = false;
    platform_reboot(0); // Reboot the platform to apply changes
    vTaskDelete(NULL);
}

void flash_set_calibration(uint8_t index, uint16_t value) {
    if (index < 1 || index > CALIBRATION_SIZE) return;
    appConfigData.calibration[index - 1] = value;
}

void flash_get_calibration(uint8_t index, uint16_t *value) {
    if (index < 1 || index > CALIBRATION_SIZE) {
        *value = 0;
    } else {
        *value = appConfigData.calibration[index - 1];
    }
}

void flash_save_config(void) {
    if (flash_app_config_saving) {
        LOGF("Configuration saving is already in progress!");
        return;
    }

    flash_app_config_saving = true;
    if (xTaskCreate(task_save_app_config, "flash_save_app_config", 1024U, NULL, configMAX_PRIORITIES - 5, NULL) != pdPASS) {
        flash_app_config_saving = false;
        LOGF("Failed to create task for saving configuration!");
    }
}

void flash_load_config(void) {
    // Read the configuration data from the external flash
    status_t status = flexspi_nor_flash_read(FLASH_FLEXSPI, APP_CONFIG_S - FLASH_FLEXSPI_AMBA_BASE, (void *)config_buff, sizeof(AppConfigData_t));
    memcpy((void *)&appConfigData, (void *)config_buff, sizeof(AppConfigData_t));

    uint32_t checksum = calculate_application_checksum();

    // Validate the checksum
    if (appConfigData.chksum != checksum) {
        LOGF("Invalid configuration checksum!");
        for (int i = 0; i < CALIBRATION_SIZE; i++) {
            appConfigData.calibration[i] = 0; // Reset calibration values
        }
        appConfigData.flipped_drive = 0;
        flash_save_config();
    }
}