#pragma once

/**
 * Interface for Motors
 */
class IMotor {
  public:
    virtual void init() = 0;
    virtual void loop(uint32_t dt_ms) {} // Optional
    virtual void calibrate() {} // Optional
    virtual void emergencyStop() = 0;
};
