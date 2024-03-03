#include "simplifio_internal.h"
#include "serial/serial.h"

/* Maybe apply one key=value pair to the setup.
 * If we don't recognize the key, return zero, caller should preserve it for our client's use.
 * Return <0 for hard errors or >0 to consume the argument.
 */
 
static int simplifio_setup_apply_kv(struct simplifio_setup *setup,const char *k,int kc,const char *v) {
  
  #define STROPT(arg,fld) if ((kc==sizeof(arg)-1)&&!memcmp(k,arg,kc)) { \
    if (v&&!v[0]) setup->fld=0; \
    else setup->fld=v; \
    return 1; \
  }
  #define INTOPT(arg,fld) if ((kc==sizeof(arg)-1)&&!memcmp(k,arg,kc)) { \
    int n; \
    if (sr_int_eval(&n,v,-1)<1) return -1; \
    setup->fld=n; \
    return 1; \
  }
  #define BOOLOPT(arg,fld) if ((kc==sizeof(arg)-1)&&!memcmp(k,arg,kc)) { \
    int n; \
    if (sr_bool_eval(&n,v,-1)<0) return -1; \
    setup->fld=n; \
    return 1; \
  }
  
  STROPT("audio-driver",audio_driver)
  INTOPT("audio-rate",audio_rate)
  INTOPT("audio-chanc",audio_chanc)
  STROPT("audio-device",audio_device)
  INTOPT("audio-buffer-size",audio_buffer_size)
  STROPT("input-driver",input_driver)
  STROPT("input-path",input_path)
  STROPT("video-driver",video_driver)
  INTOPT("w",w)
  INTOPT("h",h)
  BOOLOPT("fullscreen",fullscreen)
  STROPT("video-device",video_device)
  
  // If you add anything here, don't forget to add to simplifio_for_each_option too.
  
  #undef STROPT
  #undef INTOPT
  #undef BOOLOPT
  return 0;
}

/* Apply argv to setup.
 * Remove consumed arguments.
 */
 
int simplifio_setup_apply_argv(struct simplifio_setup *setup,int argc,char **argv) {
  int argi=1;
  while (argi<argc) {
    
    /* All arguments we consume take one of these forms:
     *  --KEY=VALUE
     *  --KEY VALUE
     *  --KEY
     *  --no-KEY
     * Important to know it all in advance, because we're rewriting on the fly.
     */
    if (!argv[argi]) { argi++; continue; }
    if (argv[argi][0]!='-') { argi++; continue; }
    if (argv[argi][1]!='-') { argi++; continue; }
    int eatc=1;
    const char *k=argv[argi]+2;
    int kc=0;
    while (k[kc]&&(k[kc]!='=')) kc++;
    const char *v=0;
    if (k[kc]=='=') v=k+kc+1;
    else if ((kc>3)&&!memcmp(k,"no-",3)) {
      k+=3;
      kc-=3;
      v="0";
    } else if ((argi+1<argc)&&argv[argi+1]&&argv[argi+1][0]&&(argv[argi+1][0]!='-')) {
      v=argv[argi+1];
      eatc=2;
    } else v="1";
    
    int err=simplifio_setup_apply_kv(setup,k,kc,v);
    if (err<0) return err;
    if (!err) { argi+=eatc; continue; }
    
    // Consumed! Remove the argument.
    argc-=eatc;
    memmove(argv+argi,argv+argi+eatc,sizeof(void*)*(argc-argi));
    argv[argc]=0;
  }
  return argc;
}

/* List options.
 */

void simplifio_for_each_option(void (*cb)(const char *k,const char *v,const char *desc,void *userdata),void *userdata) {
  char tmp[256];
  int tmpc;
  #define ADDDRIVER(name) if (USE_##name) { \
    if (tmpc) tmp[tmpc++]='|'; \
    memcpy(tmp+tmpc,#name,sizeof(#name)-1); \
    tmpc+=sizeof(#name)-1; \
  }
  
  tmpc=0;
  ADDDRIVER(pulse)
  ADDDRIVER(alsafd)
  ADDDRIVER(asound)
  ADDDRIVER(macaudio)
  ADDDRIVER(msaudio)
  tmp[tmpc]=0;
  cb("--audio-driver",tmp,"Audio driver name.",userdata);
  
  cb("--audio-rate","HZ","Audio output rate.",userdata);
  cb("--audio-chanc","1|2","Audio channel count.",userdata);
  cb("--audio-device","STRING","Audio device name, if driver needs it.",userdata);
  cb("--audio-buffer-size","BYTES","Audio buffer size, if driver needs it.",userdata);
  
  tmpc=0;
  ADDDRIVER(evdev)
  ADDDRIVER(machid)
  ADDDRIVER(mshid)
  tmp[tmpc]=0;
  cb("--input-driver",tmp,"Input driver name.",userdata);
  
  cb("--input-path","STRING","Path for input devices, if driver needs it.",userdata);
  
  tmpc=0;
  ADDDRIVER(glx)
  ADDDRIVER(drmgx)
  ADDDRIVER(x11fb)
  ADDDRIVER(drmfb)
  ADDDRIVER(bcm)
  ADDDRIVER(macwm)
  ADDDRIVER(mswm)
  tmp[tmpc]=0;
  cb("--video-driver",tmp,"Video driver name.",userdata);
  
  cb("--w","PIXELS","Window or screen width.",userdata);
  cb("--h","PIXELS","Window or screen height.",userdata);
  cb("--fullscreen","BOOLEAN","Start in fullscreen if possible.",userdata);
  cb("--video-device","STRING","Video device name, if driver needs it.",userdata);
  
  #undef ADDDRIVER
}
