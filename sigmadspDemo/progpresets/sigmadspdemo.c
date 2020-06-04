#include "sigmadsppresets.h"
#ifdef DSP_GENERIC_OK
#include "sigmadspresponse.h"
//define DSP_FILE_ACCESS
#ifdef DSP_FILE_ACCESS
#include "sigmadspfiles.h"
#endif
#include <stdio.h>
#include <arduino.h>

//temporary buffer for storing data blocks for sigmadsp download
static sigmadspBuffer_t dspBuffer;

void dspWriteSPI(long addr, long num, unsigned long * ptr){
#if 0
    // write "num" words (32bits) to an adress over SPI channel;
    int i;
    for (i=0; i<num; i++) {
        if ((i % 10)== 0) {
            if (i == 0)
                 printf("%5ld [%2ld] ",addr+i,num-i);
            else printf("%5ld (%2ld) ",addr+i,num-i); }
        printf("%8X,",(unsigned int)*(ptr+i));
        if ((i % 10) == 9)printf("\n"); }
    if ((num % 10)!= 0)printf("\n");
#endif
}

// read the buffer and send the blocks of data to the DSP
void dspSendBufferParam(unsigned long * ptr){
    while (*ptr) {
        long addr = *ptr++;              // address in sigmadsp PARAM (DM0)
        long num  = *ptr++;              // number of 32 bits words to send
        dspWriteSPI(addr, num, ptr);    // call SPI routine
        ptr += num; }           // move to next block until NULL
}


// apply all the bank filter on the response table
void dspBodeApplyFilterBlock(dspBode_t * t, int N, dspFilterBlock_t * fb){
    dspBodeApplyFilterBank(t, N, &fb->filterHP[0], FILTER_HIPASS_SIZE);
    dspBodeApplyFilterBank(t, N, &fb->filterLP[0], FILTER_LOPASS_SIZE);
    dspBodeApplyFilterBank(t, N, &fb->filter[0], fb->numberOfFilters);
}


// used to simplify the syntax.
#define leftin    0
#define rightin   1
#define out1  2
#define out2  3
#define out3  4
#define out4  5

// create a preset in memory and then save it as a text file
void createPresets(dspPreset_t * pp){
    //  { ftype, bypass, invert, unused, freq, Q, gain }
    dspFilter_t fin1 = { FPEAK, 0,0,0, 125, 1.0, 2.0 };  // +6db at 125hz
    dspFilter_t fin2 = { FPEAK, 0,0,0, 400, 1.0, 0.75 };  //
    dspFilter_t fin3 = { FPEAK, 0,0,0, 1000, 1.0, 1.25 };  //
    dspFilter_t fin4 = { FPEAK, 0,0,0, 4000, 1.0, 0.5 };  // +6db at 125hz

    dspFilter_t fout1LP = { LPLR4, 0,0,0, 398 };            // low pass for out 1
    dspFilter_t fout11 = { FNOTCH, 0,0,0, 50, 10.0, 1.0 };  //
    dspFilter_t fout12 = { FPEAK, 0,0,0, 250, 3.0, 1.1 };  //

    dspFilter_t fout2HP = { HPLR4, 0,0,0, 398 };            // high pass for out2
    dspFilter_t fout2LP = { LPLR4, 0,0,0, 2000 };           // low pass for out2
    dspFilter_t fout21 = { FPEAK, 0,0,0, 1250, 3.0, 0.75 };  // -2.5dB at 1250hz with sharp Q=3
    dspFilter_t fout22 = { FPEAK, 0,0,0, 2750, 2.0, 1.25 };

    dspFilter_t fout3HP = { HPLR4, 0,0,0, 2000 };           // high pass for out3
    dspFilter_t fout31 = { FPEAK, 0,0,0, 3250, 3.0, 0.75 };  // -2.5dB at 1250hz with sharp Q=3
    dspFilter_t fout32 = { FPEAK, 0,0,0, 5750, 2.0, 1.25 };

    dspFilter_t fout4SUB = { FSUB, 0,0,0, 500, 1 };          // 2 ms=(1/500) linked to out1 low pass

    int i;
    for (i=0; i<4; i++)
        dspPresetReset(&pp[i], i+1);    // reset the preset and set them to number "i+1"

    dspPreset_t * preset;
    preset = &pp[0];      // pointer on the firt preset

    preset->fb[leftin].gain = 0.5;             // -6db by default as a preconditioning gain.
    preset->fb[leftin].delayMicroSec = 0.0;    // no need for delaying inputs here
    dspPresetSetFilterEQ(preset, leftin,  0, fin1);
    dspPresetSetFilterEQ(preset, leftin,  1, fin2);
    dspPresetSetFilterEQ(preset, leftin,  2, fin3);
    dspPresetSetFilterEQ(preset, leftin,  3, fin4);

    preset->fb[rightin] = preset->fb[leftin]; // same config as left, but this overwrite the mixer so:
    preset->fb[rightin].mixer[leftin]  = 0.0;  // correcting mixer that was just overwritten above
    preset->fb[rightin].mixer[rightin] = 1.0;  //

    dspPresetSetFilterLP(preset, out1, 0, fout1LP); // bass
    dspPresetSetFilterEQ(preset, out1, 0, fout11); // bass eq
    dspPresetSetFilterEQ(preset, out1, 1, fout12); // bass eq

    dspPresetSetFilterHP(preset, out2, 0, fout2HP); // medium hp
    dspPresetSetFilterLP(preset, out2, 0, fout2LP);
    dspPresetSetFilterEQ(preset, out2, 0, fout21);
    dspPresetSetFilterEQ(preset, out2, 0, fout22);

    dspPresetSetFilterHP(preset, out3, 0, fout3HP); //treble hp
    dspPresetSetFilterEQ(preset, out3, 0, fout31); //treble eq
    dspPresetSetFilterEQ(preset, out3, 0, fout32); //treble eq
    dspPresetSetDelay(preset, out3, 0.1/340.0 * 1000000.0);  // delayed by 10cm for sound speed 340m/s

    dspPresetSetFilterHP(preset, out4, 0, fout4SUB); // substractive filter from out1


    pp[1] = pp[0];        // copy this preset in the next one
    preset = &pp[1];
    preset->presetNumber = 2;               // because it was overwritten by the above copy
    // change LR4 by LR6, other mean of directly accessing the table
    preset->fb[out1].filterLP[0].ftype = LPLR6;
    preset->fb[out2].filterHP[0].ftype = HPLR6;

    pp[2] = pp[0];        // copy this preset in the next one
    preset = &pp[2];
    preset->presetNumber = 3;               // because it was overwritten by the above copy
    // remove peq on inputs
    dspPresetSetFilterEQ(preset, leftin,   0, dspFilterNone);
    dspPresetSetFilterEQ(preset, rightin,  0, dspFilterNone);

    pp[3] = pp[1];        // copy this preset in the next one
    preset = &pp[3];
    preset->presetNumber = 4;               // because it was overwritten by the above copy
    // remove peq on inputs
    dspPresetSetFilterEQ(preset, leftin,   0, dspFilterNone);
    dspPresetSetFilterEQ(preset, rightin,  0, dspFilterNone);
}

// storage for presets
#define maxPreset 10
dspPreset_t tablePreset[maxPreset];

//storage for frequency bins (geometrical)
#define numberOfFreq 100
dspBode_t bode[numberOfFreq];


static inline int getTime() {    
    return micros();
}


int sigmadspdemo() {

    printf("SIGMADSP Library. %d Inputs x %d filters , %d Outputs x %d filters\n",
            DSP_INPUTS, (int)filterBankSize[0], DSP_OUTPUTS, (int)filterBankSize[DSP_INPUTS]);

#ifdef DSP_FILE_ACCESS // no file access for microcontroler version
    miniParseFile(&tablePreset[0], 1,"/Users/Fabrice/Documents/MiniDSP/nanoSHARC-2x8-96k/setting/setting1.xml");
    miniParseFile(&tablePreset[0],2,"/Users/Fabrice/Documents/MiniDSP/nanoSHARC-2x8-96k/setting/setting2.xml");
    miniParseFile(&tablePreset[0],3,"/Users/Fabrice/Documents/MiniDSP/nanoSHARC-2x8-96k/setting/setting3.xml");
    miniParseFile(&tablePreset[0],4,"/Users/Fabrice/Documents/MiniDSP/nanoSHARC-2x8-96k/setting/setting4.xml");

    dspPresetWriteTextfile(&tablePreset[0], 1, "testnanosharc.dsp");
#endif

    createPresets(&tablePreset[0]); // create aa basic 3way crossover in 4 version
    dspPreset_t * pPreset = &tablePreset[0];
    int ok=1;

#ifdef DSP_FILE_ACCESS // no file access for microcontroler version
    // save the table of 4 presets
    dspPresetWriteTextfile(&tablePreset[0], 4, "testpreset.dsp");
    // load presets
    ok = dspPresetReadTextfile(pPreset, 10, "testpreset.dsp");
#endif

    int t = getTime();
    // typicall routine for calculating all the data to be downloaded to the sigmadsp
    if (ok){
        int ch;
        DSP_FOR_ALL_CHANNELS(pPreset, ch) {
            dspPresetConvert(pPreset, ch , &dspBuffer[0], 192000.0, 1.0, 0.0 );
            dspSendBufferParam(&dspBuffer[0]);
        }
    }
    int t1 = getTime()-t;
    //if (t1) printf("dspPresetConvert[all] = %d\n",t1);


    float min,max;
    t = getTime();
    dspBodeFreqInit(10.0, 100000.0, 1, &bode[0], numberOfFreq, 192000.0);
    t1 = getTime()-t;
    int total = t1;
    //if (t1) printf("dspBodeFreqInit[%d] = %d\n",numberOfFreq,t1);

    t = getTime();
    dspBodeResponseInit(&bode[0], numberOfFreq, 1.0);
    t1 = getTime()-t;
    total += t1;
    //if (t1) printf("dspBodeResponseInit[%d] = %d\n",numberOfFreq,t1);
    t = getTime();
    dspBodeApplyFilterBlock(&bode[0], numberOfFreq, &tablePreset[0].fb[0]);
    dspBodeApplyFilterBlock(&bode[0], numberOfFreq, &tablePreset[3].fb[0]);
    t1 = getTime()-t;
    total += t1;
    //if (t1) printf("dspBodeApplyFilterBlock[%d] = %d\n",numberOfFreq,t1);
    t = getTime();
    dspBodeMagnitudeDB(&bode[0], numberOfFreq, 1.0, &min, &max);
    t1 = getTime()-t;
    total += t1;
    //if (t1) printf("dspBodeMagnitudeDB[%d] = %d\n",numberOfFreq,t1);
    t = getTime();
    dspBodePhase(&bode[0], numberOfFreq);
    t1 = getTime()-t;
    total += t1; 
    //if (t1) printf("dspBodePhase[%d] = %d\n",numberOfFreq,t1);
    //if (total) printf("total compute time for response magnitude and phase for %d points = %d\n",numberOfFreq,total);
    //printf("filter response for both input 1 and output 2 (medium):\n");
    //printf("min = %fdB, max = %fdB\n",min,max);
    //for (long i=0; i<numberOfFreq; i++)
    //    printf("%2ld %5.0f %5.2fdB %4ld°\n",i+1, bode[i].freq,bode[i].mag,(long)(bode[i].phase+0.5));

return total;
}

#endif //DSP_GENERIC_OK
