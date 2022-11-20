#if BITS_USE_platform_linux || BITS_USE_platform_linux_evdev

#include "platform/linux/evdev/evdev.h"
#include <stdio.h>
#include <signal.h>
#include <stdlib.h>

static volatile int bits_demo_evdev_sigc=0;

static void bits_demo_evdev_rcvsig(int sigid) {
  switch (sigid) {
    case SIGINT: if (++bits_demo_evdev_sigc>=3) {
        fprintf(stderr,"Too many unprocessed signals.\n");
        exit(1);
      } break;
  }
}

static int bits_demo_evdev_cb_enumerate(
  int type,int code,int usage,int lo,int hi,int value,void *userdata
) {
  fprintf(stderr,
    "  %02x:%04x %08x (%d..%d) =%d\n",
    type,code,usage,lo,hi,value
  );
  return 0;
}

static void bits_demo_evdev_cb_connect(struct evdev_device *device) {
  fprintf(stderr,"%s %p\n",__func__,device);
  
  char name[256];
  int namec=evdev_device_get_name(name,sizeof(name),device);
  if ((namec>=0)&&(namec<=sizeof(name))) {
    fprintf(stderr,"Name: '%.*s'\n",namec,name);
  } else {
    fprintf(stderr,"Failed to fetch device name.\n");
  }
  
  int vid=evdev_device_get_vid(device);
  int pid=evdev_device_get_pid(device);
  int version=evdev_device_get_version(device);
  int bustype=evdev_device_get_bustype(device);
  fprintf(stderr,"vid=0x%04x pid=0x%04x version=0x%04x bustype=%d\n",vid,pid,version,bustype);
  
  evdev_device_enumerate(device,bits_demo_evdev_cb_enumerate,device);
}

static void bits_demo_evdev_cb_disconnect(struct evdev_device *device) {
  fprintf(stderr,"%s %p\n",__func__,device);
}

static void bits_demo_evdev_cb_button(struct evdev_device *device,int type,int code,int value) {
  if ((type==4)&&(code==4)) { // MSC_SCAN: HID usage codes, unfortunately delivered too late for our purposes.
    fprintf(stderr,"%s %p usage=0x%08x\n",__func__,device,value);
  } else {
    fprintf(stderr,"%s %p.(%d,0x%04x)=%d\n",__func__,device,type,code,value);
  }
}

static void bits_demo_evdev_cb_lost_inotify(struct evdev *evdev) {
  fprintf(stderr,"%s %p\n",__func__,evdev);
}

int bits_demo_evdev(int argc,char **argv) {
  fprintf(stderr,"%s...\n",__func__);
  
  signal(SIGINT,bits_demo_evdev_rcvsig);
  
  struct evdev_delegate delegate={
    .userdata=0,
    .connect=bits_demo_evdev_cb_connect,
    .disconnect=bits_demo_evdev_cb_disconnect,
    .button=bits_demo_evdev_cb_button,
    .lost_inotify=bits_demo_evdev_cb_lost_inotify,
  };
  struct evdev_setup setup={ // <-- not necessary if you want these defaults:
    .path="/dev/input", // "/dev/input"
    .use_inotify=1, // 1
    .grab_devices=1, // 1
    .guess_hid=1, // 1
  };
  struct evdev *evdev=evdev_new(&delegate,&setup);
  if (!evdev) {
    fprintf(stderr,"evdev_new() failed\n");
    return 1;
  }
  
  fprintf(stderr,"Created evdev context %p.\n",evdev);
  fprintf(stderr,"Will terminate at SIGINT or after no more than 30 seconds.\n");
  int ttl=300;
  while ((ttl-->0)&&!bits_demo_evdev_sigc) {
    if (evdev_update(evdev,100)<0) {
      fprintf(stderr,"!!! evdev_update failed !!!\n");
      break;
    }
  }
  
  evdev_del(evdev);
  fprintf(stderr,"...%s\n",__func__);
  return 0;
}

#else
int bits_demo_evdev(int argc,char **argv) { return 1; }
#endif
