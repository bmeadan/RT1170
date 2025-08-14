#ifndef __ENCODER_RLS_H__
#define __ENCODER_RLS_H__

#ifdef __cplusplus
extern "C" {
#endif

#define SAMPLING_PERIOD  100 // miliSec

void readEncoderTask(void *pvParameters);
void init_EncoderRLS(void);
void calibrateRLSEncoder(void);
float getRLSEncoderAngle(void);

#ifdef __cplusplus
}
#endif

#endif // __ENCODER_RLS_H__
