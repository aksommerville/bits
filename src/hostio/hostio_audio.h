/* hostio_audio.h
 */
 
#ifndef HOSTIO_AUDIO_H
#define HOSTIO_AUDIO_H

#include <stdint.h>

struct hostio_audio;
struct hostio_audio_type;
struct hostio_audio_delegate;
struct hostio_audio_setup;

struct hostio_audio {
  const struct hostio_audio_type *type;
  struct hostio_audio_delegate delegate;
  int rate,chanc;
  int playing;
};

struct hostio_audio_delegate {
  void *userdata;
  void (*cb_pcm_out)(int16_t *v,int c,struct hostio_audio *driver);
};

struct hostio_audio_setup {
  int rate;
  int chanc;
  const char *device;
  int buffer_size;
};

struct hostio_audio_type {
  const char *name;
  const char *desc;
  int objlen;
  int appointment_only;
  void (*del)(struct hostio_audio *driver);
  int (*init)(struct hostio_audio *driver,const struct hostio_audio_setup *setup);
  int (*update)(struct hostio_audio *driver);
  int (*lock)(struct hostio_audio *driver);
  void (*unlock)(struct hostio_audio *driver);
  void (*play)(struct hostio_audio *driver,int play);
};

const struct hostio_audio_type *hostio_audio_type_by_index(int p);
const struct hostio_audio_type *hostio_audio_type_by_name(const char *name,int namec);

#endif
