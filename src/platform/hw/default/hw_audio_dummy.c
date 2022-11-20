/* hw_audio_dummy.c
 * Trivial PCM driver that generates a signal on update, and throws it away.
 * Could easily be modified to record the signal, or to measure performance.
 * Most audio drivers will prefer an IO thread, and use lock/unlock instead of update.
 * Unlike most "dummy" drivers, this one is *not* marked request_only.
 * If audio really is unavailable on the host, it's usually preferable to proceed without sound. This driver facilitates that.
 */

#include "platform/hw/hw.h"
#include "platform/hw/hw_audio.h"
#include <sys/time.h>

#define HW_AUDIO_DUMMY_RATE_MIN       200
#define HW_AUDIO_DUMMY_RATE_DEFAULT 44100
#define HW_AUDIO_DUMMY_RATE_MAX    200000
#define HW_AUDIO_DUMMY_CHANC_MIN     1
#define HW_AUDIO_DUMMY_CHANC_DEFAULT 1
#define HW_AUDIO_DUMMY_CHANC_MAX     8

#define HW_AUDIO_DUMMY_BUFFER_SIZE 1024 /* samples */

/* Any longer than this between updates and we assume the clock is broken.
 * (rather than generating enough PCM to fill the potentially enormous interval)
 */
#define HW_AUDIO_DUMMY_PANIC_TIME_US 500000

struct hw_audio_dummy {
  struct hw_audio hdr;
  int16_t buffer[HW_AUDIO_DUMMY_BUFFER_SIZE];
  int bufframec;
  int bufsamplec; // <=HW_AUDIO_DUMMY_BUFFER_SIZE, and a multiple of chanc
  int64_t lasttime; // us
};

#define AUDIO ((struct hw_audio_dummy*)audio)

static int64_t hw_audio_dummy_now() {
  struct timeval tv={0};
  gettimeofday(&tv,0);
  return (int64_t)tv.tv_sec*1000000+tv.tv_usec;
}

static void _hw_audio_dummy_del(struct hw_audio *audio) {
}

static int _hw_audio_dummy_init(
  struct hw_audio *audio,
  const struct hw_audio_setup *setup
) {

  if (audio->rate<=0) audio->rate=HW_AUDIO_DUMMY_RATE_DEFAULT;
  else if (audio->rate<HW_AUDIO_DUMMY_RATE_MIN) audio->rate=HW_AUDIO_DUMMY_RATE_MIN;
  else if (audio->rate>HW_AUDIO_DUMMY_RATE_MAX) audio->rate=HW_AUDIO_DUMMY_RATE_MAX;
  
  if (audio->chanc<=0) audio->chanc=HW_AUDIO_DUMMY_CHANC_DEFAULT;
  else if (audio->chanc<HW_AUDIO_DUMMY_CHANC_MIN) audio->chanc=HW_AUDIO_DUMMY_CHANC_MIN;
  else if (audio->chanc>HW_AUDIO_DUMMY_CHANC_MAX) audio->chanc=HW_AUDIO_DUMMY_CHANC_MAX;
  
  AUDIO->bufframec=HW_AUDIO_DUMMY_BUFFER_SIZE/audio->chanc;
  AUDIO->bufsamplec=AUDIO->bufsamplec*audio->chanc;

  return 0;
}

static int _hw_audio_dummy_update(struct hw_audio *audio) {

  // Clock initialization is deferred to the first update, so we don't count any subsequent startup time.
  if (!AUDIO->lasttime) {
    AUDIO->lasttime=hw_audio_dummy_now();
    return 0;
  }
  
  int64_t now=hw_audio_dummy_now();
  int64_t elapsedus=now-AUDIO->lasttime;
  AUDIO->lasttime=now;
  if ((elapsedus<=0)||(elapsedus>HW_AUDIO_DUMMY_PANIC_TIME_US)) {
    // Clock fault or long-running op on this thread. Reset and don't generate any PCM.
    return 0;
  }
  
  if (!audio->playing) return 0;
  
  int framec=(elapsedus*audio->rate)/1000000;
  if (framec<1) return 0;
  int samplec=framec*audio->chanc;
  
  if (!audio->delegate->pcm_out) return 0;
  while (samplec>AUDIO->bufsamplec) {
    audio->delegate->pcm_out(AUDIO->buffer,AUDIO->bufsamplec,audio->delegate->userdata);
    samplec-=AUDIO->bufsamplec;
  }
  if (samplec>0) {
    audio->delegate->pcm_out(AUDIO->buffer,samplec,audio->delegate->userdata);
  }

  return 0;
}

const struct hw_audio_type hw_audio_type_dummy={
  .name="dummy",
  .desc="Fake audio driver that keeps accurate time but discards output.",
  .objlen=sizeof(struct hw_audio_dummy),
  .request_only=0,
  .del=_hw_audio_dummy_del,
  .init=_hw_audio_dummy_init,
  .update=_hw_audio_dummy_update,
};
