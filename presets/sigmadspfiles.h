#ifndef SIGMADSPFILES_H
#define SIGMADSPFILES_H

#include "sigmadsppresets.h"
#ifdef DSP_GENERIC_OK

extern void dspPresetWriteTextfile(dspPreset_t * p, long numberOfPresets, char * filename);
extern long  dspPresetReadTextfile(dspPreset_t * p, long numberOfPresets, char * filename);
extern long  miniParseFile(dspPreset_t * p, long preset, char * filename);
#endif
#endif