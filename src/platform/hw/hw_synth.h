/* hw_synth.h
 */
 
#ifndef HW_SYNTH_H
#define HW_SYNTH_H

struct hw_synth {
  const struct hw_synth_type *type;
  const struct hw_delegate *delegate;
  int refc;
  int rate;
  int chanc;
};

struct hw_synth_setup {
  int rate;
  int chanc;
};

struct hw_synth_type {
  const char *name;
  const char *desc;
  int objlen;
  int request_only;
  void (*del)(struct hw_synth *synth);
  int (*init)(struct hw_synth *synth);
  int (*configure)(struct hw_synth *synth,const void *v,int c);
  void (*update)(int16_t *v,int c,struct hw_synth *synth);
  void (*events)(struct hw_synth *synth,const void *v,int c);
};

void hw_synth_del(struct hw_synth *synth);
int hw_synth_ref(struct hw_synth *synth);

struct hw_synth *hw_synth_new(
  const struct hw_synth_type *type,
  const struct hw_delegate *delegate,
  const struct hw_synth_setup *setup
);

int hw_synth_configure(struct hw_synth *synth,const void *v,int c);
void hw_synth_update(int16_t *v,int c,struct hw_synth *synth);
void hw_synth_events(struct hw_synth *synth,const void *v,int c);

#endif
