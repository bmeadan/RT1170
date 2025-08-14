#pragma once
#include <stdint.h>

/**
 * Interface for Encoders
 */
class IEncoder {
  public:
    virtual void init() = 0;
};

/*
 * Interface for Quadrature Encoders
 */
class IQuadratureEncoder : public IEncoder {
  public:
    virtual int32_t getCount() = 0;
    virtual void reset()       = 0;
};

/*
 * Interface for Absolute Encoders
 */
class IAbsoluteEncoder : public IEncoder {
  public:
    virtual float getAngle() = 0;
    virtual void setZero()   = 0;
};
