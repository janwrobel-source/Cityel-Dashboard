#pragma once

#include "cityel_state.h"

namespace CityElDemo {

void applyDemoPreset(CityElState &car);
void begin(CityElState &car, bool seed_on_boot = false);
bool update(CityElState &car);

}
