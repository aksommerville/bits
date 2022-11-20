/* hw_video.h
 */
 
#ifndef HW_VIDEO_H
#define HW_VIDEO_H

// Use one of these at init, to let the driver decide:
#define HW_VIDEO_ACCELERATION_WHATEVER  0
#define HW_VIDEO_ACCELERATION_FB        1
#define HW_VIDEO_ACCELERATION_GX        2
// You can also be specific. These are the symbols (acceleration) should end up at:
#define HW_VIDEO_ACCELERATION_OPENGL1       100
#define HW_VIDEO_ACCELERATION_OPENGL2       101
#define HW_VIDEO_ACCELERATION_OPENGLES2     102
#define HW_VIDEO_ACCELERATION_OPENVG        103
#define HW_VIDEO_ACCELERATION_METAL         104
#define HW_VIDEO_ACCELERATION_VULKAN        105
#define HW_VIDEO_ACCELERATION_DUMMY         106
#define HW_VIDEO_ACCELERATION_FB_RGBX       200
#define HW_VIDEO_ACCELERATION_FB_BGRX       201
#define HW_VIDEO_ACCELERATION_FB_XRGB       202
#define HW_VIDEO_ACCELERATION_FB_XBGR       203
#define HW_VIDEO_ACCELERATION_FB_Y8         204
#define HW_VIDEO_ACCELERATION_FB_I8         205
#define HW_VIDEO_ACCELERATION_FB_Y1BE       206
#define HW_VIDEO_ACCELERATION_FB_BGR332BE   207
#define HW_VIDEO_ACCELERATION_FB_BGR565LE   208
#define HW_VIDEO_ACCELERATION_FB_RGB565LE   209
#define HW_VIDEO_ACCELERATION_FB_XBGR4444BE 210
#define HW_VIDEO_ACCELERATION_FB_THUMBY     211

struct hw_video {
  const struct hw_video_type *type;
  const struct hw_delegate *delegate;
  int refc;
  int w,h; // window or screen
  int fbw,fbh; // framebuffer if applicable
  int fullscreen;
  int acceleration;
};

struct hw_video_setup {
  char *title;
  void *iconrgba;
  int iconw,iconh;
  int w,h;
  int fbw,fbh;
  int fullscreen;
  int acceleration;
};

struct hw_video_type {
  const char *name;
  const char *desc;
  int objlen;
  int request_only;
  int supplies_system_keyboard;
  void (*del)(struct hw_video *video);
  int (*init)(struct hw_video *video,const struct hw_video_setup *setup);
  int (*update)(struct hw_video *video);
  void (*set_fullscreen)(struct hw_video *video,int state);
  void (*suppress_screensaver)(struct hw_video *video);
  int (*begin_gx)(struct hw_video *video);
  void (*end_gx)(struct hw_video *video);
  void *(*begin_fb)(struct hw_video *video);
  void (*end_fb)(struct hw_video *video,void *fb);
};

void hw_video_del(struct hw_video *video);
int hw_video_ref(struct hw_video *video);

struct hw_video *hw_video_new(
  const struct hw_video_type *type,
  const struct hw_delegate *delegate,
  const struct hw_video_setup *setup
);

int hw_video_update(struct hw_video *video);
void hw_video_set_fullscreen(struct hw_video *video,int state);
void hw_video_suppress_screensaver(struct hw_video *video);
int hw_video_begin_gx(struct hw_video *video);
void hw_video_end_gx(struct hw_video *video);
void *hw_video_begin_fb(struct hw_video *video);
void hw_video_end_fb(struct hw_video *video,void *fb);

int hw_video_acceleration_is_gx(int acceleration);
int hw_video_acceleration_is_fb(int acceleration);
int hw_video_acceleration_fb_size(int acceleration,int w,int h); // => fb total size in bytes
int hw_video_acceleration_fb_stride(int acceleration,int w); // => fb stride in bytes

#endif
