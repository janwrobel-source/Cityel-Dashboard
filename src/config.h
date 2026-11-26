#pragma once

#include <stdint.h>

// --- 1. Serial & Diagnostic Logging ---
constexpr uint32_t SERIAL_BAUD_RATE     = 115200;
constexpr bool     HARNESS_DEBUG_SERIAL = true;
constexpr bool     SIF_DEBUG_SERIAL     = true;
constexpr bool     JKBMS_DEBUG_SERIAL   = true;

// --- 2. I2C Bus Timing & Recovery ---
constexpr uint32_t I2C_CLOCK_HZ         = 100000;
constexpr uint16_t I2C_TIMEOUT_MS       = 20;
constexpr uint32_t EXPANDER_RETRY_MS    = 3000UL; // Auto-recovery probe interval if expander is offline

// --- 3. Harness I2C IO Expander ---
enum class ExpanderType : uint8_t {
    PCF8575,
    TCA9555,
    MCP23017
};

constexpr ExpanderType EXPANDER_TYPE    = ExpanderType::PCF8575;

// --- 4. FarDriver One-LIN / SIF & Odometer ---
constexpr bool     SIF_INVERTED_BY_OPTO = true;
constexpr float    FARD_SPEED_DIVISOR   = 1.0f;
// Default: Byte 2, Bit 6 (0x40 = Park / Charge Lock on DKD/Protocol 21)
constexpr uint8_t  FARD_CHARGER_BYTE    = 2;
constexpr uint8_t  FARD_CHARGER_MASK    = 0x40;
constexpr uint32_t SIF_STALE_TIMEOUT_US = 1200000UL; // Zero speed after 1.2s without SIF packets
constexpr uint32_t FARD_UART_BAUD       = 19200;     // FarDriver isolated UART2 baud rate

// --- 5. JK-BMS Interface & Thermal Limits ---
// Select JK-BMS physical interface: false = RS485 UART, true = TWAI CAN (250 kbps)
#ifndef JKBMS_USE_CAN
#define JKBMS_USE_CAN false
#endif

constexpr int8_t   LIMIT_BATT_TEMP_C    = 50;
constexpr int8_t   LIMIT_BMS_MOS_C      = 80;
constexpr uint32_t JKBMS_STALE_MS       = 3000UL; // Zero current display if BMS goes silent for 3s

// --- 6. Panel Front Lighting (PWM) & Audio Clicks ---
constexpr uint32_t FRONT_LIGHT_PWM_HZ   = 5000;
constexpr uint8_t  FRONT_LIGHT_PWM_BITS = 8;
constexpr uint8_t  FRONT_LIGHT_DUTY_ON  = 160; // PWM duty (0-255) when vehicle lights are on
constexpr uint8_t  FRONT_LIGHT_DUTY_OFF = 0;
constexpr float    AUDIO_CLICK_VOLUME   = 1.0f;

// --- 7. E-Paper Refresh Scheduling & Bench Demo ---
constexpr uint32_t EPD_FULL_REFRESH_INTERVAL_MS = 60000UL;  // Clean ghosting after >=60s once stationary
constexpr uint32_t EPD_MAX_DRIVING_PARTIAL_MS   = 300000UL; // Safety cap (5 min) during continuous driving
constexpr uint16_t EPD_PARTIALS_BEFORE_FULL     = 1;        // Require >=1 partial update before full refresh
constexpr bool     ENABLE_BENCH_DEMO_BUTTONS    = true;
constexpr uint32_t DEMO_ACTIVATE_HOLD_MS        = 1000UL;   // Hold EXIT for 1s to enter/exit bench demo mode
