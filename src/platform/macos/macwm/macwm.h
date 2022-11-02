/* macwm.h
 * Interface to MacOS window manager.
 * Single window.
 * You must establish contact with the operating system (eg with 'macioc').
 */

#ifndef MACWM_H
#define MACWM_H

struct macwm;

struct macwm_delegate {
  void *userdata;
  void (*close)(void *userdata);
  void (*resize)(void *userdata,int w,int h);
  int (*key)(void *userdata,int keycode,int value);
  void (*text)(void *userdata,int codepoint);
  void (*mmotion)(void *userdata,int x,int y);
  void (*mbutton)(void *userdata,int btnid,int value);
  void (*mwheel)(void *userdata,int dx,int dy);
};

struct macwm_setup {
  int w,h;
  int fullscreen;
  const char *title;
};

void macwm_del(struct macwm *macwm);

struct macwm *macwm_new(
  const struct macwm_delegate *delegate,
  const struct macwm_setup *setup
);

void macwm_get_size(int *w,int *h,const struct macwm *macwm);
int macwm_get_fullscreen(const struct macwm *macwm);

#define MACWM_FULLSCREEN_ON       1
#define MACWM_FULLSCREEN_OFF      0
#define MACWM_FULLSCREEN_TOGGLE  -1
void macwm_set_fullscreen(struct macwm *macwm,int state);

int macwm_update(struct macwm *macwm);

//TODO rendering. are we doing framebuffers, opengl, metal....

#define MACWM_MBUTTON_LEFT 0
#define MACWM_MBUTTON_RIGHT 1
#define MACWM_MBUTTON_MIDDLE 2

int macwm_hid_from_mac_keycode(int maccode);

/* We use this internally, to drop any keys held when we lose input focus.
 * (that's a bug in Apple's event management IMHO, but easy enough to work around).
 * The public can use it too, if you ever have a need to forget all held keys.
 * This does fire callbacks.
 */
void macwm_release_keys(struct macwm *macwm);

#endif
