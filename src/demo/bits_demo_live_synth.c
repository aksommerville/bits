#include <stdio.h>
#include <signal.h>
#include <string.h>
#include <stdlib.h>
#include "platform/linux/ossmidi/ossmidi.h"
#include "platform/linux/alsapcm/alsapcm.h"
#include "synth/synth_minor/synth_minor.h"
#include "format/midi/midi.h"

/* Globals.
 */
 
static volatile int _ls_sigc=0;

#define LS_DEVICE_LIMIT 16
static struct ls_midi_device {
  int devid;
  struct midi_stream_reader reader;
} ls_devicev[LS_DEVICE_LIMIT]={0};
static int ls_devicec=0;

static struct alsapcm *alsapcm=0;
static struct synth_minor *synth=0;
static int ls_locked=0;

/* MIDI-In device list.
 */
 
static int ls_devicev_search(int devid) {
  int i=0; for (;i<ls_devicec;i++) {
    if (ls_devicev[i].devid==devid) return i;
  }
  return -1;
}
 
static void ls_drop_midi_device(int devid) {
  int p=ls_devicev_search(devid);
  if (p>=0) {
    ls_devicec--;
    memmove(ls_devicev+p,ls_devicev+p+1,sizeof(struct ls_midi_device)*(ls_devicec-p));
  }
}

static void ls_add_midi_device(int devid) {
  if (ls_devicec>=LS_DEVICE_LIMIT) return;
  struct ls_midi_device *device=ls_devicev+ls_devicec++;
  device->devid=devid;
  memset(&device->reader,0,sizeof(struct midi_stream_reader));
}

static struct ls_midi_device *ls_get_midi_device(int devid) {
  int p=ls_devicev_search(devid);
  if (p<0) return 0;
  return ls_devicev+p;
}

/* Callbacks.
 */
 
static void _ls_cb_signal(int sigid) {
  switch (sigid) {
    case SIGINT: if (++_ls_sigc>=3) {
        fprintf(stderr,"Too many unprocessed signals.\n");
        exit(1);
      } break;
  }
}

static void _ls_cb_pcm_out(int16_t *v,int c,void *userdata) {
  synth_minor_update(v,c,synth);
}

static void _ls_cb_events(int devid,const void *v,int c,void *userdata) {
  if (!c) {
    ls_drop_midi_device(devid);
  } else if ((c==2)&&!memcmp(v,"\xf7\xf0",2)) {
    ls_add_midi_device(devid);
  } else {
    struct ls_midi_device *device=ls_get_midi_device(devid);
    if (!device) return;
    if (!ls_locked) {
      if (alsapcm_lock(alsapcm)<0) return;
      ls_locked=1;
    }
    midi_stream_reader_more(&device->reader,v,c);
    struct midi_event event;
    while (1) {
      int err=midi_stream_reader_next(&event,&device->reader);
      if (err<=0) return;
      //ls_event(&event);
      synth_minor_event(synth,&event);
    }
  }
}

/* Main.
 */
 
int bits_demo_live_synth(int argc,char **argv) {
  fprintf(stderr,"%s...\n",__func__);
  
  signal(SIGINT,_ls_cb_signal);
  
  struct alsapcm_delegate alsadelegate={
    .pcm_out=_ls_cb_pcm_out,
  };
  struct alsapcm_setup alsasetup={
    .rate=44100,
    .chanc=1,
  };
  alsapcm=alsapcm_new(&alsadelegate,&alsasetup);
  if (!alsapcm) return 1;
  
  struct ossmidi_delegate ossdelegate={
    .events=_ls_cb_events,
  };
  struct ossmidi *ossmidi=ossmidi_new(&ossdelegate);
  if (!ossmidi) return -1;
  
  if (!(synth=synth_minor_new(
    alsapcm_get_rate(alsapcm),
    alsapcm_get_chanc(alsapcm)
  ))) return 1;
  
  alsapcm_set_running(alsapcm,1);
  
  fprintf(stderr,"Running. SIGINT to quit...\n");
  while (!_ls_sigc) {
    if (alsapcm_update(alsapcm)<0) return 1;
    if (ossmidi_update(ossmidi,1000)<0) return 1;
    if (ls_locked) {
      alsapcm_unlock(alsapcm);
      ls_locked=0;
    }
  }
  
  ossmidi_del(ossmidi);
  alsapcm_del(alsapcm);
  synth_minor_del(synth);
  fprintf(stderr,"Normal exit.\n");
  return 0;
}
