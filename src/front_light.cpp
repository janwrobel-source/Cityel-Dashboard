#include "front_light.h"
#include "pins.h"

#include <Arduino.h>

namespace {

constexpr uint32_t PWM_FREQ_HZ = 5000;
constexpr uint8_t PWM_RESOLUTION_BITS = 8;
constexpr uint8_t FRONT_LIGHT_DUTY_ON = 160; // Constant PWM duty (0-255) when car lights are on
constexpr uint8_t FRONT_LIGHT_DUTY_OFF = 0;

bool pwm_ready = false;
bool last_lights_on = false;
bool state_initialized = false;

}

namespace CityElFrontLight {

bool begin() {
    pwm_ready = ledcAttach(FRONT_LIGHT_PIN, PWM_FREQ_HZ, PWM_RESOLUTION_BITS);
    if (!pwm_ready) {
        Serial.println("[LIGHT] ERROR: Failed to attach PWM on FRONT_LIGHT_PIN!");
        return false;
    }

    ledcWrite(FRONT_LIGHT_PIN, FRONT_LIGHT_DUTY_OFF);
    state_initialized = false;
    Serial.printf("[LIGHT] Panel Front Light PWM ready on GPIO %u\n", FRONT_LIGHT_PIN);
    return true;
}

void update(const CityElState &car) {
    if (!pwm_ready) return;

    bool lights_on = !car.power_off && (car.parking_lights || car.low_beam || car.high_beam);
    if (!state_initialized || lights_on != last_lights_on) {
        last_lights_on = lights_on;
        state_initialized = true;
        uint8_t duty = lights_on ? FRONT_LIGHT_DUTY_ON : FRONT_LIGHT_DUTY_OFF;
        ledcWrite(FRONT_LIGHT_PIN, duty);
    }
}

}

