#include "RSBL8512.h"
#include "fsl_gpio.h"
#include "project.h"
#include "flash/flash.h"

extern "C" {
#include "lights/lights.h"
#include "platform/platform.h"
#include <FreeRTOS.h>
#include <stdio.h>
#include <task.h>
}

static SMS_STS *sts = nullptr;

#define ANGLE_LIMIT 985
#define ANGLE_SPEED 4095 / 100 / 5

static void stservo_start(u8 ID, s16 Position, s16 Speed, u8 ACC) {
    u8 IDS[1]        = {ID};
    s16 Positions[1] = {Position};
    u16 Speeds[1]    = {Speed};
    u8 ACCs[1]       = {255};
    // sts->SyncWritePosEx(IDS, 1, Positions, Speeds, ACCs);
    while (!sts->WriteSpe(ID, Speed, 255)) {
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}
static void stservo_stop(u8 ID, s16 Position) {
    u8 IDS[1]        = {ID};
    u16 Speeds[1]    = {0};
    s16 Positions[1] = {0};
    u8 ACCs[1]       = {255};
    // sts->SyncWritePosEx(IDS, 1, Positions, Speeds, ACCs);
    sts->WriteSpe(ID, 0, 255);
}

void RSBL8512::init() {
    if (sts == nullptr) {
        sts = new SMS_STS();
    }

    flash_get_calibration(sts_serial_id, &calibration);
}

void RSBL8512::calibrate() {
    calibration = raw_pos > 0 ? (uint16_t)raw_pos : 0;
    flash_set_calibration(sts_serial_id, calibration);
    acc_pos = 0;
    last_pos = -1;
}

float RSBL8512::getPosition() {
    float angle = (acc_pos / 4095.0f / 3.0f) * 360.0f;
    if (angle < -180) {
        angle += 360;
    } else if (angle > 180) {
        angle -= 360;
    }
    return angle;
}

void RSBL8512::moveTo(float target) {
   
    float diff = target - getPosition();
    static float pCoeff = 2.0f;

    if(abs(diff) > 0.5f){
        stservo_start(sts_serial_id, 2048, (s16)(pCoeff *(diff*4095*3/360)), 255);
    }else{
        stservo_start(sts_serial_id, 2048, 0, 255);
    }
    
    return;
}

void RSBL8512::setAxesMode(uint8_t mode){
    if (mode == 0) {
        axesMode = AXES_MODE_MANUAL;
    } else {
        axesMode = AXES_MODE_AUTO;
    }
}

void RSBL8512::loop(uint32_t dt_ms) {

    if (get_time() - last_cycle_time < 100) {
        return;
    }

    if (!is_init) {
      
        while (!sts->WheelMode(sts_serial_id, SMS_MODE_CONTINUOUS)) {
            return;
        }
        is_init = true;
    } else {

        while ((raw_pos = sts->ReadPos(sts_serial_id)) == -1) {
            vTaskDelay(pdMS_TO_TICKS(10));
        }
        
        int32_t pos = raw_pos - calibration;
        while (pos < 0) {
            pos += 4095;
        }

        if (pos < 300 || pos > 3700) {

            if (!position_known) {
                if (pos > 3300) {
                    acc_pos = pos - 4095;
                } else if (pos < 700) {
                    acc_pos = pos;
                }
            }

            position_known = true;
        }

        if (position_known) {
            if (last_pos == -1) {
                last_pos = pos;
            }

            int diff = pos - last_pos;

            acc_pos += diff;
            if (diff > 2000) {
                acc_pos -= 4095;
            } else if (diff < -2000) {
                acc_pos += 4095;
            }

            last_pos = pos;
        }

        if (!position_known && !torque_limited) {
            // while (!sts->writeWord(sts_serial_id, SMS_STS_TORQUE_LIMIT_L, 500)) {
            //     vTaskDelay(pdMS_TO_TICKS(10));
            // }
            torque_limited = true;
        } else if (position_known && torque_limited) {
            // while (!sts->writeWord(sts_serial_id, SMS_STS_TORQUE_LIMIT_L, 0)) {
            //     vTaskDelay(pdMS_TO_TICKS(10));
            // }
            torque_limited = false;
        }

        while ((last_curr = sts->ReadCurrent(sts_serial_id)) == -1) {
            vTaskDelay(pdMS_TO_TICKS(10));
        }

        uint16_t curr_limit = 1000;

        if (torque_limited) {
            curr_limit = 80;
        }

        if(set_value == 0){
            is_cmd_zero = true;
        }else{
            is_cmd_zero = false;
        }

        if(set_value > 0){
            last_torque = get_time();
        }

        if ( sto || last_curr > curr_limit || (set_value == 0 && last_curr > 200 && get_time() - last_torque > 3000)) {
            while (sts->writeWord(sts_serial_id, SMS_STS_TORQUE_ENABLE, 0) == -1) {
                vTaskDelay(pdMS_TO_TICKS(10));
            }
            last_torque = 0;
        
        } else {

            if (position_known) {
                if (sts_serial_id == 2) {
                    if (acc_pos < -2900 && set_value < 0) {
                        set_value = 0;
                    } else if (acc_pos > 2800 && set_value > 0) {
                        set_value = 0;
                    }

                    if (acc_pos < -2400 && set_value < 0) {
                        set_value /= 3;
                    } else if (acc_pos > 2500 && set_value > 0) {
                        set_value /= 3;
                    }

                } else {
                    if (acc_pos < -2700 && set_value < 0) {
                        set_value = 0;
                    } else if (acc_pos > 2200 && set_value > 0) {
                        set_value = 0;
                    }

                    if (acc_pos < -2400 && set_value < 0) {
                        set_value /= 3;
                    } else if (acc_pos > 1900 && set_value > 0) {
                        set_value /= 3;
                    }
                }
            }
            uint8_t acc = 255;

            if (torque_limited) {
                acc = 100;
                set_value /= 3;
            }
            
          
            if(axesMode == AXES_MODE_MANUAL){
                stservo_start(sts_serial_id, 2048, set_value * ANGLE_SPEED, acc);
            }else{
                if(position_known && is_cmd_zero){
                    if(sts_serial_id == 2){
                        moveTo(0);
                    }else{

                        if(!torque_disable){
                            while (sts->writeWord(sts_serial_id, SMS_STS_TORQUE_ENABLE, 0) == -1) {
                                vTaskDelay(pdMS_TO_TICKS(10));
                            }
                            torque_disable = true;
                        }

                    }
                }else{
                    torque_disable = false;
                    stservo_start(sts_serial_id, 2048, set_value * ANGLE_SPEED, 255);
                }
            }
        }
    }

    last_cycle_time = get_time();
}

void RSBL8512::setValue(int8_t value) {
    // TODO - get torque disabled / sts from config
    sto = false;
    set_value = value;
}
