#include "front_light.h"
#include "config.h"
#include "pins.h"

#include <Arduino.h>

namespace {

bool pwm_ready = false;
bool last_lights_on = false;
bool state_initialized = false;

}

namespace CityElFrontLight {

bool begin() {
    pwm_ready = ledcAttach(FRONT_LIGHT_PIN, FRONT_LIGHT_PWM_HZ, FRONT_LIGHT_PWM_BITS);
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
