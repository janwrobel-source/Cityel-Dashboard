#pragma once

#include "cityel_state.h"

namespace CityElAudio {

void buildRelayWaveforms(float volume = 0.85f);
bool begin(float volume = 0.85f);
void playRelayClick(bool is_tick);
void updateTurnSignalClicks(const CityElState &car);

}
