/*
 * sigmadspfilter.c
 *
 *  Created on: 14 may 2020
 *      Author: fabriceo
 *
 */

//#include <stdio.h>

#include "sigmadspfilters.h" // this also include the sigmastudio generic file
#include  <math.h>

const char * dspFilterNames[] = { "NONE ",
  "LPBE1","LPBE2","LPBE3","LPBE4","LPBE5","LPBE6","LPBE7","LPBE8",
  "HPBE1","HPBE2","HPBE3","HPBE4","HPBE5","HPBE6","HPBE7","HPBE8",
  "LPBe1","LPBe2","LPBe3","LPBe4","LPBe5","LPBe6","LPBe7","LPBe8",
  "HPBe1","HPBe2","HPBe3","HPBe4","HPBe5","HPBe6","HPBe7","HPBe8",
  "LPBU1","LPBU2","LPBU3","LPBU4","LPBU5","LPBU6","LPBU7","LPBU8",
  "HPBU1","HPBU2","HPBU3","HPBU4","HPBU5","HPBU6","HPBU7","HPBU8",
  "LPLR1","LPLR2","LPLR3","LPLR4","LPLR5","LPLR6","LPLR7","LPLR8",
  "HPLR1","HPLR2","HPLR3","HPLR4","HPLR5","HPLR6","HPLR7","HPLR8",
  "LP1  ","LP2  ","HP1  ","HP2  ","AllP1","AllP2","FSUB ","BP0dB",
  "LSh1 ","LSh2 ","HSh1 ","HSh2 ","PEQ  ","NOTCH","BPQ  "};


// number of biquad cells used by a filter
const char dspFilterCells[] = { 0,
        1,1,2,2,0,3,0,4,    // 0 means unsupported
        1,1,2,2,0,3,0,4,
        1,1,2,2,0,3,0,4,
        1,1,2,2,0,3,0,4,
        1,1,2,2,0,3,0,4,
        1,1,2,2,0,3,0,4,
        1,1,2,2,0,3,0,4,
        1,1,2,2,0,3,0,4,
        1,1,1,1,1,1,4,1,
        1,1,1,1,1,1,1 };

const char dspFilterHP[] = { 0,
        0,0,0,0,0,0,0,0,
        1,1,1,1,1,1,1,1,
        0,0,0,0,0,0,0,0,
        1,1,1,1,1,1,1,1,
        0,0,0,0,0,0,0,0,
        1,1,1,1,1,1,1,1,
        0,0,0,0,0,0,0,0,
        1,1,1,1,1,1,1,1,
        0,0,1,1,0,0,1,0,
        0,0,0,0,0,0,0
};

const char dspFilterLP[] = { 0,
        1,1,1,1,1,1,1,1,
        0,0,0,0,0,0,0,0,
        1,1,1,1,1,1,1,1,
        0,0,0,0,0,0,0,0,
        1,1,1,1,1,1,1,1,
        0,0,0,0,0,0,0,0,
        1,1,1,1,1,1,1,1,
        0,0,0,0,0,0,0,0,
        1,1,0,0,0,0,0,0,
        0,0,0,0,0,0,0
};


enum filterTypes dspFilterNameSearch(char *s){
    long n = sizeof(dspFilterNames)/sizeof(dspFilterNames[0]);
    long i;
    for (i=0; i<n; i++) {
        char * p = s;
        char * q = (char *)dspFilterNames[i];
        while (*p == *q) { p++; q++; }
        if (*q == 0) return i;
    }
    return FNONE;
}

const char dspFilterQrequired[] = { // only for filters starting at LP1
            0,1,0,1,0,1,1,1,
            0,1,0,1,1,1,1  };

int dspFilterNeedQ(enum filterTypes ftype){
    if (ftype < FLP1) return 0;
    return dspFilterQrequired[ ftype - FLP1 ];
}

int dspFilterNeedGain(enum filterTypes ftype){
    if (ftype < FSUB) return 0;
    return 1;
}

int dspFilterIsHP(enum filterTypes ftype){
    return dspFilterHP[ftype];
}

int dspFilterIsLP(enum filterTypes ftype){
    return dspFilterLP[ftype];
}

const char dspFilterEQ[] = {
        0,0,0,0,1,1,0,0,
        1,1,1,1,1,1,0
};

int dspFilterIsEQ(enum filterTypes ftype){
    if (ftype < FLP1) return 0;
    return dspFilterEQ[ ftype - FLP1 ];
}


dspBiquadCoefs_t tempBiquad[tempBiquadMax];         // temporary array of max 4 biquad cell
long tempBiquadIndex = 0;
float dspSamplingFreq = 192000.0;

#ifndef M_PI
#define M_PI (3.1415926536)
#endif
#ifndef M_SQRT1_2
#define M_SQRT1_2 (0.7071067812)
#endif

// calc the biquad coefficient for a given filter type.
void dspFilter1stOrder( long type,
        float fs,
        float freq,
        float gain,
        dspFilterParam_t * b0,
        dspFilterParam_t * b1,
        dspFilterParam_t * b2,
        dspFilterParam_t * a1,
        dspFilterParam_t * a2
 )
{

    if (type == FNONE) {
        *b0 = 1.0; *b1 = 0.0; *b2 = 0.0; *a1 = 0.0; *a2 = 0.0; 
        return;
    } else  {
    dspFilterParam_t a0, w0, tw2, alpha;
    w0 = M_PI * 2.0 * freq / fs;
    tw2 = tan(w0/2.0);
    a0 = gain;
    *a2 = 0;
    *b2 = 0;
    switch (type) {
    case FLP1: {
        alpha = 1.0 + tw2;
        *a1 = ((1.0-tw2)/alpha) / a0;
        *b0 = tw2/alpha / a0;
        *b1 = *b0;
    break; }
    case FHP1: {
        alpha = 1.0 + tw2;
        *a1 = ((1.0-tw2)/alpha) /a0;
        *b0 =  1.0/alpha / a0;
        *b1 = -1.0/alpha / a0;
    break; }
#if 0   // alternative
    case FLP1: {
        alpha = (1.0-tw2)/(1.0+tw2);
        *b0 = (1.0-alpha)/2.0;
        *b1 = *b0;
        *a1 = alpha;
    break; }
    case FHP1: {
        alpha = (1.0-tw2)/(1.0+tw2);
        *b0 = 1.0/(1.0+tw2);
        *b1 = -*b0;
        *a1 = alpha;
    break; }
#endif
    case FHS1: {
        dspFilterParam_t A = sqrt(gain);
        a0 = A*tw2+1.0;
        *a1 = -(A*tw2-1.0)/a0;
        *b0 = (A*tw2+gain)/a0;
        *b1 = (A*tw2-gain)/a0;
        break; }
    case FLS1: {
        dspFilterParam_t A = sqrt(gain);
        a0 = tw2+A;
        *a1 = -(tw2-A)/a0;
        *b0 = (gain*tw2+A)/a0;
        *b1 = (gain*tw2-A)/a0;
        break; }
    case FAP1: {
        break;
    }
    } //switch
  }
}

// calc the biquad coefficient for a given filter type
void dspFilter2ndOrder( long type,
        float   fs,
        float   freq,
        float   Q,
        float   gain,
        dspFilterParam_t * b0,
        dspFilterParam_t * b1,
        dspFilterParam_t * b2,
        dspFilterParam_t * a1,
        dspFilterParam_t * a2
 )
{

    if (type == FNONE) {
        *b0 = 1.0; *b1 = 0.0; *b2 = 0.0; *a1 = 0.0; *a2 = 0.0; return;
    } else  {
    dspFilterParam_t a0, w0, cw0, sw0, alpha; // tw2
    w0 = M_PI * 2.0 * freq / fs;
    cw0 = cos(w0);
    sw0 = sin(w0);
    //tw2 = tan(w0/2.0);
    if (Q != 0.0) alpha = sw0 / 2.0 / Q; else alpha = 1;
    a0 = (1.0 + alpha);
    *a1 = -(-2.0 * cw0) / a0;       // sign is changed to accomodate convention
    *a2 = -(1.0 - alpha ) / a0;     // and coeficients are normalized vs a0
    switch (type) {
    case FLP2: {
        *b1 = (1.0 - cw0) / a0 * gain;
        *b0 = *b1 / 2.0;
        *b2 = *b0;
        break; }
    case FHP2: {
        *b1 = -(1.0 + cw0) / a0 * gain;
        *b0 = - *b1 / 2.0;
        *b2 = *b0;
        break; }
    case FAP2: {
        *b0 = -*a2 * gain;
        *b1 = -*a1 * gain;
        *b2 =  gain;
        break; }
    case FNOTCH: {
        *b0 = 1.0 / a0 * gain;
        *b1 = -*a1 * gain;
        *b2 = *b0;
        break; }
    case FBPQ : { // peak gain = Q
        *b0= sw0/2.0 / a0;
        *b1 = 0;
        *b2 = -sw0/2.0 / a0;
        break; }
    case FBP0DB : { // 0DB peak gain
        *b0= alpha / a0;
        *b1 = 0;
        *b2 = -alpha / a0;
    break; }
    case FPEAK: {
        dspFilterParam_t A = sqrt(gain);
        a0 = 1.0 + alpha / A;
        *a1 = 2.0 * cw0 / a0;
        *a2 = -(1.0 - alpha / A ) / a0;
        *b0 = (1.0 + alpha * A) / a0;
        *b1 = -2.0 * cw0 / a0;
        *b2 = (1.0 - alpha * A) / a0;
        break; }
    case FLS2: {
        dspFilterParam_t A = sqrt(gain);
        dspFilterParam_t sqA = sqrt( A );
        a0 = ( A + 1.0) + ( A - 1.0 ) * cw0 + 2.0 * sqA * alpha;
        *a1 = -(-2.0 *( (A-1.0) + (A+1.0) * cw0 ) ) / a0;
        *a2 = -( (A + 1.0) + (A - 1.0) * cw0 - 2.0 * sqA * alpha ) / a0;
        *b0 = ( A * ( ( A + 1.0) - ( A - 1.0) * cw0 + 2.0 * sqA * alpha ) ) / a0;
        *b1 = ( 2.0 * A * ( ( A - 1.0 ) - ( A + 1.0 ) * cw0 ) ) / a0;
        *b2 = ( A * ( ( A + 1.0 ) - ( A - 1.0 ) * cw0 - 2.0 * sqA * alpha )) / a0;;
        break; }
    case FHS2: {
        dspFilterParam_t A = sqrt(gain);
        dspFilterParam_t sqA = sqrt( A );
        a0 = ( A + 1.0) - ( A - 1.0 ) * cw0 + 2.0 * sqA * alpha;
        *a1 = -(2.0 *( (A-1.0) - (A+1.0) * cw0 ) ) / a0;
        *a2 = -( (A + 1.0) - (A - 1.0) * cw0 - 2.0 * sqA * alpha ) / a0;
        *b0 = ( A * ( ( A + 1.0) + ( A - 1.0) * cw0 + 2.0 * sqA * alpha ) ) / a0;
        *b1 = ( -2.0 * A * ( ( A - 1.0 ) + ( A + 1.0 ) * cw0 ) ) / a0;
        *b2 = ( A * ( ( A + 1.0 ) + ( A - 1.0 ) * cw0 - 2.0 * sqA * alpha )) / a0;
        break; }
    } //switch
    }
}

// special case : generate a null biquad if the filter is "subtractive"
long dsp_FilterSUB(){
    dspBiquadCoefs_t coefs = { 0.0, 0.0, 0.0, 0.0, 0.0 };
    if (tempBiquadIndex < tempBiquadMax)
        tempBiquad[tempBiquadIndex++] = coefs;
    return 1;
}

long dsp_Filter2ndOrder(long type, dspFilterParam_t freq, dspFilterParam_t Q, float gain){

    dspBiquadCoefs_t coefs;

    dspFilter2ndOrder(type, dspSamplingFreq, freq, Q, gain, &coefs.b0, &coefs.b1, &coefs.b2, &coefs.a1, &coefs.a2);

    if (tempBiquadIndex < tempBiquadMax)
        tempBiquad[tempBiquadIndex++] = coefs;

return 1;
}

long dsp_Filter1stOrder(long type, dspFilterParam_t freq, float gain){

    dspBiquadCoefs_t coefs;

    dspFilter1stOrder(type, dspSamplingFreq, freq, gain, &coefs.b0, &coefs.b1, &coefs.b2, &coefs.a1, &coefs.a2);

    if (tempBiquadIndex < tempBiquadMax)
        tempBiquad[tempBiquadIndex++] = coefs;

    return 1;
}


long dsp_LP_BES2(dspFilterParam_t freq) {   // low pass cutoff freq is in phase with high pass cutoff
    return dsp_Filter2ndOrder(FLP2, freq, 0.57735026919 , 1.0); }

long dsp_LP_BES2_3DB(dspFilterParam_t freq) {   // cutoff is at -3db, but not in phase with high pass
    return dsp_LP_BES2(freq * 1.27201964951);
}

long dsp_HP_BES2(dspFilterParam_t freq) {   // high pass cutoff freq is in phase with low pass cutoff
    return dsp_Filter2ndOrder(FHP2, freq, 0.57735026919 , 1.0); }

long dsp_HP_BES2_3DB(dspFilterParam_t freq) {   // cutoff is at -3db, but not in phase with low pass
    return dsp_HP_BES2(freq / 1.27201964951);
}

long  dsp_LP_BUT2(dspFilterParam_t freq) {
    return dsp_Filter2ndOrder(FLP2, freq, M_SQRT1_2 , 1.0); }

long dsp_HP_BUT2(dspFilterParam_t freq) {
    return dsp_Filter2ndOrder(FHP2, freq, M_SQRT1_2 , 1.0);
}

long dsp_LP_LR2(dspFilterParam_t freq) { // -6db cutoff at fc ?
    return dsp_Filter2ndOrder(FLP2, freq, 0.5 , 1.0); }

long dsp_HP_LR2(dspFilterParam_t freq) { // -6db cutoff ? 180 deg phase shift vs low pass
    return dsp_Filter2ndOrder(FHP2, freq, 0.5 , 1.0); }



long dsp_LP_BES3(dspFilterParam_t freq) {   // low pass cutoff freq is in phase with high pass cutoff
    long tmp =
    dsp_Filter2ndOrder(FLP2, freq * 0.941600026533, 0.691046625825 , 1.0);
    tmp += dsp_Filter1stOrder(FLP1, freq * 1.03054454544, 1.0);
    return tmp;
}

long dsp_LP_BES3_3DB(dspFilterParam_t freq) {   // cutoff is at -3db, but not in phase with high pass
    long tmp =
    dsp_Filter2ndOrder(FLP2, freq * 1.32267579991, 0.691046625825 , 1.0);
    tmp += dsp_Filter1stOrder(FLP1, freq * 1.44761713315,  1.0);
    return tmp;
}

long dsp_HP_BES3(dspFilterParam_t freq) {   // high pass cutoff freq is in phase with low pass cutoff
    long tmp =
    dsp_Filter2ndOrder(FHP2, freq / 0.941600026533 , 0.691046625825 , 1.0);
    tmp += dsp_Filter1stOrder(FHP1, freq / 1.03054454544,    1.0);
    return tmp;
}

long dsp_HP_BES3_3DB(dspFilterParam_t freq) {   // cutoff is at -3db, but not in phase with low pass
    long tmp =
    dsp_Filter2ndOrder(FHP2, freq / 1.32267579991 , 0.691046625825 , 1.0);
    tmp += dsp_Filter1stOrder(FHP1, freq / 1.44761713315,   1.0);
    return tmp;
}

long  dsp_LP_BUT3(dspFilterParam_t freq) {
    long tmp =
    dsp_Filter2ndOrder(FLP2, freq, 1.0 , 1.0);
    tmp += dsp_Filter1stOrder(FLP1, freq,  1.0);
    return tmp;
}

long dsp_HP_BUT3(dspFilterParam_t freq) {
    long tmp =
    dsp_Filter2ndOrder(FHP2, freq, 1.0 , 1.0);
    tmp += dsp_Filter1stOrder(FHP1, freq,  1.0);
    return tmp;
}

long dsp_LP_LR3(dspFilterParam_t freq) { // -6db cutoff at fc ?
    long tmp =
    dsp_Filter2ndOrder(FLP2, freq, 0.5 , 1.0);
    tmp += dsp_Filter1stOrder(FLP1, freq,  1.0);
    return tmp;
}

long dsp_HP_LR3(dspFilterParam_t freq) { // -6db cutoff ? 180 deg phase shift vs low pass
    long tmp =
    dsp_Filter2ndOrder(FHP2, freq, 0.5 , 1.0);
    tmp += dsp_Filter1stOrder(FHP1, freq,  1.0);
    return tmp;
}



long dsp_LP_BES4(dspFilterParam_t freq) {   // low pass cutoff freq is IN PHASE with high pass cutoff
    long tmp =
    dsp_Filter2ndOrder(FLP2, freq * 0.944449808226 , 0.521934581669 , 1.0);
    tmp += dsp_Filter2ndOrder(FLP2, freq * 1.05881751607  , 0.805538281842 , 1.0);
    return tmp;
}

long dsp_LP_BES4_3DB(dspFilterParam_t freq) {   // cutoff is at -3db, but not in phase with low pass
    long tmp =
    dsp_Filter2ndOrder(FLP2, freq * 1.43017155999  , 0.521934581669 , 1.0);
    tmp += dsp_Filter2ndOrder(FLP2, freq * 1.60335751622  , 0.805538281842 , 1.0);
    return tmp;
}
long dsp_HP_BES4(dspFilterParam_t freq) {       // high pass cutoff freq is in phase with low pass cutoff
    long tmp =
    dsp_Filter2ndOrder(FHP2, freq / 0.944449808226 , 0.521934581669 , 1.0);
    tmp += dsp_Filter2ndOrder(FHP2, freq / 1.05881751607  , 0.805538281842 , 1.0);
    return tmp;
}

long dsp_HP_BES4_3DB(dspFilterParam_t freq) {       // cutoff is at -3db, but not in phase with low pass
    long tmp =
    dsp_Filter2ndOrder(FHP2, freq / 1.43017155999  , 0.521934581669 , 1.0);
    tmp += dsp_Filter2ndOrder(FHP2, freq / 1.60335751622  , 0.805538281842 , 1.0);
    return tmp;
}

long dsp_LP_BUT4(dspFilterParam_t freq) {   // -3db cutoff
    long tmp =
    dsp_Filter2ndOrder(FLP2, freq, 0.54119610 , 1.0);
    tmp += dsp_Filter2ndOrder(FLP2, freq, 1.3065630 , 1.0);
    return tmp;
}

long dsp_HP_BUT4(dspFilterParam_t freq) {   // -3db cutoff
    long tmp =
    dsp_Filter2ndOrder(FHP2, freq, 0.54119610 , 1.0);
    tmp += dsp_Filter2ndOrder(FHP2, freq, 1.3065630 , 1.0);
    return tmp;
}

long dsp_LP_LR4(dspFilterParam_t freq) {   // -6db cutoff
    long tmp =
    dsp_Filter2ndOrder(FLP2, freq, M_SQRT1_2 , 1.0);
    tmp += dsp_Filter2ndOrder(FLP2, freq, M_SQRT1_2 , 1.0);
    return tmp;
}

long dsp_HP_LR4(dspFilterParam_t freq) {   // -6db cutoff
    long tmp =
    dsp_Filter2ndOrder(FHP2, freq, M_SQRT1_2 , 1.0);
    tmp += dsp_Filter2ndOrder(FHP2, freq, M_SQRT1_2 , 1.0);
    return tmp;
}

long dsp_LP_BES6(dspFilterParam_t freq) {
    long tmp =
    dsp_Filter2ndOrder(FLP2, freq * 0.928156550439 , 0.510317824749 , 1.0);
    tmp += dsp_Filter2ndOrder(FLP2, freq * 0.977488555538 , 0.611194546878 , 1.0);
    tmp += dsp_Filter2ndOrder(FLP2, freq * 1.10221694805  , 1.02331395383 , 1.0);
    return tmp;
}

long dsp_LP_BES6_3DB(dspFilterParam_t freq) {
    long tmp =
    dsp_Filter2ndOrder(FLP2, freq * 1.60391912877 , 0.510317824749 , 1.0);
    tmp += dsp_Filter2ndOrder(FLP2, freq * 1.68916826762 , 0.611194546878 , 1.0);
    tmp += dsp_Filter2ndOrder(FLP2, freq * 1.9047076123  , 1.02331395383  , 1.0);
    return tmp;
}

long dsp_HP_BES6(dspFilterParam_t freq) {
    long tmp =
    dsp_Filter2ndOrder(FHP2, freq / 0.928156550439 , 0.510317824749 , 1.0);
    tmp += dsp_Filter2ndOrder(FHP2, freq / 0.977488555538 , 0.611194546878 , 1.0);
    tmp += dsp_Filter2ndOrder(FHP2, freq / 1.10221694805  , 1.02331395383  , 1.0);
    return tmp;
}

long dsp_HP_BES6_3DB(dspFilterParam_t freq) {
    long tmp =
    dsp_Filter2ndOrder(FHP2, freq / 1.60391912877 , 0.510317824749 , 1.0);
    tmp += dsp_Filter2ndOrder(FHP2, freq / 1.68916826762 , 0.611194546878 , 1.0);
    tmp += dsp_Filter2ndOrder(FHP2, freq / 1.9047076123  , 1.02331395383  , 1.0);
    return tmp;
}

long dsp_LP_BUT6(dspFilterParam_t freq) {   // -3db cutoff
    long tmp =
    dsp_Filter2ndOrder(FLP2, freq, 0.51763809 , 1.0);
    tmp += dsp_Filter2ndOrder(FLP2, freq, M_SQRT1_2  , 1.0);
    tmp += dsp_Filter2ndOrder(FLP2, freq, 1.9318517  , 1.0);
    return tmp;
}

long dsp_HP_BUT6(dspFilterParam_t freq) {   // -3db cutoff
    long tmp =
    dsp_Filter2ndOrder(FHP2, freq, 0.51763809 , 1.0);
    tmp += dsp_Filter2ndOrder(FHP2, freq, M_SQRT1_2  , 1.0);
    tmp += dsp_Filter2ndOrder(FHP2, freq, 1.9318517  , 1.0);
    return tmp;
}

long dsp_LP_LR6(dspFilterParam_t freq) {   // TODO  Q ?
    long tmp =
    dsp_Filter2ndOrder(FLP2, freq, 0.5 , 1.0);
    tmp += dsp_Filter2ndOrder(FLP2, freq, 1.0 , 1.0);
    tmp += dsp_Filter2ndOrder(FLP2, freq, 1.0 , 1.0);
    return tmp;
}

long dsp_HP_LR6(dspFilterParam_t freq) {   // ?
    long tmp =
    dsp_Filter2ndOrder(FHP2, freq, 0.5 , 1.0);
    tmp += dsp_Filter2ndOrder(FHP2, freq, 1.0 , 1.0);
    tmp += dsp_Filter2ndOrder(FHP2, freq, 1.0 , 1.0);
    return tmp;
}

long dsp_LP_BES8(dspFilterParam_t freq) {
    long tmp =
    dsp_Filter2ndOrder(FLP2, freq * 0.920583104484 , 0.505991069397 , 1.0);
    tmp += dsp_Filter2ndOrder(FLP2, freq * 0.948341760923 , 0.559609164796 , 1.0);
    tmp += dsp_Filter2ndOrder(FLP2, freq * 1.01102810214  , 0.710852074442 , 1.0);
    tmp += dsp_Filter2ndOrder(FLP2, freq * 1.13294518316  , 1.22566942541 , 1.0);
    return tmp;
}

long dsp_LP_BES8_3(dspFilterParam_t freq) {
    long tmp =
    dsp_Filter2ndOrder(FLP2, freq * 1.77846591177  , 0.505991069397 , 1.0);
    tmp += dsp_Filter2ndOrder(FLP2, freq * 1.8320926012   , 0.559609164796 , 1.0);
    tmp += dsp_Filter2ndOrder(FLP2, freq * 1.95319575902  , 0.710852074442 , 1.0);
    tmp += dsp_Filter2ndOrder(FLP2, freq * 2.18872623053  , 1.22566942541  , 1.0);
    return tmp;
}

long dsp_HP_BES8(dspFilterParam_t freq) {
    long tmp =
    dsp_Filter2ndOrder(FHP2, freq / 0.920583104484 , 0.505991069397 , 1.0);
    tmp += dsp_Filter2ndOrder(FHP2, freq / 0.948341760923 , 0.559609164796 , 1.0);
    tmp += dsp_Filter2ndOrder(FHP2, freq / 1.01102810214  , 0.710852074442 , 1.0);
    tmp += dsp_Filter2ndOrder(FHP2, freq / 1.13294518316  , 1.22566942541  , 1.0);
    return tmp;
}

long dsp_HP_BES8_3DB(dspFilterParam_t freq) {
    long tmp =
    dsp_Filter2ndOrder(FHP2, freq / 1.77846591177  , 0.505991069397 , 1.0);
    tmp += dsp_Filter2ndOrder(FHP2, freq / 1.8320926012   , 0.559609164796 , 1.0);
    tmp += dsp_Filter2ndOrder(FHP2, freq / 1.95319575902  , 0.710852074442 , 1.0);
    tmp += dsp_Filter2ndOrder(FHP2, freq / 2.18872623053  , 1.22566942541  , 1.0);
    return tmp;
}

long dsp_LP_BUT8(dspFilterParam_t freq) {   // -3db cutoff
    long tmp =
    dsp_Filter2ndOrder(FLP2, freq, 0.50979558 , 1.0);
    tmp += dsp_Filter2ndOrder(FLP2, freq, 0.60134489 , 1.0);
    tmp += dsp_Filter2ndOrder(FLP2, freq, 0.89997622 , 1.0);
    tmp += dsp_Filter2ndOrder(FLP2, freq, 2.5629154  , 1.0);
    return tmp;
}

long dsp_HP_BUT8(dspFilterParam_t freq) {   // -3db cutoff
    long tmp =
    dsp_Filter2ndOrder(FHP2, freq, 0.50979558 , 1.0);
    tmp += dsp_Filter2ndOrder(FHP2, freq, 0.60134489 , 1.0);
    tmp += dsp_Filter2ndOrder(FHP2, freq, 0.89997622 , 1.0);
    tmp += dsp_Filter2ndOrder(FHP2, freq, 2.5629154  , 1.0);
    return tmp;
}

long dsp_LP_LR8(dspFilterParam_t freq) {   // -6db cutoff ?
    long tmp =
    dsp_LP_BUT4(freq);
    tmp += dsp_LP_BUT4(freq);
    return tmp;
}

long dsp_HP_LR8(dspFilterParam_t freq) {   // -6db cutoff ?
    long tmp =
    dsp_HP_BUT4(freq);
    tmp += dsp_HP_BUT4(freq);
    return tmp;
}

int dsp_filter(enum filterTypes type, float freq, float Q, float gain) {
    int tmp=0;
    switch (type) {
    case FNONE:  tmp = dsp_Filter2ndOrder(FNONE,0,0,0); break;
    case LPBE2 : tmp = dsp_LP_BES2(freq); break;
    case LPBE3 : tmp = dsp_LP_BES3(freq); break;
    case LPBE4 : tmp = dsp_LP_BES4(freq); break;
    case LPBE6 : tmp = dsp_LP_BES6(freq); break;
    case LPBE8 : tmp = dsp_LP_BES8(freq); break;
    case HPBE2 : tmp = dsp_HP_BES2(freq); break;
    case HPBE3 : tmp = dsp_HP_BES3(freq); break;
    case HPBE4 : tmp = dsp_HP_BES4(freq); break;
    case HPBE6 : tmp = dsp_HP_BES6(freq); break;
    case HPBE8 : tmp = dsp_HP_BES8(freq); break;
    case LPBE3db2 : tmp = dsp_LP_BES2(freq); break;
    case LPBE3db3 : tmp = dsp_LP_BES3(freq); break;
    case LPBE3db4 : tmp = dsp_LP_BES4(freq); break;
    case LPBE3db6 : tmp = dsp_LP_BES6(freq); break;
    case LPBE3db8 : tmp = dsp_LP_BES8(freq); break;
    case HPBE3db2 : tmp = dsp_HP_BES2(freq); break;
    case HPBE3db3 : tmp = dsp_HP_BES3(freq); break;
    case HPBE3db4 : tmp = dsp_HP_BES4(freq); break;
    case HPBE3db6 : tmp = dsp_HP_BES6(freq); break;
    case HPBE3db8 : tmp = dsp_HP_BES8(freq); break;
    case LPBU2 : tmp = dsp_LP_BUT2(freq); break;
    case LPBU3 : tmp = dsp_LP_BUT3(freq); break;
    case LPBU4 : tmp = dsp_LP_BUT4(freq); break;
    case LPBU6 : tmp = dsp_LP_BUT6(freq); break;
    case LPBU8 : tmp = dsp_LP_BUT8(freq); break;
    case HPBU2 : tmp = dsp_HP_BUT2(freq); break;
    case HPBU3 : tmp = dsp_HP_BUT3(freq); break;
    case HPBU4 : tmp = dsp_HP_BUT4(freq); break;
    case HPBU6 : tmp = dsp_HP_BUT6(freq); break;
    case HPBU8 : tmp = dsp_HP_BUT8(freq); break;
    case LPLR2 : tmp = dsp_LP_LR2(freq); break;
    case LPLR3 : tmp = dsp_LP_LR3(freq); break;
    case LPLR4 : tmp = dsp_LP_LR4(freq); break;
    case LPLR6 : tmp = dsp_LP_LR6(freq); break;
    case LPLR8 : tmp = dsp_LP_LR8(freq); break;
    case HPLR2 : tmp = dsp_HP_LR2(freq); break;
    case HPLR3 : tmp = dsp_HP_LR3(freq); break;
    case HPLR4 : tmp = dsp_HP_LR4(freq); break;
    case HPLR6 : tmp = dsp_HP_LR6(freq); break;
    case HPLR8 : tmp = dsp_HP_LR8(freq); break;
    case FLP2  : tmp = dsp_Filter2ndOrder(FLP2,freq,Q,gain); break;
    case FHP2  : tmp = dsp_Filter2ndOrder(FHP2,freq,Q,gain); break;
    case FLS2  : tmp = dsp_Filter2ndOrder(FLS2,freq,Q,gain); break;
    case FHS2  : tmp = dsp_Filter2ndOrder(FHS2,freq,Q,gain); break;
    case FAP2  : tmp = dsp_Filter2ndOrder(FAP2,freq,Q,gain); break;
    case FPEAK : tmp = dsp_Filter2ndOrder(FPEAK,freq,Q,gain); break;
    case FNOTCH: tmp = dsp_Filter2ndOrder(FNOTCH,freq,Q,gain); break;
    case FBP0DB: tmp = dsp_Filter2ndOrder(FBP0DB,freq,Q,gain); break;
    case FBPQ:   tmp = dsp_Filter2ndOrder(FBPQ,freq,Q,gain); break;
    case FSUB: tmp = dsp_FilterSUB(); break;
//first order
    case FLP1: tmp = dsp_Filter1stOrder(FLP1,freq, gain); break;
    case FHP1: tmp = dsp_Filter1stOrder(FHP1,freq, gain); break;
    case FLS1: tmp = dsp_Filter1stOrder(FLS1,freq, gain); break;
    case FHS1: tmp = dsp_Filter1stOrder(FHS1,freq, gain); break;
    case FAP1: tmp = dsp_Filter1stOrder(FAP1,freq, gain); break;
    default:
        tmp = dsp_Filter1stOrder(FNONE, 1.0, 1.0); break;
        //dspprintf("NOT SUPPORTED (type = %d)\n",type);

    }
    return tmp;
}
