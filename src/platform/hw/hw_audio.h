/* hw_audio.h
 */
 
#ifndef HW_AUDIO_H
#define HW_AUDIO_H

struct hw_audio {
  const struct hw_audio_type *type;
  const struct hw_delegate *delegate;
  int refc;
  int rate;
  int chanc;
  int playing;
};

struct hw_audio_setup {
  char *device;
  int rate;
  int chanc;
};

struct hw_audio_type {
  const char *name;
  const char *desc;
  int objlen;
  int request_only;
  void (*del)(struct hw_audio *audio);
  int (*init)(struct hw_audio *audio,const struct hw_audio_setup *setup);
  void (*play)(struct hw_audio *audio,int play);
  int (*update)(struct hw_audio *audio);
  int (*lock)(struct hw_audio *audio);
  void (*unlock)(struct hw_audio *audio);
};

void hw_audio_del(struct hw_audio *audio);
int hw_audio_ref(struct hw_audio *audio);

struct hw_audio *hw_audio_new(
  const struct hw_audio_type *type,
  const struct hw_delegate *delegate,
  const struct hw_audio_setup *setup
);

void hw_audio_play(struct hw_audio *audio,int play);
int hw_audio_update(struct hw_audio *audio);
int hw_audio_lock(struct hw_audio *audio);
void hw_audio_unlock(struct hw_audio *audio);

#endif
