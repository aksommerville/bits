#include "test/test.h"
#include "opt/gcfg/gcfg.h"

ITEST(gcfg_path) {
  char path[1024];
  int pathc=gcfg_compose_path(path,sizeof(path),"bits",-1,"highscore",-1);
  ASSERT_CALL(pathc,"gcfg_compose_path")
  ASSERT_INTS_OP(pathc,<,sizeof(path))
  ASSERT_INTS(path[pathc],0)
  if ((pathc<30)||memcmp(path,"/home/",6)||memcmp(path+pathc-30,"/.config/aksomm/bits/highscore",30)) {
    FAIL("Unexpected gcfg path: '%.*s'",pathc,path)
  }
  return 0;
}

/* Appointment only.
 * Read the global input config file and dump it for perusal.
 */
XXX_ITEST(gcfg_read_input_config) {
  struct gcfg_input *input=gcfg_input_get();
  ASSERT(input)
  fprintf(stderr,"Acquired input config.\n");
  struct gcfg_input_device *device=input->devicev;
  int devicei=input->devicec;
  for (;devicei-->0;device++) {
    fprintf(stderr,"DEVICE %04x:%04x:%04x '%.*s'\n",device->header.vid,device->header.pid,device->header.version,device->header.namec,device->header.name);
    struct gcfg_input_button *button=device->buttonv;
    int buttoni=device->buttonc;
    for (;buttoni-->0;button++) {
      fprintf(stderr,"  %d=>%d(%.*s) '%.*s' (%.*s)\n",button->srcbtnid,button->btnid,button->btnnamec,button->btnname,button->namec,button->name,button->commentc,button->comment);
    }
  }
  gcfg_input_del(input);
  return 0;
}
