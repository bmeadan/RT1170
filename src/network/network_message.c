#include "network_message.h"
#include "network_sockets.h"

static uint8_t tx_direct_buffer[PROTOCOL_BUFFER_SIZE];

void NetworkMessage_Send(NetworkMessageType_t type, const api_message_t *api_msg, struct sockaddr_in *dest_addr) {
    NetworkMessage_t msg;
    uint8_t *buffer = tx_direct_buffer;
    msg.type        = type;
    msg.dest_addr   = *dest_addr;
    msg.length      = (size_t)api_msg->length + PROTOCOL_TL_BYTES;

    memcpy(tx_direct_buffer, api_msg, PROTOCOL_TL_BYTES - 1);
    if (api_msg->payload != NULL && api_msg->length) {
        memcpy(tx_direct_buffer + PROTOCOL_TL_BYTES - 1, api_msg->payload, api_msg->length);
    }

    tx_direct_buffer[msg.length - 1] = calculateChecksum(api_msg);
    socket_send_data(&msg, tx_direct_buffer);
}
