#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <sys/time.h>
#include <limits.h>
#include <stdint.h>
#include <math.h>

#if BITS_USE_platform_macos
  #include <OpenGL/gl.h>
#else
  #include <GL/gl.h>
#endif

/* Some trivial software rendering, just to see what's up.
 */

#define FBW 160
#define FBH 90
static uint8_t fb[FBW*FBH*4]={0};

/* Audio.
 */

#if BITS_USE_platform_macos
  #include "platform/macos/macaudio/macaudio.h"
  static struct macaudio *macaudio=0;
#endif

static double aup=0.0;
#define NOTE_PITCH 440.0
#define EXPECTED_SAMPLE_RATE 44100.0
static double audp=(M_PI*2.0*NOTE_PITCH)/EXPECTED_SAMPLE_RATE;
static double auddp=0.000001;
static double audplo=0.010;
static double audphi=0.100;

static void ioc_cb_pcm_out(int16_t *v,int c,void *userdata) {
  while (c>=2) {
    v[0]=sin(aup)*10000.0;
    v[1]=v[0];
    v+=2;
    c-=2;
    aup+=audp;
    if (aup>=M_PI) aup-=M_PI*2.0;
    audp+=auddp;
    if ((audp>audphi)&&(auddp>0.0)) auddp=-auddp;
    else if ((audp<audplo)&&(auddp<0.0)) auddp=-auddp;
  }
  //memset(v,0,c<<1);
}

static void ioc_cb_midi_in(const void *v,int c,void *userdata) {
  fprintf(stderr,"%s c=%d\n",__func__,c);
}

/* Window manager.
 */

#if BITS_USE_platform_macos
  #include "platform/macos/macwm/macwm.h"
  static struct macwm *macwm=0;
#endif

static void ioc_cb_close(void *userdata) {
  fprintf(stderr,"%s\n",__func__);
}

static void ioc_cb_resize(void *userdata,int w,int h) {
  //fprintf(stderr,"%s %d,%d\n",__func__,w,h);
}

static int ioc_cb_key(void *userdata,int keycode,int value) {
  fprintf(stderr,"%s 0x%08x=%d\n",__func__,keycode,value);
  return 0;
}

static void ioc_cb_text(void *userdata,int codepoint) {
  fprintf(stderr,"%s U+%x\n",__func__,codepoint);
}

static void ioc_cb_mmotion(void *userdata,int x,int y) {
  //fprintf(stderr,"%s %d,%d\n",__func__,x,y);
}

static void ioc_cb_mbutton(void *userdata,int btnid,int value) {
  fprintf(stderr,"%s %d=%d\n",__func__,btnid,value);
}

static void ioc_cb_mwheel(void *userdata,int dx,int dy) {
  fprintf(stderr,"%s %+d,%+d\n",__func__,dx,dy);
}

/* Input manager.
 */

#if BITS_USE_platform_macos
  #include "platform/macos/machid/machid.h"
  static struct machid *machid=0;
#endif

static int ioc_devid_next=1;

static int ioc_cb_devid_next(struct machid *machid) {
  if (ioc_devid_next>=INT_MAX) return -1;
  return ioc_devid_next++;
}

static int ioc_cb_filter(struct machid *machid,int vid,int pid,int page,int usage) {
  fprintf(stderr,"%s %04x:%04x usage=%04x%04x\n",__func__,vid,pid,page,usage);
  // Doing the same as machid's default. In real life, we wouldn't both implementing this.
  if (page==0x0001) {
    if (usage==0x0004) return 1;
    if (usage==0x0005) return 1;
  }
  return 0;
}

static int ioc_cb_list_button(int btnid,int usage,int lo,int hi,int value,void *userdata) {
  fprintf(stderr,
    "  btnid=0x%08x usage=0x%08x range=%d..%d value=%d\n",
    btnid,usage,lo,hi,value
  );
  return 0;
}

static void ioc_cb_connect(struct machid *machid,int devid) {
  fprintf(stderr,"%s %d\n",__func__,devid);
  #if BITS_USE_platform_macos
    int vid=0,pid=0;
    const char *name=machid_get_ids(&vid,&pid,machid,devid);
    fprintf(stderr,"%04x:%04x %s\n",vid,pid,name);
    machid_enumerate(machid,devid,ioc_cb_list_button,0);
  #endif
}

static void ioc_cb_disconnect(struct machid *machid,int devid) {
  fprintf(stderr,"%s %d\n",__func__,devid);
}

static void ioc_cb_button(struct machid *machid,int devid,int btnid,int value) {
  fprintf(stderr,"%s %d.0x%08x=%d\n",__func__,devid,btnid,value);
}

/* Inversion-of-Control test.
 * There will be at least two IoC units: macioc genioc
 */

#if BITS_USE_platform_macos
  #include "platform/macos/macioc/macioc.h"
#endif

static int updatec=0;
static int64_t starttime=0;

static int64_t now() {
  struct timeval tv={0};
  gettimeofday(&tv,0);
  return (int64_t)tv.tv_sec*1000000ll+tv.tv_usec;
}

static void ioc_cb_quit(void *userdata) {
  if (updatec>0) {
    int64_t endtime=now();
    if (endtime>starttime) {
      int64_t elapsedus=endtime-starttime;
      double elapseds=elapsedus/1000000.0;
      double rate=updatec/elapseds;
      fprintf(stderr,"%s updatec=%d elapsed=%.03fs avg=%.03fHz\n",__func__,updatec,elapseds,rate);
    }
  }

  #if BITS_USE_platform_macos
    macwm_del(macwm); macwm=0;
    machid_del(machid); machid=0;
    macaudio_del(macaudio); macaudio=0;
  #endif
}

static void ioc_cb_focus(void *userdata,int focus) {
  fprintf(stderr,"%s %d\n",__func__,focus);
}

static int ioc_cb_init(void *userdata) {
  fprintf(stderr,"%s\n",__func__);

  #if BITS_USE_platform_macos
    struct macwm_delegate macwm_delegate={
      .close=ioc_cb_close,
      .resize=ioc_cb_resize,
      .key=ioc_cb_key,
      .text=ioc_cb_text,
      .mmotion=ioc_cb_mmotion,
      .mbutton=ioc_cb_mbutton,
      .mwheel=ioc_cb_mwheel,
    };
    struct macwm_setup macwm_setup={
      .w=640,
      .h=360,
      .fullscreen=0,
      .title="AK Bits Demo",
      .rendermode=MACWM_RENDERMODE_FRAMEBUFFER, // _FRAMEBUFFER _OPENGL _METAL
      .fbw=FBW,
      .fbh=FBH,
    };
    if (!(macwm=macwm_new(&macwm_delegate,&macwm_setup))) {
      fprintf(stderr,"Failed to initialize macwm\n");
      return -1;
    }

    struct machid_delegate machid_delegate={
      .userdata=0,
      .devid_next=ioc_cb_devid_next,
      .filter=ioc_cb_filter,
      .connect=ioc_cb_connect,
      .disconnect=ioc_cb_disconnect,
      .button=ioc_cb_button,
    };
    if (!(machid=machid_new(&machid_delegate))) {
      fprintf(stderr,"Failed to initialize machid\n");
      return -1;
    }

    struct macaudio_delegate macaudio_delegate={
      .userdata=0,
      .pcm_out=ioc_cb_pcm_out,
      .midi_in=ioc_cb_midi_in,
    };
    struct macaudio_setup macaudio_setup={
      .rate=44100,
      .chanc=2,
    };
    if (!(macaudio=macaudio_new(&macaudio_delegate,&macaudio_setup))) {
      fprintf(stderr,"Failed to initialize macaudio\n");
      return -1;
    }
    macaudio_play(macaudio,1);
  #endif
  
  starttime=now();
  return 0;
}

static uint8_t luma=0x80;

static void ioc_cb_update(void *userdata) {
  //fprintf(stderr,"%s\n",__func__);

  luma++;
  memset(fb,luma,sizeof(fb));
  
  #if BITS_USE_platform_macos

    if (machid_update(machid,0.0)<0) {
      fprintf(stderr,"machid_update failed\n");
      macioc_terminate(1);
    }
  
    if (macwm_render_begin(macwm)>=0) {
      int screenw,screenh;
      macwm_get_size(&screenw,&screenh,macwm);
      glClearColor(0.5f,0.25f,0.0f,1.0f);
      glClear(GL_COLOR_BUFFER_BIT);
      glViewport(0,0,screenw,screenh);
      glMatrixMode(GL_PROJECTION);
      glLoadIdentity();
      glBegin(GL_TRIANGLES);
        glColor4ub(0xff,0x00,0x00,0xff); glVertex2f( 0.5f, 0.5f);
        glColor4ub(0x00,0xff,0x00,0xff); glVertex2f(-0.5f, 0.5f);
        glColor4ub(0x00,0x00,0xff,0xff); glVertex2f( 0.0f,-0.5f);
      glEnd();
      macwm_render_end(macwm);
    } else { // software rendering...
      macwm_send_framebuffer(macwm,fb);
    }
  #endif
  updatec++;
}

static int akbits_demo_ioc(int argc,char **argv) {

  #if BITS_USE_platform_macos
    struct macioc_delegate delegate={
      .rate=60,
      .userdata=0,
      .focus=ioc_cb_focus,
      .quit=ioc_cb_quit,
      .init=ioc_cb_init,
      .update=ioc_cb_update,
    };
    return macioc_main(argc,argv,&delegate);

  #elif BITS_USE_genioc
    fprintf(stderr,"TODO %s genioc\n",__func__);
    return 1;

  #else
    fprintf(stderr,"%s, no ioc unit compiled\n",__func__);
    return 1;
  #endif
}

/* Main, TOC.
 */

int main(int argc,char **argv) {
  //fprintf(stderr,"This demo program can be modified during development to test whatever I'm working on.\n");
  return akbits_demo_ioc(argc,argv);
  return 0;
}
