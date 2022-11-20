#include "platform/hw/hw.h"
#include "platform/hw/hw_video.h"
#include <stdlib.h>

#define HW_VIDEO_DUMMY_SCREEN_MIN        1
#define HW_VIDEO_DUMMY_SCREEN_DEFAULT  256
#define HW_VIDEO_DUMMY_SCREEN_MAX     4096
#define HW_VIDEO_DUMMY_FB_MIN            1
#define HW_VIDEO_DUMMY_FB_DEFAULT      256
#define HW_VIDEO_DUMMY_FB_MAX         1024

struct hw_video_dummy {
  struct hw_video hdr;
  void *fb;
};

#define VIDEO ((struct hw_video_dummy*)video)

static void _hw_video_dummy_del(struct hw_video *video) {
  if (VIDEO->fb) free(VIDEO->fb);
}

static int _hw_video_dummy_init(
  struct hw_video *video,
  const struct hw_video_setup *setup
) {

  if (video->w<=0) video->w=HW_VIDEO_DUMMY_SCREEN_DEFAULT;
  else if (video->w<HW_VIDEO_DUMMY_SCREEN_MIN) video->w=HW_VIDEO_DUMMY_SCREEN_MIN;
  else if (video->w>HW_VIDEO_DUMMY_SCREEN_MAX) video->w=HW_VIDEO_DUMMY_SCREEN_MAX;
  
  if (video->h<=0) video->h=HW_VIDEO_DUMMY_SCREEN_DEFAULT;
  else if (video->h<HW_VIDEO_DUMMY_SCREEN_MIN) video->h=HW_VIDEO_DUMMY_SCREEN_MIN;
  else if (video->h>HW_VIDEO_DUMMY_SCREEN_MAX) video->h=HW_VIDEO_DUMMY_SCREEN_MAX;
  
  video->fullscreen=1;
  
  switch (video->acceleration) {
    case HW_VIDEO_ACCELERATION_WHATEVER: video->acceleration=HW_VIDEO_ACCELERATION_DUMMY; break;
    case HW_VIDEO_ACCELERATION_FB: video->acceleration=HW_VIDEO_ACCELERATION_FB_XRGB; break;
    case HW_VIDEO_ACCELERATION_GX: video->acceleration=HW_VIDEO_ACCELERATION_DUMMY; break;
  }
  if (hw_video_acceleration_is_gx(video->acceleration)) {
    // We will pretend to support every GX regime, by doing nothing.
  } else if (hw_video_acceleration_is_fb(video->acceleration)) {
    
    if (video->fbw<=0) video->fbw=HW_VIDEO_DUMMY_FB_DEFAULT;
    else if (video->fbw<HW_VIDEO_DUMMY_FB_MIN) video->fbw=HW_VIDEO_DUMMY_FB_MIN;
    else if (video->fbw>HW_VIDEO_DUMMY_FB_MAX) video->fbw=HW_VIDEO_DUMMY_FB_MAX;
    
    if (video->fbh<=0) video->fbw=HW_VIDEO_DUMMY_FB_DEFAULT;
    else if (video->fbh<HW_VIDEO_DUMMY_FB_MIN) video->fbh=HW_VIDEO_DUMMY_FB_MIN;
    else if (video->fbh>HW_VIDEO_DUMMY_FB_MAX) video->fbh=HW_VIDEO_DUMMY_FB_MAX;
    
    int fbsize=hw_video_acceleration_fb_size(video->acceleration,video->fbw,video->fbh);
    if (fbsize<1) return -1;
    if (!(VIDEO->fb=malloc(fbsize))) return -1;
  } else {
    // (acceleration) is not a known GX or Framebuffer mode, reject it.
    return -1;
  }

  return 0;
}

static int _hw_video_dummy_begin_gx(struct hw_video *video) {
  if (!hw_video_acceleration_is_gx(video->acceleration)) return -1;
  return 0;
}

static void _hw_video_dummy_end_gx(struct hw_video *video) {
}

static void *_hw_video_dummy_begin_fb(struct hw_video *video) {
  return VIDEO->fb; // If not in an FB mode, (VIDEO->fb) is null, which is the right answer.
}

static void _hw_video_dummy_end_fb(struct hw_video *video,void *fb) {
  if (!fb||(fb!=VIDEO->fb)) return;
}

const struct hw_video_type hw_video_type_dummy={
  .name="dummy",
  .desc="Fake video driver that does nothing.",
  .objlen=sizeof(struct hw_video_dummy),
  .request_only=1,
  .supplies_system_keyboard=0,
  .del=_hw_video_dummy_del,
  .init=_hw_video_dummy_init,
  .begin_gx=_hw_video_dummy_begin_gx,
  .end_gx=_hw_video_dummy_end_gx,
  .begin_fb=_hw_video_dummy_begin_fb,
  .end_fb=_hw_video_dummy_end_fb,
};
