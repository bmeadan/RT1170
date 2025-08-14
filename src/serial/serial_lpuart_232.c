#include "serial_lpuart_232.h"
#include "FreeRTOS.h"
#include "board.h"
#include "clock_config.h"
#include "fsl_clock.h"
#include "fsl_common.h"
#include "fsl_device_registers.h"
#include "fsl_gpio.h"
#include "fsl_lpuart.h"
#include "pin_gpio.h"
#include "platform/platform.h"
#include "logger/logger.h"
#include "task.h"
#include <stdarg.h>

#define J22_BAUDRATE 115200U

#if J22_LPUART_INSTANCE == 11
#define J22_LPUART_BASE       LPUART11
#define J22_LPUART_IRQn       LPUART11_IRQn
#define J22_LPUART_IRQHandler LPUART11_IRQHandler
#define J22_LPUART_CLOCK_FREQ CLOCK_GetRootClockFreq(kCLOCK_Root_Lpuart11)
#elif J22_LPUART_INSTANCE == 12
#define J22_LPUART_BASE       LPUART12
#define J22_LPUART_IRQn       LPUART12_IRQn
#define J22_LPUART_IRQHandler LPUART12_IRQHandler
#define J22_LPUART_CLOCK_FREQ CLOCK_GetRootClockFreq(kCLOCK_Root_Lpuart12)
#elif J22_LPUART_INSTANCE == 3
#define J22_LPUART_BASE       LPUART3
#define J22_LPUART_IRQn       LPUART3_IRQn
#define J22_LPUART_IRQHandler LPUART3_IRQHandler
#define J22_LPUART_CLOCK_FREQ CLOCK_GetRootClockFreq(kCLOCK_Root_Lpuart3)
#else
#error "Unsupported RSBL LPUART instance"
#endif

#define DEMO_RING_BUFFER_SIZE 256
static uint8_t demoRingBuffer[DEMO_RING_BUFFER_SIZE];
static volatile uint16_t txIndex; /* Index of the data to send out. */
static volatile uint16_t rxIndex; /* Index of the memory to save new arrived data. */

/*******************************************************************************
 * Code
 ******************************************************************************/
void J22_LPUART_IRQHandler(void) {
    uint32_t flags = LPUART_GetStatusFlags(J22_LPUART_BASE);

    if (flags & kLPUART_RxOverrunFlag) {
        
        LPUART_ClearStatusFlags(J22_LPUART_BASE, kLPUART_RxOverrunFlag);
    }

    if (flags & kLPUART_RxDataRegFullFlag) {
        uint8_t data = LPUART_ReadByte(J22_LPUART_BASE);

        demoRingBuffer[rxIndex] = data;
        rxIndex = (rxIndex + 1) % DEMO_RING_BUFFER_SIZE;
    }

    SDK_ISR_EXIT_BARRIER;
}

void init_serial_lpuart_232(void)
{
    int status;
    static bool initialized = false;
    if (initialized) {
        return;
    } else {
        initialized = true;
    }

    lpuart_config_t config;

    // 1. Get default config
    LPUART_GetDefaultConfig(&config);
    config.baudRate_Bps = J22_BAUDRATE;
    config.enableTx = true;
    config.enableRx = true;  // RX not needed
    config.enableRxRTS = false;
    config.enableTxCTS = false;

    // 2. Init LPUART with proper clock (usually IPG)
    status = LPUART_Init(J22_LPUART_BASE, &config, J22_LPUART_CLOCK_FREQ);
    if (LPUART_Init(J22_LPUART_BASE, &config, J22_LPUART_CLOCK_FREQ) != kStatus_Success) {
        LOGF("LPUART init failed for J22 LPUART: %d", status);
        return;
    }

    // 3. Enable RX interrupt
    LPUART_EnableInterrupts(J22_LPUART_BASE, kLPUART_RxDataRegFullInterruptEnable);

    // 4. Enable interrupt in NVIC
    status = EnableIRQ(J22_LPUART_IRQn);
    if (status != kStatus_Success) {
        LOGF("Failed to enable IRQ for J22 LPUART: %d", status);
        return;
    }
}

int serial_lpuart_send_bytes_232(const uint8_t *data, size_t length) {
    /* Send data only when LPUART TX register is empty*/
    GPIO_PinWrite(J22_EN_GPIO, J22_EN_PIN, 1);
    for (int i = 0; i < length; i++) {
        while (!(kLPUART_TxDataRegEmptyFlag & LPUART_GetStatusFlags(J22_LPUART_BASE))) {
            // vTaskDelay(pdMS_TO_TICKS(1));
            SDK_DelayAtLeastUs(10, CLOCK_GetFreq(kCLOCK_CpuClk));
        }
        LPUART_WriteByte(J22_LPUART_BASE, data[i]);
    }
    while (!(kLPUART_TransmissionCompleteFlag & LPUART_GetStatusFlags(J22_LPUART_BASE))) {
        // vTaskDelay(pdMS_TO_TICKS(1));
        SDK_DelayAtLeastUs(10, CLOCK_GetFreq(kCLOCK_CpuClk));
    }

    // vTaskDelay(pdMS_TO_TICKS(1));
    SDK_DelayAtLeastUs(120, CLOCK_GetFreq(kCLOCK_CpuClk));
    GPIO_PinWrite(J22_EN_GPIO, J22_EN_PIN, 0);
    return length;
}

static int get_buffer_size() {
    if (rxIndex > txIndex) {
        return rxIndex - txIndex;
    } else if (txIndex > rxIndex) {
        return rxIndex + DEMO_RING_BUFFER_SIZE - txIndex;
    }

    return 0;
}

int serial_lpuart_recv_bytes_232(uint8_t *data, uint16_t len) {

    int left         = len;
    int size         = 0;
    int curr_idx     = 0;
    uint32_t timeout = len;

    size = get_buffer_size();

    if (size < len) {

        return -1;
    }

    if (rxIndex > txIndex) {

        int read_num = MIN(len, size);
        memcpy(data, demoRingBuffer + txIndex, read_num);
        txIndex += read_num;
        txIndex %= DEMO_RING_BUFFER_SIZE;
        return read_num;

    } else if (txIndex > rxIndex) {

        int read_num = MIN(DEMO_RING_BUFFER_SIZE - txIndex, left);

        memcpy(data, demoRingBuffer + txIndex, read_num);

        left -= read_num;
        curr_idx += read_num;
        txIndex += read_num;
        txIndex %= DEMO_RING_BUFFER_SIZE;

        if (left > 0) {
            read_num = MIN(left, rxIndex);

            memcpy(data + curr_idx, demoRingBuffer, read_num);
            txIndex += read_num;
            txIndex %= DEMO_RING_BUFFER_SIZE;
            return read_num + curr_idx;
        } else {
            return read_num;
        }
    }

    return -1;
}

void serial_lpuart_read_flush_232(void) {
    txIndex = rxIndex;
}

void serial_lpuart_write_flush_232(void) {
    vTaskDelay(pdMS_TO_TICKS(5));
}