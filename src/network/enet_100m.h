#ifndef __ENET_100M_H__
#define __ENET_100M_H__

#include "ethernetif.h"
#include "lwip/netifapi.h"
#include "lwip/opt.h"
#include "lwip/tcpip.h"
#include "netif/ethernet.h"
#include "pin_enet.h"
#include "project.h"

u32_t addr_100m;
struct netif netif_100m;
volatile int udp_sock_100m = -1;
void init_enet_100m(void);
void init_networking_100m(void);
void check_networking_100m(void);

#endif /* __ENET_100M_H__ */
