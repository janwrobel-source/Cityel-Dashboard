#include <Arduino.h>
#include <Fonts/FreeSansBold9pt7b.h>
#include <Fonts/FreeSansBold12pt7b.h>
#include <Fonts/FreeSansBold18pt7b.h>
#include <Fonts/FreeSansBold24pt7b.h>
#include "dashboard_icons.h"
#include "dashboard_renderer.h"
#include "cityel_logo.h"

namespace {

constexpr uint8_t BAR_SEGMENT_COUNT = 10;
constexpr int16_t BAR_SEGMENT_HEIGHT = 12;
constexpr int16_t BAR_TOP_INSET = 4;
constexpr int16_t BAR_BOTTOM_INSET = 4;

constexpr uint8_t DITHER_MATRIX[8][8] = {
    {0, 32, 8, 40, 2, 34, 10, 42},
    {48, 16, 56, 24, 50, 18, 58, 26},
    {12, 44, 4, 36, 14, 46, 6, 38},
    {60, 28, 52, 20, 62, 30, 54, 22},
    {3, 35, 11, 43, 1, 33, 9, 41},
    {51, 19, 59, 27, 49, 17, 57, 25},
    {15, 47, 7, 39, 13, 45, 5, 37},
    {63, 31, 55, 23, 61, 29, 53, 21}
};

int16_t barSegmentY(int16_t y, int16_t height, uint8_t index_from_bottom) {
    uint8_t index_from_top = BAR_SEGMENT_COUNT - 1 - index_from_bottom;
    int16_t interior_height = height - BAR_TOP_INSET - BAR_BOTTOM_INSET;
    int16_t total_gaps = interior_height - (BAR_SEGMENT_COUNT * BAR_SEGMENT_HEIGHT);
    int16_t base_gap = total_gaps / (BAR_SEGMENT_COUNT - 1);
    int16_t extra_gaps = total_gaps % (BAR_SEGMENT_COUNT - 1);
    int16_t distributed_gap = (index_from_top * extra_gaps + BAR_SEGMENT_COUNT - 2) / (BAR_SEGMENT_COUNT - 1);
    return y + BAR_TOP_INSET + (index_from_top * (BAR_SEGMENT_HEIGHT + base_gap)) + distributed_gap;
}

void drawDitheredRect(DashboardDisplay &display, int16_t x, int16_t y, int16_t width, int16_t height,
                      uint8_t numerator, uint8_t denominator) {
    uint8_t threshold = (numerator * 64 + denominator / 2) / denominator;
    for (int16_t row = 0; row < height; row++) {
        for (int16_t column = 0; column < width; column++) {
            if (DITHER_MATRIX[(y + row) & 7][(x + column) & 7] < threshold) {
                display.drawPixel(x + column, y + row, GxEPD_BLACK);
            }
        }
    }
}

void drawCenteredText(DashboardDisplay &display, const char *text, int16_t cx, int16_t cy) {
    int16_t bounds_x = 0, bounds_y = 0;
    uint16_t text_w = 0, text_h = 0;
    display.getTextBounds(text, 0, 0, &bounds_x, &bounds_y, &text_w, &text_h);
    display.setCursor(cx - (int16_t)(text_w / 2) - bounds_x,
                      cy - (int16_t)(text_h / 2) - bounds_y);
    display.print(text);
}

void drawCenteredTextBaseline(DashboardDisplay &display, const char *text, int16_t cx, int16_t baseline_y) {
    int16_t bounds_x = 0, bounds_y = 0;
    uint16_t text_w = 0, text_h = 0;
    display.getTextBounds(text, 0, 0, &bounds_x, &bounds_y, &text_w, &text_h);
    display.setCursor(cx - (int16_t)(text_w / 2) - bounds_x, baseline_y);
    display.print(text);
}

void drawSpeedArch(DashboardDisplay &display, uint8_t speed_kmh) {
    const int16_t cx = 396, cy = 202, r_in = 116, r_out = 140;
    uint8_t clamped_spd = constrain(speed_kmh, 0, 75);
    float active_angle_rad = M_PI * (1.0f - ((float)clamped_spd / 75.0f));
    float cos_a = cosf(active_angle_rad);
    float sin_a = sinf(active_angle_rad);

    for (int16_t y = cy - r_out; y <= cy; y++) {
        for (int16_t x = cx - r_out; x <= cx + r_out; x++) {
            int32_t dx = x - cx, dy = cy - y;
            int32_t dist_sq = dx * dx + dy * dy;
            if (dist_sq >= (r_in * r_in) && dist_sq <= (r_out * r_out)) {
                if (dist_sq <= (r_in + 2) * (r_in + 2) || dist_sq >= (r_out - 2) * (r_out - 2) || dy <= 1) {
                    display.drawPixel(x, y, GxEPD_BLACK);
                } else if (((float)dy * cos_a - (float)dx * sin_a) >= 0.0f && (((x + y) & 1) == 0)) {
                    display.drawPixel(x, y, GxEPD_BLACK);
                }
            }
        }
    }

    // Leading-edge solid bar at current speed
    if (clamped_spd > 0 && clamped_spd < 75) {
        int16_t x_in  = cx + (int16_t)roundf(r_in * cos_a);
        int16_t y_in  = cy - (int16_t)roundf(r_in * sin_a);
        int16_t x_out = cx + (int16_t)roundf(r_out * cos_a);
        int16_t y_out = cy - (int16_t)roundf(r_out * sin_a);
        for (int16_t off = -1; off <= 1; off++) {
            display.drawLine(x_in + off, y_in, x_out + off, y_out, GxEPD_BLACK);
        }
    }

    // Uniform 5 km/h minor ticks & 15 km/h major ticks + labels (0, 15, 30, 45, 60, 75)
    display.setFont(&FreeSansBold9pt7b);
    display.setTextColor(GxEPD_BLACK);

    for (uint8_t st = 0; st <= 75; st += 5) {
        float ang = M_PI * (1.0f - ((float)st / 75.0f));
        float ca = cosf(ang);
        float sa = sinf(ang);
        bool major = (st % 15 == 0);
        int16_t tick_len = major ? 7 : 4;
        int16_t x1 = cx + (int16_t)roundf((r_out + 1) * ca);
        int16_t y1 = cy - (int16_t)roundf((r_out + 1) * sa);
        int16_t x2 = cx + (int16_t)roundf((r_out + tick_len) * ca);
        int16_t y2 = cy - (int16_t)roundf((r_out + tick_len) * sa);
        display.drawLine(x1, y1, x2, y2, GxEPD_BLACK);
        if (major) {
            display.drawLine(x1 + 1, y1, x2 + 1, y2, GxEPD_BLACK);
            int16_t lx = cx + (int16_t)roundf((r_out + 18) * ca);
            int16_t ly = cy - (int16_t)roundf((r_out + 13) * sa);
            char lbl[4];
            snprintf(lbl, sizeof(lbl), "%u", st);
            drawCenteredText(display, lbl, lx, ly);
        }
    }
}

void drawCurrentBar(DashboardDisplay &display, int16_t amps) {
    const int16_t bx = 114, by = 100, bw = 52, bh = 152;
    const int16_t zero_y = barSegmentY(by, bh, BAR_SEGMENT_COUNT - 3) - 2;

    display.drawRect(bx, by, bw, bh, GxEPD_BLACK);
    display.drawRect(bx + 1, by + 1, bw - 2, bh - 2, GxEPD_BLACK);

    // Full-width zero line & 50A interval ticks (+50, 0, -50, -100, -150, -200)
    display.fillRect(bx - 3, zero_y, bw + 8, 2, GxEPD_BLACK);
    display.fillRect(bx + bw, by, 4, 2, GxEPD_BLACK);
    display.fillRect(bx + bw, by + bh - 2, 4, 2, GxEPD_BLACK);
    for (uint8_t seg_idx : {5, 3, 1}) {
        int16_t ty = barSegmentY(by, bh, seg_idx) - 2;
        display.fillRect(bx + bw, ty, 3, 2, GxEPD_BLACK);
    }

    display.setFont(&FreeSansBold9pt7b);
    display.setTextColor(GxEPD_BLACK);
    display.setCursor(bx + bw + 5, by + 12);  display.print("+50");
    display.setCursor(bx + bw + 7, zero_y + 5); display.print("0");
    display.setCursor(bx + bw + 5, by + bh);  display.print("-200");

    int16_t clamped = constrain(amps, -200, 50);
    if (clamped > 0) {
        uint8_t magnitude = clamped;
        uint8_t chg_segs = magnitude / 25;
        for (uint8_t s = 0; s < chg_segs; s++) {
            uint8_t index_from_bottom = BAR_SEGMENT_COUNT - 2 + s;
            display.fillRect(bx + 4, barSegmentY(by, bh, index_from_bottom), bw - 8,
                             BAR_SEGMENT_HEIGHT, GxEPD_BLACK);
        }
        uint8_t remainder = magnitude % 25;
        if (remainder > 0) {
            uint8_t index_from_bottom = BAR_SEGMENT_COUNT - 2 + chg_segs;
            drawDitheredRect(display, bx + 4, barSegmentY(by, bh, index_from_bottom), bw - 8,
                             BAR_SEGMENT_HEIGHT, remainder, 25);
        }
    } else if (clamped < 0) {
        uint16_t magnitude = abs(clamped);
        uint8_t draw_segs = magnitude / 25;
        for (uint8_t s = 0; s < draw_segs; s++) {
            uint8_t index_from_bottom = BAR_SEGMENT_COUNT - 3 - s;
            display.fillRect(bx + 4, barSegmentY(by, bh, index_from_bottom), bw - 8,
                             BAR_SEGMENT_HEIGHT, GxEPD_BLACK);
        }
        uint8_t remainder = magnitude % 25;
        if (remainder > 0) {
            uint8_t index_from_bottom = BAR_SEGMENT_COUNT - 3 - draw_segs;
            drawDitheredRect(display, bx + 4, barSegmentY(by, bh, index_from_bottom), bw - 8,
                             BAR_SEGMENT_HEIGHT, remainder, 25);
        }
    }
}

void drawBatteryBar(DashboardDisplay &display, uint8_t soc) {
    const int16_t bx = 28, by = 100, bw = 52, bh = 152;
    display.fillRect(bx + 16, by - 4, 20, 4, GxEPD_BLACK);
    display.drawRect(bx, by, bw, bh, GxEPD_BLACK);
    display.drawRect(bx + 1, by + 1, bw - 2, bh - 2, GxEPD_BLACK);
    display.fillRect(bx + bw, by + bh / 2, 4, 2, GxEPD_BLACK);
    uint8_t level = constrain(soc, 0, 100);
    uint8_t segs = level / 10;
    for (uint8_t i = 0; i < segs; i++)
        display.fillRect(bx + 4, barSegmentY(by, bh, i), bw - 8, BAR_SEGMENT_HEIGHT, GxEPD_BLACK);
    uint8_t remainder = level % 10;
    if (remainder > 0 && segs < BAR_SEGMENT_COUNT) {
        drawDitheredRect(display, bx + 4, barSegmentY(by, bh, segs), bw - 8,
                         BAR_SEGMENT_HEIGHT, remainder, 10);
    }
}

void drawOdometerDigitTile(DashboardDisplay &display, int16_t x, int16_t y, int16_t width, int16_t height,
                           char digit, bool tenths) {
    uint16_t tile_color = tenths ? GxEPD_WHITE : GxEPD_BLACK;
    if (tenths) display.fillRoundRect(x + 1, y + 1, width - 2, height - 2, 4, tile_color);
    else        display.fillRect(x, y, width, height, tile_color);
    display.setFont(&FreeSansBold12pt7b);
    display.setTextColor(tenths ? GxEPD_BLACK : GxEPD_WHITE);

    char text[2] = {digit, '\0'};
    int16_t bounds_x, bounds_y;
    uint16_t text_width, text_height;
    display.getTextBounds(text, 0, 0, &bounds_x, &bounds_y, &text_width, &text_height);
    display.setCursor(x + (width - text_width) / 2 - bounds_x,
                      y + (height - text_height) / 2 - bounds_y);
    display.print(digit);
}

void drawOdometerRoller(DashboardDisplay &display, uint32_t odo_km, uint8_t tenths) {
    const int16_t ox = 282, oy = 215, ow = 198, oh = 42;
    constexpr int16_t digit_spacing = 27;
    constexpr int16_t digit_count = 6;
    const int16_t tile_width = digit_spacing;
    const int16_t tile_height = oh - 10;
    const int16_t tile_y = oy + 5;
    const int16_t tiles_x = ox + (ow - ((digit_count + 1) * tile_width)) / 2;
    display.fillRoundRect(ox, oy, ow, oh, 4, GxEPD_BLACK);
    char buf[8];
    snprintf(buf, sizeof(buf), "%06lu", (unsigned long)(odo_km % 1000000UL));
    for (int16_t i = 0; i < digit_count; i++) {
        int16_t tx = tiles_x + (i * tile_width);
        drawOdometerDigitTile(display, tx, tile_y, tile_width, tile_height, buf[i], false);
        if (i > 0) {
            display.drawFastVLine(tx, tile_y + 3, tile_height - 6, GxEPD_WHITE);
        }
    }
    drawOdometerDigitTile(display, tiles_x + (digit_count * tile_width), tile_y, tile_width, tile_height,
                          (char)('0' + (tenths % 10)), true);
    display.setFont(&FreeSansBold12pt7b);
    display.setTextColor(GxEPD_BLACK);
    display.setCursor(ox + ow + 5, oy + 31);
    display.print("km");
}

void drawClockBadge(DashboardDisplay &display, uint8_t hour, uint8_t minute, ClockSetMode set_mode) {
    const int16_t cx_box = 348, cy_box = 5, cw = 96, ch = 32;
    const int16_t cx_mid = cx_box + cw / 2;
    const int16_t cy_mid = cy_box + ch / 2;

    display.drawRoundRect(cx_box, cy_box, cw, ch, 5, GxEPD_BLACK);
    display.drawRoundRect(cx_box + 1, cy_box + 1, cw - 2, ch - 2, 4, GxEPD_BLACK);

    if (set_mode == ClockSetMode::HOURS) {
        display.fillRoundRect(cx_mid - 40, cy_box + 4, 38, ch - 8, 3, GxEPD_BLACK);
    } else if (set_mode == ClockSetMode::MINUTES) {
        display.fillRoundRect(cx_mid + 2, cy_box + 4, 38, ch - 8, 3, GxEPD_BLACK);
    }

    display.setFont(&FreeSansBold12pt7b);

    // Use reference digit "8" for a constant vertical baseline across all digits
    int16_t ref_bx = 0, ref_by = 0;
    uint16_t ref_w = 0, ref_h = 0;
    display.getTextBounds("8", 0, 0, &ref_bx, &ref_by, &ref_w, &ref_h);
    const int16_t baseline_y = cy_mid - (int16_t)(ref_h / 2) - ref_by;

    // Fixed horizontal centers for the 4 tabular digit slots
    const int16_t digit_cx[4] = {
        (int16_t)(cx_mid - 29),
        (int16_t)(cx_mid - 13),
        (int16_t)(cx_mid + 13),
        (int16_t)(cx_mid + 29)
    };
    const char digits[4] = {
        (char)('0' + ((hour / 10) % 10)),
        (char)('0' + (hour % 10)),
        (char)('0' + ((minute / 10) % 10)),
        (char)('0' + (minute % 10))
    };

    for (uint8_t i = 0; i < 4; i++) {
        bool inverted = (i < 2 && set_mode == ClockSetMode::HOURS) ||
                        (i >= 2 && set_mode == ClockSetMode::MINUTES);
        display.setTextColor(inverted ? GxEPD_WHITE : GxEPD_BLACK);
        char d_str[2] = {digits[i], '\0'};
        drawCenteredTextBaseline(display, d_str, digit_cx[i], baseline_y);
    }

    // Vertically and horizontally centered colon dots
    display.fillRect(cx_mid - 1, cy_mid - 6, 3, 3, GxEPD_BLACK);
    display.fillRect(cx_mid - 1, cy_mid + 3, 3, 3, GxEPD_BLACK);
}

void drawGearCircle(DashboardDisplay &display, int16_t cx, int16_t cy, char label, bool active) {
    if (active) { display.fillCircle(cx, cy, 20, GxEPD_BLACK); display.setTextColor(GxEPD_WHITE); }
    else { display.drawCircle(cx, cy, 20, GxEPD_BLACK); display.drawCircle(cx, cy, 19, GxEPD_BLACK); display.setTextColor(GxEPD_BLACK); }
    display.setFont(&FreeSansBold12pt7b);
    char text[2] = {label, '\0'};
    drawCenteredText(display, text, cx, cy);
}

void drawLevelBox(DashboardDisplay &display, int16_t x, int16_t y, char label, bool active) {
    if (active) { display.fillRoundRect(x, y, 40, 40, 5, GxEPD_BLACK); display.setTextColor(GxEPD_WHITE); }
    else { display.drawRoundRect(x, y, 40, 40, 5, GxEPD_BLACK); display.drawRoundRect(x + 1, y + 1, 38, 38, 4, GxEPD_BLACK); display.setTextColor(GxEPD_BLACK); }
    display.setFont(&FreeSansBold12pt7b);
    char text[2] = {label, '\0'};
    drawCenteredText(display, text, x + 20, y + 20);
}

void drawCityElLogo(DashboardDisplay &display) {
    const int16_t lx = (792 - CITYEL_LOGO_WIDTH) / 2;
    const int16_t ly = 40;
    display.drawBitmap(lx, ly, CITYEL_LOGO_BITMAP, CITYEL_LOGO_WIDTH, CITYEL_LOGO_HEIGHT, GxEPD_BLACK);
}

} // namespace

void renderDashboard(DashboardDisplay &display, const CityElState &car, bool full_refresh) {
    if (full_refresh) display.setFullWindow();
    else              display.setPartialWindow(0, 0, 792, 272);

    display.firstPage();
    do {
        display.fillScreen(GxEPD_WHITE);

        if (car.power_off) {
            drawCityElLogo(display);
            drawOdometerRoller(display, car.odometer_km, car.odo_tenths);
            continue;
        }

        display.fillRect(12, 42, 768, 2, GxEPD_BLACK);
        if (car.turn_left)       CityElIcons::drawTurnLeft(display,       20,  6, GxEPD_BLACK);
        if (car.hazard_lights)   CityElIcons::drawHazard(display,         96,  6, GxEPD_BLACK);
        if (car.parking_lights)  CityElIcons::drawParkingLights(display,  154, 6, GxEPD_BLACK);
        if (car.low_beam)        CityElIcons::drawLowBeam(display,        212, 6, GxEPD_BLACK);
        if (car.high_beam)       CityElIcons::drawHighBeam(display,       270, 6, GxEPD_BLACK);
        drawClockBadge(display, car.clock_hour, car.clock_min, car.clock_set_mode);
        if (car.handbrake)       CityElIcons::drawHandbrake(display,      490, 6, GxEPD_BLACK);
        if (car.brake_fluid_err) CityElIcons::drawBrakeFluidFault(display,548, 6, GxEPD_BLACK);
        if (car.charger_conn)    CityElIcons::drawChargerIcon(display,    606, 6, GxEPD_BLACK);
        if (car.isOverheated())  CityElIcons::drawOverheatWarning(display,664, 6, GxEPD_BLACK);
        if (car.turn_right)      CityElIcons::drawTurnRight(display,      740, 6, GxEPD_BLACK);

        display.setFont(&FreeSansBold12pt7b);
        display.setTextColor(GxEPD_BLACK);
        char soc_str[8];
        snprintf(soc_str, sizeof(soc_str), "%u%%", car.battery_soc);
        drawCenteredTextBaseline(display, "BAT", 28 + 26, 68);
        drawCenteredTextBaseline(display, soc_str, 28 + 26, 92);
        drawBatteryBar(display, car.battery_soc);

        char amp_str[8];
        snprintf(amp_str, sizeof(amp_str), "%+dA", car.current_amps);
        drawCenteredTextBaseline(display, "AMP", 114 + 26, 68);
        drawCenteredTextBaseline(display, amp_str, 114 + 26, 92);
        drawCurrentBar(display, car.current_amps);

        drawSpeedArch(display, car.speed_kmh);
        display.setFont(&FreeSansBold24pt7b);
        display.setTextSize(2);
        char spd_str[4];
        snprintf(spd_str, sizeof(spd_str), "%u", car.speed_kmh);
        drawCenteredTextBaseline(display, spd_str, 396, 174);
        display.setTextSize(1);

        display.setFont(&FreeSansBold12pt7b);
        drawCenteredTextBaseline(display, "km/h", 396, 202);
        drawOdometerRoller(display, car.odometer_km, car.odo_tenths);

        drawGearCircle(display, 638, 76, 'F', car.gear == 'F');
        drawGearCircle(display, 690, 76, 'N', car.gear == 'N');
        drawGearCircle(display, 742, 76, 'R', car.gear == 'R');

        display.setFont(&FreeSansBold12pt7b);
        drawCenteredTextBaseline(display, "FAN", 716, 128);
        CityElIcons::drawFanIcon(display, 620, 138, GxEPD_BLACK);
        drawLevelBox(display, 672, 136, '1', car.fan_level == 1);
        drawLevelBox(display, 720, 136, '2', car.fan_level == 2);

        drawCenteredTextBaseline(display, "HEAT", 716, 206);
        CityElIcons::drawDefrosterIcon(display, 620, 216, GxEPD_BLACK);
        drawLevelBox(display, 672, 214, '1', car.heat_level == 1);
        drawLevelBox(display, 720, 214, '2', car.heat_level == 2);
    } while (display.nextPage());
}