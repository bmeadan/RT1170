#include "AlonDriveMotor.h"
#include "motors.h"
#include "platform/platform.h"
#include <cstdlib>

void AlonDriveMotor::init() {
    driver.init();
}

void AlonDriveMotor::loop(uint32_t dt_ms) {
    if (isPulsed && get_time() % 200 < 50) {
        driver.setPWM(0);
    } else {
        driver.setDirection(inverse ? lastValue > 0 : lastValue < 0);
        driver.setPWM(abs(lastValue));
    }
}
