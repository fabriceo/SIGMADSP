
#include <Arduino.h>
#include "SPI.h"
#include "sigmadspSPI.h"
#include "sigmadsp_IC_1.h"        // import default_download_IC_1() and DEVICE_ARCHITECTURE_IC_1
#include "sigmadsp_IC_1_PARAM.h"  // used to access sigmastudio elements

const int CS_ADAU = D8;           // GPIO pin for ChipSelect

int val;
void setup() {
  //Serial.begin(115200);
  SPI.begin();
  DSP.begin( DEVICE_ARCHITECTURE_IC_1, CS_ADAU);
  default_download_IC_1();
}

int totaltime;

unsigned int count;
void loop() {
  count++;
  delay(1000);
}