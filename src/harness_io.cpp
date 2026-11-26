#include "harness_io.h"
#include "config.h"
#include "pins.h"

#include <Arduino.h>
#include <Wire.h>

namespace {

constexpr uint8_t MAX_I2C_FAILURES = 3;

bool expander_online = false;
uint8_t consecutive_i2c_errors = 0;
uint16_t stable_state = 0xFFFF;
uint16_t last_raw_read = 0xFFFF;
uint32_t last_bounce_ms = 0;
uint32_t last_poll_ms = 0;
uint32_t last_latch_refresh_ms = 0;
uint32_t last_retry_ms = 0;

bool configureInputs() {
    if (EXPANDER_TYPE == ExpanderType::PCF8575) {
        // Quasi-bidirectional: writing 1s enables weak ~100uA pull-ups for input reading
        Wire.beginTransmission(EXPANDER_I2C_ADDR);
        Wire.write(0xFF);
        Wire.write(0xFF);
        return (Wire.endTransmission() == 0);
    }
    if (EXPANDER_TYPE == ExpanderType::TCA9555) {
        Wire.beginTransmission(EXPANDER_I2C_ADDR);
        Wire.write(0x06); // Configuration Port 0 & 1 = all inputs
        Wire.write(0xFF);
        Wire.write(0xFF);
        return (Wire.endTransmission() == 0);
    }

    Wire.beginTransmission(EXPANDER_I2C_ADDR);
    Wire.write(0x00); // IODIRA & IODIRB = all inputs
    Wire.write(0xFF);
    Wire.write(0xFF);
    if (Wire.endTransmission() != 0) return false;

    Wire.beginTransmission(EXPANDER_I2C_ADDR);
    Wire.write(0x0C); // GPPUA & GPPUB = 100k pull-ups enabled
    Wire.write(0xFF);
    Wire.write(0xFF);
    return (Wire.endTransmission() == 0);
}

bool readRaw16(uint16_t &out_raw) {
    if (EXPANDER_TYPE != ExpanderType::PCF8575) {
        uint8_t reg = (EXPANDER_TYPE == ExpanderType::TCA9555) ? 0x00 : 0x12;
        Wire.beginTransmission(EXPANDER_I2C_ADDR);
        Wire.write(reg);
        if (Wire.endTransmission(false) != 0) return false;
    }

    if (Wire.requestFrom((uint8_t)EXPANDER_I2C_ADDR, (uint8_t)2) == 2) {
        uint8_t port0 = Wire.read();
        uint8_t port1 = Wire.read();
        out_raw = (uint16_t)port0 | ((uint16_t)port1 << 8);
        return true;
    }
    return false;
}

bool applyBitsToCar(uint16_t bits, CityElState &car) {
    auto active = [&](uint8_t bit) -> bool {
        return (bits & (1U << bit)) == 0;
    };

    bool new_turn_left   = active(HARNESS_BIT_TURN_LEFT);
    bool new_turn_right  = active(HARNESS_BIT_TURN_RIGHT);
    bool new_parking     = active(HARNESS_BIT_PARKING_LT);
    bool new_low_beam    = active(HARNESS_BIT_LOW_BEAM);
    bool new_high_beam   = active(HARNESS_BIT_HIGH_BEAM);
    bool new_handbrake   = active(HARNESS_BIT_HANDBRAKE);
    bool new_brake_fluid = active(HARNESS_BIT_BRAKE_FLUID);
    bool new_hazard      = active(HARNESS_BIT_HAZARD);

    bool fan1  = active(HARNESS_BIT_FAN_1);
    bool fan2  = active(HARNESS_BIT_FAN_2);
    bool heat1 = active(HARNESS_BIT_HEAT_1);
    bool heat2 = active(HARNESS_BIT_HEAT_2);

    uint8_t new_fan  = fan2 ? 2 : (fan1 ? 1 : 0);
    uint8_t new_heat = heat2 ? 2 : (heat1 ? 1 : 0);

    bool changed = (car.turn_left != new_turn_left) ||
                   (car.turn_right != new_turn_right) ||
                   (car.parking_lights != new_parking) ||
                   (car.low_beam != new_low_beam) ||
                   (car.high_beam != new_high_beam) ||
                   (car.handbrake != new_handbrake) ||
                   (car.brake_fluid_err != new_brake_fluid) ||
                   (car.hazard_lights != new_hazard) ||
                   (car.fan_level != new_fan) ||
                   (car.heat_level != new_heat);

    car.turn_left       = new_turn_left;
    car.turn_right      = new_turn_right;
    car.parking_lights  = new_parking;
    car.low_beam        = new_low_beam;
    car.high_beam       = new_high_beam;
    car.handbrake       = new_handbrake;
    car.brake_fluid_err = new_brake_fluid;
    car.hazard_lights   = new_hazard;
    car.fan_level       = new_fan;
    car.heat_level      = new_heat;

    return changed;
}

bool probeAndInitialize(CityElState &car, bool log_when_missing) {
    Wire.beginTransmission(EXPANDER_I2C_ADDR);
    if (Wire.endTransmission() != 0 || !configureInputs()) {
        expander_online = false;
        if (log_when_missing) {
            Serial.printf("[IO] I2C Expander not found at 0x%02X -> Harness inputs idle (will retry).\n",
                          EXPANDER_I2C_ADDR);
        }
        return false;
    }

    uint16_t initial_raw = 0xFFFF;
    if (!readRaw16(initial_raw)) {
        expander_online = false;
        return false;
    }

    expander_online = true;
    consecutive_i2c_errors = 0;
    stable_state = initial_raw;
    last_raw_read = initial_raw;
    last_bounce_ms = millis();
    last_poll_ms = millis();
    last_latch_refresh_ms = millis();

    const char *chip_name = (EXPANDER_TYPE == ExpanderType::PCF8575) ? "PCF8575" :
                            (EXPANDER_TYPE == ExpanderType::TCA9555) ? "TCA9555" : "MCP23017";
    Serial.printf("[IO] %s Online at 0x%02X (16 optocoupler inputs active-LOW, initial=0x%04X)\n",
                  chip_name, EXPANDER_I2C_ADDR, stable_state);

    applyBitsToCar(stable_state, car);
    return true;
}

}

namespace CityElHarness {

bool begin(CityElState &car) {
    last_retry_ms = millis();
    return probeAndInitialize(car, true);
}

void syncToState(CityElState &car) {
    if (expander_online) {
        applyBitsToCar(stable_state, car);
    }
}

bool update(CityElState &car) {
    uint32_t now = millis();

    if (!expander_online) {
        if (now - last_retry_ms >= EXPANDER_RETRY_MS) {
            last_retry_ms = now;
            if (probeAndInitialize(car, false)) {
                return true;
            }
        }
        return false;
    }

    if (now - last_poll_ms < 15) return false;
    last_poll_ms = now;

    // Periodically re-assert 0xFFFF input latches on PCF8575 so an EMI glitch can never stick a pin low
    if (EXPANDER_TYPE == ExpanderType::PCF8575 && (now - last_latch_refresh_ms >= 2000UL)) {
        last_latch_refresh_ms = now;
        configureInputs();
    }

    uint16_t raw = 0xFFFF;
    if (!readRaw16(raw)) {
        if (++consecutive_i2c_errors >= MAX_I2C_FAILURES) {
            expander_online = false;
            last_retry_ms = now;
            Serial.println("[IO] WARNING: I2C Expander lost connection -> Scheduling auto-recovery...");
        }
        return false;
    }
    consecutive_i2c_errors = 0;

    if (raw != last_raw_read) {
        last_bounce_ms = now;
        last_raw_read = raw;
    }

    if ((now - last_bounce_ms) < 25 || raw == stable_state) {
        return false;
    }

    stable_state = raw;
    bool changed = applyBitsToCar(stable_state, car);

    if (HARNESS_DEBUG_SERIAL) {
        Serial.printf("[IO] Bits: 0x%04X | L:%d R:%d Haz:%d P:%d Lo:%d Hi:%d Brk:%d Fl:%d Chg:%d Fan:%u Heat:%u\n",
            stable_state, car.turn_left, car.turn_right, car.hazard_lights, car.parking_lights,
            car.low_beam, car.high_beam, car.handbrake, car.brake_fluid_err,
            car.charger_conn, car.fan_level, car.heat_level);
    }

    return changed;
}

}
