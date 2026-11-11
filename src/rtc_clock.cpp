#include "rtc_clock.h"
#include "button_input.h"
#include "pins.h"

#include <Wire.h>
#include <sys/time.h>
#include <time.h>

namespace {

constexpr uint8_t DS3231_ADDR = 0x68;
constexpr uint32_t CLOCK_SET_TIMEOUT_MS = 30000UL;

uint8_t bcd2dec(uint8_t val) {
    return ((val >> 4) * 10) + (val & 0x0F);
}

uint8_t dec2bcd(uint8_t val) {
    return ((val / 10) << 4) | (val % 10);
}

}

namespace CityElRTC {

void setEsp32InternalClock(uint8_t hour, uint8_t minute, uint8_t second) {
    struct tm t = {};
    t.tm_year = 2026 - 1900;
    t.tm_mon = 0;
    t.tm_mday = 1;
    t.tm_hour = hour % 24;
    t.tm_min = minute % 60;
    t.tm_sec = second % 60;

    time_t epoch = mktime(&t);
    struct timeval tv = { .tv_sec = epoch, .tv_usec = 0 };
    settimeofday(&tv, NULL);
}

void readEsp32InternalClock(uint8_t &hour, uint8_t &minute) {
    time_t now;
    time(&now);
    struct tm timeinfo;
    localtime_r(&now, &timeinfo);
    hour = (uint8_t)timeinfo.tm_hour;
    minute = (uint8_t)timeinfo.tm_min;
}

bool setTime(uint8_t hour, uint8_t minute) {
    setEsp32InternalClock(hour, minute, 0);

    Wire.beginTransmission(DS3231_ADDR);
    Wire.write(0x00);
    Wire.write(dec2bcd(0));
    Wire.write(dec2bcd(minute % 60));
    Wire.write(dec2bcd(hour % 24));
    if (Wire.endTransmission() != 0) return false;

    Wire.beginTransmission(DS3231_ADDR);
    Wire.write(0x0F);
    Wire.write(0x00);
    return (Wire.endTransmission() == 0);
}

bool syncFromDS3231() {
    Wire.beginTransmission(DS3231_ADDR);
    Wire.write(0x00);
    if (Wire.endTransmission(false) != 0) return false;

    if (Wire.requestFrom((uint8_t)DS3231_ADDR, (uint8_t)3) == 3) {
        uint8_t sec = bcd2dec(Wire.read() & 0x7F);
        uint8_t min = bcd2dec(Wire.read() & 0x7F);
        uint8_t hour = bcd2dec(Wire.read() & 0x3F);
        setEsp32InternalClock(hour, min, sec);
        return true;
    }
    return false;
}

bool begin(CityElState &car) {
    pinMode(BTN_CONF_PIN, INPUT_PULLUP);
    pinMode(BTN_UP_PIN, INPUT_PULLUP);
    pinMode(BTN_DOWN_PIN, INPUT_PULLUP);

    Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN, 100000);
    Wire.setTimeOut(20);

    setEsp32InternalClock(car.clock_hour, car.clock_min, 0);

    Wire.beginTransmission(DS3231_ADDR);
    if (Wire.endTransmission() != 0) {
        Serial.println("[RTC] DS3231 not connected -> Running on ESP32-S3 Internal Clock!");
        return false;
    }

    Wire.beginTransmission(DS3231_ADDR);
    Wire.write(0x0F);
    Wire.endTransmission(false);
    if (Wire.requestFrom((uint8_t)DS3231_ADDR, (uint8_t)1) == 1) {
        if (Wire.read() & 0x80) {
            car.rtc_battery_low = true;
            Serial.println("[RTC] WARNING: DS3231 OSF set -> Clock backup battery low/missing!");
        } else {
            car.rtc_battery_low = false;
        }
    }

    if (!car.rtc_battery_low && syncFromDS3231()) {
        readEsp32InternalClock(car.clock_hour, car.clock_min);
        Serial.printf("[RTC] Synced DS3231 -> ESP32 Internal Clock: %02u:%02u\n", car.clock_hour, car.clock_min);
        return true;
    }
    return false;
}

bool update(CityElState &car) {
    static uint32_t last_resync_ms = 0;
    static uint32_t last_set_activity_ms = 0;
    uint32_t now = millis();
    bool time_changed = false;

    bool rocker_pressed = CityElButtons::pressed(BTN_CONF_PIN, 0, 0);
    bool up_pressed     = CityElButtons::pressed(BTN_UP_PIN, 500, 120);
    bool down_pressed   = CityElButtons::pressed(BTN_DOWN_PIN, 500, 120);

    if (rocker_pressed) {
        if (car.clock_set_mode == ClockSetMode::NONE) {
            car.clock_set_mode = ClockSetMode::HOURS;
        } else if (car.clock_set_mode == ClockSetMode::HOURS) {
            car.clock_set_mode = ClockSetMode::MINUTES;
        } else {
            car.clock_set_mode = ClockSetMode::NONE;
            setTime(car.clock_hour, car.clock_min);
            car.rtc_battery_low = false;
        }
        last_set_activity_ms = now;
        time_changed = true;
    } else if (car.clock_set_mode != ClockSetMode::NONE && (up_pressed || down_pressed)) {
        if (car.clock_set_mode == ClockSetMode::HOURS) {
            car.clock_hour = up_pressed ? (uint8_t)((car.clock_hour + 1) % 24)
                                        : (uint8_t)((car.clock_hour + 23) % 24);
        } else if (car.clock_set_mode == ClockSetMode::MINUTES) {
            car.clock_min = up_pressed ? (uint8_t)((car.clock_min + 1) % 60)
                                       : (uint8_t)((car.clock_min + 59) % 60);
        }
        setTime(car.clock_hour, car.clock_min);
        car.rtc_battery_low = false;
        last_set_activity_ms = now;
        time_changed = true;
    }

    if (car.clock_set_mode != ClockSetMode::NONE &&
        (now - last_set_activity_ms >= CLOCK_SET_TIMEOUT_MS)) {
        car.clock_set_mode = ClockSetMode::NONE;
        time_changed = true;
    }

    if (car.clock_set_mode == ClockSetMode::NONE && !car.rtc_battery_low && (now - last_resync_ms >= 3600000UL)) {
        last_resync_ms = now;
        syncFromDS3231();
    }

    uint8_t internal_h = car.clock_hour, internal_m = car.clock_min;
    readEsp32InternalClock(internal_h, internal_m);
    if (internal_h != car.clock_hour || internal_m != car.clock_min) {
        car.clock_hour = internal_h;
        car.clock_min = internal_m;
        if (!car.rtc_battery_low || car.clock_set_mode != ClockSetMode::NONE) {
            time_changed = true;
        }
    }

    return time_changed;
}

}
