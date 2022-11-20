#include "hw_internal.h"

/* Object lifecycle.
 */
 
void hw_synth_del(struct hw_synth *synth) {
  if (!synth) return;
  if (synth->refc-->1) return;
  if (synth->type->del) synth->type->del(synth);
  free(synth);
}

int hw_synth_ref(struct hw_synth *synth) {
  if (!synth) return -1;
  if (synth->refc<1) return -1;
  if (synth->refc==INT_MAX) return -1;
  synth->refc++;
  return 0;
}

struct hw_synth *hw_synth_new(
  const struct hw_synth_type *type,
  const struct hw_delegate *delegate,
  const struct hw_synth_setup *setup
) {
  if (!type||!delegate) return 0;
  
  struct hw_synth *synth=calloc(1,type->objlen);
  if (!synth) return 0;
  
  synth->refc=1;
  synth->type=type;
  synth->delegate=delegate;
  
  if (setup) {
    synth->rate=setup->rate;
    synth->chanc=setup->chanc;
  }
  
  if (type->init&&(type->init(synth)<0)) {
    hw_synth_del(synth);
    return 0;
  }
  
  return synth;
}

/* Hook wrappers.
 */

int hw_synth_configure(struct hw_synth *synth,const void *v,int c) {
  if (!synth) return -1;
  if (!synth->type->configure) return 0;
  return synth->type->configure(synth,v,c);
}

void hw_synth_update(int16_t *v,int c,struct hw_synth *synth) {
  if (!synth) return;
  if (!synth->type->update) return;
  synth->type->update(v,c,synth);
}

void hw_synth_events(struct hw_synth *synth,const void *v,int c) {
  if (!synth) return;
  if (!synth->type->events) return;
  synth->type->events(synth,v,c);
}
