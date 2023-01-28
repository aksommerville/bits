#include "akdrm_internal.h"

/* Measure framebuffer.
 */
 
int akdrm_fbfmt_measure_stride(int fbfmt,int w) {
  if (w<1) return 0;
  if (w>INT_MAX>>2) return 0; // sanity limit
  switch (fbfmt) {
    case AKDRM_FBFMT_XRGB:
    case AKDRM_FBFMT_XBGR:
    case AKDRM_FBFMT_RGBX:
    case AKDRM_FBFMT_BGRX:
      return w<<2;
    case AKDRM_FBFMT_Y8:
      return w;
  }
  return 0;
}

int akdrm_fbfmt_measure_buffer(int fbfmt,int w,int h) {
  // All formats we support are stored LRTB, so this can be nice and simple.
  if (h<1) return 0;
  int stride=akdrm_fbfmt_measure_stride(fbfmt,w);
  if (stride<1) return 0;
  if (stride>INT_MAX/h) return 0;
  return stride*h;
}
