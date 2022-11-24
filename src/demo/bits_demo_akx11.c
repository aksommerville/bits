#if BITS_USE_platform_linux || BITS_USE_platform_linux_akx11

#include "platform/linux/akx11/akx11.h"
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <GL/gl.h>

#define _akx11_demo_iconw 16
#define _akx11_demo_iconh 16
static const unsigned char _akx11_demo_icon[_akx11_demo_iconw*_akx11_demo_iconh*4]={
#define _ 0,0,0,0,
#define K 0,0,0,255,
#define W 255,255,255,255,
#define R 255,0,0,255,
#define G 0,255,0,255,
#define B 0,0,255,255,
  _ _ K K K K K K K K K K K K _ _
  _ K R R R R R R G G G G G G K _
  K R W W R R R R G G G W G G G K
  K R W R W R R R G G W G G G G K
  K R W W R R R R G G W G W G G K
  K R W R W R R R G G W G W G G K
  K R W R W R R R G G G W W G G K
  K R R R R R R R G G G G G G G K
  K B B B B B B B _ _ _ _ _ _ _ K
  K B B W W B B B _ _ _ W _ _ _ K
  K B B W B W B B _ _ W _ W _ _ K
  K B B W W B B B _ _ W W W _ _ K
  K B B W B W B B _ _ W _ W _ _ K
  K B B W W B B B _ _ W _ W _ _ K
  _ K B B B B B B _ _ _ _ _ _ K _
  _ _ K K K K K K K K K K K K _ _
};

static struct akx11 *akx11=0;
static int _akx11_demo_quit=0;
static int _akx11_demo_framec=0;

static void _cb_close(void *userdata) {
  fprintf(stderr,"%s\n",__func__);
  _akx11_demo_quit=1;
}

static void _cb_focus(void *userdata,int focus) {
  fprintf(stderr,"%s %d\n",__func__,focus);
}

static void _cb_resize(void *userdata,int w,int h) {
  //fprintf(stderr,"%s %d,%d\n",__func__,w,h);
}

static int _cb_key(void *userdata,int keycode,int value) {
  fprintf(stderr,"%s 0x%08x=%d\n",__func__,keycode,value);
  return 0; // nonzero to ack; then no text event
}

static void _cb_text(void *userdata,int codepoint) {
  fprintf(stderr,"%s U+%x\n",__func__,codepoint);
}

static void _cb_mmotion(void *userdata,int x,int y) {
  //fprintf(stderr,"%s %d,%d\n",__func__,x,y);
}

static void _cb_mbutton(void *userdata,int btnid,int value) {
  fprintf(stderr,"%s %d=%d\n",__func__,btnid,value);
}

static void _cb_mwheel(void *userdata,int dx,int dy) {
  fprintf(stderr,"%s %+d,%+d\n",__func__,dx,dy);
}

// WARNING: We don't check output bounds. Ensure you ask for something valid.
static void _akx11_demo_render_glyph(
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

static void _akx11_demo_render_rgb(uint32_t *dst,int dstw,int dsth,int rshift,int gshift,int bshift) {
  memset(dst,0x80,dstw*dsth*4);
  
  _akx11_demo_render_glyph(
    dst+20*dstw+20,dstw,
    0xff<<rshift,
    "XXX \n"
    "X  X\n"
    "XXX \n"
    "X  X\n"
    "X  X\n"
  );
  _akx11_demo_render_glyph(
    dst+20*dstw+25,dstw,
    0xff<<gshift,
    " XX \n"
    "X   \n"
    "X XX\n"
    "X  X\n"
    " XX \n"
  );
  _akx11_demo_render_glyph(
    dst+20*dstw+30,dstw,
    0xff<<bshift,
    "XXX \n"
    "X  X\n"
    "XXX \n"
    "X  X\n"
    "XXX \n"
  );
  _akx11_demo_render_glyph(
    dst+20*dstw+35,dstw,
    0xffffffff,
    "W   W\n"
    "W   W\n"
    "W W W\n"
    "W W W\n"
    " W W \n"
  );
  _akx11_demo_render_glyph(
    dst+20*dstw+41,dstw,
    0,
    "X  X\n"
    "X X \n"
    "XX  \n"
    "X X \n"
    "X  X\n"
  );
}

static void _akx11_demo_render() {
  _akx11_demo_framec++;
  // Typical clients use GX or FB, a static choice, and don't need to check every time.
  int mode=akx11_get_video_mode(akx11);
  switch (mode) {
  
    case AKX11_VIDEO_MODE_OPENGL: {
        if (akx11_begin_gx(akx11)<0) {
          fprintf(stderr,"akx11_begin_gx failed!\n");
          _akx11_demo_quit=1;
          return;
        }
        glClearColor(0.5f,0.25f,0.0f,1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        glBegin(GL_TRIANGLES);
          glColor4ub(0xff,0x00,0x00,0xff); glVertex2f( 0.0f, 0.8f); // red at the top
          glColor4ub(0x00,0xff,0x00,0xff); glVertex2f(-0.8f,-0.8f); // green lower left
          glColor4ub(0x00,0x00,0xff,0xff); glVertex2f( 0.8f,-0.8f); // blue lower right
        glEnd();
        akx11_end_gx(akx11);
      } break;
      
    case AKX11_VIDEO_MODE_FB_PURE:
    case AKX11_VIDEO_MODE_FB_GX: {
        void *fb=akx11_begin_fb(akx11);
        if (!fb) {
          fprintf(stderr,"akx11_begin_fb failed!\n");
          _akx11_demo_quit=1;
          return;
        }
        // Again, kind of weird for a client to ask for this instead of dictating at new:
        int fbfmt=akx11_get_fbfmt(akx11);
        int fbw,fbh;
        akx11_get_fb_size(&fbw,&fbh,akx11);
        switch (fbfmt) {
          case AKX11_FBFMT_XRGB: _akx11_demo_render_rgb(fb,fbw,fbh,16,8,0); break;
          case AKX11_FBFMT_XBGR: _akx11_demo_render_rgb(fb,fbw,fbh,0,8,16); break;
          case AKX11_FBFMT_RGBX: _akx11_demo_render_rgb(fb,fbw,fbh,24,16,8); break;
          case AKX11_FBFMT_BGRX: _akx11_demo_render_rgb(fb,fbw,fbh,8,16,24); break;
          default: memset(fb,_akx11_demo_framec,akx11_fbfmt_measure_buffer(fbfmt,fbw,fbh));
        }
        akx11_end_fb(akx11,fb);
      } break;
      
  }
}

int bits_demo_akx11(int argc,char **argv) {
  struct akx11_delegate delegate={
    .close=_cb_close,
    .focus=_cb_focus,
    .resize=_cb_resize,
    .key=_cb_key,
    .text=_cb_text,
    .mmotion=_cb_mmotion,
    .mbutton=_cb_mbutton,
    .mwheel=_cb_mwheel,
  };
  struct akx11_setup setup={
    .title="Bits Demo",
    .iconrgba=_akx11_demo_icon,
    .iconw=_akx11_demo_iconw,
    .iconh=_akx11_demo_iconh,
    .w=640,
    .h=360,
    .fbw=160,
    .fbh=90,
    .fullscreen=0,
    .video_mode=AKX11_VIDEO_MODE_AUTO,
    .fbfmt=AKX11_FBFMT_ANYTHING,
    .scale_limit=8,
    .display=0,
  };
  if (!(akx11=akx11_new(&delegate,&setup))) {
    fprintf(stderr,"akx11_new failed\n");
    return 1;
  }
  while (!_akx11_demo_quit) {
    if (akx11_update(akx11)<0) {
      fprintf(stderr,"akx11_update failed\n");
      akx11_del(akx11);
      return 1;
    }
    _akx11_demo_render();
    usleep(20000);
  }
  akx11_del(akx11);
  return 0;
}

#else
int bits_demo_akx11(int argc,char **argv) { return 1; }
#endif
