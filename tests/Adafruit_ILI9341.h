#ifndef ADAFRUIT_ILI9341_H
#define ADAFRUIT_ILI9341_H

#include "Adafruit_GFX.h"

class Adafruit_ILI9341 : public Adafruit_GFX {
public:
    Adafruit_ILI9341(int8_t cs, int8_t dc, int8_t rst = -1)
        : Adafruit_GFX(240, 320), _cs(cs), _dc(dc), _rst(rst) {}

    void begin(uint32_t freq = 0) { (void)freq; }

private:
    int8_t _cs, _dc, _rst;
};

#endif // ADAFRUIT_ILI9341_H

