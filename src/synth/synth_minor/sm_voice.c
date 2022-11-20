#include "synth_minor_internal.h"

/* Cleanup.
 */
 
void sm_voice_cleanup(struct sm_voice *voice) {
}

/* Update.
 */
 
int sm_voice_update(float *v,int c,struct sm_voice *voice) {
  for (;c-->0;v++) {
    if (voice->ttl) {
      if (!--(voice->ttl)) return 0;
    }
    voice->p+=voice->dp*voice->bend;
    if (voice->p>=M_PI*2.0f) voice->p-=M_PI*2.0f;
    
    //(*v)+=sinf(voice->p)*voice->level; // sine wave, nice and neat
    (*v)+=((voice->p/M_PI)-1.0f)*voice->level; // saw, okey doke
    
    if (voice->dlevel>0.0f) {
      voice->level+=voice->dlevel;
      if (voice->level>=voice->stoplevel) {
        voice->level=voice->stoplevel;
        voice->stoplevel=voice->level*0.5f; // decay...
        voice->dlevel=(voice->stoplevel-voice->level)/1000;
      }
    } else if (voice->dlevel<0.0f) {
      voice->level+=voice->dlevel;
      if (voice->level<=voice->stoplevel) {
        voice->level=voice->stoplevel;
        voice->dlevel=0.0f;
      }
    }
  }
  return 1;
}
