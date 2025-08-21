#pragma once
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
typedef struct {
  int32_t yaw;    // centideg
  int32_t pitch;  // centideg
  int32_t roll;   // centideg
} imu_data_t;

imu_data_t get_imu_data(void);
#ifdef __cplusplus
}
#endif
