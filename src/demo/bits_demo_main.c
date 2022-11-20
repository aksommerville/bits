#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <sys/time.h>
#include <limits.h>
#include <stdint.h>
#include <math.h>
#include <unistd.h>

#if BITS_USE_platform_hw
  #include "platform/hw/hw.h"
#else
  #error "This demo requires unit 'platform/hw'"
#endif

/* Window manager.
 */
  
static void demo_cb_wm_close(void *userdata) {
  fprintf(stderr,"%s\n",__func__);
}

static void demo_cb_wm_focus(void *userdata,int focus) {
  fprintf(stderr,"%s %d\n",__func__,focus);
}

static void demo_cb_wm_resize(void *userdata,int w,int h) {
  fprintf(stderr,"%s %d,%d\n",__func__,w,h);
}

/* Render.
 */
 
static void demo_cb_wm_render(void *userdata) {
}

/* Keyboard.
 */
  
static int demo_cb_wm_key(void *userdata,int keycode,int value) {
  fprintf(stderr,"%s 0x%08x=%d\n",__func__,keycode,value);
  return 0; // >0 to ack, then we won't get a related 'text' event.
}

static void demo_cb_wm_text(void *userdata,int codepoint) {
  fprintf(stderr,"%s U+%x\n",__func__,codepoint);
}

/* Mouse.
 */
  
static void demo_cb_wm_mmotion(void *userdata,int x,int y) {
  fprintf(stderr,"%s %d,%d\n",__func__,x,y);
}

static void demo_cb_wm_mbutton(void *userdata,int btnid,int value) {
  fprintf(stderr,"%s %d=%d\n",__func__,btnid,value);
}

static void demo_cb_wm_mwheel(void *userdata,int dx,int dy) {
  fprintf(stderr,"%s %+d,%+d\n",__func__,dx,dy);
}

/* Audio. TODO I think hw is going to handle this internally.
 */
  
static void demo_cb_pcm_out(int16_t *v,int c,void *userdata) {
  memset(v,0,c<<1);
}

static void demo_cb_midi_in(void *userdata,int devid,const void *v,int c) {
  fprintf(stderr,"%s [%d] %d\n",__func__,devid,c);
}

/* Raw input.
 */
  
static void demo_cb_input_connect(void *userdata,int devid) {
  fprintf(stderr,"%s %d\n",__func__,devid);
}

static void demo_cb_input_disconnect(void *userdata,int devid) {
  fprintf(stderr,"%s %d\n",__func__,devid);
}

static void demo_cb_input_button(void *userdata,int devid,int btnid,int value) {
  fprintf(stderr,"%s %d.0x%08x=%d\n",__func__,devid,btnid,value);
}

/* Mapped input.
 */
 
static void demo_cb_input_mapped(void *userdata,int devid,int plrid,int btnid,int value,uint16_t state) {
  fprintf(stderr,"%s devid=%d plrid=%d btnid=%d value=%d state=0x%04x\n",__func__,devid,plrid,btnid,value,state);
}

/* Command line.
 */
  
static int demo_cb_arg(void *userdata,const char *k,int kc,const char *v,int vc) {
  fprintf(stderr,"%s '%.*s'='%.*s'\n",__func__,kc,k,vc,v);
  return 0; // >0 to ack, wrapper won't try to use it
}

/* Update.
 */

static void demo_cb_update(void *userdata) {
}

/* Sleep. TODO This only needs implemented if we plug in our own poller. Unlikely.
 */

static int demo_cb_sleep(void *userdata,int us) {
  usleep(us);
  return 0;
}

/* Startup.
 */
 
static int demo_cb_init(void *userdata) {
  fprintf(stderr,"%s\n",__func__);
  return 0;
}

/* Cleanup.
 */
 
static void demo_cb_quit(void *userdata) {
  fprintf(stderr,"%s\n",__func__);
}

/* Main.
 */
 
int bits_demo_evdev(int argc,char **argv);
int bits_demo_alsa(int argc,char **argv);

int main(int argc,char **argv) {

  // One-off demos, enable as desired.
  //return bits_demo_evdev(argc,argv);
  return bits_demo_alsa(argc,argv);

  // Default demo, using the high-level 'hw' interface.
  struct hw_delegate delegate={
    .userdata=0,
    .quit=demo_cb_quit,
    .init=demo_cb_init,
    .wm_close=demo_cb_wm_close,
    .wm_focus=demo_cb_wm_focus,
    .wm_resize=demo_cb_wm_resize,
    .wm_render=demo_cb_wm_render,
    .wm_key=demo_cb_wm_key,
    .wm_text=demo_cb_wm_text,
    .wm_mmotion=demo_cb_wm_mmotion,
    .wm_mwheel=demo_cb_wm_mwheel,
    .pcm_out=demo_cb_pcm_out,
    .midi_in=demo_cb_midi_in,
    .input_connect=demo_cb_input_connect,
    .input_disconnect=demo_cb_input_disconnect,
    .input_button=demo_cb_input_button,
    .input_mapped=demo_cb_input_mapped,
    .arg=demo_cb_arg,
    .update=demo_cb_update,
    .sleep=demo_cb_sleep,
  };
  return hw_main(argc,argv,&delegate);
}
