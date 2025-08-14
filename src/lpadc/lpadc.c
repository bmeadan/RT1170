#include "lpadc.h"
#include "FreeRTOS.h"
#include "fsl_lpadc.h"
#include "semphr.h"
#include "project.h"

#define BATTERY_MILI_VOLTAGE_REFERENCE 7.954545f
#define ADC_INTERVAL_MS                10
#define APP_CMDID                      1U
#define APP_LPADC_VREF_SOURCE          kLPADC_ReferenceVoltageAlt2
#define EMA_ALPHA_SHIFT                6     // = 1/64 smoothing

#if (defined(APP_LPADC_USE_HIGH_RESOLUTION) && APP_LPADC_USE_HIGH_RESOLUTION)
const uint32_t g_LpadcFullRange   = 65536U;
const uint32_t g_LpadcResultShift = 0U;
#else
const uint32_t g_LpadcFullRange   = 4096U;
const uint32_t g_LpadcResultShift = 3U;
#endif /* APP_LPADC_USE_HIGH_RESOLUTION */

static lpadc_conv_trigger_config_t triggerConfig = {
    .targetCommandId       = APP_CMDID,
    .enableHardwareTrigger = false,
};

static uint16_t last_result;
static volatile uint16_t results[ADC_INPUTS];
static SemaphoreHandle_t adcDoneSemaphore;

static lpadc_conv_command_config_t mLpadcCommandConfigStruct;
static lpadc_conv_trigger_config_t mLpadcTriggerConfigStruct;

void ADC1_IRQHandler(void) {
    lpadc_conv_result_t adcResult;
    if (LPADC_GetConvResult(LPADC1, &adcResult)) {
        // Convert the result to mV
        last_result = (adcResult.convValue >> 3) * BATTERY_MILI_VOLTAGE_REFERENCE;
        xSemaphoreGiveFromISR(adcDoneSemaphore, pdFALSE);
    }

    SDK_ISR_EXIT_BARRIER;
}

static uint16_t smooth_adc(uint16_t previous, uint16_t new_sample) {
    if (previous == 0) {
        return new_sample; // If previous is zero, return the new sample directly
    }
    return previous + ((new_sample - previous) >> EMA_ALPHA_SHIFT);
}

static uint16_t read_adc_channel(uint32_t channel, lpadc_sample_channel_mode_t mode) {
    lpadc_conv_command_config_t cmdConfig = {
        .channelNumber       = channel,
        .sampleChannelMode   = mode,
        .hardwareAverageMode = kLPADC_HardwareAverageCount32};

    LPADC_SetConvTriggerConfig(LPADC1, 0U, &triggerConfig);
    LPADC_SetConvCommandConfig(LPADC1, APP_CMDID, &cmdConfig);
    LPADC_DoSoftwareTrigger(LPADC1, 1U);

    if (xSemaphoreTake(adcDoneSemaphore, pdMS_TO_TICKS(100)) != pdTRUE) {
        return 0;
    }

    return last_result;
}

static void adc_task(void *pvParameters) {
    uint16_t val;
    while (1) {
        // val = read_adc_channel(0U, kLPADC_SampleChannelSingleEndSideA); // GPIO_AD_00Add commentMore actions
        // results[IPROP_1] = smooth_adc(results[IPROP_1], val);

        // val = read_adc_channel(1U, kLPADC_SampleChannelSingleEndSideA); // GPIO_AD_02
        // results[IPROP_2] = smooth_adc(results[IPROP_2], val);

        // val = read_adc_channel(2U, kLPADC_SampleChannelSingleEndSideA); // GPIO_AD_04
        // results[IPROP_3] = smooth_adc(results[IPROP_3], val);
        
        val = read_adc_channel(3U, kLPADC_SampleChannelSingleEndSideA); // GPIO_AD_12
        results[SENSE_12V] = smooth_adc(results[SENSE_12V], val);

        vTaskDelay(pdMS_TO_TICKS(ADC_INTERVAL_MS));
    }

    vTaskDelete(NULL);
}

void init_adc(void) {
    static lpadc_config_t mLpadcConfigStruct;
    LPADC_GetDefaultConfig(&mLpadcConfigStruct);
    mLpadcConfigStruct.enableAnalogPreliminary = true;
    mLpadcConfigStruct.referenceVoltageSource  = APP_LPADC_VREF_SOURCE;

    adcDoneSemaphore = xSemaphoreCreateBinary();

    LPADC_Init(LPADC1, &mLpadcConfigStruct);

    NVIC_SetPriority(ADC1_IRQn, 5);
    LPADC_EnableInterrupts(LPADC1, kLPADC_FIFO0WatermarkInterruptEnable);
    EnableIRQ(ADC1_IRQn);

    xTaskCreate(adc_task, "ADC Task", 1024U, NULL, configMAX_PRIORITIES-4, NULL);
}

uint16_t get_adc_input(adc_input_t input) {
    return input >= ADC_INPUTS ? 0 : results[input];
}

uint16_t get_battery_voltage(void) {
    uint16_t input = get_adc_input(SENSE_12V);
    if (input == 0) {
        return 0; // No valid reading
    }

#if BATTERY_MAX_VOLTAGE == 24000
    return 19000 + (input - 5400) * (24000 - 19000) / (7000 - 5400);
#else
    return 9800 + (input - 3118) * (12600 - 9800) / ( 4056 - 3118);
#endif
}

uint16_t get_battery_level(void) {
    uint16_t voltage = get_battery_voltage();
    if (voltage == 0) {
        return 0; // No valid reading
    }

    uint16_t level = (voltage - BATTERY_MIN_VOLTAGE) * 10000 / (BATTERY_MAX_VOLTAGE - BATTERY_MIN_VOLTAGE);
    if (level > 10000) {
        level = 10000; // Cap at 100%
    } else if (level < 0) {
        level = 0; // Cap at 0%
    }

    return level;
}
