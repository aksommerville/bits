# linux.mk

linux_MIDDIR:=mid/linux
linux_OUTDIR:=out/linux

# "xinerama" is a flag to akx11, not a real unit.
linux_UNITS:=demo serial format synth platform/linux platform/genioc platform/hw xinerama

linux_CCDEF:=$(call DEFUNITS,$(linux_UNITS))
linux_CCINC:=-Isrc -I$(linux_MIDDIR)
linux_CCOPT:=-c -MMD -O3
linux_CCWARN:=-Werror -Wimplicit -Wno-parentheses -Wno-pointer-sign -Wno-comment
linux_CC:=gcc $(linux_CCOPT) $(linux_CCDEF) $(linux_CCINC) $(linux_CCWARN)
linux_CXX:=g++ $(linux_CCOPT) $(linux_CCDEF) $(linux_CCINC) $(linux_CCWARN)
linux_OBJC:=gcc -xobjective-c $(linux_CCOPT) $(linux_CCDEF) $(linux_CCINC) $(linux_CCWARN)
linux_AS:=gcc -xassembler-with-cpp $(linux_CCOPT) $(linux_CCDEF) $(linux_CCINC) $(linux_CCWARN)
linux_AR:=ar rc
linux_LD:=gcc
linux_LDPOST:=-lm -lz -lpulse-simple -lX11 -lGL -lGLX -lXinerama

# This can accept files under linux_MIDDIR too, if you want to generate source files.
linux_SRCFILES:=$(filter $(addprefix src/,$(addsuffix /%,$(linux_UNITS))),$(SRCFILES))

linux_CFILES:=$(filter %.c %.cxx %.m %.S,$(linux_SRCFILES))
linux_OFILES:=$(patsubst src/%,$(linux_MIDDIR)/%,$(addsuffix .o,$(basename $(linux_CFILES))))
-include $(linux_OFILES:.o=.d)
$(eval $(call COMPILE,$(linux_MIDDIR),.c,$(linux_CC) -o $$@ $$<))
$(eval $(call COMPILE,$(linux_MIDDIR),.cxx,$(linux_CXX) -o $$@ $$<))
$(eval $(call COMPILE,$(linux_MIDDIR),.m,$(linux_OBJC) -o $$@ $$<))
$(eval $(call COMPILE,$(linux_MIDDIR),.S,$(linux_AS) -o $$@ $$<))

linux_EXE:=$(linux_OUTDIR)/akbits
all:$(linux_EXE)
$(linux_EXE):$(linux_OFILES);$(PRECMD) $(linux_LD) -o$@ $^ $(linux_LDPOST)
linux-run:$(linux_EXE);trap '' INT ; $(linux_EXE)
