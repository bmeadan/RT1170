#ifndef _LED_H_
#define _LED_H_
#include <stdbool.h>
#include <stdint.h>

typedef enum led_color {
    LED_OFF     = 0,
    LED_RED     = 1,
    LED_GREEN   = 2,
    LED_YELLOW  = 3,
    LED_BLUE    = 4,
    LED_MAGENTA = 5,
    LED_CYAN    = 6,
    LED_WHITE   = 7
} led_color_t;

void set_red_led(bool on);
void set_green_led(bool on);
void set_rgb_led(led_color_t color);
void set_flashlights(uint8_t config);
void init_lights(void);

#endif /* _LED_H_ */