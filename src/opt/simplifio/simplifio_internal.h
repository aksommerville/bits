#ifndef SIMPLIFIO_INTERNAL_H
#define SIMPLIFIO_INTERNAL_H

#include "simplifio.h"
#include <string.h>
#include <limits.h>
#include <stdlib.h>
#include <stdio.h>

/* TODO As I'm writing this, not all drivers are implemented yet (bcm and all the "mac" and "ms" ones).
 * Making some assumptions about what those will look like. Expect some touch-up.
 */

/* Convention is to leave "USE_" symbols unset if zero, and that's a good convention.
 * But we require them all to exist. So we define as zero here as needed.
 */
#if USE_pulse
  #include "opt/pulse/pulse.h"
#else
  #define USE_pulse 0
#endif
#if USE_alsafd
  #include "opt/alsafd/alsafd.h"
#else
  #define USE_alsafd 0
#endif
#if USE_asound
  #include "opt/asound/asound.h"
#else
  #define USE_asound 0
#endif
#if USE_macaudio
  #include "opt/macaudio/macaudio.h"
#else
  #define USE_macaudio 0
#endif
#if USE_msaudio
  #include "opt/msaudio/msaudio.h"
#else
  #define USE_msaudio 0
#endif
#if USE_evdev
  #include "opt/evdev/evdev.h"
#else
  #define USE_evdev 0
#endif
#if USE_machid
  #include "opt/machid/machid.h"
#else
  #define USE_machid 0
#endif
#if USE_mshid
  #include "opt/mshid/mshid.h"
#else
  #define USE_mshid 0
#endif
#if USE_glx
  #include "opt/glx/akglx.h"
#else
  #define USE_glx 0
#endif
#if USE_drmgx
  #include "opt/drmgx/drmgx.h"
#else
  #define USE_drmgx 0
#endif
#if USE_x11fb
  #include "opt/x11fb/x11fb.h"
#else
  #define USE_x11fb 0
#endif
#if USE_drmfb
  #include "opt/drmfb/drmfb.h"
#else
  #define USE_drmfb 0
#endif
#if USE_bcm
  #include "opt/bcm/bcm.h"
#else
  #define USE_bcm 0
#endif
#if USE_macwm
  #include "opt/macwm/macwm.h"
#else
  #define USE_macwm 0
#endif
#if USE_mswm
  #include "opt/mswm/mswm.h"
#else
  #define USE_mswm 0
#endif

extern struct simplifio {
  struct simplifio_delegate delegate;
  int input_devid;
  
  /* Audio drivers.
   * Listed here in the default preference order.
   */
  #if USE_pulse
    struct pulse *pulse;
  #endif
  #if USE_alsafd
    struct alsafd *alsafd;
  #endif
  #if USE_asound
    struct asound *asound;
  #endif
  #if USE_macaudio
    struct macaudio *macaudio;
  #endif
  #if USE_msaudio
    struct msaudio *msaudio;
  #endif
  
  /* Input drivers.
   * Listed here in the default preference order.
   */
  #if USE_evdev
    struct evdev *evdev;
  #endif
  #if USE_machid
    struct machid *machid;
  #endif
  #if USE_mshid
    struct mshid *mshid;
  #endif
  
  /* Video drivers.
   * Listed here in the default preference order.
   */
  #if USE_glx
    struct glx *glx;
  #endif
  #if USE_drmgx
    int drmgx; // nonzero=initialized
  #endif
  #if USE_x11fb
    struct x11fb *x11fb;
  #endif
  #if USE_drmfb
    struct drmfb *drmfb;
  #endif
  #if USE_bcm
    int bcm; // nonzero=initialized
  #endif
  #if USE_macwm
    struct macwm *macwm;
  #endif
  #if USE_mswm
    struct mswm *mswm;
  #endif
  
} simplifio;

#endif
