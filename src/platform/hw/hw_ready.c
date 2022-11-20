#include "hw_internal.h"

/* Iterate types from a comma-delimited list, or indexing function.
 */
 
static int hw_for_each_type(
  struct hw *hw,
  const char *names,
  const void *type_by_name(const char *name,int namec),
  const void *type_by_index(int p),
  int (*cb)(struct hw *hw,const void *type,int explicit),
  const char *tname
) {
  int err=0,tryc=0;
  if (names) {
    while (*names) {
      if ((unsigned char)(*names)<=0x20) { names++; continue; }
      if (*names==',') { names++; continue; }
      int tokenc=0;
      while (names[tokenc]&&(names[tokenc]!=',')) tokenc++;
      const void *type=type_by_name(names,tokenc);
      if (type) {
        tryc++;
        if (err=cb(hw,type,1)) goto _done_;
      }
      names+=tokenc;
    }
  } else {
    int p=0; for (;;p++) {
      const void *type=type_by_index(p);
      if (!type) break;
      tryc++;
      if (err=cb(hw,type,0)) goto _done_;
    }
  }
 _done_:;
  return err;
}

/* Initialize video and render.
 */
 
static int hw_init_video_1(struct hw *hw,const struct hw_video_type *type,int explicit) {
  if (type->request_only&&!explicit) return 0;
  fprintf(stderr,"TODO %s %s\n",__func__,type->name);
  return -1;
}

static int hw_init_render_1(struct hw *hw,const struct hw_render_type *type,int explicit) {
  if (type->request_only&&!explicit) return 0;
  fprintf(stderr,"TODO %s %s\n",__func__,type->name);
  return -1;
}
 
static int hw_init_video(struct hw *hw) {

  if (hw_for_each_type(
    hw,
    hw->video_names,
    (void*)hw_video_type_by_name,
    (void*)hw_video_type_by_index,
    (void*)hw_init_video_1,
    "video"
  )<0) return -1;
  if (hw->video) {
    fprintf(stderr,"%s: Using video driver '%s'.\n",hw->exename,hw->video->type->name);
  } else {
    fprintf(stderr,"%s: Failed to initialize any video driver.\n",hw->exename);
    return -1;
  }
  
  if (hw_for_each_type(
    hw,
    hw->render_names,
    (void*)hw_render_type_by_name,
    (void*)hw_render_type_by_index,
    (void*)hw_init_render_1,
    "render"
  )<0) return -1;
  if (hw->render) {
    fprintf(stderr,"%s: Using renderer '%s'.\n",hw->exename,hw->render->type->name);
  } else {
    fprintf(stderr,"%s: Failed to initialize any renderer.\n",hw->exename);
    return -1;
  }
  
  return 0;
}

/* Initialize audio and synth.
 */
 
static int hw_init_audio_1(struct hw *hw,const struct hw_audio_type *type,int explicit) {
  if (type->request_only&&!explicit) return 0;
  fprintf(stderr,"TODO %s %s\n",__func__,type->name);
  return -1;
}
 
static int hw_init_synth_1(struct hw *hw,const struct hw_synth_type *type,int explicit) {
  if (type->request_only&&!explicit) return 0;
  fprintf(stderr,"TODO %s %s\n",__func__,type->name);
  return -1;
}
 
static int hw_init_audio(struct hw *hw) {
  
  if (hw_for_each_type(
    hw,
    hw->audio_names,
    (void*)hw_audio_type_by_name,
    (void*)hw_audio_type_by_index,
    (void*)hw_init_audio_1,
    "audio"
  )<0) return -1;
  if (hw->audio) {
    fprintf(stderr,
      "%s: Using audio driver '%s' rate=%d chanc=%d\n",
      hw->exename,hw->audio->type->name,hw->audio->rate,hw->audio->chanc
    );
  } else {
    fprintf(stderr,"%s: Failed to initialize any audio driver.\n",hw->exename);
    return -1;
  }
  
  if (hw_for_each_type(
    hw,
    hw->synth_names,
    (void*)hw_synth_type_by_name,
    (void*)hw_synth_type_by_index,
    (void*)hw_init_synth_1,
    "synth"
  )<0) return -1;
  if (hw->synth) {
    fprintf(stderr,"%s: Using synthesizer '%s'.\n",hw->exename,hw->synth->type->name);
  } else {
    fprintf(stderr,"%s: Failed to initialize any synthsizer.\n",hw->exename);
    return -1;
  }
  
  return 0;
}

/* Initialize input.
 */
 
static int hw_init_input_1(struct hw *hw,const struct hw_input_type *type,int explicit) {
  if (type->request_only&&!explicit) return 0;
  
  if (hw->inputc>=hw->inputa) {
    int na=hw->inputa+8;
    if (na>INT_MAX/sizeof(void*)) return 0;
    void *nv=realloc(hw->inputv,sizeof(void*)*na);
    if (!nv) return 0;
    hw->inputv=nv;
    hw->inputa=na;
  }
  
  struct hw_input *input=hw_input_new(type,&hw->delegate);
  if (!input) {
    fprintf(stderr,"%s: Failed to initialize input driver '%s'. Proceeding without.\n",hw->exename,type->name);
    return 0;
  }
  fprintf(stderr,"%s: Using input driver '%s'.\n",hw->exename,type->name);
  return 0;
}
 
static int hw_init_input(struct hw *hw) {
  
  if (hw_for_each_type(
    hw,
    hw->input_names,
    (void*)hw_input_type_by_name,
    (void*)hw_input_type_by_index,
    (void*)hw_init_input_1,
    "input"
  )<0) return -1;
  
  return 0;
}

/* Ready.
 */
 
int hw_ready(struct hw *hw) {
  if (!hw) return -1;
  if (hw->ready) return 0;
  
  if (hw_init_video(hw)<0) return -1;
  if (hw_init_audio(hw)<0) return -1;
  if (hw_init_input(hw)<0) return -1;
  
  hw->ready=1;
  return 0;
}
