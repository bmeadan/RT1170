#ifndef __NETWORK_SOCkETS_H__
#define __NETWORK_SOCkETS_H__
#include "enet_100m.h"
#include "enet_1g.h"
#include "network_message.h"
#include <lwip/netif.h>
#include <stdint.h>

#define SOCKET_RECEIVE_TIMEOUT 50   // Timeout in milliseconds (50 ms)
#define UDP_PORT               1234

#ifdef NETWORK_MASTER_100M
#define MASTER_SOCKET    udp_sock_100m
#define SLAVE_SOCKET     udp_sock_1g
#define NETWORK_SLAVE_IP NETWORK_IP_ADDR_100M
#else
#define MASTER_SOCKET    udp_sock_1g
#define SLAVE_SOCKET     udp_sock_100m
#define NETWORK_SLAVE_IP NETWORK_IP_ADDR_1G
#endif

int init_socket(struct netif *netif);
int socket_send_data(NetworkMessage_t *msg, uint8_t *buffer);
int socket_receive_from(int sockfd, uint8_t *buffer, size_t length, struct sockaddr_in *source_addr);

#endif /* __NETWORK_SOCkETS_H__ */
