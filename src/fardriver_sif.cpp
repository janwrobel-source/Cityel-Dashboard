#include "fardriver_sif.h"
#include "pins.h"

#include <Preferences.h>
#include <Wire.h>
#include <esp_rom_crc.h>

#define SIF_INVERTED_BY_OPTO  true
#define SIF_DEBUG_SERIAL      true
#define FARD_SPEED_DIVISOR    1.0f
#define ODO_FRAM_ADDR         0x50

// Default: Byte 2, Bit 6 (0x40 = Park / Charge Lock on DKD/Protocol 21).
// Watch "[SIF] Raw:" in Serial Monitor when triggering the FarDriver Charger pin to verify!
#define FARD_CHARGER_BYTE     2
#define FARD_CHARGER_MASK     0x40

namespace {

static bool fram_online = false;

struct OdoRecord {
    uint32_t total_tenths_km;
    uint32_t crc;
};

Preferences odo_prefs;
uint32_t current_tenths = 0;
uint32_t last_saved_tenths = 0;
uint32_t sub_tenth_mm = 0;
uint64_t last_pkt_us = 0;
uint32_t zero_speed_since_ms = 0;

volatile uint8_t sif_rx_buf[12];
volatile bool sif_new_pkt = false;

uint32_t calcCrc(uint32_t val) {
    uint32_t crc = esp_rom_crc32_le(0xA5A55A5AUL, reinterpret_cast<const uint8_t*>(&val), sizeof(val));
    return (crc == 0) ? 1UL : crc;
}

void IRAM_ATTR onEdgeISR() {
    static uint64_t last_edge_us = 0;
    static uint32_t low_duration_us = 0;
    static uint8_t rx_work[12];
    static int8_t bit_index = -1;

    uint64_t now = esp_timer_get_time();
    uint32_t dur = (uint32_t)(now - last_edge_us);
    last_edge_us = now;

    bool wire_high = digitalRead(FARD_SIF_PIN);
    if (SIF_INVERTED_BY_OPTO) wire_high = !wire_high;

    if (wire_high) {
        low_duration_us = dur;
    } else {
        uint32_t high_duration_us = dur;

        if (low_duration_us > 10000 && low_duration_us > (high_duration_us * 12)) {
            bit_index = 0;
            for (int i = 0; i < 12; i++) rx_work[i] = 0;
            return;
        }

        if (bit_index >= 0 && bit_index < 96) {
            if (low_duration_us < 120 || high_duration_us < 120 ||
                low_duration_us > 6000 || high_duration_us > 6000) {
                bit_index = -1;
                return;
            }

            if (low_duration_us > high_duration_us) {
                rx_work[bit_index >> 3] |= (0x80 >> (bit_index & 7));
            }

            if (++bit_index == 96) {
                uint8_t xor_sum = 0;
                for (int i = 0; i < 11; i++) xor_sum ^= rx_work[i];
                if (xor_sum == rx_work[11] && !sif_new_pkt) {
                    for (int i = 0; i < 12; i++) sif_rx_buf[i] = rx_work[i];
                    sif_new_pkt = true;
                }
                bit_index = -1;
            }
        }
    }
}

void saveOdometer(uint32_t tenths_val) {
    OdoRecord rec = { tenths_val, calcCrc(tenths_val) };
    if (fram_online) {
        for (uint8_t slot = 0; slot < 2; slot++) {
            uint16_t addr = slot * sizeof(OdoRecord);
            Wire.beginTransmission(ODO_FRAM_ADDR);
            Wire.write((uint8_t)(addr >> 8));
            Wire.write((uint8_t)(addr & 0xFF));
            Wire.write((const uint8_t*)&rec, sizeof(OdoRecord));
            Wire.endTransmission();
        }
    } else {
        odo_prefs.putBytes("slotA", &rec, sizeof(OdoRecord));
        odo_prefs.putBytes("slotB", &rec, sizeof(OdoRecord));
    }
    last_saved_tenths = tenths_val;
    Serial.printf("[ODO] Saved to NVM: %lu.%lu km\n", tenths_val / 10UL, tenths_val % 10UL);
}

}

namespace CityElFarDriver {

void begin(CityElState &car) {
    // 1. Auto-detect I2C FRAM at 0x50 (Wire.begin was already called by CityElRTC::begin)
    Wire.beginTransmission(ODO_FRAM_ADDR);
    fram_online = (Wire.endTransmission() == 0);

    OdoRecord slotA = {0, 0}, slotB = {0, 0};

    if (fram_online) {
        Serial.println("[ODO] I2C FRAM detected at 0x50! Using real-time 0.1km FRAM storage.");
        for (uint8_t slot = 0; slot < 2; slot++) {
            uint16_t addr = slot * sizeof(OdoRecord);
            Wire.beginTransmission(ODO_FRAM_ADDR);
            Wire.write((uint8_t)(addr >> 8));
            Wire.write((uint8_t)(addr & 0xFF));
            if (Wire.endTransmission(false) == 0 &&
                Wire.requestFrom((uint8_t)ODO_FRAM_ADDR, (uint8_t)sizeof(OdoRecord)) == sizeof(OdoRecord)) {
                uint8_t *dst = (uint8_t*)(slot == 0 ? &slotA : &slotB);
                for (size_t i = 0; i < sizeof(OdoRecord); i++) dst[i] = Wire.read();
            }
        }
    } else {
        Serial.println("[ODO] FRAM not found at 0x50 -> Using ESP32 NVS Flash (save-on-stop).");
        odo_prefs.begin("cityel_odo", false);
        odo_prefs.getBytes("slotA", &slotA, sizeof(OdoRecord));
        odo_prefs.getBytes("slotB", &slotB, sizeof(OdoRecord));
    }

    // Validate Slot A, fall back to Slot B, or seed initial mileage on very first boot
    if (slotA.crc != 0 && slotA.crc == calcCrc(slotA.total_tenths_km)) {
        current_tenths = slotA.total_tenths_km;
    } else if (slotB.crc != 0 && slotB.crc == calcCrc(slotB.total_tenths_km)) {
        current_tenths = slotB.total_tenths_km;
    } else {
        current_tenths = (car.odometer_km * 10UL) + (car.odo_tenths % 10);
        saveOdometer(current_tenths);
    }

    last_saved_tenths = current_tenths;
    car.odometer_km = current_tenths / 10UL;
    car.odo_tenths  = current_tenths % 10UL;

    // 2. Attach FarDriver One-LIN Interrupt on GPIO 15
    pinMode(FARD_SIF_PIN, INPUT_PULLUP);
    attachInterrupt(digitalPinToInterrupt(FARD_SIF_PIN), onEdgeISR, CHANGE);
    Serial.printf("[SIF] FarDriver One-LIN ready on GPIO %d | Odo: %lu.%u km\n",
                  FARD_SIF_PIN, car.odometer_km, car.odo_tenths);
}

bool update(CityElState &car) {
    bool changed = false;
    uint64_t now_us = esp_timer_get_time();

    if (sif_new_pkt) {
        uint8_t pkt[12];
        noInterrupts();
        for (int i = 0; i < 12; i++) pkt[i] = sif_rx_buf[i];
        sif_new_pkt = false;
        interrupts();

        // 1. Decode Speed (Bytes 7 & 8 Big-Endian)
        uint16_t raw_spd = (pkt[7] << 8) | pkt[8];
        float speed_f    = (float)raw_spd / FARD_SPEED_DIVISOR;
        uint8_t new_spd  = (uint8_t)constrain(roundf(speed_f), 0, 99);

        // 2. Trapezoidal Odometer Integration (mm = avg_kmh * dt_us / 3600)
        if (last_pkt_us > 0) {
            uint64_t dt_us = now_us - last_pkt_us;
            if (dt_us < 500000ULL) {
                float avg_kmh = 0.5f * ((float)car.speed_kmh + speed_f);
                sub_tenth_mm += (uint32_t)((avg_kmh * (float)dt_us) / 3600.0f);

                while (sub_tenth_mm >= 100000UL) { // 100,000 mm = 0.1 km
                    sub_tenth_mm -= 100000UL;
                    current_tenths++;
                    car.odometer_km = current_tenths / 10UL;
                    car.odo_tenths  = current_tenths % 10UL;
                    changed         = true;
                    if (fram_online) saveOdometer(current_tenths);
                }
            }
        }
        last_pkt_us = now_us;

        if (new_spd != car.speed_kmh) {
            car.speed_kmh = new_spd;
            changed       = true;
        }

        if (car.speed_kmh == 0) {
            if (zero_speed_since_ms == 0) zero_speed_since_ms = millis();
        } else {
            zero_speed_since_ms = 0;
        }

        // 3. Decode Dedicated Charger Connected Pin from FarDriver SIF Packet
        bool fard_chg = (pkt[FARD_CHARGER_BYTE] & FARD_CHARGER_MASK) != 0;
        if (fard_chg != car.charger_conn) {
            car.charger_conn = fard_chg;
            changed          = true;
        }

        // 4. Decode Gear from Byte 2 (Force 'N' if Charger Immobilizer / Park is active!)
        char new_gear = 'N';
        if (!car.charger_conn) {
            if (pkt[2] & 0x04)      new_gear = 'R';
            else if (pkt[2] & 0x03) new_gear = 'F';
        }
        if (new_gear != car.gear) {
            car.gear = new_gear;
            changed  = true;
        }

        // 5. Decode Controller Fault / Overheat from Byte 1 (Bit 4)
        bool ctrl_fault = (pkt[1] & 0x10) != 0;
        if (ctrl_fault != car.overheat_controller) {
            car.overheat_controller = ctrl_fault;
            changed                 = true;
        }

        // 6. Serial Debug Print (shows Byte 2 & Byte 3 in binary/hex to spot the Charger bit!)
        if (SIF_DEBUG_SERIAL) {
            static uint32_t last_dbg_ms = 0;
            if (millis() - last_dbg_ms > 1000) {
                last_dbg_ms = millis();
                Serial.printf("[SIF] Raw: %02X %02X [%02X %02X] %02X %02X %02X %02X %02X %02X %02X %02X | Spd:%u Gear:%c Chg:%d\n",
                    pkt[0], pkt[1], pkt[2], pkt[3], pkt[4], pkt[5],
                    pkt[6], pkt[7], pkt[8], pkt[9], pkt[10], pkt[11],
                    car.speed_kmh, car.gear, car.charger_conn);
            }
        }
    }

    // 7. Save-on-Stop (NVS fallback mode): persist mileage once stopped for >= 2s
    if (!fram_online && current_tenths != last_saved_tenths &&
        car.speed_kmh == 0 && zero_speed_since_ms != 0 &&
        (millis() - zero_speed_since_ms >= 2000UL)) {
        saveOdometer(current_tenths);
    }

    // 8. Stale Signal Watchdog: drop speed to 0 km/h if no packet for > 1.2s
    if (last_pkt_us > 0 && (now_us - last_pkt_us > 1200000ULL)) {
        if (car.speed_kmh != 0) {
            car.speed_kmh = 0;
            changed       = true;
        }
        if (!fram_online && current_tenths != last_saved_tenths) {
            saveOdometer(current_tenths);
        }
    }

    return changed;
}
}
