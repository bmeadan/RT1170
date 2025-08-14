#include "network.h"
#include "FreeRTOS.h"
#include "enet_100m.h"
#include "enet_1g.h"
#include "fsl_enet.h"
#include "fsl_adapter_gpio.h"
#include "lights/lights.h"
#include "network_message.h"
#include "network_sockets.h"
#include "network_entity.h"
#include "queue.h"
#include "usb/host_video.h"
#include "logger/logger.h"
#include "motors/motors.h"
#include "lwip/stats.h"
#include "platform/platform.h"
#include "protocol/protocol_telemetry.h"
#include <compat/posix/sys/socket.h>

static NetworkConnection_t conn = {.connected = false, .remote_addr = {0}};
static struct sockaddr_in slave_addr;
static volatile uint32_t last_active_time_master;

QueueHandle_t slaveTxQueue;
QueueHandle_t masterTxQueue;

bool is_master_connected() {
    return conn.connected;
}

struct netif *my_ip4_route_hook(const ip4_addr_t *src, const ip4_addr_t *dest) {
    if (src->addr == addr_100m) {
        return &netif_100m;
    } else if (src->addr == addr_1g) {
        return &netif_1g;
    } else {
        return NULL;
    }
}

void BOARD_ENETFlexibleConfigure(enet_config_t *config) {
    if (config->userData == &netif_1g) {
        config->miiMode = kENET_RgmiiMode;
    } else {
        config->miiMode = kENET_RmiiMode;
    }
}
static void log_connection(struct sockaddr_in *addr, bool connected) {
    if (connected) {
        LOGF("Connected to master IP: %s:%d", inet_ntoa(addr->sin_addr), ntohs(addr->sin_port));
    } else {
        LOGF("Disconnected from master IP: %s:%d", inet_ntoa(addr->sin_addr), ntohs(addr->sin_port));
    }
}

static int socket_receive_data_master(uint8_t *buffer, size_t length, struct sockaddr_in *source_addr) {
    static api_message_t master_msg;
    static uint8_t payload[PROTOCOL_MAX_MTU];
    static uint32_t last_connected_time_master = 0;
    
    uint32_t current_time = get_time();

    master_msg.payload = payload;

    if (conn.connected && current_time - last_connected_time_master > CONNECTION_TIMEOUT) {
        log_connection(&conn.remote_addr, false);
        conn.remote_addr.sin_port = 0;
        conn.connected            = false;
        emergency_stop_motors();
        set_network_entity_id(0);
    }

    if (MASTER_SOCKET < 0) {
        vTaskDelay(pdMS_TO_TICKS(10));
        return -1;
    }

    int ret = socket_receive_from(MASTER_SOCKET, buffer, length, source_addr);
    if (ret > 0) {
        protocol_error_t err = parse_message(buffer, ret, &master_msg);
        if (err != PROTOCOL_ERROR_NONE) {
            LOGF("handle_message: parse error %d", err);
            return -1;
        }

        if (!conn.connected) {
            conn.remote_addr = *source_addr;
            conn.connected   = true;
            log_connection(&conn.remote_addr, true);
        } else if (conn.remote_addr.sin_port != source_addr->sin_port) {
            // NetworkMessage_Send(NETWORK_MESSAGE_MASTER, get_busy_message(), source_addr);
            return -1;
        }

        last_connected_time_master = last_active_time_master = get_time(); 
        if (master_msg.opcode == API_GCS_STATE) {
            set_network_entity_id(master_msg.address);
        }
        master_message_handler(&master_msg);
        return ret;
    } 

    return -1;
}

static int socket_receive_data_slave(uint8_t *buffer, size_t length, struct sockaddr_in *source_addr) {
    static api_message_t slave_msg;
    static uint8_t payload[PROTOCOL_MAX_MTU];
    static uint32_t last_connected_time_slave = 0;
    uint32_t current_time = get_time();

    slave_msg.payload = payload;

    if (current_time - last_connected_time_slave > CONNECTION_TIMEOUT) {
        set_slave_network_entity_id(0);
        last_connected_time_slave = 0;
    }

    if (SLAVE_SOCKET < 0) {
        vTaskDelay(pdMS_TO_TICKS(10));
        return -1;
    }

    while (1) {
        int ret = socket_receive_from(SLAVE_SOCKET, buffer, length, source_addr);
        if (ret > 0) {
            protocol_error_t err = parse_message(buffer, ret, &slave_msg);
            if (err != PROTOCOL_ERROR_NONE) {
                LOGF("handle_message: parse error %d", err);
                return -1;
            }

            last_connected_time_slave = get_time();
            if (slave_msg.opcode == API_TELEMETRY) {
                set_slave_network_entity_id(slave_msg.address);
            }
            
            slave_message_handler(&slave_msg);
            return ret;
        } else {
            return ret;
        }
    }
    return -1;
}

void send_message_to_slave(const api_message_t *api_msg) {
    if (uxQueueSpacesAvailable(slaveTxQueue) == 0) {
        return; // Queue is full, cannot send message
    }

    if (xQueueSend(slaveTxQueue, api_msg, 0) == pdPASS) {
        // Successfully sent to the queue
    } else {
        // Failed to send to the queue
    }
}

int send_message_to_master(const api_message_t *api_msg) {
    if (!conn.connected) {
        return -1; // Not connected to master
    }

    if (uxQueueSpacesAvailable(masterTxQueue) == 0) {
        return -1; // Queue is full, cannot send message
    }

    if (xQueueSend(masterTxQueue, api_msg, 0) == pdPASS) {
        return 0; // Successfully sent to the queue
    } else {
        return -1; // Failed to send to the queue
    }
}

static void task_network_check(void *pvParameters) {
    init_networking_1g();
    init_networking_100m();

    time_from_boot("NETWORK INITIALIZED");
    ethernetif_wait_linkup(&netif_100m, ETHERNETIF_WAIT_FOREVER);
    time_from_boot("ENET 100M LINK UP");

    while (1) {
#ifndef DEBUG
        if (get_time() - last_active_time_master > ACTIVE_TIMEOUT) {
            platform_reboot(0);
        }
#endif

        check_networking_1g();
        check_networking_100m();
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

static void task_network_slave(void *pvParameters) {
    int size = 0;
    static uint8_t rx_buffer[PROTOCOL_BUFFER_SIZE];
    struct sockaddr_in source_addr;
    while (1) {
        size = socket_receive_data_slave(rx_buffer, PROTOCOL_BUFFER_SIZE, &source_addr);
    }
}

static void task_network_master(void *pvParameters) {
    int size = 0;
    static uint8_t rx_buffer[PROTOCOL_BUFFER_SIZE];
    struct sockaddr_in source_addr;
    while (1) {
        size = socket_receive_data_master(rx_buffer, PROTOCOL_BUFFER_SIZE, &source_addr);
    }
}

static void task_network_master_tx(void *pvParameters) {
    api_message_t msg;
    while (1) {
        if (xQueueReceive(masterTxQueue, &msg, pdMS_TO_TICKS(1)) == pdPASS) {
            NetworkMessage_Send(NETWORK_MESSAGE_MASTER, &msg, &conn.remote_addr);
        }
    }
}

static void task_network_slave_tx(void *pvParameters) {
    api_message_t msg;
    while (1) {
        if (xQueueReceive(slaveTxQueue, &msg, pdMS_TO_TICKS(1)) == pdPASS) {
            NetworkMessage_Send(NETWORK_MESSAGE_SLAVE, &msg, &slave_addr);
        }
    }
}

static void task_network_telemetry(void *pvParameters) {
    while (1) {
        send_telemetry();
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

void init_network(void) {
    init_enet_100m();
    init_enet_1g();

    tcpip_init(NULL, NULL);
    HAL_GpioPreInit();
    masterTxQueue = xQueueCreate(200, sizeof(api_message_t));
    slaveTxQueue = xQueueCreate(200, sizeof(api_message_t));

    // Initialize the slave address
    slave_addr.sin_family      = AF_INET;
    slave_addr.sin_addr.s_addr = inet_addr(NETWORK_SLAVE_IP);
    slave_addr.sin_port        = htons(UDP_PORT);
    slave_addr.sin_len         = sizeof(struct sockaddr_in);

    if (xTaskCreate(task_network_check, "network_check", 2048L / sizeof(StackType_t), NULL, configMAX_PRIORITIES - 5, NULL) != pdPASS) {
        LWIP_ASSERT("network_check(): Task creation failed.", 0);
    }

    if (xTaskCreate(task_network_slave, "network_slave", 4096L / sizeof(StackType_t), NULL, configMAX_PRIORITIES - 6, NULL) != pdPASS) {
        LWIP_ASSERT("network_slave(): Task creation failed.", 0);
    }

    if (xTaskCreate(task_network_master, "network_master", 4096L / sizeof(StackType_t), NULL, configMAX_PRIORITIES - 6, NULL) != pdPASS) {
        LWIP_ASSERT("network_master(): Task creation failed.", 0);
    }

    if (xTaskCreate(task_network_slave_tx, "network_slave_tx", 4096L / sizeof(StackType_t), NULL, configMAX_PRIORITIES - 6, NULL) != pdPASS) {
        LWIP_ASSERT("network_slave(): Task creation failed.", 0);
    }

    if (xTaskCreate(task_network_master_tx, "network_master_tx", 4096L / sizeof(StackType_t), NULL, configMAX_PRIORITIES - 6, NULL) != pdPASS) {
        LWIP_ASSERT("network_master(): Task creation failed.", 0);
    }

    if (xTaskCreate(task_network_telemetry, "network_telemetry", 2048L / sizeof(StackType_t), NULL, configMAX_PRIORITIES - 3, NULL) != pdPASS) {
        LWIP_ASSERT("network_telemetry(): Task creation failed.", 0);
    }
}