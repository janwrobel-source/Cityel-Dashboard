#pragma once

#include <Arduino.h>
#include "cityel_state.h"

namespace CityElBMS {

bool begin();
bool update(CityElState &car);

}
