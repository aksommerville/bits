/* hostio_video_dummy.c
 * Skeletal video driver, can be used as a stub, and commented as an example template.
 */

#include "hostio_internal.h"

/* Object definition.
 */
 
struct hostio_video_dummy {
  struct hostio_video hdr;
};

#define DRIVER ((struct hostio_video_dummy*)driver)

/* Delete.
 */
 
static void _dummy_del(struct hostio_video *driver) {
}

/* Init.
 */
 
static int _dummy_init(struct hostio_video *driver,const struct hostio_video_setup *setup) {

  // Respect setup as applicable:
  //   device, title, iconrgba

  // Try to respect setup if it requests a specific size.
  if ((setup->w>0)&&(setup->h>0)) {
    driver->w=setup->w;
    driver->h=setup->h;
    
  // If a framebuffer size is provided, try to match its aspect ratio.
  // Bonus points if you also produce an integer multiple of the framebuffer size.
  } else if ((setup->fbw>0)&&(setup->fbh>0)) {
    driver->w=setup->fbw;
    driver->h=setup->fbh;
    
  // No size guidance, that's perfectly legal. Make up something sensible.
  } else {
    driver->w=640;
    driver->h=360;
  }
  
  // Try to respect fullscreen if requested.
  if (setup->fullscreen) {
    driver->fullscreen=1;
  }
  
  // If your driver provides a cursor, arrange for it to start hidden.
  driver->cursor_visible=0;
  
  return 0;
}

/* Render fences.
 */
 
static int _dummy_begin(struct hostio_video *driver) {
  // Ensure GX context is active, etc.
  return 0;
}

static int _dummy_end(struct hostio_video *driver) {
  // Swap frames and whatever.
  return 0;
}

/* Type definition.
 */
 
const struct hostio_video_type hostio_video_type_dummy={
  .name="dummy",
  .desc="Fake video driver that does nothing.",
  .objlen=sizeof(struct hostio_video_dummy),
  .appointment_only=1, // Normally zero, ie do include in the default list.
  .provides_input=0, // Nonzero if you're backed by a window manager with system keyboard and/or pointer.
  .del=_dummy_del,
  .init=_dummy_init,
  
  // Must implement if (provides_input), and call delegate as appropriate:
  //int update(struct hostio_video *driver)
  
  // Optional, implement if they make sense:
  //void show_cursor(struct hostio_video *driver,int show);
  //void set_fullscreen(struct hostio_video *driver,int fullscreen);
  //void suppress_screensaver(struct hostio_video *driver);
  
  .begin=_dummy_begin,
  .end=_dummy_end,
};
