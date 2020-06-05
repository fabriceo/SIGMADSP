/*
 * sigmadsppresets.c
 *
 *  Created on: 14 may 2020
 *      Author: fabriceo
 *
 */

#include "sigmadsppresets.h" // this also include the sigmastudio generic file and filters
const dspBiquadCoefs_t dspBiquadNone = { 1.0, 0.0, 0.0, 0.0, 0.0 };

#ifdef DSP_GENERIC_OK
// calculate full preset checksum
long dspPresetChecksumCompute(dspPreset_t * p){
    long sum = 0;
    long * pint = (long*)&p->fb[0];    // creat a pointer on 32bit data starting at the first filterblock
    int i;
    for (i=0; i< (sizeof(p->fb)/sizeof(long)); i++) sum += *pint++; // go trough each words
    if (sum == 0) sum = -1; // makse sure the sum is never 0
    return sum;
}

void dspPresetChecksumUpdate(dspPreset_t * p){
    p->checksum = dspPresetChecksumCompute(p);
}

long dspPresetChecksumVerify(dspPreset_t * p){
    long sum = dspPresetChecksumCompute(p);
    return (sum == p->checksum);
}

static void dspPresetResetAllFilterBank(dspFilterBlock_t * fb, int chanMax) {
    int i,j;
    for (i=0; i<chanMax; i++) {
        fb->numberOfFilters = filterBankSize[i];
        for (j=0; j < DSP_INPUTS; j++) fb->mixer[j] = (i==j) ? 1.0 : 0.0;
        for (j=0; j < FILTER_BANK_SIZE; j++)  fb->filter[j] = dspFilterNone;
        for (j=0; j < FILTER_HIPASS_SIZE; j++)
            fb->filterHP[j] = dspFilterNone;
        for (j=0; j < FILTER_LOPASS_SIZE; j++)
            fb->filterLP[j] = dspFilterNone;
        fb->gain = 1.0;
        fb->delayMicroSec = 0.0;
        fb->mute = 0;
        fb->invert = 0;
        fb++; }
}

void dspPresetReset(dspPreset_t * p, int numPreset){
    p->presetNumber      = numPreset;
    p->filterBankSize    = FILTER_BANK_SIZE;
    p->numberOfInputs    = DSP_INPUTS;
    p->numberOfOutputs   = DSP_OUTPUTS;
    dspPresetResetAllFilterBank( &p->fb[0],  DSP_IO_TOTAL);
    dspPresetChecksumUpdate(p);
}

void dspPresetSetFilterEQ(dspPreset_t * p, int channel, int numFilter, dspFilter_t filter){
    if (channel < DSP_IO_TOTAL)
        if (numFilter < p->fb[channel].numberOfFilters)
            p->fb[channel].filter[numFilter] = filter;
}

void dspPresetSetFilterHP(dspPreset_t * p, int channel, int numFilter, dspFilter_t filter){
    if (channel < DSP_IO_TOTAL)
        if (numFilter < FILTER_HIPASS_SIZE)
            p->fb[channel].filterHP[numFilter] = filter;
}

void dspPresetSetFilterLP(dspPreset_t * p, int channel, int numFilter, dspFilter_t filter){
    if (channel < DSP_IO_TOTAL)
        if (numFilter < FILTER_LOPASS_SIZE)
            p->fb[channel].filterLP[numFilter] = filter;
}

void dspPresetSetGain(dspPreset_t * p, int channel, float gain){
    if (channel < DSP_IO_TOTAL)
        p->fb[channel].gain = gain;
}

void dspPresetSetMute(dspPreset_t * p, int channel, int mute){
    if (channel < DSP_IO_TOTAL)
        p->fb[channel].mute = (mute ? 1 : 0);
}

void dspPresetSetInvert(dspPreset_t * p, int channel, int invert){
    if (channel < DSP_IO_TOTAL)
        p->fb[channel].invert = (invert ? 1 : 0);
}

void dspPresetSetDelay(dspPreset_t * p, int channel, float delay){
    if (channel < DSP_IO_TOTAL)
        p->fb[channel].delayMicroSec = delay;
}


// go through the list of filters and ensure that any FNONE is at the end of the list

void dspPresetMoveNoneFilters(dspPreset_t * p, int ch){
    int numFilters = p->fb[ch].numberOfFilters;
    dspFilter_t * f = &p->fb[ch].filter[0];
    int i,j;
    for (i=0; i < (numFilters-1); i++) {
        if (f[i].ftype == FNONE) { // park it at the end and shift the table to the left
            for (j=i; j < (numFilters-1); j++) f[j] = f[j+1];
            f[numFilters-1] = dspFilterNone; }
            }
}

// cleanup the filter bank, by reordering filters by growing frequency
void dspPresetSortFilters(dspPreset_t * p, int ch){
    dspPresetMoveNoneFilters(p, ch);
    dspFilter_t * f = &p->fb[ch].filter[0];
    dspFilter_t temp;
    int numFilters = p->fb[ch].numberOfFilters;
    int en_desordre = 1;
    int i,j;
    for ( i = 0; i < numFilters && en_desordre; ++i) {
        en_desordre = 0;
        for (j = 1; j < p->filterBankSize - i; ++j) {
            if (f[j-1].freq > f[j].freq) {
                temp = f[j-1];
                f[j-1] = f[j];
                f[j] = temp;
                en_desordre = 1; }
        }
    }
}

int dspPresetCalcCellsUsedEQ(dspPreset_t * p, int channel) {
    int numCells = 0;
    int i;
    for (i=0; i < p->fb[channel].numberOfFilters; i++)
        numCells += dspFilterCells[ p->fb[channel].filter[i].ftype ];
    return numCells;
}

int dspPresetCalcCellsUsedHP(dspPreset_t * p, int channel) {
    int numCells = 0;
    int i;
    for (i=0; i < FILTER_HIPASS_SIZE; i++)
        numCells += dspFilterCells[ p->fb[channel].filterHP[i].ftype ];
    return numCells;
}


int dspPresetCalcCellsUsedLP(dspPreset_t * p, int channel) {
    int numCells = 0;
    int i;
    for (i=0; i < FILTER_LOPASS_SIZE; i++)
        numCells += dspFilterCells[ p->fb[channel].filterLP[i].ftype ];
    return numCells;
}

// generate a table of index pointing on the filter used according to the filter required cells
// eg. if filter[0].ftype = LPBU2 and filter[1].ftype = LPBE3, table = { 0+1,1+1,1+1, 0,0,0,0,0,0,0... }
int dspPresetCalcTableCellsUsed(dspPreset_t * p, int ch, char * table) {
    dspPresetMoveNoneFilters(p, ch);
    int posi = 0;
    int numCells;
    int i,j;
    for (i=0; i < p->filterBankSize; i++) {
        numCells = dspFilterCells[ p->fb[ch].filter[i].ftype ];
        if ((posi+numCells) <= p->filterBankSize)
            for (j=0; j<numCells; j++)  
                table[posi+j] = i;
        posi += numCells;
    }
    for (i=posi; i< p->filterBankSize; i++) table[i] = p->filterBankSize; // indicate the space is free by filling with maximum value
    return posi;
}

//generate the sigmadsp datas for a given filter bank of "max" number of filter
static unsigned long * dspPresetConvertFilter(unsigned long * dest, dspFilter_t *f, int max){
    *dest++ = max * 5;  // number of words generated
    int numCells = 0;
    int cells;
    float * coefs;
    int i,j,k;
    for (i=0; i < max; i++) {
        tempBiquadIndex = 0;
        if ((f[i].ftype != FNONE) && (f[i].bypass == 0)) {
            cells = dsp_filter(f[i].ftype, f[i].freq, f[i].Q, f[i].gain); // generate coeficient in the tempBiquadtable and return number of biquad cells
            //if (f[i].ftype == 52) printf("cells = %d\n",cells);
            coefs = &tempBiquad[0].b0;
            if ((cells + numCells) <= max) {
                if (f[i].invert) {
                    *coefs = - *coefs; coefs++;
                    *coefs = - *coefs; coefs++;
                    *coefs = - *coefs; coefs -= 2; }
                for (j=0; j < cells; j++)
                    for (k=0; k < 5; k++) *dest++ = dspQ8_24(*coefs++);
                numCells += cells; }
        }
    }//for
    if (numCells < max) {
        tempBiquad[0] = dspBiquadNone;
        for (i=numCells; i< max; i++) {
            coefs = &tempBiquad[0].b0;
            for (k=0; k < 5; k++) *dest++ = dspQ8_24(*coefs++); }
    }
    return dest;
}
// to be called for each channels (inputs then outputs) pointing on an array for getting data to download to sigmadsp
unsigned long dspPresetConvert(dspPreset_t * p, int channel, unsigned long * dest, float fs, float gainMul, float delayAdd) {
    unsigned long * initialDest = dest;
    int i, j;
    dspFilterBlock_t * fb = &p->fb[channel];
    dspSamplingFreq = fs;

    // calculate mixer addres by checking if the channel is in the input space or output space
    long mixerAddr;
    if (channel < DSP_INPUTS) mixerAddr = mixerInAddr + channel * DSP_INPUTS;   // mixer has same number of input & output
    else
        if (delaySubAddr)
            mixerAddr = mixerOutAddr + (channel- DSP_INPUTS) * DSP_IO_TOTAL;    // due to substractive model, mixer contains also outputs
        else
            mixerAddr = mixerOutAddr + (channel- DSP_INPUTS) * DSP_INPUTS;      // otherwise mixer is N inputs and M outputs

    // generate information to download to mixer
    *dest++ = mixerAddr;
    if ((channel>=DSP_INPUTS)&&(delaySubAddr)) { // special case for substractive delay
        *dest++ = DSP_IO_TOTAL; // number of value for the mixer
        if (fb->filterHP[0].ftype == FSUB) {
            for (i=0; i<DSP_INPUTS; i++) *dest++ = 0;
            int q = fb->filterHP[0].Q;          // then get the output number from which the "sub" is calculated
            for (i=0; i<DSP_OUTPUTS; i++)
                if ((i+1) == q) // special case : the Q is containing the lowpas index to be substracted
                    *dest++ = dspQ8_24(1.0);
                else
                    *dest++ = 0 ;
            //generate information for the optional delay block for substractive crossover
            if ((q >= 0 ) && ( q <= DSP_OUTPUTS)) { // check relevance
                *dest++ = delaySubAddr + q -1;      // point on the delay algorythm
                *dest++ = 1;                        // 1 word to write
                float delay = fs / fb->filterHP[0].freq; // convert filter frequency to a number of samples
                float max = DSP_DELAY_SUB_SAMPLES(q) -1.0;  // check that we do not overwrite the max value defined in "generic.h"
                if (delay > max) delay = max;
                *dest++ = delay; }
        } else {
            for (i=0; i<DSP_INPUTS; i++) *dest++ = dspQ8_24(fb->mixer[i]);
            for (i=0; i<DSP_OUTPUTS; i++) *dest++ = 0;  }
    } else {
        *dest++ = DSP_INPUTS; // number of value for the mixer
        for (i=0; i<DSP_INPUTS; i++) *dest++ = dspQ8_24(fb->mixer[i]);
    }

    // generate information for the gain
    long gainAddr;
    if (channel < DSP_INPUTS) gainAddr = gainInAddr + channel;
    else gainAddr = gainOutAddr + channel- DSP_INPUTS;
    *dest++ = gainAddr;
    *dest++ = 1;    // 1 word is following
    if (fb->mute) gainMul = 0.0;
    if (fb->invert) gainMul = -gainMul;
    *dest++ = dspQ8_24(fb->gain * gainMul);

    //generate information for the delay
    if (channel >= DSP_INPUTS) {
        *dest++ = delayOutAddr + channel - DSP_INPUTS;
        *dest++ = 1;    // 1 word is following
        float delay = fb->delayMicroSec + delayAdd;
        float max = DSP_DELAY_OUT_SAMPLES(channel - DSP_INPUTS+1);  // check that we do not overwrite the max value defined in "generic.h"
        if (delay > max) delay = max;
        *dest++ = fs / 1000000.0 * delay;   // this gives a number of samples at "fs"
    }

    //generate filters
    if (filterHPAddr[channel]) {
        *dest++ = filterHPAddr[channel];
        if (fb->filterHP[0].ftype != FSUB) {
            dest = dspPresetConvertFilter(dest, &fb->filterHP[0], FILTER_HIPASS_SIZE);
        } else {
            *dest++ = FILTER_HIPASS_SIZE * 5;
            // remplissage biquadnone
            for (i=0; i < FILTER_HIPASS_SIZE; i++)
                for (j=0; j<5; j++) *dest++ = (j == 0) ? dspQ8_24(1.0) : 0; }
    }
    if (filterLPAddr[channel]) {
        *dest++ = filterLPAddr[channel];
        int typ = fb->filterLP[0].ftype;
        //printf("lowpass (%d) = %d %s %f\n",channel,typ,dspFilterNames[typ],fb->filterLP[0].freq);
        dest = dspPresetConvertFilter(dest, &fb->filterLP[0], FILTER_LOPASS_SIZE); }
    if (filterBankAddr[channel]) {
        *dest++ = filterBankAddr[channel];
        dest = dspPresetConvertFilter(dest, &fb->filter[0], fb->numberOfFilters); }
    *dest++ = 0; // end of buffer
    return (unsigned long)(dest - initialDest);
}
#endif //DSP_GENERIC_OK