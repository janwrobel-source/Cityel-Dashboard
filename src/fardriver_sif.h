#pragma once

#include <Arduino.h>
#include "cityel_state.h"

namespace CityElFarDriver {

void begin(CityElState &car);
bool update(CityElState &car);
void flushOdometer();

}
