#include "test/test.h"
#include "opt/ossmidi/ossmidi.h"
#include "opt/midi/midi.h"
#include <signal.h>

/* Signal handler.
 */
 
static volatile int sigc=0;

static void rcvsig(int sigid) {
  switch (sigid) {
    case SIGINT: if (++sigc>=3) {
        fprintf(stderr,"Too many unprocessed signals.\n");
        exit(1);
      } break;
  }
}

/* Poll the MIDI input bus and dump all events.
 */
 
static int ossmidi_live_devid_next=1;

static void ossmidi_live_cb_connect(struct ossmidi *ossmidi,struct ossmidi_device *device) {
  fprintf(stderr,"%p %s\n",device,__func__);
  struct ossmidi_device_details details={0};
  if (ossmidi_device_lookup_details(&details,device)>=0) {
    fprintf(stderr,"  name: '%.*s'\n",details.namec,details.name);
  } else {
    fprintf(stderr,"  Failed to fetch device details.\n");
  }
}

static void ossmidi_live_cb_disconnect(struct ossmidi *ossmidi,struct ossmidi_device *device) {
  fprintf(stderr,"%p %s\n",device,__func__);
}

static void ossmidi_live_cb_event(struct ossmidi *ossmidi,struct ossmidi_device *device,const struct midi_event *event) {
  fprintf(stderr,"%p %s %02x %02x %02x %02x +%d\n",device,__func__,event->chid,event->opcode,event->a,event->b,event->c);
}
 
XXX_ITEST(ossmidi_live) {
  signal(SIGINT,rcvsig);
  struct ossmidi_delegate delegate={
    .cb_connect=ossmidi_live_cb_connect,
    .cb_disconnect=ossmidi_live_cb_disconnect,
    .cb_event=ossmidi_live_cb_event,
  };
  struct ossmidi *ossmidi=ossmidi_new(&delegate);
  ASSERT(ossmidi)
  fprintf(stderr,"Dumping MIDI events until SIGINT...\n");
  while (!sigc) {
    ASSERT_CALL(ossmidi_update(ossmidi,100))
  }
  fprintf(stderr,"Normal exit.\n");
  ossmidi_del(ossmidi);
  return 0;
}
