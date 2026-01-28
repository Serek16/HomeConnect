#include "PCPower.h"
#include <Arduino.h>

PCPower::PCPower(uint8_t powerSWPin, uint8_t powerLEDPin,
                 uint16_t shortPressMs, uint16_t longPressMs)
  : powerSWPin(powerSWPin), powerLEDPin(powerLEDPin),
    shortPressMs(shortPressMs), longPressMs(longPressMs) {
}

PCPower::PCPower(uint8_t powerSWPin, uint8_t powerLEDPin)
  : PCPower(powerSWPin, powerLEDPin, 100, 5000) {}

PCPower::~PCPower() {
}

bool PCPower::isOn() const {
  int inverted_state = digitalRead(powerLEDPin);  // rezystor podciągający odwraca stan
  return inverted_state ? false : true;
}

bool PCPower::turnOn() const {
  if (isOn()) {
    return false;
  }
  pressShort();
  return true;
}

bool PCPower::turnOff() const {
  if (!isOn()) {
    return false;
  }
  pressLong();
  return true;
}

void PCPower::pressShort() const {
  digitalWrite(powerSWPin, HIGH);
  delay(shortPressMs);
  digitalWrite(powerSWPin, LOW);
}

void PCPower::pressLong() const {
  digitalWrite(powerSWPin, HIGH);
  delay(longPressMs);
  digitalWrite(powerSWPin, LOW);
}
