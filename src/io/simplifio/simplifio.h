/* simplifio.h
 * Required: serial
 * Optional:
 *
 * Alternative to "hostio", without the abstraction layer.
 * We initialize video, audio, and input units directly, employing the preprocessor heavily.
 */
 
#ifndef SIMPLIFIO_H
#define SIMPLIFIO_H

#include <stdint.h>

/* Delegate hooks all take the exact same shape as hostio, but (void*) instead of the driver pointer.
 */
struct simplifio_delegate {
  void *userdata;
  
  // Audio:
  void (*cb_pcm_out)(int16_t *v,int c,void *userdata);
  
  // Input:
  void (*cb_connect)(void *userdata,int devid);
  void (*cb_disconnect)(void *userdata,int devid);
  void (*cb_button)(void *userdata,int devid,int btnid,int value);
  
  // Video:
  void (*cb_close)(void *userdata);
  void (*cb_focus)(void *userdata,int focus);
  void (*cb_resize)(void *userdata,int w,int h);
  int (*cb_key)(void *userdata,int keycode,int value);
  void (*cb_text)(void *userdata,int codepoint);
  void (*cb_mmotion)(void *userdata,int x,int y);
  void (*cb_mbutton)(void *userdata,int btnid,int value);
  void (*cb_mwheel)(void *userdata,int dx,int dy);
};

struct simplifio_setup {

  // Normally hard-coded:
  const char *title;
  const void *iconrgba;
  int iconw,iconh;
  int fbw,fbh;

  // Normally open for user config:
  const char *video_driver;
  const char *audio_driver;
  const char *input_driver;
  int audio_rate;
  int audio_chanc;
  const char *audio_device;
  int audio_buffer_size;
  const char *input_path;
  int w,h;
  int fullscreen;
  const char *video_device;
};

/* After initializing (setup) with your defaults, call this with (argc,argv) exactly as provided to the program.
 * We will rewrite (argv) on the fly, and return the new (argc).
 */
int simplifio_setup_apply_argv(struct simplifio_setup *setup,int argc,char **argv);

/* To aid in printing your "--help" text.
 * (k) will have leading dashes eg "--fullscreen".
 * (v) is null if the option doesn't take a parameter, otherwise something like "INT", "PATH", "HZ".
 * (desc) is a free-form description of the option, in English.
 */
void simplifio_for_each_option(void (*cb)(const char *k,const char *v,const char *desc,void *userdata),void *userdata);

/* Startup, shutdown, and maintenance: No surprises.
 * Update may or may not block for vsync, drivers have some leeway there.
 * So you'll want a backup clock.
 */

void simplifio_quit();

int simplifio_init(
  const struct simplifio_delegate *delegate,
  const struct simplifio_setup *setup
);

int simplifio_update();

/* API routed to the enabled drivers.
 */

const char *simplifio_audio_get_driver_name();
int simplifio_audio_get_rate();
int simplifio_audio_get_chanc();
void simplifio_audio_play(int play);
int simplifio_audio_lock();
void simplifio_audio_unlock();

const char *simplifio_video_get_driver_name();
void simplifio_video_get_size(int *w,int *h);
int simplifio_video_provides_input();
int simplifio_video_get_fullscreen();
void simplifio_video_show_cursor(int show);
void simplifio_video_set_fullscreen(int fullscreen);

struct simplifio_fb_description {
  int w,h; // pixels
  int stride; // bytes
  int pixelsize; // bits
  
  // Masks are relevant if a pixel is read as native words. pixelsize 8, 16, 32 only.
  // It is recommended to declare both masks and chorder, if possible.
  int rmask,gmask,bmask,amask;
  
  // 8-bit channels but not necessarily neat words (eg 24-bit RGB) can use chorder instead.
  // Should contain "rgbyax" in the order stored, NULs at the end as placeholders.
  char chorder[4];
};

int simplifio_video_describe_fb(struct simplifio_fb_description *desc);
void *simplifio_video_begin_fb();
int simplifio_video_end_fb();

int simplifio_video_supports_gx();
int simplifio_video_begin_gx();
int simplifio_video_end_gx();

const char *simplifio_input_get_driver_name();
void simplifio_input_disconnect(int devid);
const char *simplifio_input_get_ids(int *vid,int *pid,int *version,int devid);
int simplifio_input_for_each_button(
  int devid,
  int (*cb)(int btnid,int hidusage,int lo,int hi,int value,void *userdata),
  void *userdata
);

#endif
