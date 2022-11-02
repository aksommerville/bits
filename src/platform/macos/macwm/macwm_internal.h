#ifndef MACWM_INTERNAL_H
#define MACWM_INTERNAL_H

#include "macwm.h"
#include "AKWindow.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define MACWM_KEY_LIMIT 16

struct macwm {
  struct macwm_delegate delegate;
  int w,h;
  int fullscreen;
  AKWindow *window;
  int modifiers;
  int keyv[MACWM_KEY_LIMIT];
  int keyc;
};

void macwm_register_key(struct macwm *macwm,int keycode);
void macwm_unregister_key(struct macwm *macwm,int keycode);

#endif
