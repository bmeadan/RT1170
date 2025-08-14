#include "AudioSAI.h"
#include "board.h"
#include "clock_config.h"
#include "logger/logger.h"
#include "pin_mux.h"
#include "FreeRTOS.h"
#include "semphr.h"
#include "protocol/protocol.h"

#define AUDIO_PACKET_SIZE      64
#define AUDIO_BUFFER_SIZE      (AUDIO_PACKET_SIZE * 4)//16384
#define AUDIO_BUFFER_ALIGNMENT 4

AT_NONCACHEABLE_SECTION_ALIGN(uint8_t txBuffer[AUDIO_BUFFER_SIZE], AUDIO_BUFFER_ALIGNMENT);
AT_NONCACHEABLE_SECTION_ALIGN(uint8_t rxBuffer[AUDIO_BUFFER_SIZE], AUDIO_BUFFER_ALIGNMENT);

volatile uint16_t txWriteIndex   = 0; // Tracks where to write in the buffer
volatile uint16_t txReadIndex    = 0; // Tracks where to read

AT_QUICKACCESS_SECTION_DATA(sai_edma_handle_t txHandle);
AT_QUICKACCESS_SECTION_DATA(sai_edma_handle_t rxHandle);
edma_handle_t dmaTxHandle;
edma_handle_t dmaRxHandle;
sai_transceiver_t saiConfig;

volatile bool txFinished = false;
volatile bool rxFinished = false;

static SemaphoreHandle_t txBufferMutex;

const clock_audio_pll_config_t audioPllConfig = {
    .loopDivider = 32,   /* PLL loop divider. Valid range for DIV_SELECT divider value: 27~54. */
    .postDivider = 1,    /* Divider after the PLL, should only be 0, 1, 2, 3, 4, 5 */
    .numerator   = 768,  /* 30 bit numerator of fractional loop divider. */
    .denominator = 1000, /* 30 bit denominator of fractional loop divider */
};

void txCallback(I2S_Type *base, sai_edma_handle_t *handle, status_t status, void *userData) {
    txFinished = true;
}

void rxCallback(I2S_Type *base, sai_edma_handle_t *handle, status_t status, void *userData) {
    rxFinished = true;
}

static void BOARD_EnableSaiMclkOutput(bool enable) {
    if (enable) {
        IOMUXC_GPR->GPR0 |= IOMUXC_GPR_GPR0_SAI1_MCLK_DIR_MASK;
    } else {
        IOMUXC_GPR->GPR0 &= (~IOMUXC_GPR_GPR0_SAI1_MCLK_DIR_MASK);
    }
}

void AudioDataCallback(uint8_t *packet, uint16_t packetSize) {
    xSemaphoreTake(txBufferMutex, portMAX_DELAY);
    for (uint16_t i = 0; i < packetSize; i++) {
        txBuffer[txWriteIndex] = packet[i];
        txWriteIndex              = (txWriteIndex + 1) % AUDIO_BUFFER_SIZE;
    }
    xSemaphoreGive(txBufferMutex);
}

void AudioPlaybackTask(void *pvParameters) {
    SAI_TxSoftwareReset(DEMO_SAI, kSAI_ResetTypeSoftware);
    while (1) {
        uint16_t diff = txWriteIndex - txReadIndex;
        if (diff >= AUDIO_PACKET_SIZE) {
            xSemaphoreTake(txBufferMutex, portMAX_DELAY);
            sai_transfer_t xfer = {0};
            xfer.data           = &txBuffer[txReadIndex];
            xfer.dataSize       = AUDIO_PACKET_SIZE;
            txReadIndex += AUDIO_PACKET_SIZE;
            txReadIndex %= AUDIO_BUFFER_SIZE;
            xSemaphoreGive(txBufferMutex);
            txFinished = false;
        
            SAI_TransferSendEDMA(DEMO_SAI, &txHandle, &xfer);
            while (!txFinished) {
                vTaskDelay(pdMS_TO_TICKS(1));
            }

            // if (txStatus == kStatus_SAI_TxIdle) {

            // }
        } else {
            // No data to play, wait
            vTaskDelay(pdMS_TO_TICKS(10));
        }
    }
    vTaskDelete(NULL);
}

void AudioCaptureTask(void *pvParameters) {
    uint16_t rxIndex = 0;
    SAI_RxSoftwareReset(DEMO_SAI, kSAI_ResetTypeSoftware);
    while (1) {
            sai_transfer_t xfer = {0};
            xfer.data           = &rxBuffer[rxIndex];
            xfer.dataSize       = AUDIO_PACKET_SIZE;
            rxFinished = false;
            SAI_TransferReceiveEDMA(DEMO_SAI, &rxHandle, &xfer);
            while(!rxFinished) {
                vTaskDelay(pdMS_TO_TICKS(1));
            }

            send_audio_frame(0, 0, &rxBuffer[rxIndex], AUDIO_PACKET_SIZE);
            rxIndex += AUDIO_PACKET_SIZE;
            rxIndex %= AUDIO_BUFFER_SIZE;
    }
    vTaskDelete(NULL);
}

void AudioSAI_Init(void) {
    edma_config_t dmaConfig = {0};

    txBufferMutex = xSemaphoreCreateMutex();

    CLOCK_InitAudioPll(&audioPllConfig);

    /*Clock setting for SAI1*/
    CLOCK_SetRootClockMux(kCLOCK_Root_Sai1, 4);
    CLOCK_SetRootClockDiv(kCLOCK_Root_Sai1, 16);

    /*Enable MCLK clock*/
    BOARD_EnableSaiMclkOutput(true);

    DMAMUX_Init(DEMO_DMAMUX);
    DMAMUX_SetSource(DEMO_DMAMUX, DEMO_TX_EDMA_CHANNEL, (uint8_t)DEMO_SAI_TX_SOURCE);
    DMAMUX_EnableChannel(DEMO_DMAMUX, DEMO_TX_EDMA_CHANNEL);
    DMAMUX_SetSource(DEMO_DMAMUX, DEMO_RX_EDMA_CHANNEL, (uint8_t)DEMO_SAI_RX_SOURCE);
    DMAMUX_EnableChannel(DEMO_DMAMUX, DEMO_RX_EDMA_CHANNEL);

    EDMA_GetDefaultConfig(&dmaConfig);
    EDMA_Init(DEMO_DMA, &dmaConfig);
    EDMA_CreateHandle(&dmaTxHandle, DEMO_DMA, DEMO_TX_EDMA_CHANNEL);
    EDMA_CreateHandle(&dmaRxHandle, DEMO_DMA, DEMO_RX_EDMA_CHANNEL);

#if defined(FSL_FEATURE_EDMA_HAS_CHANNEL_MUX) && FSL_FEATURE_EDMA_HAS_CHANNEL_MUX
    EDMA_SetChannelMux(DEMO_DMA, DEMO_TX_EDMA_CHANNEL, DEMO_SAI_TX_EDMA_CHANNEL);
    EDMA_SetChannelMux(DEMO_DMA, DEMO_RX_EDMA_CHANNEL, DEMO_SAI_RX_EDMA_CHANNEL);
#endif

    SAI_Init(DEMO_SAI);
    SAI_TransferTxCreateHandleEDMA(DEMO_SAI, &txHandle, txCallback, NULL, &dmaTxHandle);
    SAI_TransferRxCreateHandleEDMA(DEMO_SAI, &rxHandle, rxCallback, NULL, &dmaRxHandle);

    /* I2S mode configurations */
    SAI_GetClassicI2SConfig(&saiConfig, DEMO_AUDIO_BIT_WIDTH, kSAI_Stereo, 1U << DEMO_SAI_CHANNEL);
    saiConfig.syncMode    = DEMO_SAI_TX_SYNC_MODE;
    saiConfig.masterSlave = DEMO_SAI_MASTER_SLAVE;
    SAI_TransferTxSetConfigEDMA(DEMO_SAI, &txHandle, &saiConfig);
    saiConfig.syncMode = DEMO_SAI_RX_SYNC_MODE;
    SAI_TransferRxSetConfigEDMA(DEMO_SAI, &rxHandle, &saiConfig);

    /* set bit clock divider */
    SAI_TxSetBitClockRate(DEMO_SAI, DEMO_AUDIO_MASTER_CLOCK, DEMO_AUDIO_SAMPLE_RATE, DEMO_AUDIO_BIT_WIDTH,
                          DEMO_AUDIO_DATA_CHANNEL);
    SAI_RxSetBitClockRate(DEMO_SAI, DEMO_AUDIO_MASTER_CLOCK, DEMO_AUDIO_SAMPLE_RATE, DEMO_AUDIO_BIT_WIDTH,
                          DEMO_AUDIO_DATA_CHANNEL);
}

void SAI_UserTxIRQHandler(void) {
    /* Clear the FEF flag */
    SAI_TxClearStatusFlags(DEMO_SAI, kSAI_FIFOErrorFlag);
    SAI_TxSoftwareReset(DEMO_SAI, kSAI_ResetTypeFIFO);
    SDK_ISR_EXIT_BARRIER;
}

void SAI_UserRxIRQHandler(void) {
    SAI_RxClearStatusFlags(DEMO_SAI, kSAI_FIFOErrorFlag);
    SAI_RxSoftwareReset(DEMO_SAI, kSAI_ResetTypeFIFO);
    SDK_ISR_EXIT_BARRIER;
}

void SAI_UserIRQHandler(void) {
    if (SAI_TxGetStatusFlag(DEMO_SAI) & kSAI_FIFOErrorFlag) {
        SAI_UserTxIRQHandler();
    }

    if (SAI_RxGetStatusFlag(DEMO_SAI) & kSAI_FIFOErrorFlag) {
        SAI_UserRxIRQHandler();
    }
    SDK_ISR_EXIT_BARRIER;
}
