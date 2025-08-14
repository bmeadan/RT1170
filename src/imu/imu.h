#ifndef IMU_H
#define IMU_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#define HEADER 0xAAAA
#define MSG_LENGTH 19

typedef struct imu_data {
    int16_t yaw;
    int16_t pitch;
    int16_t roll;
    int16_t acc_x;
    int16_t acc_y;
    int16_t acc_z;
} imu_data_t;

typedef struct imu_data_offset {
    int16_t yaw;
    int16_t pitch;
    int16_t roll;
} imu_data_offset_t;

void init_imu(void);
void calibrate_imu(void);
imu_data_t get_imu_data(void);

#ifdef __cplusplus
}
#endif

#endif // IMU_H
