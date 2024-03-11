/* hostio_live.c
 * To run: make test-hostio_live
 * Stands a hostio context with video, audio, and input drivers.
 * Dump some test content to screen and speaker, and log all events.
 * Intended for troubleshooting drivers.
 */

#include "test/test.h"
#include "opt/hostio/hostio.h"
#include "opt/fs/fs.h"
#include "opt/rawimg/rawimg.h"
#include <signal.h>
#include <unistd.h>
#include <limits.h>

/* Test configuration.
 */

// 0 for defaults, or a string of comma-delimited driver names.
#define HOSTIO_LIVE_VIDEO_DRIVERS 0
#define HOSTIO_LIVE_AUDIO_DRIVERS 0
#define HOSTIO_LIVE_INPUT_DRIVERS 0

// Window or screen size at init. Recommend 0,0 to let the driver decide.
#define HOSTIO_LIVE_WINDOW_W 0
#define HOSTIO_LIVE_WINDOW_H 0

/* I didn't implement a backup frame clock, so you have to set this if the video driver doesn't block on vsync.
 * (drivers typically do that, but they're not required to).
 * 16666, if you see the demo running too fast.
 */
#define HOSTIO_LIVE_FRAME_SLEEP_US 16666

// Frequency in Hertz, level in 0..32767. Zero either to disable.
// Press 's' to play a sound if enabled.
#define TONE_FREQ 300
#define AUDIO_LEVEL 5000

// If the driver supports framebuffers we use it.
// But for those that don't, we do a cheap OpenGL cudgel right here too. Optionally.
#define ENABLE_OPENGL 1
 
/* Globals.
 */

static volatile int sigc=0;
static int closed_window=0;
static struct hostio_video_fb_description fbdesc={0};
static int vframec=0;
static struct rawimg *spritebits=0;
static int spritex=100,spritey=40,spritedx=1,spritedy=1;
static int16_t audio_level=0;

/* Signals.
 */
 
static void cb_signal(int sigid) {
  switch (sigid) {
    case SIGINT: if (++sigc>=3) {
        fprintf(stderr,"Too many unprocessed signals.\n");
        exit(1);
      } break;
  }
}

/* Generate some audio.
 */
 
static int audio_half_period=0;
static int audio_remaining=0;
static int audio_periods_remaining=0;

static void synthesize_audio(int16_t *v,int c) { // mono only, and audio_half_period must be positive
  while (c>0) {
    int cpc=c;
    if (cpc>audio_remaining) cpc=audio_remaining;
    c-=cpc;
    audio_remaining-=cpc;
    for (;cpc-->0;v++) *v=audio_level;
    if (audio_remaining<=0) {
      audio_remaining=audio_half_period;
      audio_level=-audio_level;
      if (--audio_periods_remaining<=0) {
        audio_level=0;
        audio_periods_remaining=0;
        audio_remaining=INT_MAX;
      }
    }
  }
}

static void expand_audio(int16_t *v,int framec,int chanc) {
  int16_t *dstp=v+framec*chanc;
  const int16_t *srcp=v+framec;
  while (framec-->0) {
    srcp--;
    int chi=chanc;
    while (chi-->0) {
      dstp--;
      *dstp=*srcp;
    }
  }
}

static void cb_pcm_out(int16_t *v,int c,struct hostio_audio *driver) {
  if (audio_half_period<1) {
    memset(v,0,c<<1);
  } else if (driver->chanc>1) {
    int framec=c/driver->chanc;
    synthesize_audio(v,framec);
    expand_audio(v,framec,driver->chanc);
  } else {
    synthesize_audio(v,c);
  }
}

static void play_sound() {
  if (!AUDIO_LEVEL||!TONE_FREQ) return;
  if (audio_half_period<1) return;
  audio_remaining=audio_half_period;
  audio_periods_remaining=44100/audio_half_period;
  audio_level=AUDIO_LEVEL;
}

/* Driver callbacks.
 */
 
static void cb_close(struct hostio_video *driver) {
  fprintf(stderr,"%s\n",__func__);
  closed_window=1;
}

static void cb_focus(struct hostio_video *driver,int focus) {
  fprintf(stderr,"%s %d\n",__func__,focus);
}

static void cb_resize(struct hostio_video *driver,int w,int h) {
  fprintf(stderr,"%s %d,%d\n",__func__,w,h);
}

static int cb_key(struct hostio_video *driver,int keycode,int value) {
  fprintf(stderr,"%s %08x=%d\n",__func__,keycode,value);
  return 0; // or 1 to acknowledge, and prevent cb_text
}

static void cb_text(struct hostio_video *driver,int codepoint) {
  struct hostio *hostio=driver->delegate.userdata;
  fprintf(stderr,"%s U+%x\n",__func__,codepoint);
  switch (codepoint) {
    case 0x1b: closed_window=1; break; // ESC is equivalent to closing the window, for us.
    case 'f': hostio_toggle_fullscreen(hostio); break;
    case 's': play_sound(); break;
  }
}

static void cb_mmotion(struct hostio_video *driver,int x,int y) {
  fprintf(stderr,"%s %d,%d\n",__func__,x,y);
}

static void cb_mbutton(struct hostio_video *driver,int btnid,int value) {
  fprintf(stderr,"%s %d=%d\n",__func__,btnid,value);
}

static void cb_mwheel(struct hostio_video *driver,int dx,int dy) {
  fprintf(stderr,"%s %+d,%+d\n",__func__,dx,dy);
}

static void cb_disconnect(struct hostio_input *driver,int devid) {
  fprintf(stderr,"%s %d\n",__func__,devid);
}

static void cb_button(struct hostio_input *driver,int devid,int btnid,int value) {
  fprintf(stderr,"%s %d.%08x=%d\n",__func__,devid,btnid,value);
  if (value==1) play_sound();
}

/* Callback: Input device connected.
 */
 
static int cb_declare_button(int btnid,int hidusage,int lo,int hi,int value,void *userdata) {
  fprintf(stderr,"  %08x usage=%08x range=%d..%d value=%d\n",btnid,hidusage,lo,hi,value);
  return 0;
}

static void cb_connect(struct hostio_input *driver,int devid) {
  if (driver->type->get_ids) {
    int vid=0,pid=0,version=0;
    const char *name=driver->type->get_ids(&vid,&pid,&version,driver,devid);
    if (name) {
      fprintf(stderr,"%s: %s:%d: '%s' %04x:%04x:%04x\n",__func__,driver->type->name,devid,name,vid,pid,version);
    } else {
      fprintf(stderr,"!!! Input driver '%s' failed to provide IDs for device %d.\n",driver->type->name,devid);
    }
  } else {
    fprintf(stderr,"%s: %s:%d: Driver does not supply vid/pid/version/name. Is that a bug?\n",__func__,driver->type->name,devid);
  }
  if (driver->type->for_each_button) {
    if (driver->type->for_each_button(driver,devid,cb_declare_button,0)<0) {
      fprintf(stderr,"%s: %s:%d: Error listing buttons.\n",__func__,driver->type->name,devid);
    }
  } else {
    fprintf(stderr,"%s: %s:%d: Driver does not supply button declarations. Is that a bug?\n",__func__,driver->type->name,devid);
  }
}

/* OpenGL 1.x cudgel to provide a fake framebuffer.
 */
#if ENABLE_OPENGL

#if USE_macos
  #include <OpenGL/gl.h>
#else
  #define GL_GLEXT_PROTOTYPES 1
  #include <GL/gl.h>
  #include <GL/glext.h>
#endif

static GLuint texid=0;

#define GLFB_W 256
#define GLFB_H 192
static uint8_t glfb[GLFB_W*GLFB_H*4];
 
static int opengl_init() {
  glGenTextures(1,&texid);
  ASSERT(texid,"glGenTextures")
  glBindTexture(GL_TEXTURE_2D,texid);
  glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP_TO_EDGE);
  
  fbdesc.w=GLFB_W;
  fbdesc.h=GLFB_H;
  fbdesc.stride=GLFB_W<<2;
  fbdesc.pixelsize=32;
  memcpy(fbdesc.chorder,"rgbx",4);
  uint32_t bodetect=0x04030201;
  if (*(uint8_t*)&bodetect==0x04) {
    fbdesc.rmask=0xff000000;
    fbdesc.gmask=0x00ff0000;
    fbdesc.bmask=0x0000ff00;
    fbdesc.amask=0x000000ff;
  } else {
    fbdesc.rmask=0x000000ff;
    fbdesc.gmask=0x0000ff00;
    fbdesc.bmask=0x00ff0000;
    fbdesc.amask=0xff000000;
  }
  return 0;
}

static void opengl_commit(struct hostio_video *video) {
  glViewport(0,0,video->w,video->h);
  glClearColor(0.0f,0.0f,0.0f,1.0f);
  glClear(GL_COLOR_BUFFER_BIT);
  glBindTexture(GL_TEXTURE_2D,texid);
  glTexImage2D(GL_TEXTURE_2D,0,GL_RGB,GLFB_W,GLFB_H,0,GL_RGBA,GL_UNSIGNED_BYTE,glfb);
  glEnable(GL_TEXTURE_2D);
  glDisable(GL_BLEND);
  int dstw=(video->h*fbdesc.w)/fbdesc.h,dsth;
  if (dstw<=video->w) { // pillarbox
    dsth=video->h;
  } else { // letterbox
    dstw=video->w;
    dsth=(video->w*fbdesc.h)/fbdesc.w;
  }
  GLfloat normw=(GLfloat)dstw/(GLfloat)video->w;
  GLfloat normh=(GLfloat)dsth/(GLfloat)video->h;
  glBegin(GL_TRIANGLE_STRIP);
    glColor4ub(0xff,0xff,0xff,0xff);
    glTexCoord2i(0,0); glVertex2f(-normw, normh);
    glTexCoord2i(0,1); glVertex2f(-normw,-normh);
    glTexCoord2i(1,0); glVertex2f( normw, normh);
    glTexCoord2i(1,1); glVertex2f( normw,-normh);
  glEnd();
}

#endif

/* Generate a frame of video.
 */
 
static void fill_rect(uint32_t *fb,int x,int y,int w,int h,int pixel) {
  if (x<0) { w+=x; x=0; }
  if (y<0) { h+=y; y=0; }
  if (x>fbdesc.w-w) w=fbdesc.w-x;
  if (y>fbdesc.h-h) h=fbdesc.h-y;
  if ((w<1)||(h<1)) return;
  int stridewords=fbdesc.stride>>2;
  fb+=y*stridewords+x;
  for (;h-->0;fb+=stridewords) {
    uint32_t *p=fb;
    int xi=w;
    for (;xi-->0;p++) *p=pixel;
  }
}

static void blit(uint32_t *fb,int dstx,int dsty,const struct rawimg *src) {
  int srcx=0,srcy=0,w=src->w,h=src->h;
  if (dstx<0) { srcx-=dstx; w+=dstx; dstx=0; }
  if (dsty<0) { srcy-=dsty; h+=dsty; dsty=0; }
  if (dstx>fbdesc.w-w) w=fbdesc.w-dstx;
  if (dsty>fbdesc.h-h) h=fbdesc.h-dsty;
  if ((w<1)||(h<1)) return;
  int srcstridewords=src->stride>>2;
  int dststridewords=fbdesc.stride>>2;
  const uint32_t *srcrow=((uint32_t*)src->v)+srcy*srcstridewords+srcx;
  uint32_t *dstrow=fb+dsty*dststridewords+dstx;
  int yi=h;
  for (;yi-->0;srcrow+=srcstridewords,dstrow+=dststridewords) {
    const uint32_t *srcp=srcrow;
    uint32_t *dstp=dstrow;
    int xi=w;
    for (;xi-->0;srcp++,dstp++) {
      if (!((*srcp)&src->amask)) continue;
      *dstp=*srcp;
    }
  }
}
 
static int render_scene(struct hostio_video *video) {
  ASSERT_INTS_OP(fbdesc.w,>=,256)
  ASSERT_INTS_OP(fbdesc.h,>=,192)
  ASSERT_INTS(fbdesc.pixelsize,32)
  ASSERT_NOT(fbdesc.stride&3)
  uint32_t *fb=0;
  #if ENABLE_OPENGL
    if (texid) {
      ASSERT_CALL(video->type->gx_begin(video))
      fb=(uint32_t*)glfb;
    } else fb=video->type->fb_begin(video);
  #else
    fb=video->type->fb_begin(video);
  #endif
  ASSERT(fb)
  
  // Fill with dark gray. Not black; we want to see where the framebuffer ends and platform's margin begins.
  fill_rect(fb,0,0,fbdesc.w,fbdesc.h,0x40404040|fbdesc.amask);
  
  /* Draw three squares, from left to right: Red, Green, Blue.
   * Notable here is we're using grays and primaries, so we don't need any complicated pixel format adjustment.
   */
  int spritew=50;
  int halfw=spritew>>1;
  fill_rect(fb,(1*fbdesc.w)/4-halfw,(fbdesc.h>>1)-halfw,spritew,spritew,fbdesc.rmask|fbdesc.amask);
  fill_rect(fb,(2*fbdesc.w)/4-halfw,(fbdesc.h>>1)-halfw,spritew,spritew,fbdesc.gmask|fbdesc.amask);
  fill_rect(fb,(3*fbdesc.w)/4-halfw,(fbdesc.h>>1)-halfw,spritew,spritew,fbdesc.bmask|fbdesc.amask);
  
  /* Draw a sprite, if we got it.
   * Also update it here. Bad practice, but hey we're not a game-architecture demo.
   */
  if (spritebits) {
    spritex+=spritedx;
    spritey+=spritedy;
    if (((spritex<0)&&(spritedx<0))||((spritex+spritebits->w>fbdesc.w)&&(spritedx>0))) spritedx=-spritedx;
    if (((spritey<0)&&(spritedy<0))||((spritey+spritebits->h>fbdesc.h)&&(spritedy>0))) spritedy=-spritedy;
    blit(fb,spritex,spritey,spritebits);
  }
  
  #if ENABLE_OPENGL
    if (texid) {
      opengl_commit(video);
      ASSERT_CALL(video->type->gx_end(video))
    } else {
      ASSERT_CALL(video->type->fb_end(video))
    }
  #else
    ASSERT_CALL(video->type->fb_end(video))
  #endif
  vframec++;
  return 0;
}

/* Having decoded our sprite image, force it to conform to the framebuffer format, or drop it.
 */
 
static void reformat_or_kill_spritebits() {
  if (!spritebits) return;
  if (
    (spritebits->pixelsize==fbdesc.pixelsize)&&
    (spritebits->rmask==fbdesc.rmask)&&
    (spritebits->gmask==fbdesc.gmask)&&
    (spritebits->bmask==fbdesc.bmask)
  ) return;
  if (spritebits->pixelsize!=32) {
    if (rawimg_force_rgba(spritebits)<0) {
      rawimg_del(spritebits);
      spritebits=0;
      return;
    }
    if (
      (spritebits->pixelsize==fbdesc.pixelsize)&&
      (spritebits->rmask==fbdesc.rmask)&&
      (spritebits->gmask==fbdesc.gmask)&&
      (spritebits->bmask==fbdesc.bmask)
    ) return;
  }
  // We're assuming that both formats are 32 bit RGBA with 8 bits for every channel.
  // (output format is allowed to have missing alpha, it usually will).
  // This is not robust enough for general use. But fine for our limited purposes, I think.
  int srp=0,sgp=0,sbp=0,sap=0,drp=0,dgp=0,dbp=0,dap=0;
  uint32_t tmp;
  if (tmp=spritebits->rmask) while (!(tmp&1)) { tmp>>=1; srp++; }
  if (tmp=spritebits->gmask) while (!(tmp&1)) { tmp>>=1; sgp++; }
  if (tmp=spritebits->bmask) while (!(tmp&1)) { tmp>>=1; sbp++; }
  if (tmp=spritebits->amask) while (!(tmp&1)) { tmp>>=1; sap++; }
  if (tmp=fbdesc.rmask) while (!(tmp&1)) { tmp>>=1; drp++; }
  if (tmp=fbdesc.gmask) while (!(tmp&1)) { tmp>>=1; dgp++; }
  if (tmp=fbdesc.bmask) while (!(tmp&1)) { tmp>>=1; dbp++; }
  if (tmp=fbdesc.amask) while (!(tmp&1)) { tmp>>=1; dap++; }
  else { tmp=~(fbdesc.rmask|fbdesc.gmask|fbdesc.bmask); while (!(tmp&1)) { tmp>>=1; dap++; }}
  int stridewords=spritebits->stride>>2;
  uint32_t *row=spritebits->v;
  int yi=spritebits->h;
  for (;yi-->0;row+=stridewords) {
    uint32_t *p=row;
    int xi=spritebits->w;
    for (;xi-->0;p++) {
      uint8_t r=(*p)>>srp;
      uint8_t g=(*p)>>sgp;
      uint8_t b=(*p)>>sbp;
      uint8_t a=(*p)>>sap;
      *p=(r<<drp)|(g<<dgp)|(b<<dbp)|(a<<dap);
    }
  }
  // In real life, you'd want to rewrite spritebits->*mask and chorder to reflect the modified format.
}

/* Main entry point.
 * This should always be "XXX_ITEST"; we're interactive so it's appointment-only.
 */
 
XXX_ITEST(hostio_live) {
  fprintf(stderr,"%s...\n",__func__);
  
  signal(SIGINT,cb_signal);
  
  struct hostio_video_delegate video_delegate={
    .cb_close=cb_close,
    .cb_focus=cb_focus,
    .cb_resize=cb_resize,
    .cb_key=cb_key,
    .cb_text=cb_text,
    .cb_mmotion=cb_mmotion,
    .cb_mbutton=cb_mbutton,
    .cb_mwheel=cb_mwheel,
  };
  struct hostio_audio_delegate audio_delegate={
    .cb_pcm_out=cb_pcm_out,
  };
  struct hostio_input_delegate input_delegate={
    .cb_connect=cb_connect,
    .cb_disconnect=cb_disconnect,
    .cb_button=cb_button,
  };
  struct hostio *hostio=hostio_new(&video_delegate,&audio_delegate,&input_delegate);
  ASSERT(hostio,"hostio_new failed!")
  hostio->video_delegate.userdata=hostio;
  hostio->audio_delegate.userdata=hostio;
  hostio->input_delegate.userdata=hostio;
  
  if (1) {
    struct hostio_video_setup setup={
      .title="Hostio Live Demo",
      .iconrgba=0,
      .iconw=0,
      .iconh=0,
      .w=HOSTIO_LIVE_WINDOW_W,
      .h=HOSTIO_LIVE_WINDOW_H,
      .fullscreen=0,
      .fbw=256,
      .fbh=192,
      .device=0,
    };
    ASSERT_CALL(hostio_init_video(hostio,HOSTIO_LIVE_VIDEO_DRIVERS,&setup))
    const char *fbsrc="Video driver declares";
    #if ENABLE_OPENGL
      if (hostio->video->type->fb_describe&&(hostio->video->type->fb_describe(&fbdesc,hostio->video)>=0)) {
        // OpenGL is enabled but we're using the driver's framebuffer anyway.
        ASSERT(hostio->video->type->fb_begin)
        ASSERT(hostio->video->type->fb_end)
      } else {
        // No native framebuffer from driver. Fake it with OpenGL.
        ASSERT_CALL(opengl_init())
        fbsrc="Using OpenGL to fake";
        ASSERT(hostio->video->type->gx_begin)
        ASSERT(hostio->video->type->gx_end)
      }
    #else
      // Driver must supply a framebuffer.
      ASSERT(hostio->video->type->fb_describe)
      ASSERT_CALL(hostio->video->type->fb_describe(&fbdesc,hostio->video))
      ASSERT(hostio->video->type->fb_begin)
      ASSERT(hostio->video->type->fb_end)
    #endif
    fprintf(stderr,
      "%s framebuffer: (%d,%d)@%d stride=%d masks=%08x,%08x,%08x,%08x order=%.4s\n",
      fbsrc,
      fbdesc.w,fbdesc.h,fbdesc.pixelsize,fbdesc.stride,
      fbdesc.rmask,fbdesc.gmask,fbdesc.bmask,fbdesc.amask,
      fbdesc.chorder
    );
  }
  
  if (1) {
    struct hostio_audio_setup setup={
      .rate=44100,
      .chanc=2,
      .device=0,
      .buffer_size=0,
    };
    ASSERT_CALL(hostio_init_audio(hostio,HOSTIO_LIVE_AUDIO_DRIVERS,&setup))
    if (TONE_FREQ>0) {
      audio_half_period=(hostio->audio->rate/(TONE_FREQ?TONE_FREQ:1))>>1; // gcc bug, I get div-by-zero warning without the selection (despite the "if" above)
      if (audio_half_period<1) audio_half_period=1;
      hostio_audio_play(hostio,1);
    }
  }
  
  if (1) {
    struct hostio_input_setup setup={
      .path=0,
    };
    ASSERT_CALL(hostio_init_input(hostio,HOSTIO_LIVE_INPUT_DRIVERS,&setup))
  }
  
  hostio_log_driver_names(hostio);
  
  // Load our sprite image.
  {
    void *serial=0;
    int serialc=file_read(&serial,"src/test/int/io/hostio/sprite.png");
    if (serialc>=0) {
      if (spritebits=rawimg_decode(serial,serialc)) {
        reformat_or_kill_spritebits();
      }
      free(serial);
    }
  }
  
  fprintf(stderr,"Hostio is running.\n");
  while (!sigc&&!closed_window) {
    ASSERT_CALL(hostio_update(hostio))
    ASSERT_CALL(render_scene(hostio->video))
    // Video drivers are not required to enforce timing (though glx, drmfb, and drmgx all do).
    // In real life you should always have a backup frame clock.
    usleep(HOSTIO_LIVE_FRAME_SLEEP_US);
  }
  
  hostio_del(hostio);
  fprintf(stderr,"%s normal exit.\n",__func__);
  return 0;
}
