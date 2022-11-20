/* hw_input.h
 */
 
#ifndef HW_INPUT_H
#define HW_INPUT_H

struct hw_input {
  const struct hw_input_type *type;
  const struct hw_delegate *delegate;
  int refc;
};

struct hw_input_type {
  const char *name;
  const char *desc;
  int objlen;
  int request_only;
  void (*del)(struct hw_input *input);
  int (*init)(struct hw_input *input);
  int (*update)(struct hw_input *input);
  const char *(*get_ids)(int *vid,int *pid,struct hw_input *input,int devid);
  int (*enumerate)(
    struct hw_input *input,
    int devid,
    int (*cb)(int btnid,int usage,int lo,int hi,int value,void *userdata),
    void *userdata
  );
};

void hw_input_del(struct hw_input *input);
int hw_input_ref(struct hw_input *input);

struct hw_input *hw_input_new(
  const struct hw_input_type *type,
  const struct hw_delegate *delegate
);

int hw_input_update(struct hw_input *input);
const char *hw_input_get_ids(int *vid,int *pid,struct hw_input *input,int devid);

int hw_input_enumerate(
  struct hw_input *input,
  int devid,
  int (*cb)(int btnid,int usage,int lo,int hi,int value,void *userdata),
  void *userdata
);

#endif
