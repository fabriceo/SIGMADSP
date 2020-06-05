
#ifndef __SIGMASTUDIOFW_H__
#define __SIGMASTUDIOFW_H__

//from original sigmastudioFW.h

typedef unsigned short ADI_DATA_U16;
typedef unsigned char  ADI_REG_TYPE;

// extern calls in order to avoid dependency with another .h file
// declared in sigmadspspi.cpp
extern void DSPwriteMultipleBytes(uint16_t address, uint16_t number, ADI_REG_TYPE *p);
extern uint16_t DSPdownloadProgram();
extern uint16_t DSPdownloadParam();

// from original sigmastudioFW.h, customized.
// DM1 access is replaced with a simple memory reset, 
// avoiding a big table of "0" in this program (compiler optimization)
/* 
 * Write to multiple Device registers 
 */

#define SIGMA_WRITE_REGISTER_BLOCK( devAddress, address, length, pData )\
    {   if (address == PROGRAM_ADDR_IC_1) {\
            if (DSPdownloadProgram()) DSPwriteMultipleBytes(address,length, &pData[0]); }\
        else if (address == PARAM_ADDR_IC_1) {\
            if (DSPdownloadParam()) DSPwriteMultipleBytes(address,length, &pData[0]); }\
        else if (address == DM1_DATA_ADDR_IC_1) {\
             DSPwriteMultipleBytes(address,length, 0); }\
        else DSPwriteMultipleBytes(address, length, &pData[0]); }
             

/* 
 * Writes delay (in ms) 
 */
#define SIGMA_WRITE_DELAY( devAddress, length, pData ) { delay(25); }

#endif