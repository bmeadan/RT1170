#pragma once
#include <stdint.h>
typedef struct { int32_t yaw; int32_t pitch; int32_t roll; } imu_data_t;
extern "C" imu_data_t get_imu_data(void); // stubbed in the sim .cpp