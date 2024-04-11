# This Makefile, etc/config.mk.example, etc/target/, etc/make/, and etc/tool/, comprise a pretty robust build environment.
# I've tried to keep it reasonable for new non-trivial projects. Copy then season to taste.

all:
.SILENT:
.SECONDARY:
PRECMD=echo "  $@" ; mkdir -p $(@D) ;

ifneq (,$(strip $(filter clean,$(MAKECMDGOALS))))

clean:;rm -rf mid out

else

etc/config.mk:|etc/config.mk.example;$(PRECMD) cp etc/config.mk.example $@ ; echo "Please edit $@, then rerun make" ; exit 1
include etc/config.mk

include etc/make/common.mk
include etc/make/tools.mk
include etc/make/tests.mk

define TARGET_RULES
  $1_MIDDIR:=mid/$1
  $1_OUTDIR:=out/$1
  include etc/target/$1.mk
endef
$(foreach T,$(TARGETS),$(eval $(call TARGET_RULES,$T)))

endif

