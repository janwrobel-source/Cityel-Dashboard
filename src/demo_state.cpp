#include <Arduino.h>
#include "demo_state.h"
#include "button_input.h"
#include "rtc_clock.h"
#include "pins.h"

namespace {

bool demo_active = false;

}

namespace CityElDemo {

void applyDemoPreset(CityElState &car) {
    car.speed_kmh           = 48;
    car.odometer_km         = 42859;
    car.odo_tenths          = 3;
    car.battery_soc         = 78;
    car.current_amps        = -42;
    car.gear                = 'F';
    car.fan_level           = 1;
    car.heat_level          = 1;
    car.turn_left           = true;
    car.turn_right          = true;
    car.hazard_lights       = true;
    car.parking_lights      = true;
    car.low_beam            = true;
    car.high_beam           = true;
    car.handbrake           = true;
    car.brake_fluid_err     = true;
    car.charger_conn        = true;
    car.overheat_controller = true;
    car.overheat_bms        = false;
    car.clock_hour          = 14;
    car.clock_min           = 32;
    demo_active             = true;
}

void begin(CityElState &car, bool seed_on_boot) {
    pinMode(BTN_EXIT_PIN, INPUT_PULLUP);
    pinMode(BTN_MENU_PIN, INPUT_PULLUP);

    if (seed_on_boot) {
        applyDemoPreset(car);
    }
}

bool update(CityElState &car) {
    bool menu_pressed = CityElButtons::pressed(BTN_MENU_PIN, 0, 0);
    if (menu_pressed) {
        car.power_off = !car.power_off;
        car.clock_set_mode = ClockSetMode::NONE;
        Serial.printf("[DEMO] Power-off screen toggled: %s\n", car.power_off ? "ON" : "OFF");
        return true;
    }

    bool exit_pressed = CityElButtons::pressed(BTN_EXIT_PIN, 0, 0);
    if (exit_pressed) {
        car.power_off = false;
        if (!demo_active) {
            applyDemoPreset(car);
            CityElRTC::setTime(car.clock_hour, car.clock_min);
            Serial.println("[DEMO] Activated bench demo preset");
            return true;
        }

        car.speed_kmh = (car.speed_kmh + 7) % 68;
        car.odo_tenths = (car.odo_tenths + 1) % 10;
        if (car.odo_tenths == 0) car.odometer_km++;

        car.current_amps = (car.current_amps <= -175) ? 45 : (car.current_amps - 30);
        car.battery_soc = (car.battery_soc <= 15) ? 100 : (car.battery_soc - 10);

        car.gear = (car.gear == 'F') ? 'N' : ((car.gear == 'N') ? 'R' : 'F');
        car.fan_level = (car.fan_level + 1) % 3;
        car.heat_level = (car.heat_level + 1) % 3;

        car.turn_left = !car.turn_left;
        car.turn_right = !car.turn_right;
        car.hazard_lights = (car.turn_left && car.turn_right);
        car.overheat_controller = !car.overheat_controller;

        Serial.printf("[DEMO] Speed: %u km/h | Current: %+dA | Gear: %c\n",
                      car.speed_kmh, car.current_amps, car.gear);
        return true;
    }

    return false;
}

}
