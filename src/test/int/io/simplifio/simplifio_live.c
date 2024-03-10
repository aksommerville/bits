#include "test/test.h"
#include "opt/simplifio/simplifio.h"
#include <unistd.h>
#include <signal.h>
#include <math.h>

/* Test config.
 */

/* Simulate command-line options.
 * Must end with a comma if not empty.
 */
#define SIMPLIFIO_LIVE_ARGV 

#define SIMPLIFIO_LIVE_FRAME_SLEEP_US 16666

#define SIMPLIFIO_LIVE_ENABLE_OPENGL 1

/* Globals.
 */
 
static int window_closed=0;
static volatile int sigc=0;
static struct simplifio_fb_description fbdesc={0};
static int screenw=0,screenh=0;

/* Signals.
 */
 
static void rcvsig(int sigid) {
  switch (sigid) {
    case SIGINT: if (++sigc>=3) {
        fprintf(stderr,"Too many unprocessed signals.\n");
        exit(1);
      } break;
  }
}

/* Audio callback.
 * When you press 's', or we get any '1' value from general input, play a brief pleasant tone.
 */
 
static double synth_level=0.0; // with prequantization, ie 0..32767
static double synth_phase=0.0;
static const double synth_dphase=(M_PI*2.0*300.0)/44100.0; // assume we get 44.1k, pitch goes off if not, whatever.
static const double synth_decay=0.999700; // should depend on master rate, but whatever, higher rate and notes last longer.
static const double synth_threshold=0.0001;
 
static void cb_pcm_out(int16_t *v,int c,void *userdata) {
  //TODO we ought to check chanc... as is, it will sound an octave high if we get stereo instead of mono.
  while (c>0) {
    if (synth_level>synth_threshold) {
      double sample=sin(synth_phase)*synth_level;
      *v=(int16_t)sample;
      synth_phase+=synth_dphase;
      if (synth_phase>=M_PI) synth_phase-=M_PI*2.0;
      synth_level*=synth_decay;
      v++;
      c--;
    } else {
      synth_phase=0.0;
      memset(v,0,c<<1);
      c=0;
    }
  }
}

static void synth_play_note() {
  if (simplifio_audio_lock()>=0) {
    synth_level=20000.0;
    simplifio_audio_unlock();
  }
}

/* Callbacks.
 */
  
static void cb_connect(void *userdata,int devid) {
  fprintf(stderr,"%s %d\n",__func__,devid);
}

static void cb_disconnect(void *userdata,int devid) {
  fprintf(stderr,"%s %d\n",__func__,devid);
}

static void cb_button(void *userdata,int devid,int btnid,int value) {
  fprintf(stderr,"%s %d.%08x=%d\n",__func__,devid,btnid,value);
  if (value==1) synth_play_note();
}
  
static void cb_close(void *userdata) {
  fprintf(stderr,"%s\n",__func__);
  window_closed=1;
}

static void cb_focus(void *userdata,int focus) {
  fprintf(stderr,"%s %d\n",__func__,focus);
}

static void cb_resize(void *userdata,int w,int h) {
  fprintf(stderr,"%s %d,%d\n",__func__,w,h);
  screenw=w;
  screenh=h;
}

static int cb_key(void *userdata,int keycode,int value) {
  fprintf(stderr,"%s %08x=%d\n",__func__,keycode,value);
  return 0; // or >0 to acknowledge, and prevent cb_text from seeing it.
}

static void cb_text(void *userdata,int codepoint) {
  fprintf(stderr,"%s U+%x\n",__func__,codepoint);
  switch (codepoint) {
    case 0x1b: window_closed=1; break; // ESC == close window
    case 's': synth_play_note(); break;
    case 'f': simplifio_video_set_fullscreen(simplifio_video_get_fullscreen()?0:1); break;
  }
}

static void cb_mmotion(void *userdata,int x,int y) {
  fprintf(stderr,"%s %d,%d\n",__func__,x,y);
}

static void cb_mbutton(void *userdata,int btnid,int value) {
  fprintf(stderr,"%s %d=%d\n",__func__,btnid,value);
}

static void cb_mwheel(void *userdata,int dx,int dy) {
  fprintf(stderr,"%s %+d,%+d\n",__func__,dx,dy);
}

/* OpenGL cudgel.
 * We'll be rendering to a software framebuffer in any case.
 * Optionally, when only GX is available, render into a private framebuffer and send that downstream as an OpenGL texture.
 */
 
#if SIMPLIFIO_LIVE_ENABLE_OPENGL

#include <GL/gl.h>

static GLuint texid=0;
static void *oglfb=0;

static int opengl_init() {

  ASSERT_INTS_OP(fbdesc.w,>,0)
  ASSERT_INTS_OP(fbdesc.h,>,0)
  fbdesc.pixelsize=-1; // 32, but we use "-1" as a signal that opengl is in play
  fbdesc.stride=fbdesc.w<<2;
  fbdesc.rmask=0x000000ff;
  fbdesc.gmask=0x0000ff00;
  fbdesc.bmask=0x00ff0000;
  fbdesc.amask=0xff000000;
  memcpy(fbdesc.chorder,"rgba",4);
  ASSERT(oglfb=calloc(fbdesc.stride,fbdesc.h))
  
  glGenTextures(1,&texid);
  ASSERT(texid)
  glBindTexture(GL_TEXTURE_2D,texid);
  glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR); // LINEAR so we can tell at a glance that GX is in use.
  glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP_TO_EDGE);
  
  return 0;
}

static void deliver_oglfb() {
  glViewport(0,0,screenw,screenh);
  glClearColor(0.0f,0.0f,0.0f,1.0f);
  glClear(GL_COLOR_BUFFER_BIT);
  glBindTexture(GL_TEXTURE_2D,texid);
  glTexImage2D(GL_TEXTURE_2D,0,GL_RGB,fbdesc.w,fbdesc.h,0,GL_RGBA,GL_UNSIGNED_BYTE,oglfb);
  glEnable(GL_TEXTURE_2D);
  glDisable(GL_BLEND);
  int dstw=(screenh*fbdesc.w)/fbdesc.h,dsth;
  if (dstw<=screenw) { // pillarbox
    dsth=screenh;
  } else { // letterbox
    dstw=screenw;
    dsth=(screenw*fbdesc.h)/fbdesc.w;
  }
  GLfloat normw=(GLfloat)dstw/(GLfloat)screenw;
  GLfloat normh=(GLfloat)dsth/(GLfloat)screenh;
  glBegin(GL_TRIANGLE_STRIP);
    glColor4ub(0xff,0xff,0xff,0xff);
    glTexCoord2i(0,0); glVertex2f(-normw, normh);
    glTexCoord2i(0,1); glVertex2f(-normw,-normh);
    glTexCoord2i(1,0); glVertex2f( normw, normh);
    glTexCoord2i(1,1); glVertex2f( normw,-normh);
  glEnd();
}

#endif

/* Render scene.
 */
 
static int spritex=50,spritey=50;
static int spritedx=1,spritedy=1;

static void fill_rect(uint32_t *fb,int x,int y,int w,int h,uint32_t pixel) {
  if (x<0) { w+=x; x=0; }
  if (y<0) { h+=y; y=0; }
  if (x>fbdesc.w-w) w=fbdesc.w-x;
  if (y>fbdesc.h-h) h=fbdesc.h-y;
  if ((w<1)||(h<1)) return;
  int stridewords=fbdesc.stride>>2;
  fb+=y*stridewords+x;
  for (;h-->0;fb+=stridewords) {
    uint32_t *p=fb;
    int xi=w; for (;xi-->0;p++) *p=pixel;
  }
}
 
static void render_scene_inner(uint32_t *fb) {
  if (fbdesc.pixelsize!=32) return;
  
  fill_rect(fb,0,0,fbdesc.w,fbdesc.h,(
    (0x80808080&fbdesc.rmask)|
    (0x40404040&fbdesc.gmask)|
    fbdesc.amask
  ));
  
  int boxw=40;
  fill_rect(fb,(1*fbdesc.w)/4-(boxw>>1),(fbdesc.h>>1)-(boxw>>1),boxw,boxw,fbdesc.rmask|fbdesc.amask);
  fill_rect(fb,(2*fbdesc.w)/4-(boxw>>1),(fbdesc.h>>1)-(boxw>>1),boxw,boxw,fbdesc.gmask|fbdesc.amask);
  fill_rect(fb,(3*fbdesc.w)/4-(boxw>>1),(fbdesc.h>>1)-(boxw>>1),boxw,boxw,fbdesc.bmask|fbdesc.amask);
  
  spritex+=spritedx;
  if (spritex<0) { spritex=0; if (spritedx<0) spritedx=-spritedx; }
  else if (spritex>=fbdesc.w) { spritex=fbdesc.w-1; if (spritedx>0) spritedx=-spritedx; }
  spritey+=spritedy;
  if (spritey<0) { spritey=0; if (spritedy<0) spritedy=-spritedy; }
  else if (spritey>=fbdesc.h) { spritey=fbdesc.h-1; if (spritedy>0) spritedy=-spritedy; }
  fb[spritey*(fbdesc.stride>>2)+spritex]=0xffffffff;
}

/* Render one frame of video.
 */
 
static int render_scene() {
  if (fbdesc.pixelsize==-1) {
    ASSERT_CALL(simplifio_video_begin_gx())
    #if SIMPLIFIO_LIVE_ENABLE_OPENGL
      fbdesc.pixelsize=32;
      render_scene_inner(oglfb);
      fbdesc.pixelsize=-1;
      deliver_oglfb();
    #endif
    ASSERT_CALL(simplifio_video_end_gx())
  } else {
    void *fb=simplifio_video_begin_fb();
    ASSERT(fb)
    render_scene_inner(fb);
    ASSERT_CALL(simplifio_video_end_fb())
  }
  return 0;
}

/* Main.
 */
 
XXX_ITEST(simplifio_live) {
  fprintf(stderr,"%s...\n",__func__);
  
  signal(SIGINT,rcvsig);
  
  struct simplifio_delegate delegate={
    .cb_pcm_out=cb_pcm_out,
    .cb_connect=cb_connect,
    .cb_disconnect=cb_disconnect,
    .cb_button=cb_button,
    .cb_close=cb_close,
    .cb_focus=cb_focus,
    .cb_resize=cb_resize,
    .cb_key=cb_key,
    .cb_text=cb_text,
    .cb_mmotion=cb_mmotion,
    .cb_mbutton=cb_mbutton,
    .cb_mwheel=cb_mwheel,
  };
  struct simplifio_setup setup={
    // Of course in real life, you only need to call out the nonzero fields.
    // We list them all because we're a demo.
    .title="Simplifio Demo",
    .iconrgba=0,
    .iconw=0,
    .iconh=0,
    .fbw=256,
    .fbh=192,
    .audio_rate=44100,
    .audio_chanc=1,
    .audio_device=0,
    .audio_buffer_size=0,
    .input_path=0,
    .w=0,
    .h=0,
    .fullscreen=0,
    .video_device=0,
  };
  
  char *argv[]={(char*)__func__,SIMPLIFIO_LIVE_ARGV 0};
  int argc=sizeof(argv)/sizeof(void*)-1;
  ASSERT_CALL(argc=simplifio_setup_apply_argv(&setup,argc,argv))
  ASSERT_INTS(argc,1,"simplifio_setup_apply_argv didn't consume all arguments")
  
  ASSERT_CALL(simplifio_init(&delegate,&setup))
  
  // Log all the driver config that's publicly exposed.
  fprintf(stderr,"Audio driver: %s rate=%d chanc=%d\n",simplifio_audio_get_driver_name(),simplifio_audio_get_rate(),simplifio_audio_get_chanc());
  fprintf(stderr,"Input driver: %s\n",simplifio_input_get_driver_name());
  simplifio_video_get_size(&screenw,&screenh);
  fprintf(stderr,"Video driver: %s size=%d,%d\n",simplifio_video_get_driver_name(),screenw,screenh);
  
  // Confirm that at least one of fb or gx mode is available.
  if (simplifio_video_describe_fb(&fbdesc)>=0) {
    fprintf(stderr,"Using framebuffer: %dx%d %dbpp\n",fbdesc.w,fbdesc.h,fbdesc.pixelsize);
  } else if (simplifio_video_supports_gx()) {
    #if SIMPLIFIO_LIVE_ENABLE_OPENGL
      fbdesc.w=setup.fbw;
      fbdesc.h=setup.fbh;
      ASSERT_CALL(opengl_init())
      fprintf(stderr,"Using OpenGL\n");
    #else
      FAIL("GX is supported but this demo was built without ENABLE_OPENGL. see source")
    #endif
  } else {
    FAIL("Neither fb nor gx mode supported!\n");
  }
  
  simplifio_audio_play(1);
  
  fprintf(stderr,"%s: Running.\n",__func__);
  
  while (!sigc&&!window_closed) {
    ASSERT_CALL(simplifio_update())
    ASSERT_CALL(render_scene())
    usleep(SIMPLIFIO_LIVE_FRAME_SLEEP_US);
  }
  
  simplifio_quit();
  fprintf(stderr,"%s: Normal exit.\n",__func__);
  return 0;
}

/* A separate one-off thing to dump options.
 */
 
static void simplifio_options_cb(const char *k,const char *v,const char *desc,void *userdata) {
  if (v) {
    fprintf(stderr,"  %s=%s : %s\n",k,v,desc);
  } else {
    fprintf(stderr,"  %s : %s\n",k,desc);
  }
}
 
XXX_ITEST(simplifio_options) {
  simplifio_for_each_option(simplifio_options_cb,0);
  return 0;
}

/* And another to test option acquisition.
 * This one needn't be ignored by default.
 */
 
ITEST(simplifio_setup) {
  {
    char *argv[]={"/my/app","--fullscreen","true","--my-option=123","--video-driver=glx","myfile","--audio-driver","alsafd","myotherfile",0};
    int argc=sizeof(argv)/sizeof(void*)-1;
    struct simplifio_setup setup={0};
    ASSERT_CALL(argc=simplifio_setup_apply_argv(&setup,argc,argv))
    ASSERT_INTS(setup.fullscreen,1)
    ASSERT_STRINGS(setup.video_driver,-1,"glx",-1)
    ASSERT_STRINGS(setup.audio_driver,-1,"alsafd",-1)
    ASSERT_INTS(argc,4)
    ASSERT_STRINGS(argv[0],-1,"/my/app",-1)
    ASSERT_STRINGS(argv[1],-1,"--my-option=123",-1)
    ASSERT_STRINGS(argv[2],-1,"myfile",-1)
    ASSERT_STRINGS(argv[3],-1,"myotherfile",-1)
    ASSERT_NOT(argv[4])
    /**
    fprintf(stderr,"argv after applying:\n");
    int i=0; for (;i<argc;i++) fprintf(stderr,"  [%d] %s\n",i,argv[i]);
    fprintf(stderr,"selected setup:\n");
    fprintf(stderr,"  fullscreen: %d\n",setup.fullscreen);
    fprintf(stderr,"  video_driver: %s\n",setup.video_driver);
    fprintf(stderr,"  audio_driver: %s\n",setup.audio_driver);
    fprintf(stderr,"  audio_rate: %d\n",setup.audio_rate);
    /**/
  }
  return 0;
}
