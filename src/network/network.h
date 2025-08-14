#ifndef __NETWORK_H__
#define __NETWORK_H__

#include "protocol/protocol.h"
#include <lwip/sockets.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define ACTIVE_TIMEOUT 5000 // Timeout in milliseconds
#define CONNECTION_TIMEOUT 300 // Timeout in milliseconds

typedef struct NetworkConnection {
    volatile bool connected;
    struct sockaddr_in remote_addr;
} NetworkConnection_t;

void init_network(void);

int send_message_to_master(const api_message_t *api_msg);
void send_message_to_slave(const api_message_t *api_msg);
bool is_master_connected();

#endif /* __NETWORK_H__ */
