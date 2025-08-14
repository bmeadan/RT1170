#include "audio.h"
#include "AudioCodec.h"
#include "AudioSAI.h"
#include "FreeRTOS.h"
#include "clock_config.h"
#include "logger/logger.h"
#include "pin_mux.h"
#include "task.h"

static void AudioI2C_Init(void) {
    /*Clock setting for LPI2C*/
    CLOCK_SetRootClockMux(kCLOCK_Root_Lpi2c5, 1);
}

static void AudioI2C_Configure(void) {
    uint8_t status;
    status = AudioCodec_open();
    if (AudioCodec_STATUS_SUCCESS != status) {
        /* Error Initializing codec */
        LOGF("Error Initializing codec");
        return;
    }

    status = AudioCodec_config(CODEC_ID,
                               CODEC_BIT_WIDTH,
                               CODEC_SAMPLE_RATE,
                               CODEC_CHANNEL,
                               CODEC_SPEAKER,
                               CODEC_MIC);

    if (AudioCodec_STATUS_SUCCESS != status) {
        LOGF("Error Configuring codec");
        AudioCodec_close();
        return;
    }

    status = AudioCodec_speakerVolCtrl(CODEC_ID, CODEC_SPEAKER, 100);
    if (AudioCodec_STATUS_SUCCESS != status) {
        LOGF("Error Setting speaker volume");
        AudioCodec_close();
        return;
    }

    status = AudioCodec_micUnmute(CODEC_ID, CODEC_MIC);
    if (AudioCodec_STATUS_SUCCESS != status) {
        LOGF("Error Unmuting microphone");
        AudioCodec_close();
        return;
    }

    status = AudioCodec_micVolCtrl(CODEC_ID, CODEC_MIC, 100);
    if (AudioCodec_STATUS_SUCCESS != status) {
        LOGF("Error Setting micrphone volume");
        AudioCodec_close();
        return;
    }
}
static void deinit_audio(void) {
    AudioCodec_close();
}

void init_audio(void) {
    BOARD_InitAudioPins();
    AudioI2C_Init();
    AudioI2C_Configure();
    AudioSAI_Init();
    xTaskCreate(AudioPlaybackTask, "audio_play", 1024U, NULL, configMAX_PRIORITIES - 7, NULL);
    xTaskCreate(AudioCaptureTask, "audio_capture", 1024U, NULL, configMAX_PRIORITIES - 7, NULL);
}