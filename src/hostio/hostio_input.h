/* hostio_input.h
 */
 
#ifndef HOSTIO_AUDIO_H
#define HOSTIO_AUDIO_H

struct hostio_input;
struct hostio_input_type;
struct hostio_input_delegate;
struct hostio_input_setup;

struct hostio_input {
  const struct hostio_input_type *type;
  struct hostio_input_delegate delegate;
};

struct hostio_input_delegate {
  void *userdata;
  void (*cb_connect)(struct hostio_input *driver,int devid);
  void (*cb_disconnect)(struct hostio_input *driver,int devid);
  void (*cb_button)(struct hostio_input *driver,int devid,int btnid,int value);
};

struct hostio_input_setup {
  int dummy;
};

struct hostio_input_type {
  const char *name;
  const char *desc;
  int objlen;
  int appointment_only;
  void (*del)(struct hostio_input *driver);
  int (*init)(struct hostio_input *driver,const struct hostio_input_setup *setup);
  int (*update)(struct hostio_input *driver);
};

const struct hostio_input_type *hostio_input_type_by_index(int p);
const struct hostio_input_type *hostio_input_type_by_name(const char *name,int namec);

int hostio_input_devid_next();

#endif
