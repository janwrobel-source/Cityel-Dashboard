#include <Arduino.h>
#include <SPI.h>
#include <esp_sleep.h>
#include "dashboard_renderer.h"
#include "rtc_clock.h"
#include "demo_state.h"
#include "fardriver_sif.h"
#include "sound_clicks.h"
#include "jkbms.h"
#include "harness_io.h"
#include "front_light.h"
#include "pins.h"

DashboardDisplay display(
    GxEPD2_579_GDEY0579T93(EPD_CS_PIN, EPD_DC_PIN, EPD_RST_PIN, EPD_BUSY_PIN)
);

CityElState car;
uint32_t last_full_refresh_ms = 0;

namespace {

portMUX_TYPE display_mux = portMUX_INITIALIZER_UNLOCKED;
TaskHandle_t display_task_handle = nullptr;
CityElState pending_car_snapshot;
bool pending_redraw = false;
bool pending_full_refresh = false;
volatile bool display_busy = false;

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
                display_busy = true;
            } else {
                display_busy = false;
            }
            portEXIT_CRITICAL(&display_mux);

            if (!do_redraw) break;
            renderDashboard(display, snap, do_full);
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

            // 1. Cut high-current LED frontlight immediately to save supercapacitor energy
            state.power_off = true;
            CityElFrontLight::update(state);

            // 2. Flush any unsaved odometer tenths to FRAM / NVS while voltage is still high
            CityElFarDriver::flushOdometer();

            // 3. Cancel queued redraws, wait if E-Ink is mid-refresh, then draw Power-Off screen
            portENTER_CRITICAL(&display_mux);
            pending_redraw = false;
            portEXIT_CRITICAL(&display_mux);
            while (display_busy) {
                delay(5);
            }

            renderDashboard(display, state, false);
            display.hibernate();
            digitalWrite(EPD_PWR_PIN, LOW);

            // 4. Enter deep sleep (wakes automatically if 12V power returns before supercap drains)
            esp_sleep_enable_ext0_wakeup(static_cast<gpio_num_t>(PWR_LOSS_PIN), 1);
            esp_deep_sleep_start();
        }
    } else {
        low_since_ms = 0;
    }
}

} // namespace

// ============================================================================
// SETUP & MAIN LOOP (CLOCK + ROCKER SWITCH + DEMO BUTTON)
// ============================================================================
void setup() {
    Serial.begin(115200);

    // 1. Configure onboard demo buttons & upstream power-loss sense pin
    pinMode(PWR_LOSS_PIN, INPUT_PULLUP);
    CityElDemo::begin(car, false);

    // 2. Turn ON the CrowPanel E-Ink 3.3V power rail (GPIO 7)
    pinMode(EPD_PWR_PIN, OUTPUT);
    digitalWrite(EPD_PWR_PIN, HIGH);
    delay(100);

    // 3. Initialize SPI & E-Paper Display (792x272 SSD1683)
    SPI.begin(EPD_SCK_PIN, -1, EPD_MOSI_PIN, EPD_CS_PIN);
    display.init(115200, true, 2, false);
    display.setRotation(0);

    // 4. Initialize RTC & Rocker Switches (GPIO 6 UP / GPIO 4 DOWN)
    //    Seeds ESP32 internal clock (and syncs from DS3231 at 0x68 if connected)
    CityElRTC::begin(car);

    // Initialize FarDriver One-LIN on GPIO 15 & load saved Odometer from NVS
    CityElFarDriver::begin(car);

    // TCA9555/MCP23017 I2C 0x20
    CityElHarness::begin();                                          

    // Initialize MAX98357A I2S Amplifier (0.85f = 85% volume)
    CityElAudio::begin(0.85f);

    // Initialize JK-BMS on IO17 (TX) & IO18 (RX)
    CityElBMS::begin();

    // Initialize Panel Front Lighting PWM & set initial state
    CityElFrontLight::begin();
    CityElFrontLight::update(car);

    // 5. Draw initial screen with 1 clean Full Refresh, then launch non-blocking Core 0 display task
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
    checkHardwarePowerLoss(car);

    bool needs_redraw = false;
    bool full_refresh = millis() - last_full_refresh_ms >= 60000UL;

    // --- 1. CLOCK & ROCKER SWITCHES (UP = Hour+, DOWN = Minute+) ---
    // Returns true whenever a minute rolls over by itself OR rocker is flicked
    if (CityElRTC::update(car)) {
        needs_redraw = true;
    }

    if (full_refresh) {
        needs_redraw = true;
    }

    // --- 2. DEMO STATE BUTTON ---
    if (CityElDemo::update(car)) {
        needs_redraw = true;
    }

    // Poll FarDriver One-LIN (Speed, Odometer, Gear F/N/R, Controller Fault)
    if (CityElFarDriver::update(car)) {
        needs_redraw = true;
    }

    // Poll JK-BMS (Battery %, Current -200..+50A, and Battery/MOSFET Overheat)
    if (CityElBMS::update(car)) {
        needs_redraw = true;
    }

    // 4. TCA9555 / MCP23017 Harness Switches (Turns, Beams, Brakes, Fan, Heat)
    if (CityElHarness::update(car)) {
        needs_redraw = true;
    }

    // --- 3. IMMEDIATE AUDIO CLICKS & FRONT LIGHTING ---
    // Updated before E-Ink rendering so blinkers and lights respond with zero latency
    CityElAudio::updateTurnSignalClicks(car);
    CityElFrontLight::update(car);

    // --- 4. NON-BLOCKING E-INK REFRESH ON CORE 0 ---
    if (needs_redraw) {
        if (full_refresh) {
            last_full_refresh_ms = millis();
        }
        requestDashboardRedraw(car, full_refresh);
    }

    delay(20); // Small yield for background tasks
}