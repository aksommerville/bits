#ifndef SYNTH_MINOR_INTERNAL_H
#define SYNTH_MINOR_INTERNAL_H

#include "synth_minor.h"
#include "format/midi/midi.h"
#include <stdlib.h>
#include <limits.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

struct sm_voice {
  uint8_t chid,noteid; // for identification
  float p;
  float dp; // pitch in radians
  float level;
  float dlevel;
  float stoplevel;
  int ttl;
  float bend;
};

struct synth_minor {
  int rate;
  int chanc;
  float *buf;
  int bufa;
  int16_t qlevel;
  struct sm_voice *voicev;
  int voicec,voicea;
  float bend;//XXX this is a channel concern, just for now we don't have channels
};

void sm_voice_cleanup(struct sm_voice *voice);
int sm_voice_update(float *v,int c,struct sm_voice *voice); // => >0 if still running ; adds to (v)

#endif
