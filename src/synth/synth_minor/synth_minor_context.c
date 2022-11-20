#include "synth_minor_internal.h"

/* Delete.
 */
 
void synth_minor_del(struct synth_minor *synth) {
  if (!synth) return;
  
  if (synth->voicev) {
    while (synth->voicec-->0) sm_voice_cleanup(synth->voicev+synth->voicec);
    free(synth->voicev);
  }
  
  free(synth);
}

/* New.
 */
 
struct synth_minor *synth_minor_new(int rate,int chanc) {
  if ((rate<200)||(rate>200000)) return 0;
  if ((chanc<1)||(chanc>2)) return 0;
  struct synth_minor *synth=calloc(1,sizeof(struct synth_minor));
  if (!synth) return 0;
  
  synth->rate=rate;
  synth->chanc=chanc;
  synth->qlevel=32000;
  synth->bend=1.0f;
  
  synth->bufa=1024;
  if (!(synth->buf=malloc(synth->bufa*sizeof(float)))) {
    synth_minor_del(synth);
    return 0;
  }
  
  return synth;
}

/* Update.
 */
 
static void synth_minor_update_signal(float *v,int c,struct synth_minor *synth) {
  memset(v,0,sizeof(float)*c);
  int i=synth->voicec;
  struct sm_voice *voice=synth->voicev+i-1;
  for (;i-->0;voice--) {
    if (sm_voice_update(v,c,voice)<=0) {
      sm_voice_cleanup(voice);
      synth->voicec--;
      memmove(voice,voice+1,sizeof(struct sm_voice)*(synth->voicec-i));
    }
  }
}

// also expands stereo; internally we are mono-only
static void synth_minor_quantize_signal(int16_t *dst,const float *src,int framec,int16_t qlevel,int chanc) {
  if (chanc==2) {
    for (;framec-->0;dst+=2,src++) {
      if (*src>=1.0f) dst[0]=dst[1]=qlevel;
      else if (*src<=-1.0f) dst[0]=dst[1]=-qlevel;
      else dst[0]=dst[1]=(*src)*qlevel;
    }
  } else {
    for (;framec-->0;dst++,src++) {
      if (*src>=1.0f) *dst=qlevel;
      else if (*src<=-1.0f) *dst=-qlevel;
      else *dst=(*src)*qlevel;
    }
  }
}

void synth_minor_update(int16_t *v,int c,struct synth_minor *synth) {
  int framec=c/synth->chanc;
  while (framec>0) {
    int updc=synth->bufa;
    if (updc>framec) updc=framec;
    synth_minor_update_signal(synth->buf,updc,synth);
    synth_minor_quantize_signal(v,synth->buf,updc,synth->qlevel,synth->chanc);
    v+=updc*synth->chanc;
    framec-=updc;
  }
}

/* Voice list.
 */
 
static struct sm_voice *sm_voice_new(struct synth_minor *synth,uint8_t chid,uint8_t noteid) {
  if (synth->voicec>=synth->voicea) {
    int na=synth->voicea+8;
    if (na>INT_MAX/sizeof(struct sm_voice)) return 0;
    void *nv=realloc(synth->voicev,sizeof(struct sm_voice)*na);
    if (!nv) return 0;
    synth->voicev=nv;
    synth->voicea=na;
  }
  struct sm_voice *voice=synth->voicev+synth->voicec++;
  memset(voice,0,sizeof(struct sm_voice));
  voice->chid=chid;
  voice->noteid=noteid;
  voice->bend=synth->bend;
  return voice;
}

static struct sm_voice *sm_voice_find(const struct synth_minor *synth,uint8_t chid,uint8_t noteid) {
  if (chid>16) return 0;
  struct sm_voice *voice=synth->voicev;
  int i=synth->voicec;
  for (;i-->0;voice++) {
    if (voice->chid!=chid) continue;
    if (voice->noteid!=noteid) continue;
    return voice;
  }
  return 0;
}

/* Note Off.
 */
 
static void synth_minor_note_off(struct synth_minor *synth,uint8_t chid,uint8_t noteid,uint8_t velocity) {
  struct sm_voice *voice=sm_voice_find(synth,chid,noteid);
  if (!voice) return;
  voice->chid=0xff;
  voice->noteid=0xff;
  voice->ttl=synth->rate/5;
  voice->stoplevel=0.0f;
  voice->dlevel=-voice->level/voice->ttl;
}

/* Note On.
 */
 
static void synth_minor_note_on(struct synth_minor *synth,uint8_t chid,uint8_t noteid,uint8_t velocity) {
  if (chid>=16) return;
  if (noteid&0x80) return;
  struct sm_voice *voice=sm_voice_new(synth,chid,noteid);
  if (!voice) return;
  int attack_framec_lo=synth->rate/10;
  int attack_framec_hi=synth->rate/100;
  int attack_framec=(attack_framec_lo*(127-velocity)+attack_framec_hi*velocity)>>7;
  float hz=midi_note_frequency[noteid];
  voice->dp=(hz*M_PI*2.0f)/synth->rate;
  voice->level=0.0f;
  const float levello=0.05f,levelhi=0.15f;
  if (velocity<=0) voice->stoplevel=levello;
  else if (velocity>=127) voice->stoplevel=levelhi;
  else voice->stoplevel=levello+(velocity*(levelhi-levello))/127.0f;
  voice->dlevel=voice->stoplevel/attack_framec;
}

/* Wheel.
 */
 
static void synth_minor_wheel(struct synth_minor *synth,uint8_t chid,uint16_t wheeli) {
  const float range=0.200f;
  synth->bend=powf(2.0f,(range*((float)wheeli-(float)0x2000))/(float)0x2000);
  struct sm_voice *voice=synth->voicev;
  int i=synth->voicec;
  for (;i-->0;voice++) voice->bend=synth->bend;
}

/* Event.
 */

void synth_minor_event(struct synth_minor *synth,const struct midi_event *event) {
  //fprintf(stderr,"%s %02x %02x %02x %02x\n",__func__,event->opcode,event->chid,event->a,event->b);
  //TODO channel config... all that basic synth stuff
  switch (event->opcode) {
    case MIDI_OPCODE_NOTE_OFF: synth_minor_note_off(synth,event->chid,event->a,event->b); break;
    case MIDI_OPCODE_NOTE_ON: synth_minor_note_on(synth,event->chid,event->a,event->b); break;
    case MIDI_OPCODE_WHEEL: synth_minor_wheel(synth,event->chid,event->a|(event->b<<7)); break;
  }
}
