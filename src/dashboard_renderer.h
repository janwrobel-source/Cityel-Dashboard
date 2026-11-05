#pragma once

#include <Arduino.h>
#include <GxEPD2_BW.h>
#include "cityel_state.h"

using DashboardDisplay = GxEPD2_BW<GxEPD2_579_GDEY0579T93, GxEPD2_579_GDEY0579T93::HEIGHT>;

void renderDashboard(DashboardDisplay &display, const CityElState &car, bool full_refresh);