#ifndef __NETWORK_MESSAGE_
#define __NETWORK_MESSAGE_
#include "protocol/protocol.h"
#include <lwip/sockets.h>
#include <stddef.h>
#include <stdint.h>

typedef enum NetworkMessageType {
    NETWORK_MESSAGE_SLAVE,
    NETWORK_MESSAGE_MASTER,
} NetworkMessageType_t;

typedef struct NetworkMessage {
    NetworkMessageType_t type;
    size_t length;
    struct sockaddr_in dest_addr;
} NetworkMessage_t;

void NetworkMessage_Send(NetworkMessageType_t type, const api_message_t *api_msg, struct sockaddr_in *dest_addr);

#endif /* __NETWORK_MESSAGE_ */
