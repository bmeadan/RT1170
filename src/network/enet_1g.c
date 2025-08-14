#include "enet_1g.h"
#include "board.h"
#include "fsl_adapter_gpio.h"
#include "logger/logger.h"
#include "fsl_enet.h"
#include "fsl_phy.h"
#include "pin_mux.h"
#include "network_sockets.h"
#include <sys/errno.h>

#if defined(BOARD_ALON_CARRIER) || defined(BOARD_TVAI_CARRIER)
    #include "fsl_phymxl8611.h"
    #define E1G_PHY_OPS &phymxl8611_ops
    static phy_mxl8611_resource_t g_phy_resource;
#elif defined(BOARD_EVK)
    #include "fsl_phyrtl8211f.h"
    #define E1G_PHY_OPS &phyrtl8211f_ops
    static phy_rtl8211f_resource_t g_phy_resource;
#else
    message(FATAL "Unsupported board for ENET 1G initialization")
#endif

#define E1G_ENET          ENET_1G
#define E1G_PHY_ADDRESS   BOARD_ENET1_PHY_ADDRESS
#define E1G_NETIF_INIT_FN ethernetif1_init
#define E1G_CLOCK_FREQ    CLOCK_GetRootClockFreq(kCLOCK_Root_Bus)

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
    clock_root_config_t rootCfg = {.mux = 4, .div = 4}; /* Generate 125M root clock. */
    CLOCK_SetRootClock(kCLOCK_Root_Enet2, &rootCfg);
}

static void IOMUXC_SelectENETClock(void) {
    IOMUXC_GPR->GPR5 |= IOMUXC_GPR_GPR5_ENET1G_RGMII_EN_MASK; /* bit1:iomuxc_gpr_enet_clk_dir
                                                                 bit0:GPR_ENET_TX_CLK_SEL(internal or OSC) */
}

static void MDIO_Init(void) {
    (void)CLOCK_EnableClock(s_enetClock[ENET_GetInstance(E1G_ENET)]);
    ENET_SetSMI(E1G_ENET, E1G_CLOCK_FREQ, false);
}

static status_t MDIO_Write(uint8_t phyAddr, uint8_t regAddr, uint16_t data) {
    return ENET_MDIOWrite(E1G_ENET, phyAddr, regAddr, data);
}

static status_t MDIO_Read(uint8_t phyAddr, uint8_t regAddr, uint16_t *pData) {
    return ENET_MDIORead(E1G_ENET, phyAddr, regAddr, pData);
}

void init_networking_1g(void) {
    ip4_addr_t netif_ipaddr, netif_netmask;
    ethernetif_config_t enet_config = {
        .phyHandle   = &phyHandle,
        .phyAddr     = E1G_PHY_ADDRESS,
        .phyOps      = E1G_PHY_OPS,
        .phyResource = &g_phy_resource,
        .srcClockHz  = E1G_CLOCK_FREQ,
#ifdef configMAC_ADDR
        .macAddress = configMAC_ADDR,
#endif
#if ETH_LINK_POLLING_INTERVAL_MS == 0
        .phyIntGpio    = BOARD_INITENET1GPINS_PHY_INTR_PERIPHERAL,
        .phyIntGpioPin = BOARD_INITENET1GPINS_PHY_INTR_CHANNEL
#endif
    };

    /* Set MAC address. */
#ifndef configMAC_ADDR
    (void)SILICONID_ConvertToMacAddr(&enet_config.macAddress);
#endif

    ip4addr_aton(NETWORK_IP_ADDR_1G, &netif_ipaddr);
    ip4addr_aton(NETWORK_NET_MASK, &netif_netmask);
    
    addr_1g = ip4_addr_get_u32(&netif_ipaddr);

    netifapi_netif_add(&netif_1g, &netif_ipaddr, &netif_netmask, NULL, &enet_config, E1G_NETIF_INIT_FN, tcpip_input);
    netifapi_netif_set_up(&netif_1g);

    // PHY_Write(&phyHandle, 0x00, 0x0140); // BMCR: bit 6 = duplex, bit 13 = speed (1G), bit 12 = auto-neg OFF
}

void check_networking_1g(void) {
    bool is_up = netif_is_up(&netif_1g);
    if (is_up && !link_up) {
        LOGF("ENET 1G link up");
    } else if (!is_up && link_up) {
        LOGF("ENET 1G link down");
    }

    if (is_up && udp_sock_1g < 0) {
        udp_sock_1g = init_socket(&netif_1g);
        if (udp_sock_1g < 0) {
            LOGF("Error occurred during socket initialization for 1G network");
        }
    }

    if (!is_up && udp_sock_1g >= 0) {
        int res = close(udp_sock_1g);
        if (res < 0) {
            LOGF("Error occurred during closing: errno %d ", errno);
        } else {
            udp_sock_1g = -1;
        }
    }

    link_up = is_up;
}

void init_enet_1g(void) {
    BOARD_InitModuleClock();
    IOMUXC_SelectENETClock();

    BOARD_InitEnet1GPins();

    GPIO_WritePinOutput(BOARD_INITENET1GPINS_PHY_RESET_GPIO, BOARD_INITENET1GPINS_PHY_RESET_GPIO_PIN, 0);
    SDK_DelayAtLeastUs(10000, CLOCK_GetFreq(kCLOCK_CpuClk));
    GPIO_WritePinOutput(BOARD_INITENET1GPINS_PHY_RESET_GPIO, BOARD_INITENET1GPINS_PHY_RESET_GPIO_PIN, 1);
    SDK_DelayAtLeastUs(30000, CLOCK_GetFreq(kCLOCK_CpuClk));

    EnableIRQ(ENET_1G_MAC0_Tx_Rx_1_IRQn);
    EnableIRQ(ENET_1G_MAC0_Tx_Rx_2_IRQn);

    MDIO_Init();
    g_phy_resource.read  = MDIO_Read;
    g_phy_resource.write = MDIO_Write;
}

// #endif /* LWIP_IPV4 && LWIP_RAW && LWIP_SOCKET */