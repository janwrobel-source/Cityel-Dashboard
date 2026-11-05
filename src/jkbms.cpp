#include "jkbms.h"
#include "pins.h"

#include <math.h>

// Select the JK-BMS interface: false = RS485 adapter, true = CAN.
#define JKBMS_USE_CAN         false
#define JKBMS_DEBUG_SERIAL    true
#define LIMIT_BATT_TEMP_C     50
#define LIMIT_BMS_MOS_C       80

#if JKBMS_USE_CAN
#include "driver/twai.h"
#else
HardwareSerial JkSerial(1);
#endif

namespace {

#if !JKBMS_USE_CAN
constexpr uint8_t JK_REQ[21] = {
    0x4E, 0x57, 0x00, 0x13, 0x00, 0x00, 0x00, 0x00,
    0x06, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x68, 0x00, 0x00, 0x01, 0x29
};
uint8_t rx_buf[320];
uint16_t rx_idx = 0;
uint32_t last_req_ms = 0;
uint32_t last_rx_byte_ms = 0;

bool parseFrame(const uint8_t *buf, uint16_t len, CityElState &car) {
    bool changed = false;
    bool temp_hot = false;
    float pack_v = 0.0f;
    int8_t mos_c = 0, bat1_c = 0, bat2_c = 0;
    uint16_t i = 11;

    while (i < len - 9) {
        uint8_t tag = buf[i++];
        if (tag == 0x79) {
            uint8_t cell_bytes = buf[i++];
            i += cell_bytes;
        }
        else if (tag == 0x80 || tag == 0x81 || tag == 0x82) {
            uint16_t raw_t = (buf[i] << 8) | buf[i + 1];
            int8_t temp_c = (raw_t <= 100) ? (int8_t)raw_t : -(int8_t)(raw_t - 100);
            if (tag == 0x80) mos_c = temp_c;
            if (tag == 0x81) bat1_c = temp_c;
            if (tag == 0x82) bat2_c = temp_c;
            if (tag == 0x80 && temp_c >= LIMIT_BMS_MOS_C) temp_hot = true;
            if (tag != 0x80 && temp_c >= LIMIT_BATT_TEMP_C) temp_hot = true;
            i += 2;
        }
        else if (tag == 0x83) {
            uint16_t raw_v = (buf[i] << 8) | buf[i + 1];
            pack_v = raw_v * 0.01f;
            i += 2;
        }
        else if (tag == 0x84) {
            uint16_t raw_c = (buf[i] << 8) | buf[i + 1];
            float amps = (raw_c & 0x7FFF) * 0.01f;
            float signed_a = (raw_c & 0x8000) ? amps : -amps;
            int16_t new_amps = constrain((int16_t)roundf(signed_a), -200, 50);
            if (new_amps != car.current_amps) {
                car.current_amps = new_amps;
                changed = true;
            }
            i += 2;
        }
        else if (tag == 0x85) {
            if (buf[i] <= 100 && buf[i] != car.battery_soc) {
                car.battery_soc = buf[i];
                changed = true;
            }
            if (JKBMS_DEBUG_SERIAL) {
                static uint32_t last_dbg = 0;
                if (millis() - last_dbg > 1500) {
                    last_dbg = millis();
                    Serial.printf("[BMS-RS485] %.2fV | %+dA | SOC:%u%% | MOS:%dC T1:%dC T2:%dC\n",
                                  pack_v, car.current_amps, car.battery_soc, mos_c, bat1_c, bat2_c);
                }
            }
            break;
        }
        else {
            break;
        }
    }

    if (temp_hot != car.overheat_bms) {
        car.overheat_bms = temp_hot;
        changed = true;
    }
    return changed;
}
#endif

}

namespace CityElBMS {

#if JKBMS_USE_CAN

bool begin() {
    twai_general_config_t g_cfg = TWAI_GENERAL_CONFIG_DEFAULT(
        (gpio_num_t)JKBMS_TX_PIN, (gpio_num_t)JKBMS_RX_PIN, TWAI_MODE_NORMAL
    );
    twai_timing_config_t t_cfg = TWAI_TIMING_CONFIG_250KBITS();
    twai_filter_config_t f_cfg = TWAI_FILTER_CONFIG_ACCEPT_ALL();

    if (twai_driver_install(&g_cfg, &t_cfg, &f_cfg) == ESP_OK && twai_start() == ESP_OK) {
        Serial.println("[BMS] JK-BMS CAN Driver Online (250 kbps on IO17/IO18)");
        return true;
    }
    Serial.println("[BMS] ERROR: Failed to start TWAI CAN driver!");
    return false;
}

bool update(CityElState &car) {
    bool changed = false;
    twai_message_t msg;

    while (twai_receive(&msg, 0) == ESP_OK) {
        if (msg.data_length_code < 8) continue;

        uint16_t pgn = msg.identifier & 0x0FFF;
        if (pgn == 0x02F4) {
            uint16_t raw_curr = msg.data[2] | (msg.data[3] << 8);
            float jk_discharge_a = (raw_curr * 0.1f) - 400.0f;
            int16_t new_amps = constrain((int16_t)roundf(-jk_discharge_a), -200, 50);
            if (new_amps != car.current_amps) {
                car.current_amps = new_amps;
                changed = true;
            }

            if (msg.data[4] <= 100 && msg.data[4] != car.battery_soc) {
                car.battery_soc = msg.data[4];
                changed = true;
            }

            if (JKBMS_DEBUG_SERIAL) {
                static uint32_t last_dbg = 0;
                if (millis() - last_dbg > 1500) {
                    last_dbg = millis();
                    uint16_t raw_v = msg.data[0] | (msg.data[1] << 8);
                    Serial.printf("[BMS-CAN] %.1fV | %+dA | SOC: %u%% | Overheat: %d\n",
                                  raw_v * 0.1f, car.current_amps, car.battery_soc, car.overheat_bms);
                }
            }
        }
        else if (pgn == 0x05F4) {
            int8_t max_temp_c = (int8_t)msg.data[0] - 50;
            bool hot = (max_temp_c >= LIMIT_BATT_TEMP_C);
            if (hot != car.overheat_bms) {
                car.overheat_bms = hot;
                changed = true;
            }
        }
    }

    return changed;
}

#else

bool begin() {
    JkSerial.setRxBufferSize(512);
    JkSerial.begin(115200, SERIAL_8N1, JKBMS_RX_PIN, JKBMS_TX_PIN);
    Serial.println("[BMS] JK-BMS RS485 Driver Online (115200 baud on IO17=TX, IO18=RX)");
    return true;
}

bool update(CityElState &car) {
    bool changed = false;

    if (millis() - last_req_ms >= 500) {
        JkSerial.write(JK_REQ, sizeof(JK_REQ));
        last_req_ms = millis();
    }

    if (rx_idx > 0 && (millis() - last_rx_byte_ms > 100)) {
        rx_idx = 0;
    }

    while (JkSerial.available()) {
        uint8_t b = JkSerial.read();
        last_rx_byte_ms = millis();

        if (rx_idx == 0 && b != 0x4E) continue;
        if (rx_idx == 1 && b != 0x57) { rx_idx = 0; continue; }

        if (rx_idx < sizeof(rx_buf)) rx_buf[rx_idx++] = b;
        else rx_idx = 0;

        if (rx_idx >= 4) {
            uint16_t expected_len = ((rx_buf[2] << 8) | rx_buf[3]) + 2;
            if (expected_len > sizeof(rx_buf) || expected_len < 20) {
                rx_idx = 0;
            } else if (rx_idx == expected_len) {
                if (rx_buf[expected_len - 5] == 0x68 &&
                    parseFrame(rx_buf, expected_len, car)) {
                    changed = true;
                }
                rx_idx = 0;
            }
        }
    }

    return changed;
}

#endif

}
