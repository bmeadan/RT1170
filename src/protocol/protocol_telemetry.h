#ifndef _PROTOCOL_TELMETRY_H
#define _PROTOCOL_TELMETRY_H
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum platform_type {
    PLATFORM_ALON_1    = 0,
    PLATFORM_ALON_2    = 1,
    PLATFORM_TVAI_2    = 2,
    PLATFORM_TVAI_3    = 3,
    PLATFORM_TVAI_5    = 4,
    PLATFORM_ALON_D    = 5,
    PLATFORM_EIN_RAAM  = 6,
    PLATFORM_TZACH     = 7,
    PLATFORM_TVAI_ALON = 8,
} platform_type_t;

typedef struct __attribute__((packed)) inertial_unit {
    int16_t imu_roll;
    int16_t imu_pitch;
    int16_t imu_yaw;
    int16_t acc_x;
    int16_t acc_y;
    int16_t acc_z;
} inertial_unit_t;

typedef struct __attribute__((packed)) telemetry_alon {
    uint8_t platform_type;
    float yaw_angle;
    float pitch_angle;
    uint16_t batteryRead;
    uint8_t hardware_version[6];
    uint8_t software_version[6];
    inertial_unit_t inertial_unit;
    
    //     float axis_encoder_angle;
    //     float imu_angles[3];
    //     float imu_raw_data[9];
    //     uint8_t cameras_rot[MAX_CAMERAS];
    //    
    //     
    //     uint8_t calibration_status;
    //     uint8_t led_status;
} telemetry_alon_t;

typedef struct __attribute__((packed)) telemetry_tvai {
    uint8_t platform_type;
    uint8_t cameras_config;
    uint8_t flashlights_config;
    uint8_t battery_read;

    inertial_unit_t inertial_unit;

    uint16_t motor1_current_sense;
    uint16_t motor2_current_sense;
    uint16_t motor3_current_sense;

    float encoder1;
    float encoder2;
    float encoder3;

    float cpu_temperature;
} telemetry_tvai_t;

extern uint32_t get_telemetry(uint8_t *buffer);
void send_telemetry();

#ifdef __cplusplus
}
#endif

#endif // _PROTOCOL_TELMETRY_H