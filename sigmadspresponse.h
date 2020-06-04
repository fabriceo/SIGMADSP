#ifndef SIGMADSP_RESPONSE_H
#define SIGMADSP_RESPONSE_H

#include "sigmadspfilters.h"

// basic structure for Complex number
typedef struct {
    float re;
    float im;
} dspComplex_t;

// structure for managing filter response and bode plot
typedef struct {
    float freq;
    dspComplex_t Z; // frequency/fs in complex domain
    dspComplex_t H; // response
    float mag;      // magnitude in dB for the bode Plot
    float phase;    // phase for bode plot
} dspBode_t;

// calculate the multiplier for producing a geometrical list of frequencies between start-stop
extern float dspFreqMultiplier(float start, float stop, int N);
// initialize the table containing the response for further printing as a bode plot
extern void dspBodeResponseInit(dspBode_t * t, int N, float gain);
// initialize the table of frequencies with the geometrical list between start and stop
extern void dspBodeFreqInit(float start, float stop, long round, dspBode_t * t, int N, float fs);
// apply a biquad on the response table to further print as a bode plot
extern void dspBodeApplyBiquad(dspBode_t * t, int N, dspBiquadCoefs_t * bq);
// apply a filter made of multiple biquad on the response table
extern void dspBodeApplyFilter(dspBode_t * t, int N, dspFilter_t * f);
// apply a list of filters made of multiple biquad on the response table
extern void dspBodeApplyFilterBank(dspBode_t * t, int N, dspFilter_t * f, int max);

// compute the magnitude of the response in deciBell. return also min and max
extern void dspBodeMagnitudeDB(dspBode_t * t, int N, float gain, float *min, float *max);
// compute the phase of the response, in degrees between -180..+180
extern void dspBodePhase(dspBode_t * t, int N);

#endif
