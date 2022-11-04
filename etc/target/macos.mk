# macos.mk

macos_MIDDIR:=mid/macos
macos_OUTDIR:=out/macos

macos_UNITS:=demo serial platform/macos

macos_CCDEF:=$(call DEFUNITS,$(macos_UNITS))
macos_CCINC:=-Isrc -I$(macos_MIDDIR)
macos_CCOPT:=-c -MMD -O3
macos_CCWARN:=-Werror -Wimplicit -Wno-parentheses -Wno-pointer-sign -Wno-comment
macos_CC:=gcc $(macos_CCOPT) $(macos_CCDEF) $(macos_CCINC) $(macos_CCWARN)
macos_CXX:=g++ $(macos_CCOPT) $(macos_CCDEF) $(macos_CCINC) $(macos_CCWARN)
macos_OBJC:=gcc -xobjective-c $(macos_CCOPT) $(macos_CCDEF) $(macos_CCINC) $(macos_CCWARN)
macos_AS:=gcc -xassembler-with-cpp $(macos_CCOPT) $(macos_CCDEF) $(macos_CCINC) $(macos_CCWARN)
macos_AR:=ar rc
macos_LD:=gcc
macos_LDPOST:=-framework Cocoa -framework Quartz -framework OpenGL -framework MetalKit -framework Metal -framework CoreGraphics -framework IOKit

# This can accept files under macos_MIDDIR too, if you want to generate source files.
macos_SRCFILES:=$(filter $(addprefix src/,$(addsuffix /%,$(macos_UNITS))),$(SRCFILES))

macos_CFILES:=$(filter %.c %.cxx %.m %.S,$(macos_SRCFILES))
macos_OFILES:=$(patsubst src/%,$(macos_MIDDIR)/%,$(addsuffix .o,$(basename $(macos_CFILES))))
-include $(macos_OFILES:.o=.d)
$(eval $(call COMPILE,$(macos_MIDDIR),.c,$(macos_CC) -o $$@ $$<))
$(eval $(call COMPILE,$(macos_MIDDIR),.cxx,$(macos_CXX) -o $$@ $$<))
$(eval $(call COMPILE,$(macos_MIDDIR),.m,$(macos_OBJC) -o $$@ $$<))
$(eval $(call COMPILE,$(macos_MIDDIR),.S,$(macos_AS) -o $$@ $$<))

# "cli" or "gui", which block below to invoke for the executable.
macos_EXE_STYLE:=gui

# For a non-GUI command line app.
# Most PCs work about like this.
ifeq ($(macos_EXE_STYLE),cli)
  macos_EXE:=$(macos_OUTDIR)/akbits
  all:$(macos_EXE)
  $(macos_EXE):$(macos_OFILES);$(PRECMD) $(macos_LD) -o$@ $^ $(macos_LDPOST)
  macos-run:$(macos_EXE);$(macos_EXE)
endif

# For a GUI app in the Mac fashion.
# You have to tweak Info.plist, Main.nib, and appicon.icns to suit the app.
ifeq ($(macos_EXE_STYLE),gui)
  macos_BUNDLE:=$(macos_OUTDIR)/AKBits.app
  macos_PLIST:=$(macos_BUNDLE)/Contents/Info.plist
  macos_NIB:=$(macos_BUNDLE)/Contents/Resources/Main.nib
  macos_EXE:=$(macos_BUNDLE)/Contents/MacOS/akbits
  macos_ICON:=$(macos_BUNDLE)/Contents/Resources/appicon.icns

  $(macos_PLIST):src/platform/macos/Info.plist;$(PRECMD) cp $< $@
  $(macos_NIB):src/platform/macos/Main.xib;$(PRECMD) ibtool --compile $@ $<
  macos_INPUT_ICONS:=$(wildcard src/platform/macos/appicon.iconset/*)
  $(macos_ICON):$(macos_INPUT_ICONS);$(PRECMD) iconutil -c icns -o $@ src/platform/macos/appicon.iconset

  $(macos_EXE):$(macos_PLIST) $(macos_NIB) $(macos_ICON)
  all:$(macos_EXE)
  $(macos_EXE):$(macos_OFILES);$(PRECMD) $(macos_LD) -o$@ $(macos_OFILES) $(macos_LDPOST)

  macos-run:$(macos_EXE);open -W $(macos_BUNDLE) --args --reopen-tty=$$(tty) --chdir=$$(pwd)
endif
