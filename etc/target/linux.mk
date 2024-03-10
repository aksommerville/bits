# The truly optional things should be set by config.mk:
#   glx x11fb drmgx drmfb alsafd asound pulse evdev qjs wamr
linux_OPT_ENABLE+=hostio simplifio fs serial midi wav http
linux_OPT_ENABLE+=bmp gif ico jpeg png qoi rlead rawimg

linux_CCWARN:=-Werror -Wimplicit
linux_CCINC:=-Isrc -I$(linux_MIDDIR)
linux_CCDEF:=$(patsubst %,-DUSE_%=1,$(linux_OPT_ENABLE))
linux_CCOPT:=-c -MMD -O3 $(linux_CCWARN) $(linux_CCINC) $(linux_CCDEF) $(linux_CC_EXTRA)
linux_LDOPT:=$(linux_LD_EXTRA)

linux_CC:=$(linux_TOOLCHAIN)gcc $(linux_CCOPT)
linux_AS:=$(linux_TOOLCHAIN)gcc -xassembler-with-cpp $(linux_CCOPT)
linux_CXX:=$(linux_TOOLCHAIN)g++ $(linux_CCOPT)
linux_OBJC:=$(linux_TOOLCHAIN)gcc -xobjective-c $(linux_CCOPT)
linux_LD:=$(linux_TOOLCHAIN)gcc $(linux_LDOPT)
linux_LDPOST:=-lm -lz -lpthread $(linux_LDPOST_EXTRA)

ifneq (,$(strip $(filter drmgx,$(linux_OPT_ENABLE))))
  linux_CC+=-I/usr/include/libdrm
  linux_LDPOST+=-ldrm -lgbm -lEGL
else ifneq (,$(strip $(filter drmfb,$(linux_OPT_ENABLE))))
  linux_CC+=-I/usr/include/libdrm
  linux_LDPOST+=-ldrm
endif
ifneq (,$(strip $(filter glx,$(linux_OPT_ENABLE))))
  linux_LDPOST+=-lX11 -lGL
else ifneq (,$(strip $(filter x11fb,$(linux_OPT_ENABLE))))
  linux_LDPOST+=-lX11
endif
ifneq (,$(strip $(filter xinerama,$(linux_OPT_ENABLE))))
  linux_LDPOST+=-lXinerama
endif
ifneq (,$(strip $(filter asound,$(linux_OPT_ENABLE))))
  linux_LDPOST+=-lasound
endif
ifneq (,$(strip $(filter pulse,$(linux_OPT_ENABLE))))
  linux_LDPOST+=-lpulse-simple
endif
ifneq (,$(strip $(filter jpeg,$(linux_OPT_ENABLE))))
  linux_LDPOST+=-ljpeg
endif
ifneq (,$(strip $(filter qjs,$(linux_OPT_ENABLE))))
  linux_CC+=-I$(QJS_SDK)
  linux_LDPOST+=$(QJS_SDK)/libquickjs.a
endif
ifneq (,$(strip $(filter wamr,$(linux_OPT_ENABLE))))
  linux_CC+=-I$(WAMR_SDK)/core/iwasm/include
  linux_LDPOST+=$(WAMR_SDK)/build/libvmlib.a
endif

linux_CFILES:=$(filter \
  src/main/% \
  $(addprefix src/opt/,$(addsuffix /%,$(linux_OPT_ENABLE))) \
,$(CFILES)) \

linux_OFILES:=$(patsubst src/%,$(linux_MIDDIR)/%.o,$(basename $(linux_CFILES)))
-include $(linux_OFILES:.o=.d)

$(linux_MIDDIR)/%.o:src/%.c;$(PRECMD) $(linux_CC) -o$@ $<
$(linux_MIDDIR)/%.o:src/%.s;$(PRECMD) $(linux_AS) -o$@ $<
$(linux_MIDDIR)/%.o:src/%.cxx;$(PRECMD) $(linux_CXX) -o$@ $<
$(linux_MIDDIR)/%.o:src/%.m;$(PRECMD) $(linux_OBJC) -o$@ $<

linux_EXE:=$(linux_OUTDIR)/myexe
all:$(linux_EXE)
$(linux_EXE):$(linux_OFILES);$(PRECMD) $(linux_LD) -o$@ $(linux_OFILES) $(linux_LDPOST)

linux-run:$(linux_EXE);$(linux_EXE) $(linux_RUN_ARGS)
