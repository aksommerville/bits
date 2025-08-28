#include "test/test.h"
#include "opt/inmgr/inmgr.h"
#include "opt/hostio/hostio.h"
#include <unistd.h>
#include <sys/signal.h>

/* Live test of inmgr. Appointment only.
 */
 
static volatile int inmgr_live_quit=0;

static void rcvsig(int sigid) {
  switch (sigid) {
    case SIGINT: if (++inmgr_live_quit>=3) {
        fprintf(stderr,"Too many unprocessed signals.\n");
        exit(1);
      } break;
  }
}

static int cb_cap(int btnid,int hidusage,int lo,int hi,int value,void *userdata) {
  inmgr_connect_more(userdata,btnid,hidusage,lo,hi,value);
  return 0;
}
 
static void cb_connect(struct hostio_input *driver,int devid) {
  //fprintf(stderr,"%s %d\n",__func__,devid);
  if (!driver->type->get_ids||!driver->type->for_each_button) {
    fprintf(stderr,"!!! Input driver '%s' does not implement get_ids or for_each_button; it really should do both.\n",driver->type->name);
    return;
  }
  int vid=0,pid=0,version=0;
  const char *name=driver->type->get_ids(&vid,&pid,&version,driver,devid);
  void *ctx=inmgr_connect_begin(devid,vid,pid,version,name,-1);
  if (!ctx) {
    fprintf(stderr,"!!! null from inmgr_connect_begin\n");
    return;
  }
  driver->type->for_each_button(driver,devid,cb_cap,ctx);
  if (inmgr_connect_end(ctx)>0) {
    fprintf(stderr,"DEVICE READY: %04x:%04x:%04x '%s'\n",vid,pid,version,name);
  } else {
    fprintf(stderr,"DEVICE NOT USABLE: %04x:%04x:%04x '%s'\n",vid,pid,version,name);
  }
}

static void cb_disconnect(struct hostio_input *driver,int devid) {
  //fprintf(stderr,"%s %d\n",__func__,devid);
  inmgr_disconnect(devid);
}

static void cb_button(struct hostio_input *driver,int devid,int btnid,int value) {
  //fprintf(stderr,"%s %d.0x%08x=%d\n",__func__,devid,btnid,value);
  inmgr_event(devid,btnid,value);
}

static void cb_quit() {
  fprintf(stderr,"%s\n",__func__);
  inmgr_live_quit++;
}
 
XXX_ITEST(inmgr_live) {
  inmgr_live_quit=0;
  signal(SIGINT,rcvsig);
  int pvinput[3]={0};

  ASSERT_CALL(inmgr_init())
  inmgr_set_player_count(2);
  inmgr_set_buttons(0xffff); // This is the default anyway.
  inmgr_set_signal(INMGR_SIGNAL_QUIT,cb_quit);

  struct hostio_input_delegate input_delegate={
    .cb_connect=cb_connect,
    .cb_disconnect=cb_disconnect,
    .cb_button=cb_button,
  };
  struct hostio_input_setup input_setup={
    .path=0,
  };
  struct hostio *hostio=hostio_new(0,0,&input_delegate);
  ASSERT(hostio,"hostio_new")
  ASSERT_CALL(hostio_init_input(hostio,0,&input_setup))
  
  while (!inmgr_live_quit) {
    usleep(10000);
    ASSERT_CALL(hostio_update(hostio))
    int i=0; for (;i<2;i++) {
      int input=inmgr_get_player(i);
      if (input!=pvinput[i]) {
        fprintf(stderr,"Player %d: 0x%04x (was 0x%04x)\n",i,input,pvinput[i]);
        pvinput[i]=input;
      }
    }
  }
  
  hostio_del(hostio);
  return 0;
}
