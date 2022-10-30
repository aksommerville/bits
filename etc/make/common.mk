# common.mk
# Included before any target configuration.

SRCFILES:=$(shell find src -type f)

define DEFUNITS # $1=units
  $(foreach U,$1,-DBITS_USE_$(subst /,_,$U)=1)
endef

define COMPILE # $1=MIDDIR,$2=SUFFIX,$3=COMMAND
  $1/%.o:src/%$2;$$(PRECMD) $3
  $1/%.o:$1/%$2;$$(PRECMD) $3
endef
