
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

// this library gives the possibility to have 2 DSP in paralell
// receiving the same program and same parameters.
// if the CS2 is different from CS then Both are activate at same time (only for write)
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

// used to swap the main CS used for reading
void sigmadspClass::swapCS(){
    int old = CS;
    CS = CS2; 
    CS2 = old;
}

// pass the architecture variable comming from .h file
// and pass the 2 chip select pin, if 2 devices are treated in paralelle with same program and parameters
void sigmadspClass::begin(const char * archi, uint16_t safeload, uint8_t chipSelect1, uint8_t chipSelect2){
    arch = archi;
    if ((arch[0]=='A')&&(arch[1]=='D')&&(arch[2]=='A')&&(arch[3]=='U')) 
        archNum = (arch[4] & 15)*1000 + (arch[5] & 15)*100 + (arch[6] & 15)*10 + (arch[7] & 15);
    currentPage = (arch[6] == '6') ? SIGMADSP_LOWER_PAGE : 0;
    safeloadAddr = safeload;
    CS  = chipSelect1;
    CS2 = chipSelect2;
    pinMode( CS, OUTPUT);
    if (CS2 != CS) pinMode( CS2, OUTPUT);
    slowSPI();
    unselect();
    // this will enable the SPI slave (default is I2C)
    for (int i=0; i<3; i++) {
        select();
        SPI.write(0);
        unselect(); }   
}

void sigmadspClass::begin(const char * archi, uint16_t safeload, uint8_t chipSelectPin){
    begin(archi, safeload, chipSelectPin, chipSelectPin);
}

void sigmadspClass::end(){
    unselect();
}

int sigmadspClass::identifyMemoryArea(uint16_t address){

    if ((address>=SIGMADSP_DM0_BEGIN)&&(address < SIGMADSP_DM0_END)) { 
        memMin = SIGMADSP_DM0_BEGIN; memMax = SIGMADSP_DM0_END; return 0; }
    if ((address>=SIGMADSP_DM1_BEGIN)&&(address < SIGMADSP_DM1_END)) {
        memMin = SIGMADSP_DM1_BEGIN; memMax = SIGMADSP_DM1_END; return 1; }
    if ((address>=SIGMADSP_PM_BEGIN)&&(address < SIGMADSP_PM_END)) {
        memMin = SIGMADSP_PM_BEGIN; memMax = SIGMADSP_PM_END;  return 2; }
    if ((address>=SIGMADSP_REG_BEGIN)&&(address < SIGMADSP_REG_END)) {
        memMin = SIGMADSP_REG_BEGIN; memMax = SIGMADSP_REG_END;  return 3; }
    memMin = address;
    memMax = address;
    return -1;
}

//select the device, send the write command and the adress
void sigmadspClass::write(uint16_t address){
    select();
    SPI.write(SIGMADSP_WRITE);
    SPI.write16(address);
}

void sigmadspClass::read(uint16_t address){
    select();
    SPI.write(SIGMADSP_READ);
    SPI.write16(address);
}

uint16_t sigmadspClass::readRegister(uint16_t address){
    uint8_t oldCS2 = CS2; CS2 = CS;
    read(address);
    uint16_t value = SPI.transfer16(0);     
    unselect();
    CS2 = oldCS2;
    return value;
}

void sigmadspClass::writeRegister(uint16_t address, uint16_t value){
    write(address);
    SPI.write16(value);
    unselect();
}

void sigmadspClass::memoryPage(uint8_t page){
    if ((page == SIGMADSP_LOWER_PAGE) || (page == SIGMADSP_UPPER_PAGE))
        if (page != currentPage) {
            writeRegister(REG_SECOND_PAGE_ENABLE_ADDR, page - 1);
            currentPage = page;
        }    
}

void sigmadspClass::writeMultipleBytes(uint16_t address, uint16_t number, unsigned char *p){
    int oldPage = currentPage;    
    if (identifyMemoryArea(address)>=0) {
        write(address);
        for (uint16_t i=0; i<number; i++) {
            if ((address+i)>=memMax) {
                if (currentPage == 0) break;
                unselect();
                memoryPage(3 - currentPage);
                address = -i;
                write(memMin); }
            if (p) SPI.write(p[i]);
            else   SPI.write(0);
        }
        unselect();
        memoryPage(oldPage);
    }
}

void sigmadspClass::writeValue(uint16_t address, uint32_t value){
    write(address);   
    SPI.write32(value);
    unselect();
}

void sigmadspClass::writeValueFloat(uint16_t address, float value){
    writeValue(address, int824(value));
}

uint32_t sigmadspClass::readValue(uint16_t address){
    uint8_t oldCS2 = CS2; CS2 = CS;
    read(address);   
    uint32_t msb = SPI.transfer16(0);     
    uint32_t lsb = SPI.transfer16(0);     
    unselect();
    CS2 = oldCS2;
    return (msb << 16) | lsb;
}

float sigmadspClass::readValueFloat(uint16_t address){
    return float824(readValue(address));
}

uint32_t * sigmadspClass::writeArray(uint16_t address, uint16_t number, uint32_t *p){
    write(address);   
    for (uint16_t i=0; i<number; i++) SPI.write32(*(p++));
    unselect();
    return p;
}

float * sigmadspClass::writeArrayFloat(uint16_t address, uint16_t number, float *p){
    write(address);   
    for (uint16_t i=0; i<number; i++) 
        if (p) SPI.write32(int824(*(p++)));
        else   SPI.write32(0);
    unselect();
    return p;
}
void sigmadspClass::writeTable(uint32_t *p){
    while (*p) {
        uint16_t address = *p++;
        uint16_t number  = *p++; 
        p = writeArray(address, number, p);
    }    
}

void sigmadspClass::softReset(uint16_t value){
    write(REG_SOFT_RESET_ADDR);
    SPI.write16(value);
    unselect();
}

void sigmadspClass::softResetCycle(){
    softReset(0);
    softReset(1);
}

// convert a float number to a fixed point integer with a mantissa of 24 bit
// eg : the value 0.5 will be coded as 0x00800000
uint32_t sigmadspClass::int824(float f){
    float maxf = (1 << 7);
    if (f >=   maxf)  
       return 0x7fffffff;
    else
        if (f < (-maxf))  
           return 0xffffffff;
    else {
         long mul = 1 << 24;
         f *= mul;
         return f;   // will convert to integer
    }
}

float sigmadspClass::float824(uint32_t i){
    return (float)i / (float)(1 << 24);
}

void sigmadspClass::writeBiquads(uint16_t address, uint16_t number, float *p){    
    if (number) {
        write(address);
        for (uint16_t i=0; i<number; i++){            
            if (p) {
                SPI.write32(int824(p[2]));  // b2
                SPI.write32(int824(p[1]));  // b1
                SPI.write32(int824(p[0]));  // b0
                SPI.write32(int824(p[4]));  // a2
                SPI.write32(int824(p[3]));  // a1
                p += 5;
            } else {
                SPI.write32(0);
                SPI.write32(0);
                SPI.write32(int824(1.0));
                SPI.write32(0);
                SPI.write32(0); }     
        }
        unselect(); 
    }
}

void sigmadspClass::writeBiquadSafeload(uint16_t address, uint16_t number, float *p){
    if (safeloadAddr) {
        for (uint16_t i=0; i<number; i++){
            write(safeloadAddr);
            SPI.write32(int824(*p++));
            SPI.write32(int824(*p++));
            SPI.write32(int824(*p++));
            SPI.write32(int824(*p++));
            SPI.write32(int824(*p++));
            if ((arch[6] == '6')) {
                SPI.write32(currentPage == SIGMADSP_LOWER_PAGE ? 5 : 0);
                SPI.write32(currentPage == SIGMADSP_UPPER_PAGE ? 5 : 0);
            } else 
                SPI.write32(5);
            unselect();
        }
    }
}

#if !defined(NO_GLOBAL_INSTANCES) && !defined(NO_GLOBAL_SIGMADSP)
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