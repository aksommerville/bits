#include "hw_internal.h"

/* Delete.
 */
 
void hw_del(struct hw *hw) {
  if (!hw) return;
  if (hw->refc-->1) return;
  
  hw_render_del(hw->render);
  hw_video_del(hw->video);
  hw_audio_del(hw->audio);
  hw_synth_del(hw->synth);
  if (hw->inputv) {
    while (hw->inputc-->0) hw_input_del(hw->inputv[hw->inputc]);
    free(hw->inputv);
  }
  
  if (hw->video_names) free(hw->video_names);
  if (hw->audio_names) free(hw->audio_names);
  if (hw->input_names) free(hw->input_names);
  if (hw->synth_names) free(hw->synth_names);
  if (hw->render_names) free(hw->render_names);
  if (hw->window_title) free(hw->window_title);
  if (hw->audio_device) free(hw->audio_device);
  if (hw->icon) free(hw->icon);
  
  free(hw);
}

/* Retain.
 */
 
int hw_ref(struct hw *hw) {
  if (!hw) return -1;
  if (hw->refc<1) return -1;
  if (hw->refc==INT_MAX) return -1;
  hw->refc++;
  return 0;
}

/* New.
 */

struct hw *hw_new(const struct hw_delegate *delegate) {
  struct hw *hw=calloc(1,sizeof(struct hw));
  if (!hw) return 0;
  
  hw->refc=1;
  if (delegate) hw->delegate=*delegate;
  
  return hw;
}

/* Trivial accessors.
 */

void *hw_get_userdata(const struct hw *hw) {
  if (!hw) return 0;
  return hw->delegate.userdata;
}

struct hw_video *hw_get_video(const struct hw *hw) {
  if (!hw) return 0;
  return hw->video;
}

struct hw_audio *hw_get_audio(const struct hw *hw) {
  if (!hw) return 0;
  return hw->audio;
}

struct hw_synth *hw_get_synth(const struct hw *hw) {
  if (!hw) return 0;
  return hw->synth;
}

struct hw_render *hw_get_render(const struct hw *hw) {
  if (!hw) return 0;
  return hw->render;
}

struct hw_input *hw_get_input(const struct hw *hw,int p) {
  if (!hw) return 0;
  if (p<0) return 0;
  if (p>=hw->inputc) return 0;
  return hw->inputv[p];
}

int hw_get_input_count(const struct hw *hw) {
  if (!hw) return 0;
  return hw->inputc;
}
