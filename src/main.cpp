#include <Arduino.h>
#include <SPI.h>
#include <Wire.h>
#include <esp_sleep.h>
#include "config.h"
#include "dashboard_renderer.h"
#include "demo_state.h"
#include "fardriver_sif.h"
#include "front_light.h"
#include "harness_io.h"
#include "jkbms.h"
#include "pins.h"
#include "rtc_clock.h"
#include "sound_clicks.h"

DashboardDisplay display(
    GxEPD2_579_GDEY0579T93(EPD_CS_PIN, EPD_DC_PIN, EPD_RST_PIN, EPD_BUSY_PIN)
);

CityElState car;
uint32_t last_full_refresh_ms = 0;

namespace {

portMUX_TYPE display_mux = portMUX_INITIALIZER_UNLOCKED;
SemaphoreHandle_t display_mutex = nullptr;
TaskHandle_t display_task_handle = nullptr;
CityElState pending_car_snapshot;
bool pending_redraw = false;
bool pending_full_refresh = false;

void enterDeepSleepUntilPowerReturns() {
    esp_sleep_enable_ext0_wakeup(static_cast<gpio_num_t>(PWR_LOSS_PIN), 1);
    esp_deep_sleep_start();
}

bool isPowerLostDebounced() {
    if (digitalRead(PWR_LOSS_PIN) != LOW) return false;
    uint32_t start_ms = millis();
    while (millis() - start_ms < 20) {
        if (digitalRead(PWR_LOSS_PIN) != LOW) return false;
        delay(1);
    }
    return true;
}

void displayTask(void * /*param*/) {
    for (;;) {
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

        for (;;) {
            CityElState snap;
            bool do_redraw = false;
            bool do_full = false;

            portENTER_CRITICAL(&display_mux);
            if (pending_redraw) {
                snap = pending_car_snapshot;
                do_redraw = true;
                do_full = pending_full_refresh;
                pending_redraw = false;
                pending_full_refresh = false;
            }
            portEXIT_CRITICAL(&display_mux);

            if (!do_redraw) break;

            if (xSemaphoreTake(display_mutex, portMAX_DELAY) == pdTRUE) {
                renderDashboard(display, snap, do_full);
                xSemaphoreGive(display_mutex);
            }
        }
    }
}

void requestDashboardRedraw(const CityElState &state, bool full_refresh) {
    portENTER_CRITICAL(&display_mux);
    pending_car_snapshot = state;
    pending_redraw = true;
    if (full_refresh) {
        pending_full_refresh = true;
    }
    portEXIT_CRITICAL(&display_mux);

    if (display_task_handle != nullptr) {
        xTaskNotifyGive(display_task_handle);
    }
}

void checkHardwarePowerLoss(CityElState &state) {
    static uint32_t low_since_ms = 0;

    if (digitalRead(PWR_LOSS_PIN) == LOW) {
        if (low_since_ms == 0) {
            low_since_ms = millis();
        } else if (millis() - low_since_ms >= 20) {
            Serial.println("[PWR] Main 12V power loss detected! Saving odometer & hibernating...");

            // 1. Cut high-current LED frontlight immediately to conserve supercapacitor energy
            state.power_off = true;
            CityElFrontLight::update(state);

            // 2. Flush any unsaved odometer tenths to FRAM / NVS while rail voltage is still high
            CityElFarDriver::flushOdometer();

            // 3. Cancel queued redraws, lock display mutex (waits cleanly if Core 0 is mid-refresh),
            //    and draw the final Power-Off screen without releasing the mutex
            portENTER_CRITICAL(&display_mux);
            pending_redraw = false;
            portEXIT_CRITICAL(&display_mux);

            if (display_mutex != nullptr) {
                xSemaphoreTake(display_mutex, pdMS_TO_TICKS(3000));
            }

            renderDashboard(display, state, true);
            display.hibernate();
            digitalWrite(EPD_PWR_PIN, LOW);

            // 4. Enter deep sleep (wakes automatically if 12V power returns before supercap drains)
            enterDeepSleepUntilPowerReturns();
        }
    } else {
        low_since_ms = 0;
    }
}

} // namespace

// ============================================================================
// SETUP & MAIN LOOP
// ============================================================================
void setup() {
    // 1. Check upstream 12V power-loss pin first to avoid draining supercap on transient brownout wakes
    pinMode(PWR_LOSS_PIN, INPUT_PULLUP);
    if (isPowerLostDebounced()) {
        enterDeepSleepUntilPowerReturns();
    }

    Serial.begin(SERIAL_BAUD_RATE);
    display_mutex = xSemaphoreCreateMutex();

    // 2. Initialize shared I2C bus before any I2C peripherals (RTC, FRAM, Harness Expander)
    Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN, I2C_CLOCK_HZ);
    Wire.setTimeOut(I2C_TIMEOUT_MS);

    // 3. Turn ON the CrowPanel E-Ink 3.3V power rail & initialize SPI display (792x272 SSD1683)
    pinMode(EPD_PWR_PIN, OUTPUT);
    digitalWrite(EPD_PWR_PIN, HIGH);
    delay(100);

    SPI.begin(EPD_SCK_PIN, -1, EPD_MOSI_PIN, EPD_CS_PIN);
    display.init(SERIAL_BAUD_RATE, true, 2, false);
    display.setRotation(0);

    // 4. Initialize Vehicle Subsystems
    CityElRTC::begin(car);
    CityElFarDriver::begin(car);
    CityElHarness::begin(car);
    CityElAudio::begin(AUDIO_CLICK_VOLUME);
    CityElBMS::begin();
    CityElDemo::begin(car, false);

    // 5. Initialize Panel Front Lighting PWM & apply initial harness switch state
    CityElFrontLight::begin();
    CityElFrontLight::update(car);

    // 6. Draw initial screen with 1 clean Full Refresh, then launch non-blocking Core 0 display task
    renderDashboard(display, car, true);
    last_full_refresh_ms = millis();

    xTaskCreatePinnedToCore(
        displayTask,
        "EpdRender",
        4096,
        nullptr,
        1,
        &display_task_handle,
        0
    );
}

void loop() {
    static bool prev_power_off = false;
    static bool prev_demo_active = false;
    static uint16_t partial_redraw_count = 0;

    checkHardwarePowerLoss(car);

    bool needs_redraw = false;
    bool full_refresh = false;

    // --- 1. BENCH DEMO CONTROLS (Hold EXIT 1s to enter/exit) ---
    if (CityElDemo::update(car)) {
        needs_redraw = true;
    }

    bool demo_now = CityElDemo::isActive();
    if (demo_now != prev_demo_active) {
        prev_demo_active = demo_now;
        needs_redraw = true;
        full_refresh = true;
    }

    // --- 2. LIVE VEHICLE TELEMETRY (Suspended while Bench Demo is active) ---
    if (!demo_now) {
        if (CityElRTC::update(car)) {
            needs_redraw = true;
        }
        if (CityElFarDriver::update(car)) {
            needs_redraw = true;
        }
        if (CityElBMS::update(car)) {
            needs_redraw = true;
        }
        if (CityElHarness::update(car)) {
            needs_redraw = true;
        }
    }

    // --- 3. IMMEDIATE AUDIO CLICKS & FRONT LIGHTING ---
    // Updated before E-Ink rendering so blinkers and lights respond with zero latency
    CityElAudio::updateTurnSignalClicks(car);
    CityElFrontLight::update(car);

    // --- 4. SMART NON-BLOCKING E-INK REFRESH ON CORE 0 ---
    if (car.power_off != prev_power_off) {
        prev_power_off = car.power_off;
        needs_redraw = true;
        full_refresh = true;
    }

    if (!car.power_off && partial_redraw_count >= EPD_PARTIALS_BEFORE_FULL) {
        uint32_t since_full_ms = millis() - last_full_refresh_ms;
        bool clean_while_stopped = (car.speed_kmh == 0 && since_full_ms >= EPD_FULL_REFRESH_INTERVAL_MS);
        bool safety_cap_driving  = (since_full_ms >= EPD_MAX_DRIVING_PARTIAL_MS);
        if (clean_while_stopped || safety_cap_driving) {
            needs_redraw = true;
            full_refresh = true;
        }
    }

    if (needs_redraw) {
        if (full_refresh) {
            last_full_refresh_ms = millis();
            partial_redraw_count = 0;
        } else {
            partial_redraw_count++;
        }
        requestDashboardRedraw(car, full_refresh);
    }

    delay(10); // Small yield for background tasks
}
