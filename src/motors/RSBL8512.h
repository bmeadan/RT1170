#pragma once
#include "fsl_common.h"
#include "interfaces/IMotor.h"
#include "stservo/SMS_STS.h"

typedef enum {
  AXES_MODE_MANUAL = 0,
  AXES_MODE_AUTO   = 1
}AxesMode;

/**
 * WaveShare RSBL85-12 stepper motor driver
 * @link https://www.waveshare.com/rsbl85-12.htm
 */
class RSBL8512 : public IMotor {

  public:
    RSBL8512(const uint8_t sts_serial_id)
        : sts_serial_id(sts_serial_id) {}

    void init() override;
    void loop(uint32_t dt_ms) override;
    void calibrate() override;
    void emergencyStop() override {
        sto = true;
        set_value = 0;
    }
  
    void setValue(int8_t value);
    void setAxesMode(uint8_t mode);
    void setTorqueDisable(uint8_t disable);
    float getPosition();  
    
  private:
    typedef enum mode_t {
        SMS_MODE_POS        = 0,
        SMS_MODE_CONTINUOUS = 1,
        SMS_MODE_NONE       = 0xFF,
    };

    const uint8_t sts_serial_id;

    volatile int8_t set_value  = 0;
    volatile int8_t curr_value = 0;

    bool is_cmd_zero = false;

    bool is_init = false;

    uint32_t last_cycle_time = 0;
    
    bool position_known = false;
    bool torque_limited = false;

    uint16_t calibration = 0;
    int32_t raw_pos     = -1;
    int32_t last_pos    = -1;
    
    bool torque_disable = false;
    bool sto = true;

    volatile int32_t last_curr = 0;
    volatile int32_t acc_pos   = 0;
    volatile int32_t last_torque = 0;
    volatile AxesMode axesMode = AXES_MODE_AUTO;

    void moveTo(float target);
};
