#include "hw_internal.h"

#if BITS_USE_serial
  #include "serial/serial.h"
#endif

/* From environment.
 */
 
int hw_config_envv(struct hw *hw,char **envv) {
  if (!hw||hw->ready) return -1;
  //TODO
  return 0;
}

/* From argv.
 */
 
int hw_config_argv(struct hw *hw,int argc,char **argv) {
  if (!hw||hw->ready) return -1;
  if ((argc>=1)&&argv[0]) hw->exename=argv[0];
  int err;
  int argp=1;
  while (argp<argc) {
  
    // Ignore empty arguments.
    const char *arg=argv[argp++];
    if (!arg||!arg[0]) continue;
    
    // No dash, positional argument (ie empty key).
    if (arg[0]!='-') {
      if ((err=hw_config_kv(hw,0,0,arg,-1))<0) return err;
      continue;
    }
    
    // Single dash alone. Reserved for future use.
    if (!arg[1]) {
      fprintf(stderr,"%s: Unexpected argument '%s'\n",hw->exename,arg);
      return -1;
    }
    
    // Single dash: Single-char key.
    if (arg[1]!='-') {
      const char *k=arg+1;
      const char *v=arg+2;
      if (!v[0]&&(argp<argc)&&argv[argp]&&(argv[argp][0]!='-')) v=argv[argp++];
      if ((err=hw_config_kv(hw,k,1,v,-1))<0) return err;
      continue;
    }
    
    // Double dash alone. Reserved for future use.
    if (!arg[2]) {
      fprintf(stderr,"%s: Unexpected argument '%s'\n",hw->exename,arg);
      return -1;
    }
    
    // Double dash: "--key=value"
    const char *k=arg+2;
    while (*k=='-') k++;
    int kc=0;
    while (k[kc]&&(k[kc]!='=')) kc++;
    const char *v=0;
    if (k[kc]=='=') v=k+kc+1;
    else if ((argp<argc)&&argv[argp]&&(argv[argp][0]!='-')) v=argv[argp++];
    if ((err=hw_config_kv(hw,k,kc,v,-1))<0) return err;
  }
  return 0;
}

/* From file (read file first).
 */
 
int hw_config_file(struct hw *hw,const char *path,int require) {
  if (!hw||hw->ready||!path) return -1;
  //TODO
  return 0;
}

/* From text.
 */
 
int hw_config_text(struct hw *hw,const char *src,int srcc,const char *path) {
  if (!hw||hw->ready) return -1;
  //TODO
  return 0;
}

/* Evalute input tokens.
 */
 
static int hw_config_eval_int(int *dst,const char *src,int srcc) {
  #if BITS_USE_serial
    return sr_int_eval(dst,src,srcc);
  #endif
  return -1;
}

static int hw_config_eval_int2(int *x,int *y,const char *src,int srcc) {
  if (!src) srcc=0; else if (srcc<0) { srcc=0; while (src[srcc]) srcc++; }
  int sepp=-1,xp=-1,i=0;
  for (;i<srcc;i++) {
    if (src[i]==',') { sepp=i; break; }
    if (src[i]=='x') xp=i; // allow that it might be a hexadecimal introducer
  }
  if (sepp<0) {
    if (xp<0) return -1;
    sepp=xp;
  }
  #if BITS_USE_serial
    if (sr_int_eval(x,src,sepp)<0) return -1;
    if (sr_int_eval(y,src+sepp+1,srcc-sepp-1)<0) return -1;
    return 0;
  #endif
  return -1;
}

static int hw_config_eval_boolean(const char *src,int srcc) {
  if (!src) srcc=0; else if (srcc<0) { srcc=0; while (src[srcc]) srcc++; }
  if (srcc==1) switch (src[0]) {
    case '0': return 0;
    case '1': return 1;
  }
  if ((srcc==4)&&!memcmp(src,"true",4)) return 1;
  if ((srcc==5)&&!memcmp(src,"false",5)) return 0;
  #if BITS_USE_serial
    int n;
    if (sr_int_eval(&n,src,srcc)>=1) {
      return n?1:0;
    }
  #endif
  return -1;
}

/* Cut key=value pair.
 */
 
int hw_config_kv(struct hw *hw,const char *k,int kc,const char *v,int vc) {
  if (!hw||hw->ready) return -1;
  if (!k) kc=0; else if (kc<0) { kc=0; while (k[kc]) kc++; }
  if (!v) vc=0; else if (vc<0) { vc=0; while (v[vc]) vc++; }
  
  if (!vc) {
    if ((kc>=3)&&!memcmp(k,"no-",3)) {
      v="0";
      vc=1;
      k+=3;
      kc-=3;
    } else {
      v="1";
      vc=1;
    }
  }
  
  if (hw->delegate.arg) {
    int err=hw->delegate.arg(hw->delegate.userdata,k,kc,v,vc);
    if (err) return err;
  }
  
  #define STRARG(argstr,fnsfx) if ((kc==sizeof(argstr)-1)&&!memcmp(k,argstr,kc)) { \
    return hw_config_set_##fnsfx(hw,v,vc); \
  }
  #define INTARG(argstr,fnsfx,lo,hi) if ((kc==sizeof(argstr)-1)&&!memcmp(k,argstr,kc)) { \
    int n; \
    if (hw_config_eval_int(&n,v,vc)<0) { \
      fprintf(stderr,"%s: Failed to evaluate '%.*s' as integer for '%.*s'\n",hw->exename,vc,v,kc,k); \
      return -1; \
    } \
    if ((n<lo)||(n>hi)) { \
      fprintf(stderr,"%s: '%.*s' out of range for '%.*s' (%d..%d)\n",hw->exename,vc,v,kc,k,lo,hi); \
      return -1; \
    } \
    return hw_config_set_##fnsfx(hw,n); \
  }
  #define BOOLARG(argstr,fnsfx) if ((kc==sizeof(argstr)-1)&&!memcmp(k,argstr,kc)) { \
    int n=hw_config_eval_boolean(v,vc); \
    if (n<0) { \
      fprintf(stderr,"%s: Failed to evaluate '%.*s' as boolean for '%.*s'\n",hw->exename,vc,v,kc,k); \
      return -1; \
    } \
    return hw_config_set_##fnsfx(hw,n); \
  }
  #define INT2ARG(argstr,fnsfx,lo,hi) if ((kc==sizeof(argstr)-1)&&!memcmp(k,argstr,kc)) { \
    int x,y; \
    if (hw_config_eval_int2(&x,&y,v,vc)<0) { \
      fprintf(stderr,"%s: Failed to evaluate '%.*s' as two integers for '%.*s'\n",hw->exename,vc,v,kc,k); \
      return -1; \
    } \
    if ((x<lo)||(x>hi)||(y<lo)||(y>hi)) { \
      fprintf(stderr,"%s: '%.*s' out of range for '%.*s' (%d..%d)\n",hw->exename,vc,v,kc,k,lo,hi); \
      return -1; \
    } \
    return hw_config_set_##fnsfx(hw,x,y); \
  }
  
  STRARG("video",video_names)
  STRARG("audio",audio_names)
  STRARG("input",input_names)
  STRARG("synth",synth_names)
  STRARG("render",render_names)
  
  INTARG("audio-rate",audio_rate,200,200000)
  INTARG("audio-chanc",audio_chanc,1,8)
  STRARG("audio-device",audio_device)
  
  INT2ARG("window-size",window_size,1,4096)
  BOOLARG("fullscreen",fullscreen)
  
  /* A few config fields are deliberately not configurable through this generic text interface:
   *  window_title,icon,iconw,iconh,fbw,fbh
   */
  
  #undef STRARG
  #undef INTARG
  #undef BOOLARG
  #undef INT2ARG
  
  fprintf(stderr,"%s: Unexpected argument '%.*s' = '%.*s'\n",hw->exename,kc,k,vc,v);
  return -1;
}

/* Replace config string.
 */
 
static int hw_config_replace_string(char **dst,const char *src,int srcc) {
  if (!src) src=0; else if (srcc<0) { srcc=0; while (src[srcc]) srcc++; }
  char *nv=0;
  if (srcc) {
    if (!(nv=malloc(srcc+1))) return -1;
    memcpy(nv,src,srcc);
    nv[srcc]=0;
  }
  if (*dst) free(*dst);
  *dst=nv;
  return 0;
}

/* Set specific fields.
 */
  
int hw_config_set_video_names(struct hw *hw,const char *v,int vc) {
  if (!hw||hw->ready) return -1;
  return hw_config_replace_string(&hw->video_names,v,vc);
}

int hw_config_set_audio_names(struct hw *hw,const char *v,int vc) {
  if (!hw||hw->ready) return -1;
  return hw_config_replace_string(&hw->audio_names,v,vc);
}

int hw_config_set_input_names(struct hw *hw,const char *v,int vc) {
  if (!hw||hw->ready) return -1;
  return hw_config_replace_string(&hw->input_names,v,vc);
}

int hw_config_set_synth_names(struct hw *hw,const char *v,int vc) {
  if (!hw||hw->ready) return -1;
  return hw_config_replace_string(&hw->synth_names,v,vc);
}

int hw_config_set_render_names(struct hw *hw,const char *v,int vc) {
  if (!hw||hw->ready) return -1;
  return hw_config_replace_string(&hw->render_names,v,vc);
}

int hw_config_set_window_title(struct hw *hw,const char *src,int srcc) {
  if (!hw||hw->ready) return -1;
  return hw_config_replace_string(&hw->window_title,src,srcc);
}

int hw_config_set_window_icon(struct hw *hw,const void *rgba,int w,int h) {
  if (!hw||hw->ready) return -1;
  if ((w<0)||(h<0)) return -1;
  if (w||h) {
    if (!rgba||!w||!h) return -1;
    void *nv=malloc(w*h*4);
    if (!nv) return -1;
    if (hw->icon) free(hw->icon);
    hw->icon=nv;
    hw->iconw=w;
    hw->iconh=h;
  } else {
    if (hw->icon) free(hw->icon);
    hw->icon=0;
    hw->iconw=0;
    hw->iconh=0;
  }
  return 0;
}

int hw_config_set_window_size(struct hw *hw,int w,int h) {
  if (!hw||hw->ready) return -1;
  if ((w<0)||(h<0)) return -1;
  hw->winw=w;
  hw->winh=h;
  return 0;
}

int hw_config_set_fullscreen(struct hw *hw,int fullscreen) {
  if (!hw||hw->ready) return -1;
  hw->fullscreen=fullscreen?1:0;
  return 0;
}

int hw_config_set_framebuffer_size(struct hw *hw,int w,int h) {
  if (!hw||hw->ready) return -1;
  if ((w<0)||(h<0)) return -1;
  hw->fbw=w;
  hw->fbh=h;
  return 0;
}

int hw_config_set_audio_rate(struct hw *hw,int rate) {
  if (!hw||hw->ready) return -1;
  if ((rate<200)||(rate>200000)) return -1;
  hw->audio_rate=rate;
  return 0;
}

int hw_config_set_audio_chanc(struct hw *hw,int chanc) {
  if (!hw||hw->ready) return -1;
  if ((chanc<1)||(chanc>8)) return -1;
  hw->chanc=chanc;
  return 0;
}

int hw_config_set_audio_device(struct hw *hw,const char *src,int srcc) {
  if (!hw||hw->ready) return -1;
  return hw_config_replace_string(&hw->audio_device,src,srcc);
}
