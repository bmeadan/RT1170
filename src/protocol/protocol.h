#ifndef _PROTOCOL_H
#define _PROTOCOL_H

#include "project.h"
#include "protocol_api.h"
#include "protocol_error.h"
#include "protocol_gcs.h"
#include "protocol_telemetry.h"
#include <stdint.h>
#include <string.h>

#define PROTOCOL_PREAMBLE    0xAA
#define PROTOCOL_MAX_MTU     1200
#define PROTOCOL_TL_BYTES    8
#define PROTOCOL_BUFFER_SIZE PROTOCOL_MAX_MTU + PROTOCOL_TL_BYTES

typedef enum error_code: uint8_t {
    ERROR_NONE = 0,
    ERROR_FLASH_INTERNAL_ERROR = 1,
    ERROR_FLASH_INVALID_PACKET_ADDRESS = 2,
    ERROR_FLASH_INVALID_PACKET_LENGTH = 3,
    ERROR_FLASH_ERASE_FAILURE = 4,
    ERROR_FLASH_WRITE_FAILURE = 5,
    ERROR_FLASH_READ_FAILURE = 6,
    ERROR_FLASH_COMPARE_FAILURE = 7,
    ERROR_FLASH_MISMATCH_CHECKSUM = 8,
    ERROR_FLASH_CREATE_TASK = 9,
    ERROR_FLASH_BUSY_TASK = 10,
} error_code_t;

protocol_error_t parse_message(uint8_t *buffer, uint32_t len, api_message_t *msg);

uint8_t calculateChecksum(const api_message_t *msg);
void send_video_frame(uint8_t slot, uint16_t fragment, uint8_t *payload, uint32_t length);
void send_ext_video_frame(uint8_t address, uint16_t fragment, uint8_t *payload, uint32_t length);
void send_audio_frame(uint8_t slot, uint16_t fragment, uint8_t *payload, uint32_t length);
void send_acknowledgment(error_code_t err);
void send_gcs_to_slave(const api_message_t *msg);
int send_log(const char *log);

api_message_t *get_busy_message();

void master_message_handler(api_message_t *msg);
void slave_message_handler(api_message_t *msg);

#endif // _PROTOCOL_H