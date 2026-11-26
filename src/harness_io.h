#pragma once

#include "cityel_state.h"

namespace CityElHarness {

bool begin(CityElState &car);
bool update(CityElState &car);
void syncToState(CityElState &car);

}
