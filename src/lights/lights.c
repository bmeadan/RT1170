#include "lights.h"
#include "FreeRTOS.h"
#include "board.h"
#include "pin_gpio.h"
#include "network/network.h"
#include "network/network_entity.h"
#include "platform/platform.h"
#include "lpadc/lpadc.h"

void set_red_led(bool on) {
#ifdef BOARD_LED_PIN_RED
    GPIO_PinWrite(BOARD_LED_GPIO, BOARD_LED_PIN_RED, on ? 1U : 0U);
#endif
}

void set_green_led(bool on) {
#ifdef BOARD_LED_PIN_GREEN
    GPIO_PinWrite(BOARD_LED_GPIO, BOARD_LED_PIN_GREEN, on ? 1U : 0U);
#endif
}

void set_flashlights(uint8_t config) {
    uint8_t id = get_network_entity_id();
#ifdef FLASHLIGHTS_ORDER_BY_ENTITY_ID
    if (id == 1) {
        GPIO_PinWrite(FLASHLIGHT_GPIO, FLASHLIGHT_PIN_1, config & 0x01 ? 1U : 0U);
        GPIO_PinWrite(FLASHLIGHT_GPIO, FLASHLIGHT_PIN_2, config & 0x01 ? 1U : 0U);
    } else if (id == 2) {
        GPIO_PinWrite(FLASHLIGHT_GPIO, FLASHLIGHT_PIN_1, config & 0x02 ? 1U : 0U);
        GPIO_PinWrite(FLASHLIGHT_GPIO, FLASHLIGHT_PIN_2, config & 0x02 ? 1U : 0U);
    }
#else
    if (id == 1) {
        GPIO_PinWrite(FLASHLIGHT_GPIO, FLASHLIGHT_PIN_1, config & 0x01 ? 1U : 0U);
        GPIO_PinWrite(FLASHLIGHT_GPIO, FLASHLIGHT_PIN_2, config & 0x02 ? 1U : 0U);
    } else if (id == 2) {
        GPIO_PinWrite(FLASHLIGHT_GPIO, FLASHLIGHT_PIN_1, config & 0x01 ? 1U : 0U);
        GPIO_PinWrite(FLASHLIGHT_GPIO, FLASHLIGHT_PIN_2, config & 0x01 ? 1U : 0U);
    }
#endif
}

void set_rgb_led(led_color_t color) {
#ifdef RGB_LED_GPIO
    GPIO_PinWrite(RGB_LED_GPIO, RGB_LED_PIN_RED, color & 0x01 ? 1U : 0U);
    GPIO_PinWrite(RGB_LED_GPIO, RGB_LED_PIN_GREEN, color & 0x02 ? 1U : 0U);
    GPIO_PinWrite(RGB_LED_GPIO, RGB_LED_PIN_BLUE, color & 0x04 ? 1U : 0U);
#endif
}

static void task_lights(void *pvParameters) {
    static uint8_t ledSubState = 0;
    led_color_t color;
    bool connected, battery_ok;

    set_rgb_led(LED_WHITE); // Set initial RGB LED to white
    vTaskDelay(pdMS_TO_TICKS(300)); // Delay to indicate the system is starting

    while (1) {
        connected = is_master_connected();
        battery_ok = get_battery_level() >= 2000; // 20% of MAX_BATTERY_VOLTAGE

        switch (platform_state)
        {
        case PLATFORM_STATE_INIT:
            color = LED_WHITE;
            break;
        case PLATFORM_STATE_FLASHING:
            color = ledSubState ? LED_MAGENTA : LED_OFF;
            break;
        case PLATFORM_STATE_SHUTDOWN:
            color = LED_MAGENTA;
            break;
        case PLATFORM_STATE_RUNNING:
            if (connected && battery_ok) {
                color = LED_GREEN;
            } else if (connected && !battery_ok) {
                color = LED_RED;
            } else if (!connected && battery_ok) {
                color = ledSubState ? LED_GREEN : LED_BLUE;
            } else {
                color = ledSubState ? LED_RED : LED_BLUE;
            }
            break;
        default:
            color = LED_OFF;
            break;
        }

        set_rgb_led(color);
        ledSubState = !ledSubState;
        vTaskDelay(pdMS_TO_TICKS(200));
    }
    vTaskDelete(NULL);
}

void init_lights(void) {
    xTaskCreate(task_lights, "lights", 1024U, NULL, configMAX_PRIORITIES - 2, NULL);
}