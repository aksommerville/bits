/* hw.h
 * Hardware abstraction for games.
 * Also for logical systems like synthesizers.
 */
 
#ifndef HW_H
#define HW_H

#include <stdint.h>

struct hw;
struct hw_video;
struct hw_audio;
struct hw_input;
struct hw_synth;
struct hw_video_type;
struct hw_audio_type;
struct hw_input_type;
struct hw_synth_type;
 
struct hw_delegate {
  void *userdata;
  
  /* Both are called when the driver stack is initialized and ready to use.
   */
  void (*quit)(void *userdata);
  int (*init)(void *userdata);
  
  void (*wm_close)(void *userdata);
  void (*wm_focus)(void *userdata,int focus);
  void (*wm_resize)(void *userdata,int w,int h);
  void (*wm_render)(void *userdata);
  
  int (*wm_key)(void *userdata,int keycode,int value);
  void (*wm_text)(void *userdata,int codepoint);
  
  void (*wm_mmotion)(void *userdata,int x,int y);
  void (*wm_mbutton)(void *userdata,int btnid,int value);
  void (*wm_mwheel)(void *userdata,int dx,int dy);
  
  void (*pcm_out)(int16_t *v,int c,void *userdata);
  void (*midi_in)(void *userdata,int devid,const void *v,int c);
  
  void (*input_connect)(void *userdata,int devid);
  void (*input_disconnect)(void *userdata,int devid);
  void (*input_button)(void *userdata,int devid,int btnid,int value);
  
  void (*input_mapped)(void *userdata,int devid,int plrid,int btnid,int value,uint16_t state);
  
  int (*arg)(void *userdata,const char *k,int kc,const char *v,int vc);
  void (*update)(void *userdata);
  int (*sleep)(void *userdata,int us);
};

/* IoC wrapper.
 * Selecting an IoC unit is a bit different from selecting IO drivers.
 * This is optional; you can use struct hw directly instead.
 *************************************************************/
 
int hw_main(int argc,char **argv,const struct hw_delegate *delegate);

struct hw *hw_global();

/* Main hardware wrangler.
 * You normally have one of these, globally.
 *************************************************************/
 
void hw_del(struct hw *hw);
int hw_ref(struct hw *hw);

struct hw *hw_new(const struct hw_delegate *delegate);

int hw_config_envv(struct hw *hw,char **envv);
int hw_config_argv(struct hw *hw,int argc,char **argv);
int hw_config_file(struct hw *hw,const char *path,int require);
int hw_config_text(struct hw *hw,const char *src,int srcc,const char *path);
int hw_config_kv(struct hw *hw,const char *k,int kc,const char *v,int vc);
int hw_config_set_video_names(struct hw *hw,const char *v,int vc);
int hw_config_set_audio_names(struct hw *hw,const char *v,int vc);
int hw_config_set_input_names(struct hw *hw,const char *v,int vc);
int hw_config_set_synth_names(struct hw *hw,const char *v,int vc);
int hw_config_set_render_names(struct hw *hw,const char *v,int vc);
int hw_config_set_window_title(struct hw *hw,const char *src,int srcc);
int hw_config_set_window_icon(struct hw *hw,const void *rgba,int w,int h);
int hw_config_set_window_size(struct hw *hw,int w,int h);
int hw_config_set_fullscreen(struct hw *hw,int fullscreen);
int hw_config_set_framebuffer_size(struct hw *hw,int w,int h);
int hw_config_set_audio_rate(struct hw *hw,int rate);
int hw_config_set_audio_chanc(struct hw *hw,int chanc);
int hw_config_set_audio_device(struct hw *hw,const char *src,int srcc);

/* Call after all configuration, and before any update.
 * This is where driver and config decisions get committed.
 */
int hw_ready(struct hw *hw);

int hw_update(struct hw *hw);

void *hw_get_userdata(const struct hw *hw);
struct hw_video *hw_get_video(const struct hw *hw);
struct hw_audio *hw_get_audio(const struct hw *hw);
struct hw_synth *hw_get_synth(const struct hw *hw);
struct hw_render *hw_get_render(const struct hw *hw);
struct hw_input *hw_get_input(const struct hw *hw,int p);
int hw_get_input_count(const struct hw *hw);

/* Driver registry.
 **********************************************************************/
 
const struct hw_video_type *hw_video_type_by_index(int p);
const struct hw_video_type *hw_video_type_by_name(const char *name,int namec);
const struct hw_audio_type *hw_audio_type_by_index(int p);
const struct hw_audio_type *hw_audio_type_by_name(const char *name,int namec);
const struct hw_input_type *hw_input_type_by_index(int p);
const struct hw_input_type *hw_input_type_by_name(const char *name,int namec);
const struct hw_synth_type *hw_synth_type_by_index(int p);
const struct hw_synth_type *hw_synth_type_by_name(const char *name,int namec);
const struct hw_render_type *hw_render_type_by_index(int p);
const struct hw_render_type *hw_render_type_by_name(const char *name,int namec);

int hw_devid_next(void *ignored);

#endif
