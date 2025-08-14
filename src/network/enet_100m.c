#include "enet_100m.h"
#include "board.h"
#include "pin_mux.h"
#include "fsl_adapter_gpio.h"
#include "logger/logger.h"
#include "fsl_enet.h"
#include "fsl_phy.h"
#include "network_sockets.h"
#include <sys/errno.h>

#if defined(BOARD_TVAI_CARRIER)
    #include "phyksz8895.h"
    #define E100M_PHY_OPS &phyksz8895_ops
    static phy_ksz8895_resource_t g_phy_resource;
#elif defined(BOARD_ALON_CARRIER) ||  defined(BOARD_EVK)
    #include "fsl_phyksz8081.h"
    #define E100M_PHY_OPS &phyksz8081_ops
    static phy_ksz8081_resource_t g_phy_resource;
#else
    message(FATAL "Unsupported board for ENET 100M initialization")
#endif

#define E100M_ENET          ENET
#define E100M_PHY_ADDRESS   BOARD_ENET0_PHY_ADDRESS
#define E100M_NETIF_INIT_FN ethernetif0_init
#define E100M_CLOCK_FREQ    CLOCK_GetRootClockFreq(kCLOCK_Root_Bus)

/* Must be after include of app.h */
#ifndef configMAC_ADDR
#include "fsl_silicon_id.h"
#endif

/*******************************************************************************
 * Variables
 ******************************************************************************/
static bool link_up = false;
static phy_handle_t phyHandle;

/*******************************************************************************
 * Code
 ******************************************************************************/
static void BOARD_InitModuleClock(void) {
    const clock_sys_pll1_config_t sysPll1Config = {
        .pllDiv2En = true,
    };
    CLOCK_InitSysPll1(&sysPll1Config);

    clock_root_config_t rootCfg = {.mux = 4, .div = 10}; /* Generate 50M root clock. */
    CLOCK_SetRootClock(kCLOCK_Root_Enet1, &rootCfg);
}

static void IOMUXC_SelectENETClock(void) {
    IOMUXC_GPR->GPR4 |= IOMUXC_GPR_GPR4_ENET_REF_CLK_DIR_MASK; /* 50M ENET_REF_CLOCK output to PHY and ENET module. */
}

static void MDIO_Init(void) {
    (void)CLOCK_EnableClock(s_enetClock[ENET_GetInstance(E100M_ENET)]);
    ENET_SetSMI(E100M_ENET, E100M_CLOCK_FREQ, false);
}

static status_t MDIO_Write(uint8_t phyAddr, uint8_t regAddr, uint16_t data) {
    return ENET_MDIOWrite(E100M_ENET, phyAddr, regAddr, data);
}

static status_t MDIO_Read(uint8_t phyAddr, uint8_t regAddr, uint16_t *pData) {
    return ENET_MDIORead(E100M_ENET, phyAddr, regAddr, pData);
}

void init_networking_100m(void) {

    ip4_addr_t netif_ipaddr, netif_netmask;
    ethernetif_config_t enet_config = {
        .phyHandle   = &phyHandle,
        .phyAddr     = E100M_PHY_ADDRESS,
        .phyOps      = E100M_PHY_OPS,
        .phyResource = &g_phy_resource,
        .srcClockHz  = E100M_CLOCK_FREQ,
#ifdef configMAC_ADDR
        .macAddress = configMAC_ADDR,
#endif
#if ETH_LINK_POLLING_INTERVAL_MS == 0
        .phyIntGpio    = BOARD_INITENETPINS_PHY_INTR_PERIPHERAL,
        .phyIntGpioPin = BOARD_INITENETPINS_PHY_INTR_CHANNEL
#endif
    };

    /* Set MAC address. */
#ifndef configMAC_ADDR
    (void)SILICONID_ConvertToMacAddr(&enet_config.macAddress);
#endif

    ip4addr_aton(NETWORK_IP_ADDR_100M, &netif_ipaddr);
    ip4addr_aton(NETWORK_NET_MASK, &netif_netmask);
    addr_100m = ip4_addr_get_u32(&netif_ipaddr);

    netifapi_netif_add(&netif_100m, &netif_ipaddr, &netif_netmask, NULL, &enet_config, E100M_NETIF_INIT_FN, tcpip_input);
    netifapi_netif_set_up(&netif_100m);
}

void check_networking_100m(void) {
    bool is_up = netif_is_up(&netif_100m);
    if (is_up && !link_up) {
        LOGF("ENET 100M link up");
    } else if (!is_up && link_up) {
        LOGF("ENET 100M link down");
    }

    if (is_up && udp_sock_100m < 0) {
        udp_sock_100m = init_socket(&netif_100m);
        if (udp_sock_100m < 0) {
            LOGF("Error occurred during socket initialization for 100M network");
        }
    }

    if (!is_up && udp_sock_100m >= 0) {
        int res = close(udp_sock_100m);
        if (res < 0) {
            LOGF("Error occurred during closing: errno %d ", errno);
        } else {
            udp_sock_100m = -1;
        }
    }

    link_up = is_up;
}

void init_enet_100m(void) {
    BOARD_InitModuleClock();
    IOMUXC_SelectENETClock();

    BOARD_InitEnetPins();

    GPIO_WritePinOutput(BOARD_INITENETPINS_PHY_RESET_GPIO, BOARD_INITENETPINS_PHY_RESET_GPIO_PIN, 0);
    SDK_DelayAtLeastUs(10000, CLOCK_GetFreq(kCLOCK_CpuClk));
    GPIO_WritePinOutput(BOARD_INITENETPINS_PHY_RESET_GPIO, BOARD_INITENETPINS_PHY_RESET_GPIO_PIN, 1);
    SDK_DelayAtLeastUs(10000, CLOCK_GetFreq(kCLOCK_CpuClk));

    MDIO_Init();
    g_phy_resource.read  = MDIO_Read;
    g_phy_resource.write = MDIO_Write;
}

// #endif /* LWIP_IPV4 && LWIP_RAW && LWIP_SOCKET */