#include "serial/serial_lpuart_232.h"
#include <stdio.h>
#include "interfaces/IMotor.h"

class RoboClaw : public IMotor {
public:
    RoboClaw(const uint8_t motor_num)
        : motor_num(motor_num) {}

    void init() override;
    void loop(uint32_t dt_ms) override;
    void calibrate() override;
    void emergencyStop() override {
        last_value = 0;
    }

    void setValue(int8_t value);
private:
    const uint8_t motor_num;
    volatile uint8_t last_value;
    
};