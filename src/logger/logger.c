#include "board.h"
#include "protocol/protocol.h"
#include "platform/platform.h"
#include "logger.h"
#include "FreeRTOS.h"
#include "queue.h"
#include <stdarg.h>

QueueHandle_t logQueue;

static void log_task(void *pvParameters) {
    int res;
    char log_buffer[LOG_CACHE_SIZE];

    vTaskDelay(pdMS_TO_TICKS(1000)); // Allow other tasks to initialize
    while (1) {
        if (xQueueReceive(logQueue, log_buffer, portMAX_DELAY) == pdTRUE) {
            while (res = send_log(log_buffer) < 0) {
                // If send_log fails, wait for a while before retrying
                vTaskDelay(pdMS_TO_TICKS(100));
            }
        }
    }
}

void init_logger(void) {
    logQueue = xQueueCreate(LOG_CACHE_ITEMS, sizeof(char) * LOG_CACHE_SIZE);
    if (logQueue == NULL) {
        return; // Failed to create the queue
    }

    xTaskCreate(log_task, "logger", 2048U, NULL, configMAX_PRIORITIES - 8, NULL);
}

void log_formatted(const char *format, ...) {
    static int counter = 0;
    char buffer[256];

    if (logQueue == NULL) {
        return; // Logger not initialized
    }

    counter++;
    int ret = snprintf(buffer, sizeof(buffer), "[%08d] ", counter);

    va_list args;
    va_start(args, format);
    vsnprintf(buffer +ret, sizeof(buffer) - ret, format, args);
    va_end(args);

    if (uxQueueSpacesAvailable(logQueue) != 0) {
        xQueueSendToBack(logQueue, buffer, portMAX_DELAY);
    }
}

int benchmark_start(void) {
    return get_time();
}

void benchmark_elapsed(const char *label, int start_time) {
    int time_ms = get_time() - start_time;
    log_formatted("%s took %d ms", label, time_ms);
}

void time_from_boot(const char *label) {
    log_formatted("%s: %d ms", label, get_time());
}