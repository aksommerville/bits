#include "test/test.h"
#include "opt/serial/sr_primitives.c"
#include "opt/serial/sr_encodings.c"
#include "opt/serial/sr_encoder.c"
#include "opt/midi/midi_file.c"

static void hexdump(const uint8_t *v,int c) {
  int p=0; for (;p<c;p+=16) {
    int i=0; for (;i<16;i++) {
      if (p+i>=c) break;
      fprintf(stderr," %02x",v[p+i]);
    }
    fprintf(stderr,"\n");
  }
}

/* Our MIDI file reader used to convert ticks to frames immediately,
 * then run the clock separately per track.
 * This has led to clock skew sometimes, prominent in Full Moon's song Nearer the Sky.
 * This test exposes the behavior, and I'll rewrite to keep clock more stable.
 *
 * I think the main issue is that the song has a track that kicks in only near the end.
 * So it has this one enormous delay, while the other tracks are blipping along bit by bit.
 */

static int midi_file_timing_20240412() {
  const struct myevent {
    int abstimems;
    int trackid;
  } myeventv[]={
    // Play track 1 harder than track 0:
    {   0,0},
    { 100,1},
    { 110,1},
    { 120,1},
    { 150,0},
    { 160,1},
    { 170,1},
    { 180,1},
    { 200,0},
    { 201,1},
    { 202,1},
    { 203,1},
    { 300,0},
    { 380,1},
    { 382,1},
    { 383,1},
    { 400,0},
    { 410,1},
    { 400,0},
    { 420,1},
    { 430,0},
    { 440,1},
    { 450,0},
    { 460,1},
    { 470,0},
    { 480,1},
    { 490,0},
    { 500,1},
    { 510,0},
    { 520,1},
    { 530,0},
    { 540,1},
    { 550,0},
    { 560,1},
    { 570,0},
    { 580,1},
    { 600,1},
    { 610,1},
    { 620,1},
    { 630,1},
    { 640,1},
    { 650,1},
    { 660,1},
    { 670,1},
    { 680,1},
    { 690,1},
    // Close with two notes at 1000ms, which should end up at the same time.
    {1000,0},
    {1000,1},
  };
  struct sr_encoder serial={0};
  int division=0x4000;
  sr_encode_raw(&serial,"MThd\0\0\0\6\x00\x01\x00\x02",12);
  sr_encode_intbe(&serial,division,2);
  const struct myevent *myevent;
  int i,time;
  sr_encode_raw(&serial,"MTrk",4);
  int lenp=serial.c;
  sr_encode_raw(&serial,"\0\0\0\0",4);
  for (myevent=myeventv,i=sizeof(myeventv)/sizeof(myeventv[0]),time=0;i-->0;myevent++) {
    if (myevent->trackid!=0) continue;
    int delay=myevent->abstimems-time;
    delay=(delay*division)/500;
    sr_encode_vlq(&serial,delay);
    sr_encode_raw(&serial,"\x90\x40\x40",3);
    time=myevent->abstimems;
  }
  int len=serial.c-lenp-4;
  ((uint8_t*)serial.v)[lenp+0]=len>>24;
  ((uint8_t*)serial.v)[lenp+1]=len>>16;
  ((uint8_t*)serial.v)[lenp+2]=len>>8;
  ((uint8_t*)serial.v)[lenp+3]=len;
  sr_encode_raw(&serial,"MTrk",4);
  lenp=serial.c;
  sr_encode_raw(&serial,"\0\0\0\0",4);
  for (myevent=myeventv,i=sizeof(myeventv)/sizeof(myeventv[0]),time=0;i-->0;myevent++) {
    if (myevent->trackid!=1) continue;
    int delay=myevent->abstimems-time;
    delay=(delay*division)/500;
    sr_encode_vlq(&serial,delay);
    sr_encode_raw(&serial,"\x90\x40\x40",3);
    time=myevent->abstimems;
  }
  len=serial.c-lenp-4;
  ((uint8_t*)serial.v)[lenp+0]=len>>24;
  ((uint8_t*)serial.v)[lenp+1]=len>>16;
  ((uint8_t*)serial.v)[lenp+2]=len>>8;
  ((uint8_t*)serial.v)[lenp+3]=len;
  //hexdump(serial.v,serial.c);
  
  int playback_rate=10000;
  struct midi_file *file=midi_file_new(serial.v,serial.c,playback_rate);
  sr_encoder_cleanup(&serial);
  ASSERT(file)
  
  int note_on_timev[256];
  int note_on_timec=0;
  int framec=0;
  for (;;) {
    struct midi_event event;
    int err=0;
    while (!(err=midi_file_next(&event,file))) {
      if (event.opcode==0x90) {
        if (note_on_timec<256) {
          note_on_timev[note_on_timec++]=(framec*1000+(playback_rate>>1))/playback_rate;
        }
      }
    }
    if (err<0) {
      ASSERT(midi_file_is_finished(file),"after %d notes and %d frames",note_on_timec,framec)
      break;
    }
    // Advance just one frame at a time, to really stress the clock.
    ASSERT_CALL(midi_file_advance(file,1))
    framec++;
  }
  
  /*
  fprintf(stderr,"%s: Processed %d note ons over %d frames\n",__func__,note_on_timec,framec);
  for (i=0;i<note_on_timec;i++) {
    fprintf(stderr,"%5d %5d\n",note_on_timev[i],(i>0)?(note_on_timev[i]-note_on_timev[i-1]):0);
  }
  
  int eventc=sizeof(myeventv)/sizeof(myeventv[0]);
  ASSERT_INTS(note_on_timec,eventc)
  const int *actual=note_on_timev;
  fprintf(stderr,"Diffs:\n");
  for (myevent=myeventv,i=eventc;i-->0;myevent++,actual++) {
    fprintf(stderr,"  %+d\n",(*actual)-myevent->abstimems);
  }
  */
  
  int eventc=sizeof(myeventv)/sizeof(myeventv[0]);
  ASSERT_INTS(note_on_timec,eventc)
  int finalea=note_on_timev[note_on_timec-2];
  int finaleb=note_on_timev[note_on_timec-1];
  ASSERT_INTS(finalea,finaleb)
  ASSERT_INTS(finalea,1000)
  
  midi_file_del(file);
  return 0;
}

/* TOC
 */

int main(int argc,char **argv) {
  UTEST(midi_file_timing_20240412)
  return 0;
}
