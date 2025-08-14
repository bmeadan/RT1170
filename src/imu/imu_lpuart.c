#include "imu_lpuart.h"
#include "FreeRTOS.h"
#include "clock_config.h"
#include "fsl_clock.h"
#include "fsl_gpio.h"
#include "fsl_lpuart.h"
#include "pin_gpio.h"
#include "logger/logger.h"
#include <string.h>

#define IMU_LPUART_BASE            LPUART6
#define IMU_LPUART_IRQn       LPUART6_IRQn
#define IMU_LPUART_IRQHandler LPUART6_IRQHandler
#define IMU_LPUART_CLOCK_FREQ CLOCK_GetRootClockFreq(kCLOCK_Root_Lpuart6)

#define BAUDRATE 115200U
#define RING_BUFFER_SIZE 64

static uint8_t ringBuffer[RING_BUFFER_SIZE];
static volatile uint16_t txIndex; /* Index of the data to send out. */
static volatile uint16_t rxIndex; /* Index of the memory to save new arrived data. */

// Ring buffer stores incoming bytes
void IMU_LPUART_IRQHandler(void) {
    uint8_t data;

    /* If new data arrived. */
    if (kLPUART_RxDataRegFullFlag & LPUART_GetStatusFlags(IMU_LPUART_BASE)) {
        data = LPUART_ReadByte(IMU_LPUART_BASE);

        /* If ring buffer is not full, add data to ring buffer. */
        ringBuffer[rxIndex] = data;
        rxIndex++;
        rxIndex %= RING_BUFFER_SIZE;

        // LOGF("ISR: byte received: %02X", data);
    }
    SDK_ISR_EXIT_BARRIER;
}

void init_imu_lpuart(void) {
    int status;
    lpuart_config_t config;

    LPUART_GetDefaultConfig(&config);
    config.baudRate_Bps = BAUDRATE;
    config.enableTx     = true;
    config.enableRx     = true;

    status = LPUART_Init(IMU_LPUART_BASE, &config, IMU_LPUART_CLOCK_FREQ);
    if (status != kStatus_Success) {
        LOGF("LPUART init failed for IMU LPUART: %d", status);
        return;
    }

    LPUART_EnableInterrupts(IMU_LPUART_BASE, kLPUART_RxDataRegFullInterruptEnable);

    status = EnableIRQ(IMU_LPUART_IRQn);
    if (status != kStatus_Success) {
        LOGF("Failed to enable IRQ for IMU LPUART: %d", status);
        return;
    }
}

static int get_buffer_size() {
    if (rxIndex > txIndex) {
        return rxIndex - txIndex;
    } else if (txIndex > rxIndex) {
        return rxIndex + RING_BUFFER_SIZE - txIndex;
    }

    return 0;
}

bool UART_ReadFromRingBuffer(uint8_t *data, uint16_t len) {
    int left = len;
    int size = get_buffer_size();
    int curr_idx = 0;

    if (size < len) {
        return false;
    }

    if (rxIndex > txIndex) {
        int read_num = MIN(len, size);
        memcpy(data, ringBuffer + txIndex, read_num);
        txIndex += read_num;
        txIndex %= RING_BUFFER_SIZE;
        return true;
    } else if (txIndex > rxIndex) {
        int read_num = MIN(RING_BUFFER_SIZE - txIndex, left);

        memcpy(data, ringBuffer + txIndex, read_num);

        left -= read_num;
        curr_idx += read_num;
        txIndex += read_num;
        txIndex %= RING_BUFFER_SIZE;

        if (left > 0) {
            read_num = MIN(left, rxIndex);

            memcpy(data + curr_idx, ringBuffer, read_num);
            txIndex += read_num;
            txIndex %= RING_BUFFER_SIZE;
            return true;
        } else {
            return true;
        }
    }
    return false;
}