#include "genioc_internal.h"
#include <unistd.h>
#include <time.h>
#include <sys/time.h>

// If the clock says to sleep one second or more, assume it's an error.
// We expect to see sleep times under 16ms.
#define GENIOC_SLEEP_LIMIT 1000000

/* Current real time.
 */
 
static int64_t genioc_clock_now() {
  struct timeval tv={0};
  gettimeofday(&tv,0);
  return (int64_t)tv.tv_sec*1000000ll+tv.tv_usec;
}

/* Init.
 */
 
void genioc_clock_init() {
  if (genioc.delegate.rate) genioc.frametime=1000000/genioc.delegate.rate;
  if (genioc.frametime<100) genioc.frametime=100000;
  genioc.starttime=genioc_clock_now();
  genioc.nexttime=genioc.starttime;
}

/* Update.
 */
 
void genioc_clock_update() {
  while (1) {
    int64_t now=genioc_clock_now();
    
    if (now>=genioc.nexttime) {
      genioc.nexttime+=genioc.frametime;
      if (now>=genioc.nexttime) {
        // More than one frame time elapsed. Reset clock.
        genioc.nexttime=now+genioc.frametime;
        genioc.clockfaultc++;
      }
      break;
    }
    
    int64_t sleeptime=genioc.nexttime-now;
    if (sleeptime>GENIOC_SLEEP_LIMIT) {
      // Unclear why this would happen. Reset clock.
      genioc.nexttime=now+genioc.frametime;
      genioc.clockfaultc++;
      break;
    }
    if (genioc.delegate.sleep) genioc.delegate.sleep(genioc.delegate.userdata,sleeptime);
    else usleep(sleeptime);
  }
}
