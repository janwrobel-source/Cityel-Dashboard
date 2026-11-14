#include "harness_io.h"
#include "pins.h"

#include <Arduino.h>
#include <Wire.h>

namespace {

constexpr bool USE_TCA9555 = true;
constexpr uint8_t EXPANDER_I2C_ADDR = 0x20;
constexpr bool HARNESS_DEBUG_SERIAL = true;

bool expander_online = false;
uint16_t stable_state = 0xFFFF;
uint16_t last_raw_read = 0xFFFF;
uint32_t last_bounce_ms = 0;
uint32_t last_poll_ms = 0;

uint16_t readRaw16() {
    uint8_t reg = USE_TCA9555 ? 0x00 : 0x12;
    Wire.beginTransmission(EXPANDER_I2C_ADDR);
    Wire.write(reg);
    if (Wire.endTransmission(false) != 0) return 0xFFFF;

    if (Wire.requestFrom((uint8_t)EXPANDER_I2C_ADDR, (uint8_t)2) == 2) {
        uint8_t port0 = Wire.read();
        uint8_t port1 = Wire.read();
        return (uint16_t)port0 | ((uint16_t)port1 << 8);
    }
    return 0xFFFF;
}

}

namespace CityElHarness {

bool begin() {
    Wire.beginTransmission(EXPANDER_I2C_ADDR);
    expander_online = (Wire.endTransmission() == 0);

    if (!expander_online) {
        Serial.println("[IO] TCA9555/MCP23017 not found at 0x20 -> Harness inputs idle (Bench Mode).");
        return false;
    }

    if (USE_TCA9555) {
        Wire.beginTransmission(EXPANDER_I2C_ADDR);
        Wire.write(0x06);
        Wire.write(0xFF);
        Wire.write(0xFF);
        Wire.endTransmission();
        Serial.println("[IO] TCA9555 Online at 0x20 (16 optocoupler inputs active-LOW)");
    } else {
        Wire.beginTransmission(EXPANDER_I2C_ADDR);
        Wire.write(0x00);
        Wire.write(0xFF);
        Wire.write(0xFF);
        Wire.endTransmission();

        Wire.beginTransmission(EXPANDER_I2C_ADDR);
        Wire.write(0x0C);
        Wire.write(0xFF);
        Wire.write(0xFF);
        Wire.endTransmission();
        Serial.println("[IO] MCP23017 Online at 0x20 (16 internal pull-ups enabled)");
    }

    stable_state = readRaw16();
    last_raw_read = stable_state;
    last_bounce_ms = millis();
    last_poll_ms = 0;
    return true;
}

bool update(CityElState &car) {
    if (!expander_online) return false;

    if (millis() - last_poll_ms < 15) return false;
    last_poll_ms = millis();

    uint16_t raw = readRaw16();
    if (raw != last_raw_read) {
        last_bounce_ms = millis();
        last_raw_read = raw;
    }

    if ((millis() - last_bounce_ms) < 25 || raw == stable_state) {
        return false;
    }

    stable_state = raw;
    auto active = [&](uint8_t bit) -> bool {
        return (stable_state & (1U << bit)) == 0;
    };

    car.turn_left       = active(HARNESS_BIT_TURN_LEFT);
    car.turn_right      = active(HARNESS_BIT_TURN_RIGHT);
    car.parking_lights  = active(HARNESS_BIT_PARKING_LT);
    car.low_beam        = active(HARNESS_BIT_LOW_BEAM);
    car.high_beam       = active(HARNESS_BIT_HIGH_BEAM);
    car.handbrake       = active(HARNESS_BIT_HANDBRAKE);
    car.brake_fluid_err = active(HARNESS_BIT_BRAKE_FLUID);
    car.hazard_lights   = active(HARNESS_BIT_HAZARD);

    bool fan1  = active(HARNESS_BIT_FAN_1);
    bool fan2  = active(HARNESS_BIT_FAN_2);
    bool heat1 = active(HARNESS_BIT_HEAT_1);
    bool heat2 = active(HARNESS_BIT_HEAT_2);

    car.fan_level = fan2 ? 2 : (fan1 ? 1 : 0);
    car.heat_level = heat2 ? 2 : (heat1 ? 1 : 0);

    if (HARNESS_DEBUG_SERIAL) {
        Serial.printf("[IO] Bits: 0x%04X | L:%d R:%d Haz:%d P:%d Lo:%d Hi:%d Brk:%d Fl:%d Chg:%d Fan:%u Heat:%u\n",
            stable_state, car.turn_left, car.turn_right, car.hazard_lights, car.parking_lights,
            car.low_beam, car.high_beam, car.handbrake, car.brake_fluid_err,
            car.charger_conn, car.fan_level, car.heat_level);
    }

    return true;
}

}
