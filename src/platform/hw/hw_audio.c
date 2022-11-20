#include "hw_internal.h"

/* Object lifecycle.
 */
 
void hw_audio_del(struct hw_audio *audio) {
  if (!audio) return;
  if (audio->refc-->1) return;
  if (audio->type->del) audio->type->del(audio);
  free(audio);
}

int hw_audio_ref(struct hw_audio *audio) {
  if (!audio) return -1;
  if (audio->refc<1) return -1;
  if (audio->refc==INT_MAX) return -1;
  audio->refc++;
  return 0;
}

struct hw_audio *hw_audio_new(
  const struct hw_audio_type *type,
  const struct hw_delegate *delegate,
  const struct hw_audio_setup *setup
) {
  if (!type||!delegate) return 0;
  
  struct hw_audio *audio=calloc(1,type->objlen);
  if (!audio) return 0;
  
  audio->refc=1;
  audio->type=type;
  audio->delegate=delegate;
  
  if (setup) {
    audio->rate=setup->rate;
    audio->chanc=setup->chanc;
  }
  
  if (type->init&&(type->init(audio,setup)<0)) {
    hw_audio_del(audio);
    return 0;
  }
  
  return audio;
}

/* Hook wrappers.
 */

void hw_audio_play(struct hw_audio *audio,int play) {
  if (!audio) return;
  if (play) {
    if (audio->playing) return;
    if (audio->type->play) audio->type->play(audio,1);
    else audio->playing=1;
  } else {
    if (!audio->playing) return;
    if (audio->type->play) audio->type->play(audio,0);
    else audio->playing=0;
  }
}

int hw_audio_update(struct hw_audio *audio) {
  if (!audio) return -1;
  if (!audio->type->update) return 0;
  return audio->type->update(audio);
}

int hw_audio_lock(struct hw_audio *audio) {
  if (!audio) return -1;
  if (!audio->type->lock) return 0;
  return audio->type->lock(audio);
}

void hw_audio_unlock(struct hw_audio *audio) {
  if (!audio) return;
  if (!audio->type->unlock) return;
  audio->type->unlock(audio);
}
