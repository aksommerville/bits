#if BITS_USE_platform_linux || BITS_USE_platform_linux_pulse

#include "platform/linux/pulse/pulse.h"
#include <stdio.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <math.h>

static volatile int pulse_sigc=0;
static int pulse_samplec=0;

static void _cb_signal(int sigid) {
  switch (sigid) {
    case SIGINT: if (++pulse_sigc>=3) {
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
  pulse_samplec+=c;
  
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

int bits_demo_pulse(int argc,char **argv) {
  fprintf(stderr,"%s...\n",__func__);
  
  signal(SIGINT,_cb_signal);
  
  struct pulse_delegate delegate={
    .userdata=0,
    .pcm_out=_cb_pcm_out,
  };
  struct pulse_setup setup={
    .rate=44100,
    .chanc=1,
    .buffersize=0,
    .servername=0,
    .appname=__FILE__,
  };
  struct pulse *pulse=pulse_new(&delegate,&setup);
  if (!pulse) {
    fprintf(stderr,"pulse_new() failed. rate=%d chanc=%d\n",setup.rate,setup.chanc);
    return 1;
  }
  
  rate=pulse_get_rate(pulse);
  chanc=pulse_get_chanc(pulse);
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
  
  pulse_set_running(pulse,1);
  
  double starttime_real=nows_real();
  double starttime_cpu=nows_cpu();
  
  while (!pulse_sigc) {
    if (pulse_update(pulse)<0) {
      fprintf(stderr,"!!! pulse_update failed !!!\n");
      break;
    }
    // We can sleep as long as we like here, it won't cause underruns because I/O happens in another thread.
    usleep(20000);
  }
  
  double endtime_real=nows_real();
  double endtime_cpu=nows_cpu();
  double elapsed_real=endtime_real-starttime_real;
  double elapsed_cpu=endtime_cpu-starttime_cpu;
  int framec=pulse_samplec/pulse_get_chanc(pulse);
  double effective_rate=framec/elapsed_real;
  double cpu_load=elapsed_cpu/elapsed_real;
  fprintf(stderr,
    "Finished PulseAudio demo. %d frames in %.03f s, effective rate %.03f. CPU load %.06f.\n",
    framec,elapsed_real,effective_rate,cpu_load
  );
  /* Expect effective rate a little higher than the requested rate -- it counts frames that haven't been delivered yet.
   * I'm getting loads of 0.018-0.019, which is not bad.
   * As expected, alsapcm performs better.
   */
  
  pulse_del(pulse);
  return 0;
}

#else
int bits_demo_pulse(int argc,char **argv) { return 1; }
#endif
