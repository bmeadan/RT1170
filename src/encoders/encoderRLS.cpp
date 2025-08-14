#include "encoderRLS.h"
#include "flash/flash.h"
#include "FreeRTOS.h"
#include "clock_config.h"
#include "pin_mux.h"
#include "task.h"
#include "board.h"
#include "logger/logger.h"


#define SSI_CLOCK_HIGH()    GPIO_PinWrite(GPIO7, 20U, 1U);
#define SSI_CLOCK_LOW()     GPIO_PinWrite(GPIO7, 20U, 0U);
#define SSI_DATA_READ()     GPIO_PinRead(GPIO7, 17U)
#define BIT_COUNT 12


static volatile uint16_t rawEncoderAngle;
static volatile uint16_t offset = 0;

void readEncoderTask(void *pvParameters)
{
    
    while(1){
        unsigned long data = 0; 

        SSI_CLOCK_LOW(); 
        SDK_DelayAtLeastUs(1, CLOCK_GetFreq(kCLOCK_CpuClk));
        SSI_CLOCK_HIGH(); 
        SDK_DelayAtLeastUs(1, CLOCK_GetFreq(kCLOCK_CpuClk));
        for (int i = 0; i < BIT_COUNT; i++)
        {
        
            SSI_CLOCK_LOW(); 
            SDK_DelayAtLeastUs(1, CLOCK_GetFreq(kCLOCK_CpuClk));
            data |= SSI_DATA_READ();

            SSI_CLOCK_HIGH(); 
            SDK_DelayAtLeastUs(1, CLOCK_GetFreq(kCLOCK_CpuClk));
            if(i < (BIT_COUNT-1))
            {
                data <<= 1;
            } 
        }
        rawEncoderAngle = (uint16_t) data;
        vTaskDelay(pdMS_TO_TICKS(SAMPLING_PERIOD));
    }
}

float getRLSEncoderAngle(void)
{
    float angle12bit = (rawEncoderAngle - offset) % 4095;
    float angle360 = (angle12bit  * 360) / 4095.0;
    return (angle360 > 180.0f) ? (360.0f - angle360) : -angle360;
}

void calibrateRLSEncoder(void) {
    offset = rawEncoderAngle;
    flash_set_calibration(1, offset);
}

void init_EncoderRLS(void)
{ 
    BOARD_InitEncoderPins();

    uint16_t saved_offset;
    flash_get_calibration(1, &saved_offset);
    offset = saved_offset;
    xTaskCreate(readEncoderTask, "read_encoder", 1024U, NULL, configMAX_PRIORITIES - 2, NULL);
}