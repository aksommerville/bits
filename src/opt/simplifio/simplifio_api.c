#include "simplifio_internal.h"

const char *simplifio_audio_get_driver_name() {
  #if USE_pulse
    if (simplifio.pulse) return "pulse";
  #endif
  #if USE_alsafd
    if (simplifio.alsafd) return "alsafd";
  #endif
  #if USE_asound
    if (simplifio.asound) return "asound";
  #endif
  #if USE_macaudio
    if (simplifio.macaudio) return "macaudio";
  #endif
  #if USE_msaudio
    if (simplifio.msaudio) return "msaudio";
  #endif
  return 0;
}

int simplifio_audio_get_rate() {
  #if USE_pulse
    if (simplifio.pulse) return pulse_get_rate(simplifio.pulse);
  #endif
  #if USE_alsafd
    if (simplifio.alsafd) return alsafd_get_rate(simplifio.alsafd);
  #endif
  #if USE_asound
    if (simplifio.asound) return asound_get_rate(simplifio.asound);
  #endif
  #if USE_macaudio
    if (simplifio.macaudio) return macaudio_get_rate(simplifio.macaudio);
  #endif
  #if USE_msaudio
    if (simplifio.msaudio) return msaudio_get_rate(simplifio.msaudio);
  #endif
  return 0;
}

int simplifio_audio_get_chanc() {
  #if USE_pulse
    if (simplifio.pulse) return pulse_get_chanc(simplifio.pulse);
  #endif
  #if USE_alsafd
    if (simplifio.alsafd) return alsafd_get_chanc(simplifio.alsafd);
  #endif
  #if USE_asound
    if (simplifio.asound) return asound_get_chanc(simplifio.asound);
  #endif
  #if USE_macaudio
    if (simplifio.macaudio) return macaudio_get_chanc(simplifio.macaudio);
  #endif
  #if USE_msaudio
    if (simplifio.msaudio) return msaudio_get_chanc(simplifio.msaudio);
  #endif
  return 0;
}

void simplifio_audio_play(int play) {
  #if USE_pulse
    if (simplifio.pulse) pulse_set_running(simplifio.pulse,play);
  #endif
  #if USE_alsafd
    if (simplifio.alsafd) alsafd_set_running(simplifio.alsafd,play);
  #endif
  #if USE_asound
    if (simplifio.asound) asound_play(simplifio.asound,play);
  #endif
  #if USE_macaudio
    if (simplifio.macaudio) macaudio_play(simplifio.macaudio,play);
  #endif
  #if USE_msaudio
    if (simplifio.msaudio) msaudio_play(simplifio.msaudio,play);
  #endif
}

int simplifio_audio_lock() {
  #if USE_pulse
    if (simplifio.pulse) return pulse_lock(simplifio.pulse);
  #endif
  #if USE_alsafd
    if (simplifio.alsafd) return alsafd_lock(simplifio.alsafd);
  #endif
  #if USE_asound
    if (simplifio.asound) return asound_lock(simplifio.asound);
  #endif
  #if USE_macaudio
    if (simplifio.macaudio) return macaudio_lock(simplifio.macaudio);
  #endif
  #if USE_msaudio
    if (simplifio.msaudio) return msaudio_lock(simplifio.msaudio);
  #endif
  return -1;
}

void simplifio_audio_unlock() {
  #if USE_pulse
    if (simplifio.pulse) pulse_unlock(simplifio.pulse);
  #endif
  #if USE_alsafd
    if (simplifio.alsafd) alsafd_unlock(simplifio.alsafd);
  #endif
  #if USE_asound
    if (simplifio.asound) asound_unlock(simplifio.asound);
  #endif
  #if USE_macaudio
    if (simplifio.macaudio) macaudio_unlock(simplifio.macaudio);
  #endif
  #if USE_msaudio
    if (simplifio.msaudio) msaudio_unlock(simplifio.msaudio);
  #endif
}

const char *simplifio_video_get_driver_name() {
  #if USE_glx
    if (simplifio.glx) return "glx";
  #endif
  #if USE_drmgx
    if (simplifio.drmgx) return "drmgx";
  #endif
  #if USE_x11fb
    if (simplifio.x11fb) return "x11fb";
  #endif
  #if USE_drmfb
    if (simplifio.drmfb) return "drmfb";
  #endif
  #if USE_bcm
    if (simplifio.bcm) return "bcm";
  #endif
  #if USE_macwm
    if (simplifio.macwm) return "macwm";
  #endif
  #if USE_mswm
    if (simplifio.mswm) return "mswm";
  #endif
  return 0;
}

void simplifio_video_get_size(int *w,int *h) {
  #if USE_glx
    if (simplifio.glx) { glx_get_size(w,h,simplifio.glx); return; }
  #endif
  #if USE_drmgx
    if (simplifio.drmgx) { drmgx_get_size(w,h); return; }
  #endif
  #if USE_x11fb
    if (simplifio.x11fb) { x11fb_get_size(w,h,simplifio.x11fb); return; }
  #endif
  #if USE_drmfb
    if (simplifio.drmfb) {
      const struct drmfb_mode *mode=drmfb_get_mode(simplifio.drmfb);
      if (mode) {
        *w=mode->w;
        *h=mode->h;
      }
      return;
    }
  #endif
  #if USE_bcm
    if (simplifio.bcm) { *w=bcm_get_width(); *h=bcm_get_height(); return; }
  #endif
  #if USE_macwm
    if (simplifio.macwm) { macwm_get_size(w,h,simplifio.macwm); return; }
  #endif
  #if USE_mswm
    if (simplifio.mswm) { mswm_get_size(w,h,simplifio.mswm); return; }
  #endif
}

int simplifio_video_provides_input() {
  #if USE_glx
    if (simplifio.glx) return 1;
  #endif
  #if USE_x11fb
    if (simplifio.x11fb) return 1;
  #endif
  #if USE_macwm
    if (simplifio.macwm) return 1;
  #endif
  #if USE_mswm
    if (simplifio.mswm) return 1;
  #endif
  return 0;
}

int simplifio_video_get_fullscreen() {
  #if USE_glx
    if (simplifio.glx) return glx_get_fullscreen(simplifio.glx);
  #endif
  #if USE_drmgx
    if (simplifio.drmgx) return 1;
  #endif
  #if USE_x11fb
    if (simplifio.x11fb) return x11fb_get_fullscreen(simplifio.x11fb);
  #endif
  #if USE_drmfb
    if (simplifio.drmfb) return 1;
  #endif
  #if USE_bcm
    if (simplifio.bcm) return 1;
  #endif
  #if USE_macwm
    if (simplifio.macwm) return macwm_get_fullscreen(simplifio.macwm);
  #endif
  #if USE_mswm
    if (simplifio.mswm) return mswm_get_fullscreen(simplifio.mswm);
  #endif
  return 0;
}

void simplifio_video_show_cursor(int show) {
  #if USE_glx
    if (simplifio.glx) { glx_show_cursor(simplifio.glx,show); return; }
  #endif
  #if USE_x11fb
    if (simplifio.x11fb) { x11fb_show_cursor(simplifio.x11fb,show); return; }
  #endif
  #if USE_macwm
    if (simplifio.macwm) { macwm_show_cursor(simplifio.macwm,show); return; }
  #endif
  #if USE_mswm
    if (simplifio.mswm) { mswm_show_cursor(simplifio.mswm,show); return; }
  #endif
}

void simplifio_video_set_fullscreen(int fullscreen) {
  #if USE_glx
    if (simplifio.glx) { glx_set_fullscreen(simplifio.glx,fullscreen); return; }
  #endif
  #if USE_x11fb
    if (simplifio.x11fb) { x11fb_set_fullscreen(simplifio.x11fb,fullscreen); return; }
  #endif
  #if USE_macwm
    if (simplifio.macwm) { macwm_set_fullscreen(simplifio.macwm,fullscreen); return; }
  #endif
  #if USE_mswm
    if (simplifio.mswm) { mswm_set_fullscreen(simplifio.mswm,fullscreen); return; }
  #endif
}

#if USE_drmfb
  #include <drm_fourcc.h>
  
  static int simplifio_fb_description_from_drm_pixel_format(
    struct simplifio_fb_description *desc,
    uint32_t format
  ) {
    switch (format) {
      // We ask for 32-bit pixels, and I think it's unlikely that we'll get anything other than 8-bit channels.
      // So not bothering with the hundreds of other possible formats.
      #define xmask pixelsize /* so "x" channels get harmlessly ignored */
      #define BYTEWISERGBA(drmtag,c0,c1,c2,c3) case DRM_FORMAT_##drmtag: { \
        desc->chorder[0]=(#c0)[0]; \
        desc->chorder[1]=(#c1)[0]; \
        desc->chorder[2]=(#c2)[0]; \
        desc->chorder[3]=(#c3)[0]; \
        desc->c0##mask=0x000000ff; \
        desc->c1##mask=0x0000ff00; \
        desc->c2##mask=0x00ff0000; \
        desc->c3##mask=0xff000000; \
        desc->pixelsize=32; \
        desc->stride=desc->w<<2; \
      } break;
      BYTEWISERGBA(XRGB8888,b,g,r,x)
      BYTEWISERGBA(XBGR8888,r,g,b,x)
      BYTEWISERGBA(RGBX8888,x,b,g,r)
      BYTEWISERGBA(BGRX8888,x,r,g,b)
      BYTEWISERGBA(ARGB8888,b,g,r,a)
      BYTEWISERGBA(ABGR8888,r,g,b,a)
      BYTEWISERGBA(RGBA8888,a,b,g,r)
      BYTEWISERGBA(BGRA8888,a,r,g,b)
      #undef BYTEWISERGBA
      #undef xmask
      default: return -1;
    }
    return 0;
  }
#endif

int simplifio_video_describe_fb(struct simplifio_fb_description *desc) {
  #if USE_x11fb
    if (simplifio.x11fb) {
      x11fb_get_framebuffer_geometry(&desc->w,&desc->h,&desc->stride,&desc->pixelsize,simplifio.x11fb);
      x11fb_get_framebuffer_masks(&desc->rmask,&desc->gmask,&desc->bmask,&desc->amask,simplifio.x11fb);
      x11fb_get_framebuffer_chorder(desc->chorder,simplifio.x11fb);
      return 0;
    }
  #endif
  #if USE_drmfb
    if (simplifio.drmfb) {
      const struct drmfb_mode *mode=drmfb_get_mode(simplifio.drmfb);
      if (!mode) return -1;
      desc->w=mode->w;
      desc->h=mode->h;
      uint32_t format=drmfb_get_pixel_format(simplifio.drmfb);
      return simplifio_fb_description_from_drm_pixel_format(desc,format);
    }
  #endif
  #if USE_macwm
    if (simplifio.macwm) return -1;//TODO
  #endif
  #if USE_mswm
    if (simplifio.mswm) return -1;//TODO
  #endif
  return -1;
}

void *simplifio_video_begin_fb() {
  #if USE_x11fb
    if (simplifio.x11fb) return x11fb_begin(simplifio.x11fb);
  #endif
  #if USE_drmfb
    if (simplifio.drmfb) return drmfb_begin(simplifio.drmfb);
  #endif
  #if USE_macwm
    if (simplifio.macwm) return macwm_fb_begin(simplifio.macwm);
  #endif
  #if USE_mswm
    if (simplifio.mswm) return mswm_fb_begin(simplifio.mswm);
  #endif
  return 0;
}

int simplifio_video_end_fb() {
  #if USE_x11fb
    if (simplifio.x11fb) return x11fb_end(simplifio.x11fb);
  #endif
  #if USE_drmfb
    if (simplifio.drmfb) return drmfb_end(simplifio.drmfb);
  #endif
  #if USE_macwm
    if (simplifio.macwm) return macwm_fb_end(simplifio.macwm);
  #endif
  #if USE_mswm
    if (simplifio.mswm) return mswm_fb_end(simplifio.mswm);
  #endif
  return -1;
}

int simplifio_video_supports_gx() {
  #if USE_glx
    if (simplifio.glx) return 1;
  #endif
  #if USE_drmgx
    if (simplifio.drmgx) return 1;
  #endif
  #if USE_x11fb
    if (simplifio.x11fb) return 0;
  #endif
  #if USE_drmfb
    if (simplifio.drmfb) return 0;
  #endif
  #if USE_bcm
    if (simplifio.bcm) return 1;
  #endif
  #if USE_macwm
    if (simplifio.macwm) return 1;
  #endif
  #if USE_mswm
    if (simplifio.mswm) return 1;
  #endif
  return 0;
}

int simplifio_video_begin_gx() {
  #if USE_glx
    if (simplifio.glx) return glx_begin(simplifio.glx);
  #endif
  #if USE_drmgx
    if (simplifio.drmgx) return 0;
  #endif
  #if USE_bcm
    if (simplifio.bcm) return 0;
  #endif
  #if USE_macwm
    if (simplifio.macwm) return macwm_gx_begin(simplifio.macwm);
  #endif
  #if USE_mswm
    if (simplifio.mswm) return mswm_gx_begin(simplifio.mswm);
  #endif
  return -1;
}

int simplifio_video_end_gx() {
  #if USE_glx
    if (simplifio.glx) return glx_end(simplifio.glx);
  #endif
  #if USE_drmgx
    if (simplifio.drmgx) return drmgx_swap();
  #endif
  #if USE_bcm
    if (simplifio.bcm) return bcm_swap();
  #endif
  #if USE_macwm
    if (simplifio.macwm) return macwm_gx_end(simplifio.macwm);
  #endif
  #if USE_mswm
    if (simplifio.mswm) return mswm_gx_end(simplifio.mswm);
  #endif
  return -1;
}

const char *simplifio_input_get_driver_name() {
  #if USE_evdev
    if (simplifio.evdev) return "evdev";
  #endif
  #if USE_machid
    if (simplifio.machid) return "machid";
  #endif
  #if USE_mshid
    if (simplifio.mshid) return "mshid";
  #endif
  return 0;
}

void simplifio_input_disconnect(int devid) {
  #if USE_evdev
    if (simplifio.evdev) {
      evdev_device_disconnect(simplifio.evdev,evdev_device_by_devid(simplifio.evdev,devid));
      return;
    }
  #endif
  #if USE_machid
    if (simplifio.machid) {
      machid_device_disconnect(simplifio.machid,devid);
      return;
    }
  #endif
  #if USE_mshid
    if (simplifio.mshid) {
      mshid_device_disconnectc(simplifio.mshid,devid);
      return;
    }
  #endif
}

const char *simplifio_input_get_ids(int *vid,int *pid,int *version,int devid) {
  #if USE_evdev
    if (simplifio.evdev) {
      struct evdev_device *device=evdev_device_by_devid(simplifio.evdev,devid);
      if (!device) return 0;
      return evdev_device_get_ids(vid,pid,version,device);
    }
  #endif
  #if USE_machid
    if (simplifio.machid) return machid_device_get_ids(vid,pid,version,simplifio.machid,devid);
  #endif
  #if USE_mshid
    if (simplifio.mshid) return mshid_device_get_ids(vid,pid,version,simplifio.mshid,devid);
  #endif
  return 0;
}

struct simplifio_for_each_button_context {
  int (*cb)(int btnid,int hidusage,int lo,int hi,int value,void *userdata);
  void *userdata;
};

#if USE_evdev
  static int simplifio_for_each_button_evdev(int type,int code,int hidusage,int lo,int hi,int value,void *userdata) {
    struct simplifio_for_each_button_context *ctx=userdata;
    return ctx->cb((type<<16)|code,hidusage,lo,hi,value,ctx->userdata);
  }
#endif

int simplifio_input_for_each_button(
  int devid,
  int (*cb)(int btnid,int hidusage,int lo,int hi,int value,void *userdata),
  void *userdata
) {
  struct simplifio_for_each_button_context ctx={.cb=cb,.userdata=userdata};
  #if USE_evdev
    if (simplifio.evdev) {
      struct evdev_device *device=evdev_device_by_devid(simplifio.evdev,devid);
      if (!device) return 0;
      return evdev_device_for_each_button(device,simplifio_for_each_button_evdev,&ctx);
    }
  #endif
  #if USE_machid
    if (simplifio.machid) return machid_for_each_button(simplifio.machid,devid,cb,userdata);
  #endif
  #if USE_mshid
    if (simplifio.mshid) return mshid_for_each_button(simplifio.mshid,devid,cb,userdata);
  #endif
  return 0;
}
