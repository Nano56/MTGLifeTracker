#ifndef ADAFRUIT_GFX_H
#define ADAFRUIT_GFX_H

#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <stdarg.h>

class Adafruit_GFX {
public:
    Adafruit_GFX(int16_t w, int16_t h) : _width(w), _height(h), cursor_x(0), cursor_y(0), textsize(1), textcolor(0xFFFF), textbgcolor(0x0000) {}
    virtual ~Adafruit_GFX() {}

    virtual void drawFastVLine(int16_t, int16_t, int16_t, uint16_t) {}
    virtual void drawFastHLine(int16_t, int16_t, int16_t, uint16_t) {}
    virtual void fillRect(int16_t, int16_t, int16_t, int16_t, uint16_t) {}
    virtual void drawRect(int16_t, int16_t, int16_t, int16_t, uint16_t) {}
    virtual void fillRoundRect(int16_t, int16_t, int16_t, int16_t, int16_t, uint16_t) {}
    virtual void drawRoundRect(int16_t, int16_t, int16_t, int16_t, int16_t, uint16_t) {}
    virtual void fillScreen(uint16_t) {}
    virtual void setRotation(uint8_t) {}

    void setCursor(int16_t x, int16_t y) { cursor_x = x; cursor_y = y; }
    void setTextSize(uint8_t s) { textsize = s; }
    void setTextColor(uint16_t c) { textcolor = c; textbgcolor = c; }
    void setTextColor(uint16_t c, uint16_t bg) { textcolor = c; textbgcolor = bg; }

    size_t print(const char* s) { (void)s; return 0; }
    size_t printf(const char* format, ...) {
        va_list arg;
        va_start(arg, format);
        char buf[256];
        vsnprintf(buf, sizeof(buf), format, arg);
        va_end(arg);
        return 0;
    }

protected:
    int16_t _width, _height;
    int16_t cursor_x, cursor_y;
    uint8_t textsize;
    uint16_t textcolor, textbgcolor;
};

#endif // ADAFRUIT_GFX_H

