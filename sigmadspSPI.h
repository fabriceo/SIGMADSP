#ifndef SIGMADSPSPI_H
#define SIGMADSPSPI_H

// this libray uses the standard arduino SPI library
// tested on ESP8266 but expected to work on all platform compatible with Arduino spi library
#include <Arduino.h>
#include "SPI.h"

#define SIGMADSP_DM0_BEGIN  0
#define SIGMADSP_DM0_END    0x5000
#define SIGMADSP_DM1_BEGIN  0x6000
#define SIGMADSP_DM1_END    0xB000
#define SIGMADSP_PM_BEGIN   0xC000
#define SIGMADSP_PM_END     0xF000

#define SIGMADSP_READ 1
#define SIGMADSP_WRITE 0
#define SIGMADSP_LOWER_PAGE 1
#define SIGMADSP_UPPER_PAGE 2


// some basic registers extracted from generic _IC_1_REG.h
#define REG_HIBERNATE_ADDR                   0xF400
#define REG_START_PULSE_ADDR                 0xF401
#define REG_START_CORE_ADDR                  0xF402
#define REG_KILL_CORE_ADDR                   0xF403
#define REG_START_ADDRESS_ADDR               0xF404
#define REG_CORE_STATUS_ADDR                 0xF405
#define REG_POWER_ENABLE0_ADDR               0xF050
#define REG_POWER_ENABLE1_ADDR               0xF051
#define REG_SOFT_RESET_ADDR                  0xF890
#define REG_SECOND_PAGE_ENABLE_ADDR          0xF899 // only on ADAU146x

class sigmadspClass {
public:
    sigmadspClass() :   archNum(0), CS(0), CS2(0), currentPage(0), SPIclock(0) {}
    void fastSPI();
    void slowSPI();
    void select();
    void unselect();
    void swapCS();
    void begin(const char * archi, uint8_t chipSelect);
    void begin(const char * archi, uint8_t chipSelect1, uint8_t chipSelect2);
    void end();
    void writeRegister(uint16_t address, uint16_t value);
    uint16_t readRegister(uint16_t address);
    void writeValue(uint16_t address, uint32_t value);
    uint32_t readValue(uint16_t address);
    uint32_t * writeArray(uint16_t address, uint16_t number, uint32_t *p);
    void writeTable(uint32_t *p);
    void writeMultipleBytes(uint16_t address, uint16_t number, unsigned char *p);
    void memoryPage(uint8_t page);
    void softReset(uint16_t value);
    void softResetCycle();
    const char * arch;
    int archNum;
private:
    uint8_t CS;
    uint8_t CS2;
    uint8_t currentPage;
    uint32_t SPIclock;
};

#if !defined(NO_GLOBAL_INSTANCES) && !defined(NO_GLOBAL_SIGMADSP)
extern SPISettings SIGMADSP_SPI_fast;
extern SPISettings SIGMADSP_SPI_slow;
extern sigmadspClass DSP;
#endif

#endif