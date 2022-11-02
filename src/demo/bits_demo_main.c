#include <stdio.h>
#include <stdint.h>
#include <sys/time.h>

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
  fprintf(stderr,"%s %d,%d\n",__func__,w,h);
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
  int64_t endtime=now();
  if (endtime>starttime) {
    int64_t elapsedus=endtime-starttime;
    double elapseds=elapsedus/1000000.0;
    double rate=updatec/elapseds;
    fprintf(stderr,"%s updatec=%d elapsed=%.03fs avg=%.03fHz\n",__func__,updatec,elapseds,rate);
  }

  #if BITS_USE_platform_macos
    macwm_del(macwm);
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
    };
    if (!(macwm=macwm_new(&macwm_delegate,&macwm_setup))) {
      fprintf(stderr,"Failed to initialize macwm\n");
      return -1;
    }
  #endif
  
  starttime=now();
  return 0;
}

static void ioc_cb_update(void *userdata) {
  //fprintf(stderr,"%s\n",__func__);
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
