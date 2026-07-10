#ifndef Display_h
#define Display_h
#include <Adafruit_SSD1306.h>

class Display {
    private:
    Adafruit_SSD1306 *display;
    public:
    Display();
    void displayText(String text);
    void displayHomeScreen(String ipLine, String orLine, String mdnsLine);
    // SCRUBBY (Step 4): bigger progress UI - large centered percentage
    // above a filled progress bar. Used by Runner during drawing.
    void displayProgress(int percent);
};
#endif
