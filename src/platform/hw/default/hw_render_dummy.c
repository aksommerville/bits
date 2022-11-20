#include "platform/hw/hw.h"
#include "platform/hw/hw_render.h"

struct hw_render_dummy {
  struct hw_render hdr;
};

#define RENDER ((struct hw_render_dummy*)render)

static void _hw_render_dummy_del(struct hw_render *render) {
}

static int _hw_render_dummy_init(
  struct hw_render *render,
  const struct hw_render_setup *setup
) {
  return 0;
}

const struct hw_render_type hw_render_type_dummy={
  .name="dummy",
  .desc="Dummy renderer.",
  .objlen=sizeof(struct hw_render_dummy),
  .request_only=1,
  .del=_hw_render_dummy_del,
  .init=_hw_render_dummy_init,
};
