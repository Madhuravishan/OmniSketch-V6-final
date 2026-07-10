#include "display.h"
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels
Display::Display() {
    display = new Adafruit_SSD1306(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);
    if (!display->begin(SSD1306_SWITCHCAPVCC, 0x3C))
    { // Address 0x3D for 128x64
        Serial.println(F("SSD1306 allocation failed"));
        throw std::invalid_argument("not ready");
    }
    delay(2000);
    display->setRotation(2);
    display->clearDisplay();
    display->setTextColor(WHITE);
    display->setTextSize(1);
    display->display();
}

void Display::displayText(String text)
{
    int16_t x1;
    int16_t y1;
    uint16_t width;
    uint16_t height;

    // SCRUBBY (Step 4): defensive setTextSize(1). displayProgress() bumps
    // the size to 3 for the big percentage; if displayText is called
    // after that without resetting, ordinary status messages would
    // print huge and run off the screen.
    display->setTextSize(1);

    display->getTextBounds(text, 0, 0, &x1, &y1, &width, &height);

    // display on horizontal and vertical center
    display->clearDisplay(); // clear display
    display->setCursor((SCREEN_WIDTH - width) / 2, (SCREEN_HEIGHT - height) / 2);
    display->println(text); // text to display
    display->display();
    Serial.println("Displayed " + text);
}

void Display::displayHomeScreen(String ipLine, String orLine, String mdnsLine) {
    display->clearDisplay();
    display->setTextSize(1);   // SCRUBBY (Step 4): defensive, same reason

    int16_t x1;
    int16_t y1;
    uint16_t width;
    uint16_t height;

    display->getTextBounds(ipLine, 0, 0, &x1, &y1, &width, &height);
    display->setCursor((SCREEN_WIDTH - width) / 2, 10);
    display->println(ipLine);

    display->getTextBounds(orLine, 0, 0, &x1, &y1, &width, &height);
    display->setCursor((SCREEN_WIDTH - width) / 2, 10 + SCREEN_HEIGHT / 3);
    display->println(orLine);

    display->getTextBounds(mdnsLine, 0, 0, &x1, &y1, &width, &height);
    display->setCursor((SCREEN_WIDTH - width) / 2, 10 + SCREEN_HEIGHT / 3 * 2);
    display->println(mdnsLine);

    display->display();
}

// =====================================================================
//  SCRUBBY (Step 4): large progress display.
//
//  Layout on the 128x64 OLED:
//
//      0 +----------------------------+
//        |                            |
//      8 |        ____                |
//        |        |  | %              |  <- size-3 text (18x24 per char)
//     32 |        ----                |     "100%"  = 72 px wide
//        |                            |     "5%"    = 36 px wide
//     40 |  +----------------------+  |  <- progress bar outline
//        |  |##############        |  |     108 px wide, 16 px tall
//     56 |  +----------------------+  |
//     63 +----------------------------+
//
//  Called from Runner::run() on each ~5% progress milestone (and at
//  100%). At 400 kHz I2C this redraw takes ~25 ms.
// =====================================================================
void Display::displayProgress(int percent) {
    if (percent < 0)   percent = 0;
    if (percent > 100) percent = 100;

    display->clearDisplay();

    // ---- Big percentage text (size 3) ----
    display->setTextSize(3);
    String text = String(percent) + "%";
    int16_t x1, y1;
    uint16_t textWidth, textHeight;
    display->getTextBounds(text, 0, 0, &x1, &y1, &textWidth, &textHeight);
    int16_t textX = (SCREEN_WIDTH - (int16_t)textWidth) / 2;
    display->setCursor(textX, 4);
    display->println(text);

    // Reset to size 1 for any displayText/displayHomeScreen calls afterwards
    display->setTextSize(1);

    // ---- Progress bar ----
    const int barWidth  = 108;
    const int barHeight = 16;
    const int barX      = (SCREEN_WIDTH - barWidth) / 2;
    const int barY      = 40;

    // Outline
    display->drawRect(barX, barY, barWidth, barHeight, WHITE);

    // Filled inner portion (2 px padding inside the outline)
    int innerMaxWidth = barWidth - 4;
    int fillWidth = (innerMaxWidth * percent) / 100;
    if (fillWidth > 0) {
        display->fillRect(barX + 2, barY + 2, fillWidth, barHeight - 4, WHITE);
    }

    display->display();
    Serial.println("Displayed progress " + String(percent) + "%");
}
