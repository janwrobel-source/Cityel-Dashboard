#pragma once
#include <Adafruit_GFX.h>

// Draws all Idea 3 Top Status Icons & Right-Stack Icons at exact pixel coordinates
namespace CityElIcons {

    // 1. Left Turn Arrow (32x32)
    inline void drawTurnLeft(Adafruit_GFX &d, int16_t x, int16_t y, uint16_t col) {
        d.fillTriangle(x + 2, y + 16, x + 16, y + 4, x + 16, y + 28, col);
        d.fillRect(x + 16, y + 10, 14, 13, col);
    }

    // 2. Right Turn Arrow (32x32)
    inline void drawTurnRight(Adafruit_GFX &d, int16_t x, int16_t y, uint16_t col) {
        d.fillTriangle(x + 30, y + 16, x + 16, y + 4, x + 16, y + 28, col);
        d.fillRect(x + 2, y + 10, 14, 13, col);
    }

    // 3. Hazard Warning Triangle with Exclamation Mark (32x32)
    inline void drawHazard(Adafruit_GFX &d, int16_t x, int16_t y, uint16_t col) {
        for (int i = 0; i < 2; i++) {
            d.drawTriangle(x + 16, y + 3 + i, x + 2 + i, y + 28 - i, x + 30 - i, y + 28 - i, col);
        }
        d.fillRoundRect(x + 15, y + 11, 3, 9, 1, col);
        d.fillCircle(x + 16, y + 23, 1, col);
    }

    // 4. Parking Lights / Side Marker Bulb (32x32)
    inline void drawParkingLights(Adafruit_GFX &d, int16_t x, int16_t y, uint16_t col) {
        d.drawCircle(x + 16, y + 16, 7, col);
        d.drawCircle(x + 16, y + 16, 6, col);
        d.fillRect(x + 13, y + 6, 6, 4, col);
        // 8 radial rays around bulb
        d.drawLine(x + 16, y + 2, x + 16, y + 5, col);
        d.drawLine(x + 16, y + 26, x + 16, y + 29, col);
        d.drawLine(x + 3,  y + 16, x + 6,  y + 16, col);
        d.drawLine(x + 26, y + 16, x + 29, y + 16, col);
        d.drawLine(x + 6,  y + 6,  x + 9,  y + 9,  col);
        d.drawLine(x + 26, y + 6,  x + 23, y + 9,  col);
        d.drawLine(x + 6,  y + 26, x + 9,  y + 23, col);
        d.drawLine(x + 26, y + 26, x + 23, y + 23, col);
    }

    // 5. Low Beam (32x32) - Angled Downward Rays + D-Shaped Headlamp
    inline void drawLowBeam(Adafruit_GFX &d, int16_t x, int16_t y, uint16_t col) {
        // Headlamp D-Dome on right half (x+14 .. x+28)
        d.drawLine(x + 15, y + 6, x + 15, y + 26, col);
        d.drawLine(x + 16, y + 6, x + 16, y + 26, col);
        d.drawCircleHelper(x + 18, y + 16, 10, 0x06, col); // Right-side arc
        d.drawCircleHelper(x + 17, y + 16, 10, 0x06, col);
        d.drawLine(x + 15, y + 6,  x + 18, y + 6,  col);
        d.drawLine(x + 15, y + 26, x + 18, y + 26, col);
        // 4 Distinct Downward-Slanted Rays on left half
        for (int i = 0; i < 4; i++) {
            int ry = y + 7 + (i * 5);
            d.drawLine(x + 12, ry,     x + 2, ry + 4, col);
            d.drawLine(x + 12, ry + 1, x + 2, ry + 5, col);
        }
    }

    // 6. High Beam (32x32) - Horizontal Straight Rays + D-Shaped Headlamp
    inline void drawHighBeam(Adafruit_GFX &d, int16_t x, int16_t y, uint16_t col) {
        // Headlamp D-Dome on right half
        d.drawLine(x + 15, y + 6, x + 15, y + 26, col);
        d.drawLine(x + 16, y + 6, x + 16, y + 26, col);
        d.drawCircleHelper(x + 18, y + 16, 10, 0x06, col);
        d.drawCircleHelper(x + 17, y + 16, 10, 0x06, col);
        d.drawLine(x + 15, y + 6,  x + 18, y + 6,  col);
        d.drawLine(x + 15, y + 26, x + 18, y + 26, col);
        // 4 Horizontal Parallel Rays on left half
        for (int i = 0; i < 4; i++) {
            int ry = y + 7 + (i * 6);
            d.fillRect(x + 2, ry, 10, 2, col);
        }
    }

    // 7. Handbrake ((P)) (32x32) - Inner circle + side brake shoes + centered bold 'P'
    inline void drawHandbrake(Adafruit_GFX &d, int16_t x, int16_t y, uint16_t col) {
        int16_t cx = x + 16, cy = y + 16;

        // 1. Inner complete circle (2px thick, radius 9..10)
        d.drawCircle(cx, cy, 10, col);
        d.drawCircle(cx, cy, 9,  col);

        // 2. Left & Right curved brake shoes (2px thick, clipped to dy = -9..+9 so top/bottom stay open!)
        for (int16_t dy = -9; dy <= 9; dy++) {
            int16_t dx = (int16_t)roundf(sqrtf(14.0f * 14.0f - (float)(dy * dy)));
            // Left shoe arc
            d.drawPixel(cx - dx,     cy + dy, col);
            d.drawPixel(cx - dx + 1, cy + dy, col);
            // Right shoe arc
            d.drawPixel(cx + dx,     cy + dy, col);
            d.drawPixel(cx + dx - 1, cy + dy, col);
        }

        // 3. Crisp, centered bold 'P' (7px wide: cx-3..cx+3, 11px tall: cy-5..cy+5)
        d.fillRect(cx - 3, cy - 5, 2, 11, col); // Vertical stem
        d.fillRect(cx - 1, cy - 5, 4, 2,  col); // Top bar of bowl
        d.fillRect(cx - 1, cy,     4, 2,  col); // Bottom bar of bowl
        d.fillRect(cx + 2, cy - 4, 2, 5,  col); // Rounded right edge of bowl
    }
    
    inline void drawBrakeFluidFault(Adafruit_GFX &d, int16_t x, int16_t y, uint16_t col) {
        int16_t cx = x + 16, cy = y + 16;

        // 1. Inner complete circle (2px thick, radius 9..10)
        d.drawCircle(cx, cy, 10, col);
        d.drawCircle(cx, cy, 9,  col);

        // 2. Thick outer brake shoes on Left & Right (3px thick, clipped to dy = -10..+10)
        for (int16_t dy = -10; dy <= 10; dy++) {
            int16_t dx = (int16_t)roundf(sqrtf(14.5f * 14.5f - (float)(dy * dy)));
            // Left thick shoe
            d.drawFastHLine(cx - dx, cy + dy, 3, col);
            // Right thick shoe
            d.drawFastHLine(cx + dx - 2, cy + dy, 3, col);
        }

        // 3. Centered Liquid Droplet inside circle
        d.fillTriangle(cx, cy - 6, cx - 4, cy, cx + 4, cy, col);
        d.fillCircle(cx, cy + 1, 4, col);
    }

    // 9. Charger Connected (32x32) - Battery Outline + Plug Inside
    inline void drawChargerIcon(Adafruit_GFX &d, int16_t x, int16_t y, uint16_t col) {
        d.drawRect(x + 2, y + 8, 28, 20, col);
        d.drawRect(x + 3, y + 9, 26, 18, col);
        d.fillRect(x + 6,  y + 5, 5, 3, col); // Left terminal
        d.fillRect(x + 21, y + 5, 5, 3, col); // Right terminal
        // Plug body & prongs inside battery
        d.drawLine(x + 4, y + 18, x + 13, y + 18, col);
        d.fillRect(x + 13, y + 14, 6, 9, col);
        d.drawLine(x + 19, y + 16, x + 23, y + 16, col);
        d.drawLine(x + 19, y + 20, x + 23, y + 20, col);
    }

    // 10. Overheating Alert (32x32) - Thermometer with Bulb, Degree Ticks & Wavy Heat Lines
    inline void drawOverheatWarning(Adafruit_GFX &d, int16_t x, int16_t y, uint16_t col) {
        int16_t cx = x + 15;

        // 1. Thermometer Bulb at bottom-center (radius 5)
        d.fillCircle(cx, y + 23, 5, col);

        // 2. Vertical Thermometer Stem (x+13..x+17, y+4..y+20)
        d.fillRect(cx - 2, y + 5, 5, 16, col);
        d.drawFastHLine(cx - 1, y + 4, 3, col); // Rounded top cap

        // 3. Three Temperature Tick Marks on the Right of the Stem
        d.fillRect(cx + 4, y + 7,  5, 2, col); // High tick
        d.fillRect(cx + 4, y + 11, 4, 2, col); // Mid tick
        d.fillRect(cx + 4, y + 15, 5, 2, col); // Low tick

        // 4. Two Wavy Heat Lines Flanking the Bulb (y+20 and y+29)
        for (int16_t dx = -11; dx <= 12; dx++) {
            int16_t wave = ((dx + 12) % 6 < 3) ? 0 : 1;
            // Upper wave (split around the thermometer stem/bulb)
            if (dx < -5 || dx > 5) {
                d.drawPixel(cx + dx, y + 20 + wave, col);
                d.drawPixel(cx + dx, y + 21 + wave, col);
            }
            // Lower wave (full width under bulb)
            d.drawPixel(cx + dx, y + 29 + wave, col);
            d.drawPixel(cx + dx, y + 30 + wave, col);
        }
    }

    // 11. Cabin Fan Icon (36x36) - Circle + 4 Swept Impeller Blades
    inline void drawFanIcon(Adafruit_GFX &d, int16_t x, int16_t y, uint16_t col) {
        int16_t cx = x + 18, cy = y + 18;
        d.drawCircle(cx, cy, 17, col);
        d.drawCircle(cx, cy, 16, col);
        d.fillCircle(cx, cy, 3, col);
        // 4 Swept impeller blades connected to center hub
        d.fillTriangle(cx, cy - 2, cx, cy - 12, cx + 5, cy - 10, col);
        d.fillCircle(cx + 2, cy - 8, 3, col);
        d.fillTriangle(cx + 2, cy, cx + 12, cy, cx + 10, cy + 5, col);
        d.fillCircle(cx + 8, cy + 2, 3, col);
        d.fillTriangle(cx, cy + 2, cx, cy + 12, cx - 5, cy + 10, col);
        d.fillCircle(cx - 2, cy + 8, 3, col);
        d.fillTriangle(cx - 2, cy, cx - 12, cy, cx - 10, cy - 5, col);
        d.fillCircle(cx - 8, cy - 2, 3, col);
    }

    // 12. Windshield Defroster / Heating Icon (36x36) - Fixed Standard ISO Symbol!
    inline void drawDefrosterIcon(Adafruit_GFX &d, int16_t x, int16_t y, uint16_t col) {
        int16_t cx = x + 18, cy = y + 18;
        d.drawCircle(cx, cy, 17, col);
        d.drawCircle(cx, cy, 16, col);
        // Curved trapezoidal windshield frame
        d.drawLine(x + 7,  y + 12, x + 29, y + 12, col); // Top arc chord
        d.drawLine(x + 10, y + 22, x + 26, y + 22, col); // Bottom arc chord
        d.drawLine(x + 7,  y + 12, x + 10, y + 22, col); // Left pillar
        d.drawLine(x + 29, y + 12, x + 26, y + 22, col); // Right pillar
        // 3 Vertical rising heat arrows through windshield
        for (int i = -1; i <= 1; i++) {
            int ax = cx + (i * 5);
            d.drawLine(ax, y + 15, ax, y + 27, col);
            d.fillTriangle(ax, y + 13, ax - 2, y + 16, ax + 2, y + 16, col);
        }
    }
}