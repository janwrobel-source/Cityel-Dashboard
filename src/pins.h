#pragma once

#include <stdint.h>

// --- 1. CrowPanel ESP32-S3 5.79" E-Paper Display (SPI) ---
constexpr uint8_t EPD_MOSI_PIN = 11;
constexpr uint8_t EPD_SCK_PIN  = 12;
constexpr uint8_t EPD_CS_PIN   = 45;
constexpr uint8_t EPD_DC_PIN   = 46;
constexpr uint8_t EPD_RST_PIN  = 47;
constexpr uint8_t EPD_BUSY_PIN = 48;
constexpr uint8_t EPD_PWR_PIN  = 7;   // Powers on the E-Ink screen 3.3V rail

// --- 2. Onboard Buttons & Rocker Switch ---
constexpr uint8_t BTN_EXIT_PIN = 1;
constexpr uint8_t BTN_MENU_PIN = 2;
constexpr uint8_t BTN_DOWN_PIN = 4;
constexpr uint8_t BTN_CONF_PIN = 5;   // Rotary/rocker switch press (CONF)
constexpr uint8_t BTN_UP_PIN   = 6;

// --- 3. I2C Bus & Device Addresses ---
constexpr uint8_t I2C_SDA_PIN       = 19;
constexpr uint8_t I2C_SCL_PIN       = 20;
constexpr uint8_t EXPANDER_I2C_ADDR = 0x20; // PCF8575 / TCA9555 / MCP23017 Harness IO
constexpr uint8_t ODO_FRAM_I2C_ADDR = 0x51; // I2C FRAM (Odometer storage)
constexpr uint8_t DS3231_I2C_ADDR   = 0x68; // DS3231 Real-Time Clock

// --- 4. FarDriver Controller One-LIN / SIF Input ---
constexpr uint8_t FARD_SIF_PIN = 15;

// --- 5. JK-BMS (RS485 UART1 / CAN TWAI) ---
constexpr uint8_t JKBMS_TX_PIN = 17;
constexpr uint8_t JKBMS_RX_PIN = 18;

// --- 6. MAX98357A I2S Audio Amplifier ---
constexpr uint8_t I2S_BCLK_PIN = 38;
constexpr uint8_t I2S_LRC_PIN  = 8;
constexpr uint8_t I2S_DOUT_PIN = 16;

// --- 7. Panel Front Lighting (PWM LED) & Power Loss Sense ---
constexpr uint8_t FRONT_LIGHT_PIN = 14;
constexpr uint8_t PWR_LOSS_PIN    = 9;   // Active-LOW upstream 12V power-loss sense (20-pin header IO9, RTC-wake capable)

// --- 8. PCF8575 / TCA9555 I2C Expander Optocoupler Input Bits ---
constexpr uint8_t HARNESS_BIT_TURN_LEFT   = 0;  // P00 <- J4-1  (SW-WS, +12V active)
constexpr uint8_t HARNESS_BIT_TURN_RIGHT  = 1;  // P01 <- J3-7  (GN-SW, +12V active)
constexpr uint8_t HARNESS_BIT_PARKING_LT  = 2;  // P02 <- J3-15 (GR,    +12V active)
constexpr uint8_t HARNESS_BIT_LOW_BEAM    = 3;  // P03 <- J22-2 (GE,    +12V active)
constexpr uint8_t HARNESS_BIT_HIGH_BEAM   = 4;  // P04 <- J3-9  (WS,    +12V active)
constexpr uint8_t HARNESS_BIT_HANDBRAKE   = 5;  // P05 <- J4-3  (RT-WS, +12V active)
constexpr uint8_t HARNESS_BIT_BRAKE_FLUID = 6;  // P06 <- J4-2  (GE-BR, +12V active)
constexpr uint8_t HARNESS_BIT_HAZARD      = 7;  // P07 <- J3-8  (SW-RT, +12V active)
constexpr uint8_t HARNESS_BIT_FAN_1       = 8;  // P10 <- J3-10 (BR-RT, 0V active-LOW + 1N4007 block)
constexpr uint8_t HARNESS_BIT_FAN_2       = 9;  // P11 <- S3-11 (BL-GE, 0V active-LOW + 1N4007 block)
constexpr uint8_t HARNESS_BIT_HEAT_1      = 10; // P12 <- J3-12 (GN-WS, +36V/Pack active)
constexpr uint8_t HARNESS_BIT_HEAT_2      = 11; // P13 <- J3-11 (GR-WS, +36V/Pack active)

