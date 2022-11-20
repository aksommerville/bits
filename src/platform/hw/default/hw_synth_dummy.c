#include "platform/hw/hw.h"
#include "platform/hw/hw_synth.h"
#include <string.h>

struct hw_synth_dummy {
  struct hw_synth hdr;
};

#define SYNTH ((struct hw_synth_dummy*)synth)

static void _hw_synth_dummy_del(struct hw_synth *synth) {
}

static int _hw_synth_dummy_init(struct hw_synth *synth) {
  return 0;
}

static int _hw_synth_dummy_configure(
  struct hw_synth *synth,
  const void *v,int c
) {
  return 0;
}

static void _hw_synth_dummy_update(int16_t *v,int c,struct hw_synth *synth) {
  memset(v,0,c<<1);
}

static void _hw_synth_dummy_events(
  struct hw_synth *synth,
  const void *v,int c
) {
}

const struct hw_synth_type hw_synth_type_dummy={
  .name="dummy",
  .desc="Dummy synthesizer.",
  .objlen=sizeof(struct hw_synth_dummy),
  .request_only=0,
  .del=_hw_synth_dummy_del,
  .init=_hw_synth_dummy_init,
  .configure=_hw_synth_dummy_configure,
  .update=_hw_synth_dummy_update,
  .events=_hw_synth_dummy_events,
};
