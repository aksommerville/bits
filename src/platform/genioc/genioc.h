/* genioc.h
 * Generic Inversion of Control.
 * Use this on platforms that don't impose their own IoC regime, ie all but MacOS.
 * Lets you code in a similar way for all.
 * We are responsible for timing.
 */
 
#ifndef GENIOC_H
#define GENIOC_H

struct genioc_delegate {
  int rate; // hz, for update(). <=0 we will not update
  void *userdata;
  
  // Each will be called exactly once.
  void (*quit)(void *userdata);
  int (*init)(void *userdata);
  
  /* Called before init.
   * Return >0 if you consumed the argument, 0 to permit generic handling, or <0 for real immediate errors.
   */
  int (*arg)(void *userdata,const char *k,int kc,const char *v,int vc);
  
  // Called at the requested (rate). Where you should do most of the work.
  void (*update)(void *userdata);
  
  // Called instead of usleep() if set. eg if you want to insert a poller.
  int (*sleep)(void *userdata,int us);
};

int genioc_main(
  int argc,char **argv,
  const struct genioc_delegate *delegate
);

void genioc_terminate(int status);

#endif
