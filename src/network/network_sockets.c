#include "logger/logger.h"
#include "network_sockets.h"
#include <compat/posix/sys/socket.h>
#include <lwip/netif.h>
#include <sys/errno.h>
#include <errno.h>

int init_socket(struct netif *netif) {
    int sockfd;
    struct sockaddr_in addr;

    // Set a timeout for receiving data
    struct timeval tv;
    tv.tv_sec  = SOCKET_RECEIVE_TIMEOUT / 1000;
    tv.tv_usec = (SOCKET_RECEIVE_TIMEOUT % 1000) * 1000;

    // Create a socket
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    // udp_sock = sockfd;
    if (sockfd < 0) {
        LOGF("Unable to create socket: errno %d", errno);
        return -1;
    }

    // Set up the address structure
    memset(&addr, 0, sizeof(addr));
    addr.sin_family      = AF_INET;
    addr.sin_addr.s_addr = netif->ip_addr.addr; // Bind to the network interface's IP address
    addr.sin_port        = htons(UDP_PORT);

    // Bind the socket to the specified port and IP address of the chosen network interface
    int err = bind(sockfd, (struct sockaddr *)&addr, sizeof(addr));
    if (err < 0) {
        LOGF("Socket unable to bind: errno %d", errno);
        close(sockfd);
        return -1;
    }

    // Set receive timeout
    setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
    // socket_init = 1;
    return sockfd; // Return the socket file descriptor
}

int socket_send_data(NetworkMessage_t *msg, uint8_t *buffer) {
    int sockfd = -1;
    if (msg->type == NETWORK_MESSAGE_MASTER) {
        sockfd = MASTER_SOCKET;
    } else if (msg->type == NETWORK_MESSAGE_SLAVE) {
        sockfd = SLAVE_SOCKET;
    }

    if (sockfd < 0) {
        // Socket is not initialized yet
        return -1;
    }

    int ret = sendto(sockfd, buffer, msg->length, 0, (struct sockaddr *)&msg->dest_addr, sizeof(msg->dest_addr));
    if (ret < 0 && errno != 0) {
        LOGF("Error occurred during sending: errno %d (socket=%d)", errno, sockfd);
    }
    return ret;
}

int socket_receive_from(int sockfd, uint8_t *buffer, size_t length, struct sockaddr_in *source_addr) {
    socklen_t source_addr_len = sizeof(*source_addr);

    int ret = recvfrom(sockfd, buffer, length, 0, (struct sockaddr *)source_addr, &source_addr_len);
    if (ret < 0 && errno != 0) {
        LOGF("Error occurred during receiving: errno %d (socket=%d)", errno, sockfd);
    }
    return ret;
}
