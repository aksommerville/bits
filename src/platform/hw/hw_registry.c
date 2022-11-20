#include "hw_internal.h"

/* Declarations for all possible types.
 * Doesn't matter whether they're actually available or not.
 */
 
extern const struct hw_video_type hw_video_type_dummy;
extern const struct hw_video_type hw_video_type_macwm;
extern const struct hw_video_type hw_video_type_x11;
extern const struct hw_video_type hw_video_type_drm;
extern const struct hw_video_type hw_video_type_mswm;

extern const struct hw_audio_type hw_audio_type_dummy;
extern const struct hw_audio_type hw_audio_type_macaudio;
extern const struct hw_audio_type hw_audio_type_alsa;
extern const struct hw_audio_type hw_audio_type_pulse;
extern const struct hw_audio_type hw_audio_type_msaudio;

extern const struct hw_input_type hw_input_type_machid;
extern const struct hw_input_type hw_input_type_evdev;
extern const struct hw_input_type hw_input_type_mshid;

extern const struct hw_synth_type hw_synth_type_dummy;
extern const struct hw_synth_type hw_synth_type_cheapsynth;
extern const struct hw_synth_type hw_synth_type_fancysynth;

extern const struct hw_render_type hw_render_type_dummy;
extern const struct hw_render_type hw_render_type_softrender;
extern const struct hw_render_type hw_render_type_gl2;
extern const struct hw_render_type hw_render_type_metal;
extern const struct hw_render_type hw_render_type_vulkan;

//XXX TEMP stubbing a few that i plan to write soon...
const struct hw_video_type hw_video_type_x11={"x11","",sizeof(struct hw_video)};
const struct hw_video_type hw_video_type_drm={"drm","",sizeof(struct hw_video)};
const struct hw_audio_type hw_audio_type_alsa={"alsa","",sizeof(struct hw_audio)};
const struct hw_audio_type hw_audio_type_pulse={"pulse","",sizeof(struct hw_audio)};
const struct hw_input_type hw_input_type_evdev={"evdev","",sizeof(struct hw_input)};

/* Type registries.
 * This is the the one point where we need to check what's enabled in the build.
 * Order matters! Put the default first.
 */
 
static const struct hw_video_type *hw_video_typev[]={
#if BITS_USE_platform_macos
  &hw_video_type_macwm,
#endif
#if BITS_USE_platform_linux
  &hw_video_type_x11,
  &hw_video_type_drm,
#endif
#if BITS_USE_platform_mswin
  &hw_video_type_mswm,
#endif
  &hw_video_type_dummy,
};

static const struct hw_audio_type *hw_audio_typev[]={
#if BITS_USE_platform_macos
  &hw_audio_type_macaudio,
#endif
#if BITS_USE_platform_linux
  &hw_audio_type_alsa,
  &hw_audio_type_pulse,
#endif
#if BITS_USE_platform_mswin
  &hw_audio_type_msaudio,
#endif
  &hw_audio_type_dummy,
};

static const struct hw_input_type *hw_input_typev[]={
#if BITS_USE_platform_macos
  &hw_input_type_machid,
#endif
#if BITS_USE_platform_linux
  &hw_input_type_evdev,
#endif
#if BITS_USE_platform_mswin
  &hw_input_type_mshid,
#endif
};

static const struct hw_synth_type *hw_synth_typev[]={
#if BITS_USE_cheapsynth
  &hw_synth_type_cheapsynth,
#endif
#if BITS_USE_fancysynth
  &hw_synth_type_fancysynth,
#endif
  &hw_synth_type_dummy,
};

static const struct hw_render_type *hw_render_typev[]={
#if BITS_USE_metal
  &hw_render_type_metal,
#endif
#if BITS_USE_vulkan
  &hw_render_type_vulkan,
#endif
#if BITS_USE_gl2
  &hw_render_type_gl2,
#endif
#if BITS_USE_softrender
  &hw_render_type_softrender,
#endif
  &hw_render_type_dummy,
};

/* Accessors for each type.
 */
 
#define ACCESSORS(tname) \
  const struct hw_##tname##_type *hw_##tname##_type_by_index(int p) { \
    if (p<0) return 0; \
    int c=sizeof(hw_##tname##_typev)/sizeof(void*); \
    if (p>=c) return 0; \
    return hw_##tname##_typev[p]; \
  } \
  const struct hw_##tname##_type *hw_##tname##_type_by_name(const char *name,int namec) { \
    if (!name) return 0; \
    if (namec<0) { namec=0; while (name[namec]) namec++; } \
    int i=sizeof(hw_##tname##_typev)/sizeof(void*); \
    const struct hw_##tname##_type **p=hw_##tname##_typev; \
    for (;i-->0;p++) { \
      if (memcmp(name,(*p)->name,namec)) continue; \
      if ((*p)->name[namec]) continue; \
      return *p; \
    } \
    return 0; \
  }
  
ACCESSORS(video)
ACCESSORS(audio)
ACCESSORS(input)
ACCESSORS(synth)
ACCESSORS(render)

#undef ACCESSORS

/* Input device ID generator.
 */
 
static int hw_devid_next_g=1;

int hw_devid_next(void *ignored) {
  if (hw_devid_next_g>=INT_MAX) return -1;
  return hw_devid_next_g++;
}
