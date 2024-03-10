all:
.SILENT:
.SECONDARY:
PRECMD=echo "  $@" ; mkdir -p $(@D) ;

WAMR_SDK:=../thirdparty/wasm-micro-runtime
WASI_SDK:=../thirdparty/wasi-sdk-16.0
QJS_SDK:=../thirdparty/quickjs/quickjs-2023-12-09

SRCFILES:=$(shell find src -type f)

# "ALL" but not test or wasm. Also, "CFILES" means "any source code".
CFILES_ALL:=$(filter-out src/test/% src/wasm/%,$(filter %.c %.m %.s %.cxx,$(SRCFILES)))

clean:;rm -rf mid out

CC:=gcc -c -MMD -O3 -Isrc -Imid -Werror -Wimplicit -I/usr/include/libdrm -I$(WAMR_SDK)/core/iwasm/include -I$(QJS_SDK)
OBJC:=$(CC) -xobjective-c
AS:=$(CC) -xassembler-with-cpp
CXX:=g++ -c -MMD -O3 -Isrc -Imid -Werror -Wimplicit
LD:=gcc
LDPOST:=-lm -lz -ljpeg -lX11 -lXinerama -lpulse-simple -lGL -lpthread -lasound -ldrm -lgbm -lEGL $(WAMR_SDK)/build/libvmlib.a $(QJS_SDK)/libquickjs.a

# Declare all the "optional" units as in-use.
USE_ALL_OPT:=$(patsubst %,-DUSE_%=1, \
  midi serial fs http wamr qjs \
  hostio evdev glx drmgx x11fb drmfb alsafd asound pulse \
  rawimg gif bmp qoi rlead ico png jpeg \
  xinerama \
)
CC+=$(USE_ALL_OPT)

mid/%.o:src/%.c;$(PRECMD) $(CC) -o$@ $<
mid/%.o:src/%.m;$(PRECMD) $(OBJC) -o$@ $<
mid/%.o:src/%.s;$(PRECMD) $(AS) -o$@ $<
mid/%.o:src/%.cxx;$(PRECMD) $(CXX) -o$@ $<

#-------------------------------------------------------------
# Build wasm modules for running with our 'wamr' unit (ie wasm-micro-runtime).

CC_WASM:=$(WASI_SDK)/bin/clang -c -MMD -O3 -Isrc -Werror -Wimplicit -Wno-comment -Wno-parentheses
LD_WASM:=$(WASI_SDK)/bin/clang -nostdlib -Wl,--no-entry -Wl,--export-table -Wl,--export-all -Wl,--import-undefined -Wl,--allow-undefined -Wl,--initial-memory=1048576
LDPOST_WASM:=

CFILES_WASM:=$(filter src/wasm/%.c,$(SRCFILES))
OFILES_WASM:=$(patsubst src/%.c,mid/%.o,$(CFILES_WASM))
-include $(OFILES_WASM:.o=.d)

mid/wasm/%.o:src/wasm/%.c;$(PRECMD) $(CC_WASM) -o$@ $<

MODS_WASM:=$(notdir $(wildcard src/wasm/*))
MODS_WASM_OUT:=$(patsubst %,out/wasm/%.wasm,$(MODS_WASM))
all:$(MODS_WASM_OUT)

out/wasm/%.wasm:mid/wasm/%/*.o;$(PRECMD) $(LD_WASM) -o$@ $^ $(LDPOST_WASM)

#-------------------------------------------------------------
# Tests.

# Same CC for unit, integration, and common. We could split those up if needed, but shouldn't be a need.
CC_TEST:=gcc -c -MMD -O0 -Isrc -Imid -Werror -Wimplicit $(USE_ALL_OPT) -I$(WAMR_SDK)/core/iwasm/include -I$(QJS_SDK)
LD_ITEST:=gcc
LDPOST_ITEST:=$(LDPOST)
LD_UTEST:=gcc
LDPOST_UTEST:=-lm

CFILES_TEST_COMMON:=$(filter src/test/common/%.c,$(SRCFILES))
OFILES_TEST_COMMON:=$(patsubst src/%.c,mid/%.o,$(CFILES_TEST_COMMON))
-include $(OFILES_TEST_COMMON:.o=.d)
GENHEADERS+=mid/test/int/itest_toc.h

CFILES_UTEST:=$(filter src/test/unit/%.c,$(SRCFILES))
OFILES_UTEST:=$(patsubst src/%.c,mid/%.o,$(CFILES_UTEST))
EXES_UTEST:=$(patsubst %.o,%,$(OFILES_UTEST))
-include $(OFILES_UTEST:.o=.d)
all:$(EXES_UTEST)

mid/test/unit/%:mid/test/unit/%.o $(OFILES_TEST_COMMON)|$(GENHEADERS);$(PRECMD) $(LD_UTEST) -o$@ $< $(OFILES_TEST_COMMON) $(LDPOST_UTEST)

CFILES_ITEST:=$(filter src/test/int/%.c,$(SRCFILES)) $(CFILES_TEST_COMMON) $(CFILES_ALL)
OFILES_ITEST:=$(patsubst src/%.c,mid/%.o,$(CFILES_ITEST))
-include $(OFILES_ITEST:.o=.d)
EXE_ITEST:=mid/test/itest
all:$(EXE_ITEST)

mid/test/%.o:src/test/%.c;$(PRECMD) $(CC_TEST) -o$@ $<
$(EXE_ITEST):$(OFILES_ITEST);$(PRECMD) $(LD_ITEST) -o$@ $^ $(LDPOST_ITEST)

mid/test/int/itest_toc.h:$(CFILES_ITEST);$(PRECMD) etc/tool/gen-itest-toc.sh $@ $^

#TODO Automation, Javascript.
test:$(EXES_UTEST) $(EXE_ITEST) $(MODS_WASM_OUT);trap '' INT ; etc/tool/run-tests.sh $(EXES_UTEST) $(EXE_ITEST)
test-%:$(EXES_UTEST) $(EXE_ITEST) $(MODS_WASM_OUT);trap '' INT ; TEST_FILTER="$*" etc/tool/run-tests.sh $(EXES_UTEST) $(EXE_ITEST)
