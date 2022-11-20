#ifndef HW_INTERNAL_H
#define HW_INTERNAL_H

#include "hw.h"
#include "hw_video.h"
#include "hw_audio.h"
#include "hw_input.h"
#include "hw_synth.h"
#include "hw_render.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>

#define HW_FRAME_RATE 60

struct hw {
  int refc;
  struct hw_delegate delegate;
  const char *exename;
  
  // These are config only, meaningless after ready:
  char *video_names;
  char *audio_names;
  char *synth_names;
  char *render_names;
  char *input_names;
  char *window_title;
  char *audio_device;
  void *icon;
  int iconw,iconh;
  int winw,winh;
  int fbw,fbh;
  int fullscreen;
  int audio_rate,chanc;
  int ready;
  
  struct hw_video *video;
  struct hw_audio *audio;
  struct hw_synth *synth;
  struct hw_render *render;
  struct hw_input **inputv;
  int inputc,inputa;
};

#endif
