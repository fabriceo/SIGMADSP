#ifndef SIGMADSP_PRESETS_H
#define SIGMADSP_PRESETS_H

#include "sigmadspgeneric.h"
#include "sigmadspfilters.h"

#if 0
//below structure using bitfield is not used in this version
typedef struct {
    long mantissa:24;
    long integer:8;
} dsp824_t;
_Static_assert(sizeof(dsp824_t)==4,"dsp8_24_t is not packed as 32bits by this compiler");
#else
#define dsp824_t long
#endif



#ifdef DSP_GENERIC_OK

#define FILTER_HIPASS_SIZE 4
#define FILTER_LOPASS_SIZE 4

// each input or output is associated with a "block" containing a filter bank and gain delay like this
typedef struct {
    float        mixer[DSP_INPUTS];             // gain coefficient for mixing each relevant input
    dspFilter_t  filterHP[FILTER_HIPASS_SIZE];  // table containing the list of Highpass filters, expected 8th order maximum
    dspFilter_t  filterLP[FILTER_LOPASS_SIZE];  // table containing the list of lowpass filters, expected 8th order maximum
    float        gain;                          // gain attached with this filter bank (upfront for input, otherwise after for output)
    float        delayMicroSec;                 // delay added at the end of the filter bank in micro seconds
    char         mute:8;                        // 1 if muted
    char         invert:8;                      // 1 if inverted
    char         numberOfFilters:8;             // number of filters supported in this bank according to sigma studio generic file loaded
    dspFilter_t  filter[FILTER_BANK_SIZE];      // table containing the list of filters, size might be larger than numberOfFilters
} dspFilterBlock_t;


// structure defining a full preset containing mixers, gains, filter and delay for all the inputs & outputs
typedef struct  dspPreset_s {
    char presetNumber:8;               // index of the preset as seen by user
    char filterBankSize:8;             // same as FILTER_BANK_SIZE : maximum number of filters seen for all banks
    char numberOfInputs:8;             // number of input channels stored in the preset
    char numberOfOutputs:8;            // number of output channels stored in the preset
    long checksum;                     // checksum of all the data stored in the preset
    dspFilterBlock_t fb[DSP_IO_TOTAL]; // a table of filter block defined in the structure above
} dspPreset_t;

//same structure but as 8.24 integer, compatible with sigmadsp > ADAU1452 (not 1701)
typedef struct {
    dsp824_t b0;
    dsp824_t b1;
    dsp824_t b2;
    dsp824_t a1;
    dsp824_t a2;
} dspBiquadCoefs824_t;



// buffer required to download a filter block configuration to the sigmadsp param memory
#define DSP_PARAM_BUFFER_SIZE (2+DSP_INPUTS  +2 +2 +2+FILTER_BANK_SIZE*sizeof(dspBiquadCoefs824_t) + 1)

typedef unsigned long sigmadspBuffer_t[DSP_PARAM_BUFFER_SIZE];   // provide a simple type sigmadspBuffer_t to declare the table within user code source,

#if 0
// convert a deciBell value to a float number. e.g. dB2gain(10.0) => 3.162277
// expected to be optimized by compiler where dB is known at compile time
static inline float dB2gain(dspFloat_t db){
    db /= 20.0;
    return pow(10,db); }
#endif

// convert a float number to a fixed point integer with a mantissa of 24 bit
// eg : the value 0.5 will be coded as 0x00800000
static inline dsp824_t dspQ8_24(float f){
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

// this macro makes life easier to go trough each of the filter banks , including inputs & outputs
#define DSP_FOR_ALL_CHANNELS(p,ch) for (ch=0; ch < (p->numberOfInputs + p->numberOfOutputs); ch++)

extern const dspFilter_t dspFilterNone; // default content for fnone filter

// return the checksum of a preset
extern long  dspPresetChecksum(dspPreset_t * p);
// calculate and then update the checksum of a preset (typically used before saving)
extern void dspPresetChecksumUpdate(dspPreset_t * p);
// calculate and then verify the checksum of a preset (typically used after loading a preset)
extern long  dspPresetChecksumVerify(dspPreset_t * p);

// reset a preset with defaut mixer, gain, delay and fnone in each filter bank
extern void dspPresetReset(dspPreset_t * p, int numPreset);

// prepare the data and biquad for downloading to sigmadsp, for a given channel within a given preset, applying extra gain and delay if needed
extern unsigned long dspPresetConvert(dspPreset_t * p, int channel, unsigned long * dest, float fs, float gainMul, float delayAdd);

// set a filter in the bank of a given channel of a given preset
extern void dspPresetSetFilterEQ(dspPreset_t * p, int channel, int numFilter, dspFilter_t filter);
extern void dspPresetSetFilterHP(dspPreset_t * p, int channel, int numFilter, dspFilter_t filter);
extern void dspPresetSetFilterLP(dspPreset_t * p, int channel, int numFilter, dspFilter_t filter);
extern void dspPresetSetGain(dspPreset_t * p, int channel, float gain);
extern void dspPresetSetMute(dspPreset_t * p, int channel, int mute);
extern void dspPresetSetInvert(dspPreset_t * p, int channel, int invert);
extern void dspPresetSetDelay(dspPreset_t * p, int channel, float delay);

// compute the number of biquad cells needed for the filter bank associated to a given channel of a given preset
extern int  dspPresetCalcCellsUsedEQ(dspPreset_t * p, int channel);
extern int  dspPresetCalcCellsUsedHP(dspPreset_t * p, int channel);
extern int  dspPresetCalcCellsUsedLP(dspPreset_t * p, int channel);
// return a comprehensive table indicating how the biquad cells available are fulfilled with the filters.
extern int  dspPresetCalcTableCellsUsed(dspPreset_t * p, int channel, char * table);

// go through the list of filters and ensure that any FNONE is moved at the end of the list
extern void dspPresetMoveNoneFilters(dspPreset_t * p, int ch);
//sort the filters by increasing frequencies. Move the FNONE filters at the end
extern void dspPresetSortFilters(dspPreset_t * p, int ch);
#endif //DSP_GENERIC_OK
#endif
