#ifndef __LPADC_H__
#define __LPADC_H__

#include <stdbool.h>
#include <stdint.h>

// Look at the following link for more information on how to use the ADC:
// https://community.nxp.com/t5/i-MX-Processors/RT1170-Multi-Channel-ADC-Read/m-p/1793756

#ifdef __cplusplus
extern "C" {
#endif
typedef enum adc_input {
    IPROP_1    = 0,
    IPROP_2    = 1,
    IPROP_3    = 2,
    SENSE_12V  = 3,
    ADC_INPUTS = 4

} adc_input_t;

void init_adc(void);
uint16_t get_adc_input(adc_input_t channel);
uint16_t get_battery_voltage(void);
uint16_t get_battery_level(void);

#ifdef __cplusplus
}
#endif

#endif // __LPADC_H__