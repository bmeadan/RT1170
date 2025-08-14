#ifndef __MOTORS_H__
#define __MOTORS_H__
#include <stdint.h>

#ifdef __cplusplus
#include <vector>
#include "interfaces/IMotor.h"
extern "C" {
#endif

typedef struct motors_action
{
    int8_t drive;
    int8_t yaw;
    int8_t pitch;
    uint8_t auto_neutral_joints;
    uint8_t drive_pulse;
    uint8_t flip_mode;
} motors_action_t;

void init_motors(void);
void calibrate_motors(void);
void emergency_stop_motors(void);
extern void set_motors(motors_action_t *action);

#ifdef __cplusplus
}
#endif

#endif /* _MOTORS_H_ */