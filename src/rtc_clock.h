#pragma once

#include <Arduino.h>
#include "cityel_state.h"

namespace CityElRTC {

void setEsp32InternalClock(uint8_t hour, uint8_t minute, uint8_t second = 0);
void readEsp32InternalClock(uint8_t &hour, uint8_t &minute);
bool setTime(uint8_t hour, uint8_t minute);
bool syncFromDS3231();
bool begin(CityElState &car);
bool update(CityElState &car);

}
