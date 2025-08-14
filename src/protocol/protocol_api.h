#ifndef _PROTOCOL_API_H
#define _PROTOCOL_API_H
#include <stdint.h>

typedef struct __attribute__((packed)) api_message {
    uint8_t start_byte;
    uint8_t address;
    uint8_t opcode;
    uint16_t length;
    uint16_t fragment;
    uint8_t *payload; /* The data */
    uint8_t checksum;
} api_message_t;

typedef enum api_command {
    API_GCS_STATE   = 0,
    API_CONFIG      = 1,
    API_TELEMETRY   = 2,
    API_VIDEO_FRAME = 3,
    API_AUDIO_SPK   = 4,
    API_AUDIO_MIC   = 5,
    API_LIDAR_FRAME = 6,
    API_BOOT        = 7,
    API_FW_START    = 8,
    API_FW_PACKET   = 9,
    API_FW_END      = 10,
    API_DRIVE_MODE  = 11,
    API_SET_TARGETS = 12,
    API_SET_PWMS    = 13,
    API_RESET_IMU   = 14,
    API_CALIBRATE   = 15,
    API_LOG         = 252,
    API_BUSY        = 253,
    API_NACK        = 254,
    API_ACK         = 255,
} api_command_t;

typedef enum api_config {
    API_CONFIG_FLIPPED_DRIVE = 0,
} api_config_t;

typedef struct __attribute__((packed)) api_fw_frame {
    uint32_t address;
    uint16_t length;
    uint8_t *payload;
} api_fw_frame_t;


#endif // _PROTOCOL_API_H
