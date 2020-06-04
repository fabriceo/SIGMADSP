// primary functions to access sigmadsp memory using SPI.H
#include "SPI.h"
#include "sigmadspSPI.h"

SPISettings SIGMADSP_SPI_fast(4000000, MSBFIRST, SPI_MODE0); 
SPISettings SIGMADSP_SPI_slow( 250000, MSBFIRST, SPI_MODE0); 

void sigmadspClass::fastSPI(){
    if (SPIclock != SIGMADSP_SPI_fast._clock) {
        SPI.beginTransaction(SIGMADSP_SPI_fast);
        SPIclock = SIGMADSP_SPI_fast._clock;
    }
}
void sigmadspClass::slowSPI(){
    if (SPIclock != SIGMADSP_SPI_slow._clock) {
        SPI.beginTransaction(SIGMADSP_SPI_slow);
        SPIclock = SIGMADSP_SPI_slow._clock;
    }
} 

void sigmadspClass::select(){
    delayMicroseconds(5);
    digitalWrite( CS, LOW);
    if (CS2 != CS) digitalWrite( CS2, LOW);
}

void sigmadspClass::unselect(){
    digitalWrite( CS, HIGH);
    if (CS2 != CS) digitalWrite( CS2, HIGH);
    delayMicroseconds(5);
}

void sigmadspClass::swapCS(){
    int old = CS;
    CS = CS2; 
    CS2 = old;
}

void sigmadspClass::begin(const char * archi, uint8_t chipSelect1, uint8_t chipSelect2){
    arch = archi;
    if ((arch[0]=='A')&&(arch[1]=='D')&&(arch[2]=='A')&&(arch[3]=='U')) {
        archNum = (arch[4] & 15)*1000 + (arch[5] & 15)*100 + (arch[6] & 15)*10 + (arch[7] & 15);
        printf("archi = ADAU%d\n",archNum);
    }
    CS  = chipSelect1;
    CS2 = chipSelect2;
    pinMode( CS, OUTPUT);
    if (CS2 != CS) pinMode( CS2, OUTPUT);
    slowSPI();
    unselect();
    for (int i=0; i<3; i++) {
        select();
        SPI.write(0);
        unselect();
    }   
}

void sigmadspClass::begin(const char * archi, uint8_t chipSelectPin){
    begin(archi, chipSelectPin, chipSelectPin);
}

void sigmadspClass::end(){
    unselect();
}

uint16_t sigmadspClass::readRegister(uint16_t address){
    uint8_t oldCS2 = CS2; CS2 = CS;
    select();
    SPI.write(SIGMADSP_READ);
    SPI.write16(address);
    uint16_t value = SPI.transfer16(0);     
    unselect();
    CS2 = oldCS2;
    return value;
}

void sigmadspClass::writeRegister(uint16_t address, uint16_t value){
    select();
    SPI.write(SIGMADSP_WRITE);
    SPI.write16(address);
    SPI.write16(value);
    unselect();
}

void sigmadspClass::memoryPage(uint8_t page){
    if (arch[6] == '6') 
        if ((page == SIGMADSP_LOWER_PAGE) || (page == SIGMADSP_UPPER_PAGE))
            if (page != currentPage) {
                writeRegister(REG_SECOND_PAGE_ENABLE_ADDR,page - 1);
                currentPage = page;
            }    
}

void sigmadspClass::writeMultipleBytes(uint16_t address, uint16_t number, unsigned char *p){
    uint16_t max = 0;
    uint16_t min = 0;
    if ((address>=SIGMADSP_DM0_BEGIN)&&(address < SIGMADSP_DM0_END)) { 
        min = SIGMADSP_DM0_BEGIN; max = SIGMADSP_DM0_END; }
    if ((address>=SIGMADSP_DM1_BEGIN)&&(address < SIGMADSP_DM1_END)) {
        min = SIGMADSP_DM1_BEGIN; max = SIGMADSP_DM1_END; }
    if ((address>=SIGMADSP_PM_BEGIN)&&(address < SIGMADSP_PM_END)) {
        min = SIGMADSP_PM_BEGIN; max = SIGMADSP_PM_END;  }
    if ((address>=0xF000)&&(address < 0xFF00)) {
        min = 0xF000; max = 0xFF00;  }
    if (max-min) {
        select();
        SPI.write(SIGMADSP_WRITE);
        SPI.write16(address);
        for (uint16_t i=0; i<number; i++) {
            if ((address+i)>=max) {
                if ((max == 0xFF00) || (arch[6] != '6')) break;
                unselect();
                memoryPage(SIGMADSP_UPPER_PAGE);
                address = -i;
                select();
                SPI.write(SIGMADSP_WRITE);
                SPI.write16(min);
                min = -1;
            }
            if (p) SPI.write(p[i]);
            else SPI.write(0);
        }
        unselect();
        if (min == -1) memoryPage(SIGMADSP_LOWER_PAGE);
    }
}


void sigmadspClass::writeValue(uint16_t address, uint32_t value){
    select();
    SPI.write(SIGMADSP_WRITE);
    SPI.write16(address);   
    SPI.write32(value);
    unselect();
}

uint32_t sigmadspClass::readValue(uint16_t address){
    uint8_t oldCS2 = CS2; CS2 = CS;
    select();
    SPI.write(SIGMADSP_READ);
    SPI.write16(address);   
    uint32_t msb = SPI.transfer16(0);     
    uint32_t lsb = SPI.transfer16(0);     
    unselect();
    CS2 = oldCS2;
    return (msb << 16) | lsb;
}

uint32_t * sigmadspClass::writeArray(uint16_t address, uint16_t number, uint32_t *p){
    select();
    SPI.write(SIGMADSP_WRITE); // write
    SPI.write16(address);   
    for (uint16_t i=0; i<number; i++) SPI.write32(p[i]);
    unselect();
    return p+number;
}

void sigmadspClass::writeTable(uint32_t *p){
    while (*p) {
        uint16_t address = *p++;
        uint16_t number  = *p++; 
        p = writeArray(address, number, p);
    }    
}


void sigmadspClass::softReset(uint16_t value){
    select();
    SPI.write(SIGMADSP_WRITE);
    SPI.write16(REG_SOFT_RESET_ADDR);
    SPI.write16(value);
    unselect();
}

void sigmadspClass::softResetCycle(){
    softReset(0);
    softReset(1);
}


#if !defined(NO_GLOBAL_INSTANCES) && !defined(NO_GLOBAL_SIGMADSP)
#error shit
sigmadspClass DSP;
void DSPwriteMultipleBytes(uint16_t address, uint16_t number, unsigned char *p) {
    if (address == REG_SOFT_RESET_ADDR) DSP.slowSPI();\
    DSP.writeMultipleBytes( address,  number,  p);
}
uint16_t DSPdownloadProgram(){
    DSP.memoryPage(SIGMADSP_LOWER_PAGE);
    DSP.fastSPI();
    return 1; // this will accept the download of the program included in the sigmadsp_IC_1.h
}
uint16_t DSPdownloadParam(){
    DSP.memoryPage(SIGMADSP_LOWER_PAGE);
    return 1; // this will accept the download of the param related to the program included in the .h
}

#endif