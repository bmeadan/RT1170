#include "RoboClaw.h"
#include "FreeRTOS.h"
#include "task.h"

void RoboClaw:: init()
{
     init_serial_lpuart_232();
}

void RoboClaw::loop(uint32_t dt_ms) {
    uint8_t cmd1[1];
    cmd1[0] = {last_value};
    serial_lpuart_send_bytes_232(cmd1, 1);
}

void RoboClaw::calibrate() {
    // TODO: Incorporate encoder calibration logic
}

void RoboClaw::setValue(int8_t value) {
    uint8_t val = 0;

    if(value > 99){
        value = 99;
    } else if(value < -99){
        value = -99;
    }

    if(value >= 0){
        val = (127 - 64)*((float)abs(value)/100)+64;
    } else if(value <= 0){
        val = (1 - 64)*((float)abs(value)/100)+64;
    }

    if(motor_num > 0){
        val += 128;
    }

    last_value = val;
}

