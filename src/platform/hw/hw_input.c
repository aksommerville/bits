#include "hw_internal.h"

/* Object lifecycle.
 */
 
void hw_input_del(struct hw_input *input) {
  if (!input) return;
  if (input->refc-->1) return;
  if (input->type->del) input->type->del(input);
  free(input);
}

int hw_input_ref(struct hw_input *input) {
  if (!input) return -1;
  if (input->refc<1) return -1;
  if (input->refc==INT_MAX) return -1;
  input->refc++;
  return 0;
}

struct hw_input *hw_input_new(
  const struct hw_input_type *type,
  const struct hw_delegate *delegate
) {
  if (!type||!delegate) return 0;
  
  struct hw_input *input=calloc(1,type->objlen);
  if (!input) return 0;
  
  input->refc=1;
  input->type=type;
  input->delegate=delegate;
  
  if (type->init&&(type->init(input)<0)) {
    hw_input_del(input);
    return 0;
  }
  
  return input;
}

/* Hook wrappers.
 */

int hw_input_update(struct hw_input *input) {
  if (!input) return -1;
  if (!input->type->update) return 0;
  return input->type->update(input);
}

const char *hw_input_get_ids(int *vid,int *pid,struct hw_input *input,int devid) {
  if (!input) return 0;
  if (!input->type->get_ids) return 0;
  int _vid; if (!vid) vid=&_vid;
  int _pid; if (!pid) pid=&_pid;
  return input->type->get_ids(vid,pid,input,devid);
}

int hw_input_enumerate(
  struct hw_input *input,
  int devid,
  int (*cb)(int btnid,int usage,int lo,int hi,int value,void *userdata),
  void *userdata
) {
  if (!input||!cb) return -1;
  if (!input->type->enumerate) return 0;
  return input->type->enumerate(input,devid,cb,userdata);
}
