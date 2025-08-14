#ifndef _PROTOCOL_GCS_H
#define _PROTOCOL_GCS_H
#include <stdint.h>

typedef enum gcs_type {
    GCS_NONE = 0,
    GCS_101  = 1,
    GCS_DS4  = 2,
} gcs_type_t;

typedef struct __attribute__((packed)) gcs_101_state {
    uint8_t gcsType;

    uint8_t leftFrontButton : 1;
    uint8_t rightFrontButton : 1;
    uint8_t leftRearButton : 1;
    uint8_t rightRearButton : 1;
    uint8_t rightSideButton : 1;
    uint8_t bandToggleSwitch1 : 1;
    uint8_t bandToggleSwitch2 : 1;
    uint8_t leftLowButton1 : 1;
    uint8_t leftLowButton2 : 1;
    uint8_t leftLowButton3 : 1;
    uint8_t rightLowButton1 : 1;
    uint8_t rightLowButton2 : 1;
    uint8_t rightLowButton3 : 1;

    int8_t leftStickY;
    int8_t leftStickX;
    int8_t rightStickY;
    int8_t rightStickX;

    int8_t leftRollerSwitch1;
    int8_t leftRollerSwitch2;
    int8_t rightRollerSwitch1;
} gcs_101_state_t;

typedef struct __attribute__((packed)) gcs_ds4_state {
    uint8_t gcsType;

    uint8_t triangle : 1;
    uint8_t circle : 1;
    uint8_t square : 1;
    uint8_t x : 1;
    uint8_t arrow_up : 1;
    uint8_t arrow_right : 1;
    uint8_t arrow_down : 1;
    uint8_t arrow_left : 1;
    uint8_t center : 1;
    uint8_t share : 1;
    uint8_t options : 1;
    uint8_t r1 : 1;
    uint8_t l1 : 1;
    uint8_t r2 : 1;
    uint8_t l2 : 1;

    int8_t leftStickY;
    int8_t leftStickX;
    int8_t rightStickY;
    int8_t rightStickX;
    int8_t flash_light;
   
} gcs_ds4_state_t;

#endif // _PROTOCOL_GCS_H
