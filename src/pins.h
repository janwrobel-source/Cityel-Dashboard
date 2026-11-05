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

// --- 3. I2C Bus (DS3231 RTC, TCA9555/MCP23017 Harness IO, FRAM) ---
constexpr uint8_t I2C_SDA_PIN  = 19;
constexpr uint8_t I2C_SCL_PIN  = 20;

// --- 4. FarDriver Controller One-LIN / SIF Input ---
constexpr uint8_t FARD_SIF_PIN = 15;

// --- 5. JK-BMS (RS485 UART1 / CAN TWAI) ---
constexpr uint8_t JKBMS_TX_PIN = 17;
constexpr uint8_t JKBMS_RX_PIN = 18;

// --- 6. MAX98357A I2S Audio Amplifier ---
constexpr uint8_t I2S_BCLK_PIN = 38;
constexpr uint8_t I2S_LRC_PIN  = 8;
constexpr uint8_t I2S_DOUT_PIN = 16;

// --- 7. Panel Front Lighting (PWM LED) ---
constexpr uint8_t FRONT_LIGHT_PIN = 14;

