#if BITS_USE_platform_linux || BITS_USE_platform_linux_akdrm

#include "platform/linux/akdrm/akdrm.h"
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <signal.h>
#include <GL/gl.h>

static struct akdrm *akdrm=0;
static volatile int _akdrm_demo_quit=0;
static int _akdrm_demo_framec=0;

static void _cb_signal(int sigid) {
  switch (sigid) {
    case SIGINT: if (++_akdrm_demo_quit>=3) {
        fprintf(stderr,"Too many unprocessed signals.\n");
        exit(1);
      } break;
  }
}

// WARNING: We don't check output bounds. Ensure you ask for something valid.
static void _akdrm_demo_render_glyph(
  uint32_t *dst,int stride,
  uint32_t pixel,
  const char *image
) {
  for (;*image;dst+=stride) {
    uint32_t *dstp=dst;
    for (;(*image)&&(*image!='\n');image++) {
      if (*image!=' ') *dstp=pixel;
      dstp++;
    }
    if (*image=='\n') image++;
  }
}

static void _akdrm_demo_render_rgb(uint32_t *dst,int dstw,int dsth,int rshift,int gshift,int bshift) {
  memset(dst,0x80,dstw*dsth*4);
  
  _akdrm_demo_render_glyph(
    dst+20*dstw+20,dstw,
    0xff<<rshift,
    "XXX \n"
    "X  X\n"
    "XXX \n"
    "X  X\n"
    "X  X\n"
  );
  _akdrm_demo_render_glyph(
    dst+20*dstw+25,dstw,
    0xff<<gshift,
    " XX \n"
    "X   \n"
    "X XX\n"
    "X  X\n"
    " XX \n"
  );
  _akdrm_demo_render_glyph(
    dst+20*dstw+30,dstw,
    0xff<<bshift,
    "XXX \n"
    "X  X\n"
    "XXX \n"
    "X  X\n"
    "XXX \n"
  );
  _akdrm_demo_render_glyph(
    dst+20*dstw+35,dstw,
    0xffffffff,
    "W   W\n"
    "W   W\n"
    "W W W\n"
    "W W W\n"
    " W W \n"
  );
  _akdrm_demo_render_glyph(
    dst+20*dstw+41,dstw,
    0,
    "X  X\n"
    "X X \n"
    "XX  \n"
    "X X \n"
    "X  X\n"
  );
}

static void _akdrm_demo_render() {
  _akdrm_demo_framec++;
  // Typical clients use GX or FB, a static choice, and don't need to check every time.
  int mode=akdrm_get_video_mode(akdrm);
  switch (mode) {
  
    case AKDRM_VIDEO_MODE_OPENGL: {
        if (akdrm_begin_gx(akdrm)<0) {
          fprintf(stderr,"akdrm_begin_gx failed!\n");
          _akdrm_demo_quit=1;
          return;
        }
        glClearColor(0.5f,0.25f,0.0f,1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        glBegin(GL_TRIANGLES);
          glColor4ub(0xff,0x00,0x00,0xff); glVertex2f( 0.0f, 0.8f); // red at the top
          glColor4ub(0x00,0xff,0x00,0xff); glVertex2f(-0.8f,-0.8f); // green lower left
          glColor4ub(0x00,0x00,0xff,0xff); glVertex2f( 0.8f,-0.8f); // blue lower right
        glEnd();
        akdrm_end_gx(akdrm);
      } break;
      
    case AKDRM_VIDEO_MODE_FB_PURE:
    case AKDRM_VIDEO_MODE_FB_GX: {
        void *fb=akdrm_begin_fb(akdrm);
        if (!fb) {
          fprintf(stderr,"akdrm_begin_fb failed!\n");
          _akdrm_demo_quit=1;
          return;
        }
        // Again, kind of weird for a client to ask for this instead of dictating at new:
        int fbfmt=akdrm_get_fbfmt(akdrm);
        int fbw,fbh;
        akdrm_get_fb_size(&fbw,&fbh,akdrm);
        switch (fbfmt) {
          case AKDRM_FBFMT_XRGB: _akdrm_demo_render_rgb(fb,fbw,fbh,16,8,0); break;
          case AKDRM_FBFMT_XBGR: _akdrm_demo_render_rgb(fb,fbw,fbh,0,8,16); break;
          case AKDRM_FBFMT_RGBX: _akdrm_demo_render_rgb(fb,fbw,fbh,24,16,8); break;
          case AKDRM_FBFMT_BGRX: _akdrm_demo_render_rgb(fb,fbw,fbh,8,16,24); break;
          default: memset(fb,_akdrm_demo_framec,akdrm_fbfmt_measure_buffer(fbfmt,fbw,fbh));
        }
        akdrm_end_fb(akdrm,fb);
      } break;
      
  }
}

int bits_demo_akdrm(int argc,char **argv) {
  signal(SIGINT,_cb_signal);
  
  // Find a suitable device and mode.
  struct akdrm_config config={0};
  if (akdrm_find_device(&config,0,60,1920,1080,160,90)<0) {
    fprintf(stderr,"Failed to locate any suitable DRM device.\n");
    return 1;
  }
  fprintf(stderr,
    "Found device: %s rate=%d size=%d,%d flags=0x%08x type=0x%08x\n",
    config.path,config.rate,config.w,config.h,config.flags,config.type
  );
  
  struct akdrm_setup setup={
    .fbw=160,
    .fbh=90,
    .video_mode=AKDRM_VIDEO_MODE_AUTO,
    .fbfmt=AKDRM_FBFMT_ANYTHING,
  };
  akdrm=akdrm_new(&config,&setup);
  if (!akdrm) {
    fprintf(stderr,"%s: Failed to open DRM device.\n",config.path);
    return 1;
  }
  akdrm_config_cleanup(&config);
  
  fprintf(stderr,"Running. SIGINT to quit...\n");
  while (!_akdrm_demo_quit) {
    usleep(20000);
    _akdrm_demo_render();
  }
  
  akdrm_del(akdrm);
  return 0;
}

#else
int bits_demo_akdrm(int argc,char **argv) { return 1; }
#endif
