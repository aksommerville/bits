/* hw_render.h
 */
 
#ifndef HW_RENDER_H
#define HW_RENDER_H

struct hw_render {
  const struct hw_render_type *type;
  const struct hw_delegate *delegate;
  int refc;
};

struct hw_render_setup {
  int fbw,fbh;
  int fbfmt;
};

struct hw_render_type {
  const char *name;
  const char *desc;
  int objlen;
  int request_only;
  void (*del)(struct hw_render *render);
  int (*init)(struct hw_render *render,const struct hw_render_setup *setup);
  //TODO render API
};

void hw_render_del(struct hw_render *render);
int hw_render_ref(struct hw_render *render);

struct hw_render *hw_render_new(
  const struct hw_render_type *type,
  const struct hw_delegate *delegate,
  const struct hw_render_setup *setup
);

#endif
