#include "protocol.h"
#include "FreeRTOS.h"
#include "lpadc/lpadc.h"
#include "motors/motors.h"
#include "network/network.h"
#include "network/network_entity.h"
#include "platform/platform.h"
#include "protocol/protocol_telemetry.h"
#include "imu/imu.h"
#include "project.h"

uint8_t calculateChecksum(const api_message_t *msg) {
    uint32_t checksum = 0;

    checksum += msg->start_byte;
    checksum += msg->address;
    checksum += msg->opcode;
    checksum += (msg->length & 0xFF);
    checksum += ((msg->length >> 8) & 0xFF);
    checksum += (msg->fragment & 0xFF);
    checksum += ((msg->fragment >> 8) & 0xFF);

    for (int i = 0; i < msg->length; i++) {
        checksum += msg->payload[i];
    }
    return (uint8_t)(checksum & 0xFF);
}

static void set_message(api_message_t *msg, uint8_t address, api_command_t opcode, uint16_t fragment, uint8_t *payload, uint32_t length) {
    msg->start_byte = PROTOCOL_PREAMBLE;
    msg->address    = address;
    msg->opcode     = (uint8_t)opcode;
    msg->fragment   = fragment;
    msg->length     = (uint16_t)length;
    msg->payload    = payload;
    msg->checksum   = 0x00; // Postpone checksum calculation until the end
}

api_message_t *get_busy_message() {
    static api_message_t busy_msg;
    static bool busy_msg_initialized = false;
    if (!busy_msg_initialized) {
        set_message(&busy_msg, 0x00, API_BUSY, 0, NULL, 0);
        busy_msg_initialized = true;
    }
    return &busy_msg;
}

protocol_error_t parse_message(uint8_t *buffer, uint32_t len, api_message_t *msg) {
    if (len < PROTOCOL_TL_BYTES || len > PROTOCOL_BUFFER_SIZE) {
        // LOGF("parse_message: buffer length error %d", len);
        return PROTOCOL_ERROR_LENGTH;
    }

    memcpy(msg, buffer, PROTOCOL_TL_BYTES - 1);
    if (msg->start_byte != PROTOCOL_PREAMBLE) {
        // LOGF("parse_message: preamble error %d", msg->start_byte);
        return PROTOCOL_ERROR_PREAMBLE;
    }

    if (len != msg->length + PROTOCOL_TL_BYTES) {
        // LOGF("parse_message: length error %d", len);
        return PROTOCOL_ERROR_LENGTH;
    }

    memcpy(msg->payload, buffer + PROTOCOL_TL_BYTES - 1, msg->length);
    msg->checksum = buffer[len - 1];

    uint8_t checksum = calculateChecksum(msg);
    if (checksum != msg->checksum) {
        // LOGF("parse_message: checksum error %d != %d", checksum, msg->checksum);
        return PROTOCOL_ERROR_CHECKSUM;
    }

    return PROTOCOL_ERROR_NONE;
}

static void send_frame(uint8_t address, api_command_t opcode, uint16_t fragment, uint8_t *payload, uint32_t length) {
    static api_message_t frame_msg;
    if (get_network_entity_id() == 0) {
        return;
    }
    set_message(&frame_msg, address, opcode, fragment, payload, length);
    send_message_to_master(&frame_msg);
}

void send_video_frame(uint8_t slot, uint16_t fragment, uint8_t *payload, uint32_t length) {
    if (platform_state != PLATFORM_STATE_RUNNING) {
        return;
    }
    uint8_t address = slot << 4 | get_network_entity_id();
    send_frame(address, API_VIDEO_FRAME, fragment, payload, length);
}

void send_ext_video_frame(uint8_t address, uint16_t fragment, uint8_t *payload, uint32_t length) {
    if (platform_state != PLATFORM_STATE_RUNNING) {
        return;
    }
    send_frame(address, API_VIDEO_FRAME, fragment, payload, length);
}

void send_audio_frame(uint8_t slot, uint16_t fragment, uint8_t *payload, uint32_t length) {
    if (platform_state != PLATFORM_STATE_RUNNING) {
        return; // Do not send audio frames while firmware update is in progress
    }
    uint8_t address = slot << 4 | get_network_entity_id();
    send_frame(address, API_AUDIO_MIC, fragment, payload, length);
}

void send_acknowledgment(error_code_t err) {
    static api_message_t ack_msg;
    static uint8_t ack_payload[1];
    uint8_t entity_id = get_network_entity_id();
    if (entity_id == 0) {
        return; // No ACK to send if entity ID is 0
    }

    ack_payload[0] = (uint8_t)err;
    api_command_t ack_command = (err == ERROR_NONE) ? API_ACK : API_NACK;
    set_message(&ack_msg, entity_id, ack_command, 0, ack_payload, sizeof(ack_payload));
    send_message_to_master(&ack_msg);
}

int send_log(const char *log) {
    static api_message_t log_msg;
    static uint8_t log_payload[PROTOCOL_MAX_MTU];
    uint8_t network_entity_id = get_network_entity_id();    

    if (network_entity_id == 0) {
        return -1; // No telemetry data to send if entity ID is 0
    }

    uint32_t length = strlen(log);
    
    if (length > PROTOCOL_MAX_MTU) {
        length = PROTOCOL_MAX_MTU; // Truncate log if it exceeds maximum MTU
    }
    
    memcpy(log_payload, log, length);
    
    set_message(&log_msg, get_network_entity_id(), API_LOG, 0, log_payload, length);
    return send_message_to_master(&log_msg);
}

void send_telemetry() {
    static api_message_t telemetry_msg;
    static uint8_t telemetry_buffer[PROTOCOL_BUFFER_SIZE];
    uint8_t network_entity_id = get_network_entity_id();    

    if (network_entity_id == 0) {
        return; // No telemetry data to send if entity ID is 0
    }
    
    uint32_t len = get_telemetry(telemetry_buffer);
    if (len == 0) {
        return; // No telemetry data to send
    }

    set_message(&telemetry_msg, get_network_entity_id(), API_TELEMETRY, 0, telemetry_buffer, len);
    send_message_to_master(&telemetry_msg);
}

void send_gcs_to_slave(const api_message_t *msg) {
    api_message_t gcs_msg;
    set_message(&gcs_msg, msg->address + 1, msg->opcode, msg->fragment, msg->payload, msg->length);
    memcpy(gcs_msg.payload, msg->payload, msg->length);
    send_message_to_slave(&gcs_msg);
}