#pragma once

#include "cityel_state.h"
#include "config.h"

namespace CityElAudio {

void buildRelayWaveforms(float volume = AUDIO_CLICK_VOLUME);
bool begin(float volume = AUDIO_CLICK_VOLUME);
void playRelayClick(bool is_tick);
void updateTurnSignalClicks(const CityElState &car);

}
