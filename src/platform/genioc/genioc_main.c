#include "genioc_internal.h"
#include <signal.h>
#include <unistd.h>
#include <stdlib.h>

struct genioc genioc={0};

/* Signal handler.
 */
 
static void genioc_rcvsig(int sigid) {
  switch (sigid) {
    case SIGINT: genioc.termstatus=1; if (++(genioc.terminate)>=3) {
        fprintf(stderr,"Too many unprocessed signals.\n");
        exit(1);
      } break;
  }
}

/* Argv.
 * (argv[0]) has already been yoinked for (exename) if applicable.
 */
 
static int genioc_process_argv(int argc,char **argv) {
  int argp=1;
  while (argp<argc) {
    const char *arg=argv[argp++];
    if (!arg||!arg[0]) continue;
    
    //TODO
    
    fprintf(stderr,"%s: Unexpected argument '%s'\n",genioc.exename,arg);
    return -1;
  }
  return 0;
}

/* Fill in delegate defaults.
 * The only required hook is (update).
 */
 
static void genioc_cb_update_default(void *userdata) {}
 
static void genioc_fill_in_delegate_defaults(struct genioc_delegate *delegate) {
  if (!delegate->update) delegate->update=genioc_cb_update_default;
}

/* Main.
 */

int genioc_main(
  int argc,char **argv,
  const struct genioc_delegate *delegate
) {
  
  if (genioc.exename) return 1; // reentry!
  if ((argc>=1)&&argv[0]) genioc.exename=argv[0];
  else genioc.exename="unknown";
  
  signal(SIGINT,genioc_rcvsig);
  
  if (delegate) genioc.delegate=*delegate;
  genioc_fill_in_delegate_defaults(&genioc.delegate);
  
  if (genioc_process_argv(argc,argv)<0) return 1;
  if (genioc.terminate) return genioc.termstatus;
  
  if (genioc.delegate.init) {
    if (genioc.delegate.init(genioc.delegate.userdata)<0) return 1;
    genioc.delegate.init=0;
  }
  
  genioc_clock_init();
  
  while (!genioc.terminate) {
    genioc.delegate.update(genioc.delegate.userdata);
    genioc_clock_update();
  }
  
  if (genioc.delegate.quit) {
    genioc.delegate.quit(genioc.delegate.userdata);
    genioc.delegate.quit=0;
  }
  
  return genioc.termstatus;
}

/* Trivial accessors.
 */

void genioc_terminate(int status) {
  genioc.terminate++;
  genioc.termstatus=status;
}
