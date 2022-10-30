all:
.SILENT:
PRECMD=echo "  $(@F)" ; mkdir -p $(@D) ;

# "make clean" is special, we don't need to load any config for it.
ifeq ($(MAKECMDGOALS),clean)
  clean:;rm -rf mid out
else
  clean:;echo "Something is broken, you got the wrong 'clean'" ; exit 1

  # Site-specific configuration comes from etc/config.mk.
  # This is not committed to the repo, but we can generate a default.
  include etc/config.mk
  etc/config.mk:etc/config.mk.default;$(PRECMD) cp $< $@
  unconfigure:;rm etc/config.mk

  include etc/make/common.mk
  $(foreach T,$(TARGETS),$(eval include etc/target/$T.mk))
  
endif
