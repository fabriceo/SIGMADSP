
#include <Arduino.h>
#include "SPI.h"
#include "sigmadspSPI.h"
#include "sigmadsp_IC_1.h"        // import default_download_IC_1() and DEVICE_ARCHITECTURE_IC_1
#include "sigmadsp_IC_1_PARAM.h"  // used to access sigmastudio elements
#include "sigmadspfilters.h"
#define dB(x) (pow(10,x/20.0))    // convert deciBell into float

const int CS_ADAU = D8;           // GPIO pin for ChipSelect
const int FS = 96000;
dspFilter_t lowpass  = { LPLR4, 0, 0, 0, 1000 };
dspFilter_t peq      = { FPEAK, 0, 0, 0, 150, 1.0, dB(+3.0) };
dspFilter_t highpass = { HPLR4, 0, 0, 0, 1000 };
dspFilter_t highShelf= { FHS2,  0, 0, 0, 8000, 0.5 , dB(+3.0)  };

void setup() {
  Serial.begin(115200);
  SPI.begin();
  DSP.begin( DEVICE_ARCHITECTURE_IC_1, MOD_SAFELOADMODULE_DATA_SAFELOAD0_ADDR, CS_ADAU);
  default_download_IC_1(); 

  DSP.writeValueFloat( MOD_GAIN1_GAINALGNS145X1GAIN_ADDR, 1.0); 

  int bq1, bq2;
  bq1 = dspFilterConvert(lowpass, FS);
  printf("number of bq = %d\n",bq1);
  DSP.writeBiquads(MOD_FILTER1_ALG0_STAGE0_B2_ADDR, bq1, dspFilterResult); // transfer computed filter
  bq2 = dspFilterConvert(peq, FS);
  DSP.writeBiquads(MOD_FILTER1_ALG0_STAGE0_B2_ADDR +bq1*5, bq2, dspFilterResult); // transfer computed filter
  DSP.writeBiquads(MOD_FILTER1_ALG0_STAGE0_B2_ADDR +bq1*5+bq2*5, MOD_FILTER1_COUNT/5 - bq1 - bq2, 0); // clear other filters  

  bq1 = dspFilterConvert(highpass, FS);
  DSP.writeBiquads(MOD_FILTER2_ALG0_STAGE0_B2_ADDR, bq1, dspFilterResult); // transfer computed filter
  bq2 = dspFilterConvert(highShelf, FS);
  DSP.writeBiquads(MOD_FILTER2_ALG0_STAGE0_B2_ADDR +bq1*5, bq2, dspFilterResult); // transfer computed filter
  DSP.writeBiquads(MOD_FILTER2_ALG0_STAGE0_B2_ADDR +bq1*5+bq2*5, MOD_FILTER2_COUNT/5 - bq1 - bq2, 0); // clear other filters  
}

unsigned int count;
void loop() {
  count++;
  delay(1000);
}