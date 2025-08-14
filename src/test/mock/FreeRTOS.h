#pragma once
#include <stdint.h>
typedef int BaseType_t;
typedef uint32_t TickType_t;
typedef void* TaskHandle_t;
#define pdTRUE 1
#define pdFALSE 0
#define tskIDLE_PRIORITY 0
#define configMAX_PRIORITIES 5
#define pdMS_TO_TICKS(ms) ((TickType_t)(ms))
extern "C" {
void vTaskDelay(TickType_t ticks);
BaseType_t xTaskCreate(void (*task)(void*), const char*, uint16_t, void*, uint32_t, TaskHandle_t*);
uint32_t ulTaskNotifyTake(BaseType_t clearOnExit, TickType_t ticksToWait);
void vTaskDelete(TaskHandle_t);
void vTaskNotifyGiveFromISR(TaskHandle_t, BaseType_t*);
void xTaskNotifyGive(TaskHandle_t);
void portYIELD_FROM_ISR(BaseType_t);
}
