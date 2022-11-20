#if BITS_USE_platform_linux || BITS_USE_platform_linux_alsapcm

#include "platform/linux/alsapcm/alsapcm.h"
#include <stdio.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <math.h>

static volatile int alsa_sigc=0;
static int alsa_samplec=0;

static void _cb_signal(int sigid) {
  switch (sigid) {
    case SIGINT: if (++alsa_sigc>=3) {
        fprintf(stderr,"Too many unprocessed signals.\n");
        exit(1);
      } break;
  }
}

static double nows_cpu() {
  struct timespec tv={0};
  clock_gettime(CLOCK_PROCESS_CPUTIME_ID,&tv);
  return (double)tv.tv_sec+(double)tv.tv_nsec/1000000000.0;
}

static double nows_real() {
  struct timespec tv={0};
  clock_gettime(CLOCK_REALTIME,&tv);
  return (double)tv.tv_sec+(double)tv.tv_nsec/1000000000.0;
}

static int simxrun=0;
static int16_t *pcmsrc=0;
static int pcmsrcp=0,pcmsrcc=0;
static int rate=0,chanc=0;

static void _cb_pcm_out(int16_t *v,int c,void *userdata) {
  alsa_samplec+=c;
  
  #if 0 // Silence for a bare-bones IO performance test. Or just do nothing (it will likely emit noise).
    memset(v,0,c<<1);//TODO demo with something fancier than silence
  #else // Emit a sensible tone.
    if (pcmsrc) {
      if (chanc==2) {
        while (c>0) {
          *(v++)=pcmsrc[pcmsrcp];
          *(v++)=pcmsrc[pcmsrcp];
          pcmsrcp++;
          if (pcmsrcp>=pcmsrcc) pcmsrcp=0;
          c-=2;
        }
      } else if (chanc==1) {
        while (c>0) {
          int cpc=pcmsrcc-pcmsrcp;
          if (cpc>c) cpc=c;
          memcpy(v,pcmsrc+pcmsrcp,cpc<<1);
          v+=cpc;
          c-=cpc;
          pcmsrcp+=cpc;
          if (pcmsrcp>=pcmsrcc) pcmsrcp=0;
        }
      } else { // >2 channels, that's weird.
        while (c>0) {
          int i=chanc;
          while (i-->0) *(v++)=pcmsrc[pcmsrcp];
          pcmsrcp++;
          if (pcmsrcp>=pcmsrcc) pcmsrcp=0;
          c-=chanc;
        }
      }
    }
  #endif
  
  if (0&&(--simxrun<0)) { // <-- enable this to test xrun handling
    fprintf(stderr,"--- simulating long synth cycle (improbable) ---\n");
    usleep(500000);
    simxrun=100;
  }
}

struct bits_demo_alsa_device_choice {
  char *base;
  int card,device;
  int ratelo,ratehi;
  int chanclo,chanchi;
};

static void bits_demo_alsa_device_choice_cleanup(
  struct bits_demo_alsa_device_choice *choice
) {
  if (choice->base) free(choice->base);
}

// Nonzero if (device) is preferable to (choice).
static int bits_demo_alsa_better_device(
  const struct bits_demo_alsa_device_choice *choice,
  const struct alsapcm_device *device
) {
  // Reject device if its modes fail to overlap (22050..48000,1..2).
  // This would be pretty weird if it ever happens.
  if (device->ratelo>48000) return 0;
  if (device->ratehi<22050) return 0;
  if (device->chanclo>2) return 0;
  if (device->chanchi<1) return 0;
  // If card:device 0:0 is an option, we want it.
  if (choice->base&&!choice->card&&!choice->device) return 0;
  if (!device->card&&!device->device) return 1;
  // If we don't have a choice yet, take whatever this is.
  if (!choice->base) return 1;
  // Could keep going, but ok this seems fine.
  return 0;
}

static int _cb_list_devices(struct alsapcm_device *device,void *userdata) {
  struct bits_demo_alsa_device_choice *choice=userdata;
  const char *flag="";
  if (bits_demo_alsa_better_device(choice,device)) {
    if (choice->base) free(choice->base);
    if (!(choice->base=strdup(device->basename))) return -1;
    choice->card=device->card;
    choice->device=device->device;
    choice->ratelo=device->ratelo;
    choice->ratehi=device->ratehi;
    choice->chanclo=device->chanclo;
    choice->chanchi=device->chanchi;
    flag="<--";
  }
  fprintf(stderr,
    "%s %d/%d/%d '%s' '%s' '%s' %d:%d r=%d..%d c=%d..%d %s\n",
    device->basename,
    device->card,device->device,device->subdevice,
    device->id,device->name,device->subname,
    device->dev_class,device->dev_subclass,
    device->ratelo,device->ratehi,
    device->chanclo,device->chanchi,
    flag
  );
  return 0;
}

int bits_demo_alsa(int argc,char **argv) {
  fprintf(stderr,"%s...\n",__func__);
  
  signal(SIGINT,_cb_signal);
  
  /* Select a device.
   * In real life, this would be influenced by the app's requirements and hopefully by user config.
   */
  char *devicename=0;
  #if 0 // The hard way, iterate devices and decide on our own.
    struct bits_demo_alsa_device_choice choice={0};
    alsapcm_list_devices(0,_cb_list_devices,&choice);
    devicename=choice.base;
  #else // The easy way, let alsapcm do all that.
    devicename=alsapcm_find_device(0,22050,48000,1,2);
  #endif // But there is an even easier way; you can leave device null.
  if (!devicename) {
    fprintf(stderr,"Unable to find a suitable PCM-Out device.\n");
    return 1;
  }
  fprintf(stderr,"Selected device '%s'\n",devicename);
  
  struct alsapcm_delegate delegate={
    .userdata=0,
    .pcm_out=_cb_pcm_out,
  };
  struct alsapcm_setup setup={
    .rate=44100,
    .chanc=1,
    .device=devicename,
  };
  struct alsapcm *alsa=alsapcm_new(&delegate,&setup);
  if (!alsa) {
    fprintf(stderr,"alsapcm_new() failed. rate=%d chanc=%d device=%s\n",setup.rate,setup.chanc,setup.device);
    return 1;
  }
  free(devicename);
  
  rate=alsapcm_get_rate(alsa);
  chanc=alsapcm_get_chanc(alsa);
  fprintf(stderr,
    "Created ALSA PCM-Out context. Requested %d@%d, got %d@%d.\n",
    setup.chanc,setup.rate,
    chanc,rate
  );
  
  // Generate a sine wave at a nice audible frequency.
  pcmsrcc=rate/440;
  if (!(pcmsrc=malloc(pcmsrcc<<1))) return 1;
  { int16_t *v=pcmsrc;
    int i=pcmsrcc;
    double t=0.0;
    double dt=(M_PI*2.0)/pcmsrcc;
    for (;i-->0;v++,t+=dt) {
      *v=sin(t)*32000;
    }
  }
  
  alsapcm_set_running(alsa,1);
  
  double starttime_real=nows_real();
  double starttime_cpu=nows_cpu();
  
  while (!alsa_sigc) {
    if (alsapcm_update(alsa)<0) {
      fprintf(stderr,"!!! alsapcm_update failed !!!\n");
      break;
    }
    // We can sleep as long as we like here, it won't cause underruns because I/O happens in another thread.
    usleep(20000);
  }
  
  double endtime_real=nows_real();
  double endtime_cpu=nows_cpu();
  double elapsed_real=endtime_real-starttime_real;
  double elapsed_cpu=endtime_cpu-starttime_cpu;
  int framec=alsa_samplec/alsapcm_get_chanc(alsa);
  double effective_rate=framec/elapsed_real;
  double cpu_load=elapsed_cpu/elapsed_real;
  fprintf(stderr,
    "Finished ALSA demo. %d frames in %.03f s, effective rate %.03f. CPU load %.06f.\n",
    framec,elapsed_real,effective_rate,cpu_load
  );
  /* Expect effective rate a little higher than the requested rate -- it counts frames that haven't been delivered yet.
   * I'm observing CPU load around 0.005, emitting silence on the NUC. This is an excellent score.
   * Any load below say 0.020 is acceptable.
   * Similar tests with libasound gave me more like 0.050. Certainly due to misconfiguration on my part, but I never did figure that out.
   */
  
  alsapcm_del(alsa);
  return 0;
}

#else
int bits_demo_alsa(int argc,char **argv) { return 1; }
#endif
