#ifndef __AUDIOSAI_H__
#define __AUDIOSAI_H__

#include "AudioCodec.h"
#include "board.h"
#include "fsl_dmamux.h"
#include "fsl_sai_edma.h"

/*******************************************************************************
 * Definitions
 ******************************************************************************/
#define DEMO_AUDIO_SAMPLE_RATE  (kSAI_SampleRate8KHz)
#define DEMO_AUDIO_DATA_CHANNEL (2U)
#define DEMO_AUDIO_BIT_WIDTH    (kSAI_WordWidth16bits)

#define CODEC_ID          (AudioCodec_TI_3254)
#define CODEC_BIT_WIDTH   (AudioCodec_16_BIT)
#define CODEC_SAMPLE_RATE (DEMO_AUDIO_SAMPLE_RATE)
#define CODEC_CHANNEL     (AudioCodec_STEREO)
#define CODEC_SPEAKER     (AudioCodec_SPEAKER_LO)
#define CODEC_MIC         (AudioCodec_MIC_ONBOARD)

/* SAI instance and clock */
#define DEMO_SAI              SAI1
#define DEMO_SAI_CHANNEL      (0)
#define DEMO_SAI_BITWIDTH     (kSAI_WordWidth16bits)
#define DEMO_SAI_IRQ          SAI1_IRQn
#define DEMO_SAI_TX_SYNC_MODE kSAI_ModeAsync
#define DEMO_SAI_RX_SYNC_MODE kSAI_ModeSync
#define DEMO_SAI_MASTER_SLAVE kSAI_Master
#define SAI_UserIRQHandler    SAI1_IRQHandler

/* DMA */
#define DEMO_DMA             DMA0
#define DEMO_DMAMUX          DMAMUX0
#define DEMO_TX_EDMA_CHANNEL (0U)   
#define DEMO_RX_EDMA_CHANNEL (1U)
#define DEMO_SAI_TX_SOURCE   kDmaRequestMuxSai1Tx
#define DEMO_SAI_RX_SOURCE   kDmaRequestMuxSai1Rx

/* Get frequency of sai1 clock */
#define DEMO_SAI_CLK_FREQ CLOCK_GetRootClockFreq(kCLOCK_Root_Sai1)

/* demo audio master clock */
#define DEMO_AUDIO_MASTER_CLOCK DEMO_SAI_CLK_FREQ

void AudioSAI_Init(void);
void AudioDataCallback(uint8_t *packet, uint16_t packetSize);
void AudioPlaybackTask(void *pvParameters);
void AudioCaptureTask(void *pvParameters);

#endif // __AUDIOSAI_H__