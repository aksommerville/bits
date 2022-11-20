#include "hw_internal.h"

struct hw *hw_g=0;
struct hw *hw_global() { return hw_g; }

/* Select IoC unit.
 * There's no abstraction for IoC implementations like other drivers.
 * (this right here is the abstraction).
 */

#define HW_IOC_NONE   0
#define HW_IOC_macioc 1
#define HW_IOC_genioc 2

#if BITS_USE_platform_macos
  #define HW_IOC HW_IOC_macioc
#elif BITS_USE_platform_genioc
  #define HW_IOC HW_IOC_genioc
#else
  #define HW_IOC HW_IOC_NONE
#endif

#if HW_IOC==HW_IOC_macioc
  #include "platform/macos/macioc.h"
#elif HW_IOC==HW_IOC_genioc
  #include "platform/genioc/genioc.h"
#endif

/* Cleanup.
 */
 
static void hw_cb_quit(void *userdata) {
  fprintf(stderr,"%s\n",__func__);
  struct hw *hw=userdata;
  hw_audio_play(hw->audio,0);
  if (hw->delegate.quit) hw->delegate.quit(hw->delegate.userdata);
}

/* Init.
 */
 
static int hw_cb_init(void *userdata) {
  fprintf(stderr,"%s\n",__func__);
  struct hw *hw=userdata;
  if (hw_ready(hw)<0) return -1;
  if (hw->delegate.init) return hw->delegate.init(hw->delegate.userdata);
  return 0;
}

/* Update.
 */
 
static void hw_cb_update(void *userdata) {
  //fprintf(stderr,"%s\n",__func__);
  struct hw *hw=userdata;
  if (hw->delegate.update) hw->delegate.update(hw->delegate.userdata);
}

/* Configure.
 */
 
static int hw_cb_arg(void *userdata,const char *k,int kc,const char *v,int vc) {
  fprintf(stderr,"%s '%.*s'='%.*s'\n",__func__,kc,k,vc,v);
  return 0; // >0 to ack
}

/* Sleep.
 */
 
static int hw_cb_sleep(void *userdata,int us) {
  return 0;
}

/* macioc
 */

#if HW_IOC==HW_IOC_macioc

static int hw_main_macioc(int argc,char **argv) {
  struct macioc_delegate delegate={
    .rate=HW_FRAME_RATE,
    .userdata=hw_g,
    .quit=hw_cb_quit,
    .init=hw_cb_init,
    //.arg=hw_cb_arg,//TODO macioc should have one, i think
    .update=hw_cb_update,
  };
  return macioc_main(argc,argv,&delegate);
}

#endif

/* genioc
 */
 
#if HW_IOC==HW_IOC_genioc

static int hw_main_genioc(int argc,char **argv) {
  struct genioc_delegate delegate={
    .rate=HW_FRAME_RATE,
    .userdata=hw_g,
    .quit=hw_cb_quit,
    .init=hw_cb_init,
    .arg=hw_cb_arg,
    .update=hw_cb_update,
    .sleep=hw_cb_sleep,
  };
  return genioc_main(argc,argv,&delegate);
}

#endif

/* Fallback, no IoC unit available.
 */
 
#if HW_IOC==HW_IOC_NONE

static int hw_main_none(int argc,char **argv) {
  const char *exename=((argc>0)&&argv[0])?argv[0]:"unknown";
  fprintf(stderr,"%s: No IoC unit compiled. [%s:%d]\n",exename,__FILE__,__LINE__);
  return 1;
}

#endif

/* Main.
 */
 
int hw_main(int argc,char **argv,const struct hw_delegate *delegate) {
  if (hw_g) return 1;
  if (!(hw_g=hw_new(delegate))) return 1;
  #if HW_IOC==HW_IOC_macioc
    int status=hw_main_macioc(argc,argv);
  #elif HW_IOC==HW_IOC_genioc
    int status=hw_main_genioc(argc,argv);
  #elif HW_IOC==HW_IOC_NONE
    int status=hw_main_none(argc,argv);
  #endif
  hw_del(hw_g);
  hw_g=0;
  return status;
}
