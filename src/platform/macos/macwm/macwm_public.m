#include "macwm_internal.h"

/* Delete.
 */

void macwm_del(struct macwm *macwm) {
  if (!macwm) return;
  [macwm->window release];
  free(macwm);
}

/* New.
 */

struct macwm *macwm_new(
  const struct macwm_delegate *delegate,
  const struct macwm_setup *setup
) {
  struct macwm *macwm=calloc(1,sizeof(struct macwm));
  if (!macwm) return 0;

  if (delegate) macwm->delegate=*delegate;

  const char *title=0;
  if (setup) {
    macwm->w=setup->w;
    macwm->h=setup->h;
    macwm->fullscreen=setup->fullscreen?1:0;
    title=setup->title;
  }
  if (macwm->w<1) macwm->w=100;
  if (macwm->h<1) macwm->h=100;

  if (!(macwm->window=[[AKWindow alloc] initWithOwner:macwm title:title])) {
    macwm_del(macwm);
    return 0;
  }

  return macwm;
}

/* Trivial accessors.
 */

void macwm_get_size(int *w,int *h,const struct macwm *macwm) {
  if (!macwm) return;
  if (w) *w=macwm->w;
  if (h) *h=macwm->h;
}

int macwm_get_fullscreen(const struct macwm *macwm) {
  if (!macwm) return 0;
  return macwm->fullscreen;
}

/* Fullscreen.
 */

void macwm_set_fullscreen(struct macwm *macwm,int state) {
  if (!macwm) return;
  if (state<0) state=macwm->fullscreen?0:1;
  if (state) {
    if (macwm->fullscreen) return;
    [macwm->window toggleFullScreen:0];
    macwm->fullscreen=1;
  } else {
    if (!macwm->fullscreen) return;
    [macwm->window toggleFullScreen:0];
    macwm->fullscreen=0;
  }
}

/* Update.
 */

int macwm_update(struct macwm *macwm) {
  if (!macwm) return -1;
  // Probably nothing to do here? Events are driven by Apple's IoC layer.
  return 0;
}

/* Release all held keys.
 */

void macwm_release_keys(struct macwm *macwm) {
  if (macwm->delegate.key) {
    while (macwm->keyc>0) {
      macwm->keyc--;
      macwm->delegate.key(macwm->delegate.userdata,macwm->keyv[macwm->keyc],0);
    }
  } else {
    macwm->keyc=0;
  }
}

void macwm_register_key(struct macwm *macwm,int keycode) {
  if (!keycode) return;
  if (macwm->keyc>=MACWM_KEY_LIMIT) return;
  int i=macwm->keyc; while (i-->0) {
    if (macwm->keyv[i]==keycode) return;
  }
  macwm->keyv[macwm->keyc++]=keycode;
}

void macwm_unregister_key(struct macwm *macwm,int keycode) {
  int i=macwm->keyc; while (i-->0) {
    if (macwm->keyv[i]==keycode) {
      macwm->keyc--;
      memmove(macwm->keyv+i,macwm->keyv+i+1,sizeof(int)*(macwm->keyc-i));
      return;
    }
  }
}
