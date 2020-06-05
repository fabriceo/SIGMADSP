#ifndef SIGMADSP_FILTER_H
#define SIGMADSP_FILTER_H

#include  <math.h>
#define dspFilterParam_t float

// the list of all supported filters.
enum filterTypes { FNONE,
        BEna1,LPBE2,LPBE3,LPBE4,BEna5,LPBE6,BEna9,LPBE8,    // bessel lowpass
        BEna2,HPBE2,HPBE3,HPBE4,BEna6,HPBE6,BEna10,HPBE8,   // bessel high pass
        BEna3,LPBE3db2,LPBE3db3,LPBE3db4,BEna7,LPBE3db6,BEna11,LPBE3db8,    // bessel at -3db cutoff
        BEna4,HPBE3db2,HPBE3db3,HPBE3db4,BEna8,HPBE3db6,BEna12,HPBE3db8,
        BUna1,LPBU2,LPBU3,LPBU4,BUna3,LPBU6,BUna5,LPBU8,    // buterworth low pass
        BUna2,HPBU2,HPBU3,HPBU4,BUna4,HPBU6,BUna6,HPBU8,    // buterworth high pass
        Fna1,LPLR2,LPLR3,LPLR4,Fna3,LPLR6,Fna4,LPLR8,       // linkwitz rilley low pass
        Fna5,HPLR2,HPLR3,HPLR4,Fna7,HPLR6,Fna8,HPLR8,       // linkwitz rilley high pass
        FLP1,FLP2,FHP1,FHP2,FAP1,FAP2,FSUB,FBP0DB,          // lowpass, highpass, allpass, soustractif, bandpass
        FLS1,FLS2,FHS1,FHS2,FPEAK,FNOTCH,FBPQ               // lowshelf, highshelf, PEQ, Notch, Band pass
};

// each user filter definition will be stored in this structure
typedef struct  {    // basic structure requires 16 bytes per user defined filter
    enum  filterTypes ftype:8;          // this is a binary number based on "enum filterTypes" above
    char  bypass:8;                     // 0 : normal  , 1 : not used/bypassed
    char  invert:8;                     // 1 : invert phase by applying negative gain
    char  locked:8;                     // 1 to indicate that the filter should not be changed
    dspFilterParam_t freq;              // corner frequency of the filter
    dspFilterParam_t Q;                 // quality factor of the filter (if relevant)
    dspFilterParam_t gain;              // boost gain for this filter (may not be used)
} dspFilter_t;
//_Static_assert(sizeof(dspFilter_t)==16,"dspFilter_t expected to be 16 bytes");


// basic structure for holding the biquad coefficient computed for a given sampling rate
typedef struct {
    dspFilterParam_t b0;    //xn
    dspFilterParam_t b1;    //xn-1
    dspFilterParam_t b2;    //xn-2
    dspFilterParam_t a1;    //yn-1
    dspFilterParam_t a2;    //yn-2
} dspBiquadCoefs_t;


//same structure but as 8.24 integer, compatible with sigmadsp > ADAU1452 (not 1701)
typedef struct {
    long b0;
    long b1;
    long b2;
    long a1;
    long a2;
} dspBiquadCoefs824_t;

// convert a deciBell value to a float number. e.g. dB2gain(10.0) => 3.162277
// expected to be optimized by compiler where dB is known at compile time
static inline float dB2gain(float db){
    db /= 20.0;
    return pow(10,db); }


// convert a float number to a fixed point integer with a mantissa of 24 bit
// eg : the value 0.5 will be coded as 0x00800000
static inline long dspQ8_24(float f){
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



// temporary arry to store the calculated biquad value for a given filter
#define tempBiquadMax 4
extern dspBiquadCoefs_t tempBiquad[tempBiquadMax];         // temporary array of max 4 biquad cell
extern long tempBiquadIndex;
extern float dspSamplingFreq;
extern float * dspFilterResult;

extern const dspFilter_t dspFilterNone; // default content for fnone filter

extern const char * dspFilterNames[];   // short comprehensive name for the filter ftype, 5 char max
extern const char   dspFilterCells[];   // number of biquad cells needed for the filter ftype (1..4) 0 for fnone
extern const char   dspFilterHP[];      // show compatibility of the filter as High Pass
extern const char   dspFilterLP[];      // show compatibility of the filter as low Pass
#ifdef __cplusplus
extern "C" {
#endif
// search for the name of a filter in the dspFilterNames table and return index found
extern enum filterTypes dspFilterNameSearch(char *s);
extern int dspFilterNeedQ(enum filterTypes ftype);
extern int dspFilterNeedGain(enum filterTypes ftype);
extern int dspFilterIsHP(enum filterTypes ftype);
extern int dspFilterIsLP(enum filterTypes ftype);
extern int dspFilterIsEQ(enum filterTypes ftype);
// compute filter coefficients and store them in "tempBiquad"
extern int dsp_filter(enum filterTypes type, float freq, float Q, float gain);
extern int dspFilterConvert(dspFilter_t f, int fs);
#ifdef __cplusplus
}
#endif
#endif
