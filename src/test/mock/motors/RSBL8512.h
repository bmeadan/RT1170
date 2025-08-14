#pragma once
#include <stdint.h>
class RSBL8512 {
public:
  RSBL8512() : pos(0) {}
  uint16_t getPosition() const { return pos; }
  void setSimPosition(uint16_t p) { pos = p; }
private:
  uint16_t pos;
};
