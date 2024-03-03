#include "simplifio_internal.h"

struct simplifio simplifio={0};

/* Quit.
 */
 
void simplifio_quit() {

  #if USE_pulse
    pulse_del(simplifio.pulse);
  #endif
  #if USE_alsafd
    alsafd_del(simplifio.alsafd);
  #endif
  #if USE_asound
    asound_del(simplifio.asound);
  #endif
  #if USE_macaudio
    macaudio_del(simplifio.macaudio);
  #endif
  #if USE_msaudio
    msaudio_del(simplifio.msaudio);
  #endif
  
  #if USE_evdev
    evdev_del(simplifio.evdev);
  #endif
  #if USE_machid
    machid_del(simplifio.machid);
  #endif
  #if USE_mshid
    mshid_del(simplifio.mshid);
  #endif
  
  #if USE_glx
    glx_del(simplifio.glx);
  #endif
  #if USE_drmgx
    if (simplifio.drmgx) drmgx_quit();
  #endif
  #if USE_x11fb
    x11fb_del(simplifio.x11fb);
  #endif
  #if USE_drmfb
    drmfb_del(simplifio.drmfb);
  #endif
  #if USE_bcm
    bcm_del(simplifio.bcm);
  #endif
  #if USE_macwm
    macwm_del(simplifio.macwm);
  #endif
  #if USE_mswm
    mswm_del(simplifio.mswm);
  #endif
  
  memset(&simplifio,0,sizeof(simplifio));
}

/* Input drivers typically have unique callback shapes.
 */
 
#if USE_evdev
  static void simplifio_cb_evdev_connect(struct evdev *evdev,struct evdev_device *device) {
    if (simplifio.input_devid>=INT_MAX) return;
    int devid=simplifio.input_devid++;
    evdev_device_set_devid(device,devid);
    if (simplifio.delegate.cb_connect) {
      simplifio.delegate.cb_connect(simplifio.delegate.userdata,devid);
    }
  }
  
  static void simplifio_cb_evdev_disconnect(struct evdev *evdev,struct evdev_device *device) {
    int devid=evdev_device_get_devid(device);
    if (simplifio.delegate.cb_disconnect) {
      simplifio.delegate.cb_disconnect(simplifio.delegate.userdata,devid);
    }
  }
  
  static void simplifio_cb_evdev_button(struct evdev *evdev,struct evdev_device *device,int type,int code,int value) {
    int devid=evdev_device_get_devid(device);
    if (simplifio.delegate.cb_button) {
      simplifio.delegate.cb_button(simplifio.delegate.userdata,devid,(type<<16)|code,value);
    }
  }
#endif
 
#if USE_machid
  static void simplifio_cb_machid_connect(struct machid *machid,struct machid_device *device) {
    if (simplifio.input_devid>=INT_MAX) return;
    int devid=simplifio.input_devid++;
    machid_device_set_devid(device,devid);
    if (simplifio.delegate.cb_connect) {
      simplifio.delegate.cb_connect(simplifio.delegate.userdata,devid);
    }
  }
  
  static void simplifio_cb_machid_disconnect(struct machid *machid,struct machid_device *device) {
    int devid=machid_device_get_devid(device);
    if (simplifio.delegate.cb_disconnect) {
      simplifio.delegate.cb_disconnect(simplifio.delegate.userdata,devid);
    }
  }
  
  static void simplifio_cb_machid_button(struct machid *machid,struct machid_device *device,int btnid,int value) {
    int devid=machid_device_get_devid(device);
    if (simplifio.delegate.cb_button) {
      simplifio.delegate.cb_button(simplifio.delegate.userdata,devid,btnid,value);
    }
  }
#endif
 
#if USE_mshid
  static void simplifio_cb_mshid_connect(struct mshid *mshid,struct mshid_device *device) {
    if (simplifio.input_devid>=INT_MAX) return;
    int devid=simplifio.input_devid++;
    mshid_device_set_devid(device,devid);
    if (simplifio.delegate.cb_connect) {
      simplifio.delegate.cb_connect(simplifio.delegate.userdata,devid);
    }
  }
  
  static void simplifio_cb_mshid_disconnect(struct mshid *mshid,struct mshid_device *device) {
    int devid=mshid_device_get_devid(device);
    if (simplifio.delegate.cb_disconnect) {
      simplifio.delegate.cb_disconnect(simplifio.delegate.userdata,devid);
    }
  }
  
  static void simplifio_cb_mshid_button(struct mshid *mshid,struct mshid_device *device,int btnid,int value) {
    int devid=mshid_device_get_devid(device);
    if (simplifio.delegate.cb_button) {
      simplifio.delegate.cb_button(simplifio.delegate.userdata,devid,btnid,value);
    }
  }
#endif

/* drmfb, alone among our video drivers, takes some more involved setup.
 * We only look for the mode that is already active.
 * Might be better to refine this, and select inactive modes that match (setup) exactly.
 */
#if USE_drmfb

struct simplifio_drmfb_init_context {
  struct drmfb_mode mode;
};

static int simplifio_drmfb_mode_cb(const struct drmfb_mode *mode,void *userdata) {
  struct simplifio_drmfb_init_context *ctx=userdata;
  if (!mode->already) return 0;
  ctx->mode=*mode;
  return 1;
}

static struct drmfb *simplifio_drmfb_new(const struct simplifio_setup *setup) {
  struct drmfb *drmfb=drmfb_new();
  if (!drmfb) return 0;
  struct simplifio_drmfb_init_context ctx={0};
  if (drmfb_for_each_mode(drmfb,simplifio_drmfb_mode_cb,&ctx)<=0) {
    drmfb_del(drmfb);
    return 0;
  }
  if (drmfb_set_mode(drmfb,&ctx.mode)<0) {
    drmfb_del(drmfb);
    return 0;
  }
  return drmfb;
}

#endif

/* Init video.
 */
 
static int simplifio_video_init(const struct simplifio_setup *setup) {

  #if USE_glx
    if (!setup->video_driver||!strcmp(setup->video_driver,"glx")) {
      struct glx_delegate delegate={
        .userdata=simplifio.delegate.userdata,
        .cb_close=simplifio.delegate.cb_close,
        .cb_focus=simplifio.delegate.cb_focus,
        .cb_resize=simplifio.delegate.cb_resize,
        .cb_key=simplifio.delegate.cb_key,
        .cb_text=simplifio.delegate.cb_text,
        .cb_mmotion=simplifio.delegate.cb_mmotion,
        .cb_mbutton=simplifio.delegate.cb_mbutton,
        .cb_mwheel=simplifio.delegate.cb_mwheel,
      };
      struct glx_setup dsetup={
        .title=setup->title,
        .iconrgba=setup->iconrgba,
        .iconw=setup->iconw,
        .iconh=setup->iconh,
        .w=setup->w,
        .h=setup->h,
        .fullscreen=setup->fullscreen,
        .fbw=setup->fbw,
        .fbh=setup->fbh,
        .device=setup->video_device,
      };
      if (simplifio.glx=glx_new(&delegate,&dsetup)) return 0;
    }
  #endif

  #if USE_drmgx
    if (!setup->video_driver||!strcmp(setup->video_driver,"drmgx")) {
      if (drmgx_init(setup->video_device)>=0) {
        simplifio.drmgx=1;
        return 0;
      }
    }
  #endif
  
  #if USE_x11fb
    if (!setup->video_driver||!strcmp(setup->video_driver,"x11fb")) {
      struct x11fb_delegate delegate={
        .userdata=simplifio.delegate.userdata,
        .cb_close=simplifio.delegate.cb_close,
        .cb_focus=simplifio.delegate.cb_focus,
        .cb_resize=simplifio.delegate.cb_resize,
        .cb_key=simplifio.delegate.cb_key,
        .cb_text=simplifio.delegate.cb_text,
        .cb_mmotion=simplifio.delegate.cb_mmotion,
        .cb_mbutton=simplifio.delegate.cb_mbutton,
        .cb_mwheel=simplifio.delegate.cb_mwheel,
      };
      struct x11fb_setup dsetup={
        .title=setup->title,
        .iconrgba=setup->iconrgba,
        .iconw=setup->iconw,
        .iconh=setup->iconh,
        .w=setup->w,
        .h=setup->h,
        .fullscreen=setup->fullscreen,
        .fbw=setup->fbw,
        .fbh=setup->fbh,
        .device=setup->video_device,
      };
      if (simplifio.x11fb=x11fb_new(&delegate,&dsetup)) return 0;
    }
  #endif
  
  #if USE_drmfb
    if (!setup->video_driver||!strcmp(setup->video_driver,"drmfb")) {
      if (simplifio.drmfb=simplifio_drmfb_new(setup)) return 0;
    }
  #endif
  
  #if USE_bcm
    if (!setup->video_driver||!strcmp(setup->video_driver,"bcm")) {
      if (simplifio.bcm=bcm_new()) return 0;
    }
  #endif
  
  #if USE_macwm
    if (!setup->video_driver||!strcmp(setup->video_driver,"macwm")) {
      struct macwm_delegate delegate={
        .userdata=simplifio.delegate.userdata,
        .cb_close=simplifio.delegate.cb_close,
        .cb_focus=simplifio.delegate.cb_focus,
        .cb_resize=simplifio.delegate.cb_resize,
        .cb_key=simplifio.delegate.cb_key,
        .cb_text=simplifio.delegate.cb_text,
        .cb_mmotion=simplifio.delegate.cb_mmotion,
        .cb_mbutton=simplifio.delegate.cb_mbutton,
        .cb_mwheel=simplifio.delegate.cb_mwheel,
      };
      struct macwm_setup dsetup={
        .title=setup->title,
        .iconrgba=setup->iconrgba,
        .iconw=setup->iconw,
        .iconh=setup->iconh,
        .w=setup->w,
        .h=setup->h,
        .fullscreen=setup->fullscreen,
        .fbw=setup->fbw,
        .fbh=setup->fbh,
      };
      if (simplifio.macwm=macwm_new(&delegate,&dsetup)) return 0;
    }
  #endif
  
  #if USE_mswm
    if (!setup->video_driver||!strcmp(setup->video_driver,"mswm")) {
      struct mswm_delegate delegate={
        .userdata=simplifio.delegate.userdata,
        .cb_close=simplifio.delegate.cb_close,
        .cb_focus=simplifio.delegate.cb_focus,
        .cb_resize=simplifio.delegate.cb_resize,
        .cb_key=simplifio.delegate.cb_key,
        .cb_text=simplifio.delegate.cb_text,
        .cb_mmotion=simplifio.delegate.cb_mmotion,
        .cb_mbutton=simplifio.delegate.cb_mbutton,
        .cb_mwheel=simplifio.delegate.cb_mwheel,
      };
      struct mswm_setup setup={
        .title=setup->title,
        .iconrgba=setup->iconrgba,
        .iconw=setup->iconw,
        .iconh=setup->iconh,
        .w=setup->w,
        .h=setup->h,
        .fullscreen=setup->fullscreen,
        .fbw=setup->fbw,
        .fbh=setup->fbh,
      };
      if (simplifio.mswm=mswm_new(&delegate,&dsetup)) return 0;
    }
  #endif
  
  return -1;
}

/* Init audio.
 */
 
static int simplifio_audio_init(const struct simplifio_setup *setup) {

  #if USE_pulse
    if (!setup->audio_driver||!strcmp(setup->audio_driver,"pulse")) {
      struct pulse_delegate delegate={
        .userdata=simplifio.delegate.userdata,
        .pcm_out=simplifio.delegate.cb_pcm_out,
      };
      struct pulse_setup dsetup={
        .rate=setup->audio_rate,
        .chanc=setup->audio_chanc,
        .buffersize=setup->audio_buffer_size,
        .appname=setup->title,
        .servername=setup->audio_device,
      };
      if (!dsetup.rate) dsetup.rate=44100;
      if (!dsetup.chanc) dsetup.chanc=2;
      if (simplifio.pulse=pulse_new(&delegate,&dsetup)) return 0;
    }
  #endif

  #if USE_alsafd
    if (!setup->audio_driver||!strcmp(setup->audio_driver,"alsafd")) {
      struct alsafd_delegate delegate={
        .userdata=simplifio.delegate.userdata,
        .pcm_out=simplifio.delegate.cb_pcm_out,
      };
      struct alsafd_setup dsetup={
        .rate=setup->audio_rate,
        .chanc=setup->audio_chanc,
        .buffersize=setup->audio_buffer_size,
        .device=setup->audio_device,
      };
      if (!dsetup.rate) dsetup.rate=44100;
      if (!dsetup.chanc) dsetup.chanc=2;
      if (simplifio.alsafd=alsafd_new(&delegate,&dsetup)) return 0;
    }
  #endif

  #if USE_asound
    if (!setup->audio_driver||!strcmp(setup->audio_driver,"asound")) {
      struct asound_delegate delegate={
        .userdata=simplifio.delegate.userdata,
        .cb_pcm_out=simplifio.delegate.cb_pcm_out,
      };
      struct asound_setup dsetup={
        .rate=setup->audio_rate,
        .chanc=setup->audio_chanc,
        .buffer_size=setup->audio_buffer_size,
        .device=setup->audio_device,
      };
      if (!dsetup.rate) dsetup.rate=44100;
      if (!dsetup.chanc) dsetup.chanc=2;
      if (simplifio.asound=asound_new(&delegate,&dsetup)) return 0;
    }
  #endif

  #if USE_macaudio
    if (!setup->audio_driver||!strcmp(setup->audio_driver,"macaudio")) {
      struct macaudio_delegate delegate={
        .userdata=simplifio.delegate.userdata,
        .cb_pcm_out=simplifio.delegate.cb_pcm_out,
      };
      struct macaudio_setup dsetup={
        .rate=setup->audio_rate,
        .chanc=setup->audio_chanc,
        .buffer_size=setup->audio_buffer_size,
      };
      if (!dsetup.rate) dsetup.rate=44100;
      if (!dsetup.chanc) dsetup.chanc=2;
      if (simplifio.macaudio=macaudio_new(&delegate,&dsetup)) return 0;
    }
  #endif

  #if USE_msaudio
    if (!setup->audio_driver||!strcmp(setup->audio_driver,"msaudio")) {
      struct msaudio_delegate delegate={
        .userdata=simplifio.delegate.userdata,
        .cb_pcm_out=simplifio.delegate.cb_pcm_out,
      };
      struct msaudio_setup dsetup={
        .rate=setup->audio_rate,
        .chanc=setup->audio_chanc,
        .buffer_size=setup->audio_buffer_size,
      };
      if (!dsetup.rate) dsetup.rate=44100;
      if (!dsetup.chanc) dsetup.chanc=2;
      if (simplifio.msaudio=msaudio_new(&delegate,&dsetup)) return 0;
    }
  #endif

  return -1;
}

/* Init input.
 */
 
static int simplifio_input_init(const struct simplifio_setup *setup) {

  #if USE_evdev
    if (!setup->input_driver||!strcmp(setup->input_driver,"evdev")) {
      struct evdev_delegate delegate={
        .userdata=simplifio.delegate.userdata,
        .cb_connect=simplifio_cb_evdev_connect,
        .cb_disconnect=simplifio_cb_evdev_disconnect,
        .cb_button=simplifio_cb_evdev_button,
      };
      if (simplifio.evdev=evdev_new(setup->input_path,&delegate)) return 0;
    }
  #endif

  #if USE_machid
    if (!setup->input_driver||!strcmp(setup->input_driver,"machid")) {
      struct machid_delegate delegate={
        .userdata=simplifio.delegate.userdata,
        .cb_connect=simplifio_cb_machid_connect,
        .cb_disconnect=simplifio_cb_machid_disconnect,
        .cb_button=simplifio_cb_machid_button,
      };
      if (simplifio.machid=machid_new(&delegate)) return 0;
    }
  #endif

  #if USE_mshid
    if (!setup->input_driver||!strcmp(setup->input_driver,"mshid")) {
      struct mshid_delegate delegate={
        .userdata=simplifio.delegate.userdata,
        .cb_connect=simplifio_cb_mshid_connect,
        .cb_disconnect=simplifio_cb_mshid_disconnect,
        .cb_button=simplifio_cb_mshid_button,
      };
      if (simplifio.mshid=mshid_new(&delegate)) return 0;
    }
  #endif

  return -1;
}

/* Init.
 */

int simplifio_init(
  const struct simplifio_delegate *delegate,
  const struct simplifio_setup *setup
) {
  simplifio.delegate=*delegate;
  simplifio.input_devid=1;
  if (
    (simplifio_video_init(setup)<0)||
    (simplifio_audio_init(setup)<0)||
    (simplifio_input_init(setup)<0)||
  0) {
    simplifio_quit();
    return -1;
  }
  return 0;
}

/* Update.
 */
 
int simplifio_update() {

  #if USE_pulse
    if (simplifio.pulse&&(pulse_update(simplifio.pulse)<0)) return -1;
  #endif
  #if USE_alsafd
    if (simplifio.alsafd&&(alsafd_update(simplifio.alsafd)<0)) return -1;
  #endif
  #if USE_macaudio
    if (simplifio.macaudio&&(macaudio_update(simplifio.macaudio)<0)) return -1;
  #endif
  #if USE_msaudio
    if (simplifio.msaudio&&(msaudio_update(simplifio.msaudio)<0)) return -1;
  #endif
  // No update hook: asound
  
  #if USE_evdev
    if (simplifio.evdev&&(evdev_update(simplifio.evdev)<0)) return -1;
  #endif
  #if USE_machid
    if (simplifio.machid&&(machid_update(simplifio.machid)<0)) return -1;
  #endif
  #if USE_mshid
    if (simplifio.mshid&&(mshid_update(simplifio.mshid)<0)) return -1;
  #endif
  
  #if USE_glx
    if (simplifio.glx&&(glx_update(simplifio.glx)<0)) return -1;
  #endif
  #if USE_x11fb
    if (simplifio.x11fb&&(x11fb_update(simplifio.x11fb)<0)) return -1;
  #endif
  #if USE_macwm
    if (simplifio.macwm&&(macwm_update(simplifio.macwm)<0)) return -1;
  #endif
  #if USE_mswm
    if (simplifio.mswm&&(mswm_update(simplifio.mswm)<0)) return -1;
  #endif
  // No update hook: drmgx drmfb bcm
  
  return 0;
}
