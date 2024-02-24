/* hostio_video.h
 */
 
#ifndef HOSTIO_VIDEO_H
#define HOSTIO_VIDEO_H

struct hostio_video;
struct hostio_video_type;
struct hostio_video_delegate;
struct hostio_video_setup;

struct hostio_video {
  const struct hostio_video_type *type;
  struct hostio_video_delegate delegate;
  int w,h,fullscreen;
  int cursor_visible;
};

struct hostio_video_delegate {
  void *userdata;
  void (*cb_close)(struct hostio_video *driver);
  void (*cb_focus)(struct hostio_video *driver,int focus);
  void (*cb_resize)(struct hostio_video *driver,int w,int h);
  int (*cb_key)(struct hostio_video *driver,int keycode,int value);
  void (*cb_text)(struct hostio_video *driver,int codepoint);
  void (*cb_mmotion)(struct hostio_video *driver,int x,int y);
  void (*cb_mbutton)(struct hostio_video *driver,int btnid,int value);
  void (*cb_mwheel)(struct hostio_video *driver,int dx,int dy);
};

struct hostio_video_setup {
  const char *title;
  const void *iconrgba;
  int iconw,iconh;
  int w,h;
  int fullscreen;
  int fbw,fbh; // If (w,h) and (fullscreen) unspecified, let these guide window size decision.
  const char *device;
};

struct hostio_video_type {
  const char *name;
  const char *desc;
  int objlen;
  int appointment_only;
  int provides_input;
  void (*del)(struct hostio_video *driver);
  int (*init)(struct hostio_video *driver,const struct hostio_video_setup *setup);
  int (*update)(struct hostio_video *driver);
  void (*show_cursor)(struct hostio_video *driver,int show);
  void (*set_fullscreen)(struct hostio_video *driver,int fullscreen);
  void (*suppress_screensaver)(struct hostio_video *driver);
  int (*begin)(struct hostio_video *driver);
  int (*end)(struct hostio_video *driver);
};

const struct hostio_video_type *hostio_video_type_by_index(int p);
const struct hostio_video_type *hostio_video_type_by_name(const char *name,int namec);

#endif
