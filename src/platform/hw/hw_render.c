#include "hw_internal.h"

/* Object lifecycle.
 */
 
void hw_render_del(struct hw_render *render) {
  if (!render) return;
  if (render->refc-->1) return;
  if (render->type->del) render->type->del(render);
  free(render);
}

int hw_render_ref(struct hw_render *render) {
  if (!render) return -1;
  if (render->refc<1) return -1;
  if (render->refc==INT_MAX) return -1;
  render->refc++;
  return 0;
}

struct hw_render *hw_render_new(
  const struct hw_render_type *type,
  const struct hw_delegate *delegate,
  const struct hw_render_setup *setup
) {
  if (!type||!delegate) return 0;
  
  struct hw_render *render=calloc(1,type->objlen);
  if (!render) return 0;
  
  render->refc=1;
  render->type=type;
  render->delegate=delegate;
  
  if (type->init&&(type->init(render,setup)<0)) {
    hw_render_del(render);
    return 0;
  }
  
  return render;
}

/* Hook wrappers.
 */
