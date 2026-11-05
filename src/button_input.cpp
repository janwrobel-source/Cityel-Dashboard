#include "button_input.h"

namespace {

constexpr uint8_t BUTTON_STATE_CAPACITY = 8;
constexpr uint32_t BUTTON_DEBOUNCE_MS = 25;

struct ButtonState {
    uint8_t pin = 0;
    bool initialized = false;
    bool raw_pressed = false;
    bool stable_pressed = false;
    uint32_t raw_changed_ms = 0;
    uint32_t pressed_ms = 0;
    uint32_t last_repeat_ms = 0;
};

ButtonState button_states[BUTTON_STATE_CAPACITY];

ButtonState *stateForPin(uint8_t pin) {
    for (ButtonState &state : button_states) {
        if (state.initialized && state.pin == pin) return &state;
    }

    for (ButtonState &state : button_states) {
        if (!state.initialized) {
            state.pin = pin;
            state.initialized = true;
            state.raw_pressed = digitalRead(pin) == LOW;
            state.raw_changed_ms = millis();
            return &state;
        }
    }

    Serial.println("[BUTTON] No free button state slots");
    return nullptr;
}

}

namespace CityElButtons {

bool pressed(uint8_t pin, uint32_t hold_delay_ms, uint32_t repeat_interval_ms) {
    ButtonState *state = stateForPin(pin);
    if (state == nullptr) return false;

    uint32_t now = millis();
    bool raw_pressed = digitalRead(pin) == LOW;
    if (raw_pressed != state->raw_pressed) {
        state->raw_pressed = raw_pressed;
        state->raw_changed_ms = now;
    }

    if (state->raw_pressed != state->stable_pressed &&
        now - state->raw_changed_ms >= BUTTON_DEBOUNCE_MS) {
        state->stable_pressed = state->raw_pressed;
        if (state->stable_pressed) {
            state->pressed_ms = now;
            state->last_repeat_ms = now;
            return true;
        }
    }

    if (state->stable_pressed && repeat_interval_ms > 0 &&
        now - state->pressed_ms >= hold_delay_ms &&
        now - state->last_repeat_ms >= repeat_interval_ms) {
        state->last_repeat_ms = now;
        return true;
    }

    return false;
}

}
