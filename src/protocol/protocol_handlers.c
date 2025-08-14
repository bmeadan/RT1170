#include "audio/audio.h"
#include "lights/lights.h"
#include "motors/motors.h"
#include "platform/platform.h"
#include "protocol.h"
#include "usb/host_video.h"
#include "flash/flash.h"
#include "flash/ExternalFlashMap.h"
#include "imu/imu.h"
#include "network/network.h"
#include "network/network_entity.h"
#include "logger/logger.h"

extern frame_data_t frames[4];
static void handle_video_frame(api_message_t *msg)
{
    static int pictureBufferIndex = 0;
    static int lastFragment = -1;
    static int lastFrameId = -1;
    int address = msg->address;

#if MAX_FRAME_BUFFER > 2
    if(frames[2].ready)
    {
        return;
    }

    int framId = (msg->fragment >> 14) & 0x01;
    int eof = (msg->fragment >> 15) & 0x1;
    int fragment = msg->fragment & 0x3FFF;
    
    if(msg->fragment == 0 || lastFrameId != framId){
        lastFragment = -1;
        frames[2].size[pictureBufferIndex] = 0;
    }
    
    if(fragment != lastFragment + 1){
        lastFragment = -1;
        frames[2].size[pictureBufferIndex] = 0;
        return;
    }

    lastFragment = fragment;
    lastFrameId = framId;
    int size = msg->length;
    int currIdx = 0;
    frames[2].is_ext = 1;
    frames[2].address[pictureBufferIndex] = address;
    memcpy(frames[2].frame[pictureBufferIndex] + frames[2].size[pictureBufferIndex], msg->payload, size);
    frames[2].size[pictureBufferIndex] += size;

    if(eof){
        frames[2].ready = 1;
        frames[2].pictureBufferIndex = pictureBufferIndex;
        frames[2].frameId = framId;
        lastFragment = -1;
        pictureBufferIndex = (pictureBufferIndex + 1) % 2;
        frames[2].size[pictureBufferIndex] = 0;
    }
 #endif
}

static protocol_error_t handle_gcs_message(const api_message_t *msg) {
    if (msg->length < 1) {
        // LOGF("handle_gcs_message: invalid payload length %d", msg->length);
        return PROTOCOL_ERROR_INVALID_PAYLOAD;
    }

    static motors_action_t action;
    gcs_type_t gcs_type = (gcs_type_t)msg->payload[0];
    switch (gcs_type) {
        case GCS_NONE:
            action.drive = 0;
            action.yaw = 0;
            action.pitch = 0;
            action.auto_neutral_joints = 0;
            action.drive_pulse = 0;
            action.flip_mode = 0;
            set_motors(&action);
            return PROTOCOL_ERROR_NONE;
        case GCS_101: {
            if (msg->length != sizeof(gcs_101_state_t)) {
                // LOGF("handle_gcs_message: invalid payload length %d", msg->length);
                return PROTOCOL_ERROR_INVALID_PAYLOAD;
            }
            gcs_101_state_t *state = (gcs_101_state_t *)msg->payload;
            action.drive = state->rightStickY;
            action.yaw = state->leftStickX;
            action.pitch = state->leftStickY;
            action.auto_neutral_joints = state->leftLowButton1;
            action.drive_pulse = state->leftLowButton2;
            action.flip_mode = 0;
            set_motors(&action);
            return PROTOCOL_ERROR_NONE;
        }
        case GCS_DS4: {
            if (msg->length != sizeof(gcs_ds4_state_t)) {
                // LOGF("handle_gcs_message: invalid payload length %d", msg->length);
                return PROTOCOL_ERROR_INVALID_PAYLOAD;
            }
            gcs_ds4_state_t *state = (gcs_ds4_state_t *)msg->payload;
            action.drive = state->rightStickY;
            action.yaw = state->leftStickX;
            action.pitch = state->leftStickY;
            action.auto_neutral_joints = state->x;
            action.drive_pulse = state->l2;
            action.flip_mode = state->l1;
            set_motors(&action);
            set_flashlights(state->flash_light);
            toggle_cam(state->arrow_left);
            return PROTOCOL_ERROR_NONE;
        }
        default:
            // LOGF("handle_gcs_message: unknown type %d", gcs_type);
            return PROTOCOL_ERROR_UNKNOWN_TYPE;
    }
}

static void handle_config(const api_message_t *msg) {
    if (msg->length < 2) {
        return;
    }

    api_config_t config_type = (api_config_t)msg->payload[0];
    switch (config_type) {
        case API_CONFIG_FLIPPED_DRIVE:
            appConfigData.flipped_drive = msg->payload[1] == 0 ? 0 : 1;
            flash_save_config();
            break;
        default:
            // LOGF("handle_config: unknown config type %d", msg->payload[0]);
            break;
    }
}


void master_message_handler(api_message_t *msg) {
    switch (msg->opcode) {
        case API_CONFIG:
            handle_config(msg);
            send_message_to_slave(msg);
            break;
        case API_AUDIO_SPK:
            AudioDataCallback(msg->payload, msg->length);
            break;
        case API_GCS_STATE:
            handle_gcs_message(msg);
            send_gcs_to_slave(msg);
            break;
        case API_BOOT:
            // First send the message to the slave
            send_message_to_slave(msg);
            // Then reboot the platform
            platform_reboot(700);
            break;
        case API_FW_START:
        case API_FW_PACKET:
        case API_FW_END: {
            int entity_id = get_network_entity_id();
            if (msg->address == entity_id) {
                flash_firmware_message(msg);
            } else {
                send_message_to_slave(msg);
            }
            break;
        }
        case API_DRIVE_MODE:
        case API_SET_TARGETS:
        case API_SET_PWMS:
            // not implemented yet
            break;
        case API_RESET_IMU:
            calibrate_imu();
            send_message_to_slave(msg);
            break;
        case API_CALIBRATE:
            calibrate_motors();
            send_message_to_slave(msg);
            break;
        default:
            LOGF("master_message_handler: bad opcode %d", msg->opcode);
            break;
    }
}

void slave_message_handler(api_message_t *msg) {
    switch (msg->opcode) {
        case API_VIDEO_FRAME:
            handle_video_frame(msg);
            break;
        case API_ACK:
        case API_NACK:
        case API_BUSY:
        case API_LOG:
        case API_TELEMETRY:
        case API_AUDIO_MIC:
        case API_LIDAR_FRAME:
            // Pass the data to the master socket
            send_message_to_master(msg);
            break;
        default:
            LOGF("slave_message_handler: bad opcode %d", msg->opcode);
            break;
    }
}