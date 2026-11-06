#pragma once

#include <stdint.h>

enum class ClockSetMode : uint8_t {
    NONE = 0,
    HOURS = 1,
    MINUTES = 2
};

struct CityElState {
    uint8_t      speed_kmh           = 0;
    uint32_t     odometer_km         = 0;
    uint8_t      odo_tenths          = 0;
    uint8_t      battery_soc         = 0;
    int16_t      current_amps        = 0;
    char         gear                = 'N';
    uint8_t      fan_level           = 0;
    uint8_t      heat_level          = 0;
    bool         turn_left           = false;
    bool         turn_right          = false;
    bool         hazard_lights       = false;
    bool         parking_lights      = false;
    bool         low_beam            = false;
    bool         high_beam           = false;
    bool         handbrake           = false;
    bool         brake_fluid_err     = false;
    bool         charger_conn        = false;
    bool         overheat_controller = false;  // FarDriver controller fault / overheat
    bool         overheat_bms        = false;  // JK-BMS battery / MOSFET overheat
    uint8_t      clock_hour          = 0;
    uint8_t      clock_min           = 0;
    ClockSetMode clock_set_mode      = ClockSetMode::NONE;
    bool         power_off           = false;

    constexpr bool isOverheated() const {
        return overheat_controller || overheat_bms;
    }
};

