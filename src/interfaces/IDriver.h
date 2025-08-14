#pragma once
#include <stdbool.h>
#include <stdint.h>

/**
 * Interface for motor drivers
 */
class IDriver {
  public:
    virtual void
    init()                                    = 0;
    virtual void setDirection(bool direction) = 0;
    virtual void setPWM(uint8_t duty_cycle)   = 0;
};
