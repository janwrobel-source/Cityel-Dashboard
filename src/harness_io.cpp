#include "harness_io.h"
#include "pins.h"

#include <Arduino.h>
#include <Wire.h>

namespace {

enum class ExpanderType : uint8_t {
    PCF8575,
    TCA9555,
    MCP23017
};

constexpr ExpanderType EXPANDER_TYPE = ExpanderType::PCF8575;
constexpr bool HARNESS_DEBUG_SERIAL = true;

bool expander_online = false;
uint16_t stable_state = 0xFFFF;
uint16_t last_raw_read = 0xFFFF;
uint32_t last_bounce_ms = 0;
uint32_t last_poll_ms = 0;
uint32_t last_latch_refresh_ms = 0;

void configureInputs() {
    if (EXPANDER_TYPE == ExpanderType::PCF8575) {
        // Quasi-bidirectional: writing 1s enables weak ~100uA pull-ups for input reading
        Wire.beginTransmission(EXPANDER_I2C_ADDR);
        Wire.write(0xFF);
        Wire.write(0xFF);
        Wire.endTransmission();
    } else if (EXPANDER_TYPE == ExpanderType::TCA9555) {
        Wire.beginTransmission(EXPANDER_I2C_ADDR);
        Wire.write(0x06); // Configuration Port 0 & 1 = all inputs
        Wire.write(0xFF);
        Wire.write(0xFF);
        Wire.endTransmission();
    } else {
        Wire.beginTransmission(EXPANDER_I2C_ADDR);
        Wire.write(0x00); // IODIRA & IODIRB = all inputs
        Wire.write(0xFF);
        Wire.write(0xFF);
        Wire.endTransmission();

        Wire.beginTransmission(EXPANDER_I2C_ADDR);
        Wire.write(0x0C); // GPPUA & GPPUB = 100k pull-ups enabled
        Wire.write(0xFF);
        Wire.write(0xFF);
        Wire.endTransmission();
    }
}

uint16_t readRaw16() {
    if (EXPANDER_TYPE != ExpanderType::PCF8575) {
        uint8_t reg = (EXPANDER_TYPE == ExpanderType::TCA9555) ? 0x00 : 0x12;
        Wire.beginTransmission(EXPANDER_I2C_ADDR);
        Wire.write(reg);
        if (Wire.endTransmission(false) != 0) return 0xFFFF;
    }

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
        Serial.printf("[IO] I2C Expander not found at 0x%02X -> Harness inputs idle (Bench Mode).\n", EXPANDER_I2C_ADDR);
        return false;
    }

    configureInputs();
    const char *chip_name = (EXPANDER_TYPE == ExpanderType::PCF8575) ? "PCF8575" :
                            (EXPANDER_TYPE == ExpanderType::TCA9555) ? "TCA9555" : "MCP23017";
    Serial.printf("[IO] %s Online at 0x%02X (16 optocoupler inputs active-LOW)\n", chip_name, EXPANDER_I2C_ADDR);

    stable_state = readRaw16();
    last_raw_read = stable_state;
    last_bounce_ms = millis();
    last_poll_ms = 0;
    last_latch_refresh_ms = millis();
    return true;
}

bool update(CityElState &car) {
    if (!expander_online) return false;

    uint32_t now = millis();
    if (now - last_poll_ms < 15) return false;
    last_poll_ms = now;

    // Periodically re-assert 0xFFFF input latches on PCF8575 so an EMI glitch can never stick a pin low
    if (EXPANDER_TYPE == ExpanderType::PCF8575 && (now - last_latch_refresh_ms >= 2000UL)) {
        last_latch_refresh_ms = now;
        configureInputs();
    }

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
