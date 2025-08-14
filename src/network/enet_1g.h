#ifndef __ENET_1G_H__
#define __ENET_1G_H__

#include "ethernetif.h"
#include "lwip/netifapi.h"
#include "lwip/opt.h"
#include "lwip/tcpip.h"
#include "netif/ethernet.h"
#include "pin_enet.h"
#include "project.h"

u32_t addr_1g;
struct netif netif_1g;
volatile int udp_sock_1g = -1;
void init_enet_1g(void);
void init_networking_1g(void);
void check_networking_1g(void);

#endif /* __ENET_1G_H__ */
