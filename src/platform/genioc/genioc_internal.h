#ifndef GENIOC_INTERNAL_H
#define GENIOC_INTERNAL_H

#include "genioc.h"
#include <stdio.h>
#include <stdint.h>

extern struct genioc {
  const char *exename; // also serves as the "running" flag
  volatile int terminate;
  int termstatus;
  struct genioc_delegate delegate;
  
  int64_t starttime;
  int64_t nexttime;
  int64_t frametime;
  int clockfaultc;
} genioc;

void genioc_clock_init();
void genioc_clock_update();

#endif
