#pragma once
#include <stdio.h>
#include <stdint.h>
typedef struct {
  int8_t drive, yaw, pitch;
  uint8_t auto_neutral_joints, drive_pulse, flip_mode;
} motors_action_t;
static inline void set_motors(const motors_action_t* a){
  printf("[motors] yaw=%d pitch=%d\n",(int)a->yaw,(int)a->pitch);
}
