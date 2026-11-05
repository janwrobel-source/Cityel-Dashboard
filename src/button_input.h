#pragma once

#include <Arduino.h>

namespace CityElButtons {

bool pressed(uint8_t pin, uint32_t hold_delay_ms, uint32_t repeat_interval_ms);

}
