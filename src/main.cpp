#include <Arduino.h>
#include <SPI.h>
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

} // namespace

// ============================================================================
// SETUP & MAIN LOOP (CLOCK + ROCKER SWITCH + DEMO BUTTON)
// ============================================================================
void setup() {
    Serial.begin(115200);

    // 1. Configure onboard demo buttons (starts with neutral car state; press MENU/EXIT for bench demo)
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