#pragma once
#include "interfaces/IDriver.h"
#include "interfaces/IMotor.h"

class AlonDriveMotor : public IMotor {
private:
  public:
    AlonDriveMotor(IDriver &driver, const bool inverse): driver(driver), inverse(inverse) {}
    void init() override;
    void loop(uint32_t dt_ms) override;
    
    void setValue(int8_t value) {
      lastValue = value;
    }

    void setPulse(uint8_t mode) {
      isPulsed = mode > 0;
    }

    void emergencyStop() override {
        lastValue = 0;
        isPulsed = false;
    }

  private:
    IDriver &driver;
    volatile bool isPulsed = false;
    volatile int8_t lastValue = 0;
    const bool inverse;
};