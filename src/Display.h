#pragma once

#include <stdint.h>

#include <SPI.h>

#include "Vector.h"
#include "Config.h"

namespace ts {

class Display {
public:
    constexpr Display()
        : _csPin(-1)
        , _rstPin(-1)
        , _dcPin(-1)
        , _busyPin(-1)
        , _pwrPin(-1)
        , _pSpi(nullptr)
    {
    }
    constexpr virtual ~Display() = default;
    
    constexpr virtual Extent2i extent() { return Extent2i{ 800, 480 }; }
    bool begin(uint16_t csPin, uint16_t rstPint, uint16_t dcPin, uint16_t busyPin, uint16_t pwrPin, SPIClass& spi = SPI);
    void end();
    void present(const uint8_t* buffer);
    void sleep();
    void clear();
    void reset();
 
private:
    static void delayMs(unsigned int delaytime);
    
    void spiTransfer(uint8_t data);
    void waitForIdle();
    void sendCommand(uint8_t command);
    void sendData(uint8_t data);
    void setLut();
	void setLut(unsigned char *lut);
    void setLutByHost(unsigned char *lut_vcom, unsigned char *lut_ww, unsigned char *lut_bw, unsigned char *lut_wb, unsigned char *lut_bb);


    uint16_t    _csPin;
    uint16_t    _rstPin;
    uint16_t    _dcPin;
    uint16_t    _busyPin;
    uint16_t    _pwrPin;
    SPIClass*   _pSpi;
};

} /* namespace ts */
