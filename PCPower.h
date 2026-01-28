#ifndef PC_POWER_H
#define PC_POWER_H

#include <stdint.h>

class PCPower {
public:
  PCPower(uint8_t powerSWPin, uint8_t powerLEDPin, uint16_t shortPressMs, uint16_t longPressMs);
  PCPower(uint8_t powerSWPin, uint8_t powerLEDPin);
  PCPower();
  ~PCPower();

  bool isOn() const;
  bool turnOn() const;
  bool turnOff() const;
  void pressShort() const;
  void pressLong() const;

private:
  uint8_t shortPressMs = 100;
  uint16_t longPressMs = 5000;

  uint8_t powerSWPin = 18;
  uint8_t powerLEDPin = 33;
};

#endif
