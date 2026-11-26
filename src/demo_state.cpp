#include <Arduino.h>
#include "demo_state.h"
#include "button_input.h"
#include "config.h"
#include "fardriver_sif.h"
#include "harness_io.h"
#include "pins.h"
#include "rtc_clock.h"

namespace {

constexpr uint32_t EXIT_DEBOUNCE_MS = 25;

bool demo_active = false;
bool exit_raw_pressed = false;
bool exit_stable_pressed = false;
bool exit_hold_handled = false;
uint32_t exit_raw_changed_ms = 0;
uint32_t exit_pressed_ms = 0;

void stepDemoValues(CityElState &car) {
    car.power_off = false;

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
    car.rtc_battery_low = !car.rtc_battery_low;

    Serial.printf("[DEMO] Speed: %u km/h | Current: %+dA | Gear: %c\n",
                  car.speed_kmh, car.current_amps, car.gear);
}

}

namespace CityElDemo {

bool isActive() {
    return demo_active;
}

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
    car.clock_set_mode      = ClockSetMode::NONE;
    car.rtc_battery_low     = false;
    car.power_off           = false;
    demo_active             = true;
}

void begin(CityElState &car, bool seed_on_boot) {
    pinMode(BTN_EXIT_PIN, INPUT_PULLUP);
    pinMode(BTN_MENU_PIN, INPUT_PULLUP);

    exit_raw_pressed = (digitalRead(BTN_EXIT_PIN) == LOW);
    exit_stable_pressed = exit_raw_pressed;
    exit_hold_handled = exit_raw_pressed;
    exit_raw_changed_ms = millis();
    exit_pressed_ms = exit_raw_changed_ms;

    if (seed_on_boot) {
        applyDemoPreset(car);
        CityElRTC::setEsp32InternalClock(car.clock_hour, car.clock_min, 0);
    }
}

bool update(CityElState &car) {
    if (!ENABLE_BENCH_DEMO_BUTTONS) return false;

    uint32_t now = millis();
    bool raw_exit = (digitalRead(BTN_EXIT_PIN) == LOW);
    if (raw_exit != exit_raw_pressed) {
        exit_raw_pressed = raw_exit;
        exit_raw_changed_ms = now;
    }

    if (exit_raw_pressed != exit_stable_pressed &&
        (now - exit_raw_changed_ms >= EXIT_DEBOUNCE_MS)) {
        exit_stable_pressed = exit_raw_pressed;
        if (exit_stable_pressed) {
            exit_pressed_ms = now;
            exit_hold_handled = false;
        } else if (demo_active && !exit_hold_handled) {
            stepDemoValues(car);
            return true;
        }
    }

    if (exit_stable_pressed && !exit_hold_handled &&
        (now - exit_pressed_ms >= DEMO_ACTIVATE_HOLD_MS)) {
        exit_hold_handled = true;
        if (!demo_active) {
            applyDemoPreset(car);
            // Seed only ESP32 RAM clock so the hardware DS3231 RTC is never overwritten
            CityElRTC::setEsp32InternalClock(car.clock_hour, car.clock_min, 0);
            Serial.println("[DEMO] Activated bench demo mode (tap EXIT to step, MENU for power-off, hold EXIT 1s to exit)");
        } else {
            demo_active = false;
            car = CityElState{};
            CityElRTC::restoreClock(car);
            CityElFarDriver::syncToState(car);
            CityElHarness::syncToState(car);
            Serial.println("[DEMO] Exited bench demo mode -> Restored live vehicle telemetry");
        }
        return true;
    }

    if (demo_active && CityElButtons::pressed(BTN_MENU_PIN, 0, 0)) {
        car.power_off = !car.power_off;
        car.clock_set_mode = ClockSetMode::NONE;
        Serial.printf("[DEMO] Power-off screen toggled: %s\n", car.power_off ? "ON" : "OFF");
        return true;
    }

    return false;
}

}
