#if BITS_USE_platform_linux || BITS_USE_platform_linux_ossmidi

#include "platform/linux/ossmidi/ossmidi.h"
#include <stdio.h>
#include <signal.h>
#include <stdlib.h>

static volatile int _sigc=0;

static void _cb_signal(int sigid) {
  switch (sigid) {
    case SIGINT: if (++_sigc>=3) {
        fprintf(stderr,"Too many unprocessed signals.\n");
        exit(1);
      } break;
  }
}

static void _cb_events(int devid,const void *v,int c,void *userdata) {
  fprintf(stderr,"%s devid=%d c=%d\n",__func__,devid,c);
}

static void _cb_connect(int devid,int fd,void *userdata) {
  fprintf(stderr,"%s devid=%d fd=%d\n",__func__,devid,fd);
}

static void _cb_disconnect(int devid,int fd,void *userdata) {
  fprintf(stderr,"%s devid=%d fd=%d\n",__func__,devid,fd);
}

static void _cb_lost_inotify(void *userdata) {
  fprintf(stderr,"%s\n",__func__);
}

int bits_demo_ossmidi(int argc,char **argv) {
  fprintf(stderr,"%s...\n",__func__);
  
  signal(SIGINT,_cb_signal);
  
  struct ossmidi_delegate delegate={
    .userdata=0,
    .events=_cb_events,
    // optional, most clients won't want:
    .connect=_cb_connect,
    .disconnect=_cb_disconnect,
    .lost_inotify=_cb_lost_inotify,
  };
  struct ossmidi *ossmidi=ossmidi_new(&delegate);
  if (!ossmidi) {
    fprintf(stderr,"ossmidi_new failed\n");
    return 1;
  }
  
  while (!_sigc) {
    if (ossmidi_update(ossmidi,1000)<0) {
      fprintf(stderr,"!!! ossmidi_update failed !!!\n");
      break;
    }
  }
  
  ossmidi_del(ossmidi);
  return 0;
}

#else
int bits_demo_ossmidi(int argc,char **argv) { return 1; }
#endif
