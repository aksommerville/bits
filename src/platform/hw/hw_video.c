#include "hw_internal.h"
#include <limits.h>

/* Object lifecycle.
 */
 
void hw_video_del(struct hw_video *video) {
  if (!video) return;
  if (video->refc-->1) return;
  if (video->type->del) video->type->del(video);
  free(video);
}

int hw_video_ref(struct hw_video *video) {
  if (!video) return -1;
  if (video->refc<1) return -1;
  if (video->refc==INT_MAX) return -1;
  video->refc++;
  return 0;
}

struct hw_video *hw_video_new(
  const struct hw_video_type *type,
  const struct hw_delegate *delegate,
  const struct hw_video_setup *setup
) {
  if (!type||!delegate) return 0;
  
  struct hw_video *video=calloc(1,type->objlen);
  if (!video) return 0;
  
  video->refc=1;
  video->type=type;
  video->delegate=delegate;
  
  if (setup) {
    video->w=setup->w;
    video->h=setup->h;
    video->fbw=setup->fbw;
    video->fbh=setup->fbh;
    video->fullscreen=setup->fullscreen;
    video->acceleration=setup->acceleration;
  }
  
  if (type->init&&(type->init(video,setup)<0)) {
    hw_video_del(video);
    return 0;
  }
  
  return video;
}

/* Hook wrappers.
 */

int hw_video_update(struct hw_video *video) {
  if (!video) return -1;
  if (!video->type->update) return 0;
  return video->type->update(video);
}

void hw_video_set_fullscreen(struct hw_video *video,int state) {
  if (!video) return;
  if (!video->type->set_fullscreen) return;
  video->type->set_fullscreen(video,state);
}

void hw_video_suppress_screensaver(struct hw_video *video) {
  if (!video) return;
  if (!video->type->suppress_screensaver) return;
  video->type->suppress_screensaver(video);
}

int hw_video_begin_gx(struct hw_video *video) {
  if (!video) return -1;
  if (!video->type->begin_gx) return -1;
  return video->type->begin_gx(video);
}

void hw_video_end_gx(struct hw_video *video) {
  if (!video) return;
  if (!video->type->end_gx) return;
  video->type->end_gx(video);
}

void *hw_video_begin_fb(struct hw_video *video) {
  if (!video) return 0;
  if (!video->type->begin_fb) return 0;
  return video->type->begin_fb(video);
}

void hw_video_end_fb(struct hw_video *video,void *fb) {
  if (!video) return;
  if (!video->type->end_fb) return;
  video->type->end_fb(video,fb);
}

/* Acceleration enum helpers.
 */
 
int hw_video_acceleration_is_gx(int acceleration) {
  if (acceleration<100) return 0;
  if (acceleration>199) return 0;
  return 1;
}

int hw_video_acceleration_is_fb(int acceleration) {
  if (acceleration<200) return 0;
  if (acceleration>299) return 0;
  return 1;
}

int hw_video_acceleration_fb_size(int acceleration,int w,int h) {
  if ((w<1)||(h<1)) return 0;
  switch (acceleration) {
    // Anything not stored LRTB needs a special case here:
    case HW_VIDEO_ACCELERATION_FB_THUMBY: return ((h+7)>>3)*w;
    // Normal LRTB formats, lean on the stride calculator:
    default: {
        int stride=hw_video_acceleration_fb_stride(acceleration,w);
        if (stride<=0) return 0;
        if (stride>INT_MAX/h) return 0;
        return stride*h;
      }
  }
  return 0;
}

int hw_video_acceleration_fb_stride(int acceleration,int w) {
  if (w<1) return 0;
  switch (acceleration) {
    case HW_VIDEO_ACCELERATION_FB_RGBX: return w<<2;
    case HW_VIDEO_ACCELERATION_FB_BGRX: return w<<2;
    case HW_VIDEO_ACCELERATION_FB_XRGB: return w<<2;
    case HW_VIDEO_ACCELERATION_FB_XBGR: return w<<2;
    case HW_VIDEO_ACCELERATION_FB_Y8: return w;
    case HW_VIDEO_ACCELERATION_FB_I8: return w;
    case HW_VIDEO_ACCELERATION_FB_Y1BE: return (w+7)>>3;
    case HW_VIDEO_ACCELERATION_FB_BGR332BE: return w;
    case HW_VIDEO_ACCELERATION_FB_BGR565LE: return w<<1;
    case HW_VIDEO_ACCELERATION_FB_RGB565LE: return w<<1;
    case HW_VIDEO_ACCELERATION_FB_XBGR4444BE: return w<<1;
    case HW_VIDEO_ACCELERATION_FB_THUMBY: return w;
  }
  return 0;
}
