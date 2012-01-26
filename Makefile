# -*- Makefile -*-
# Makefile for source package: kernel
#
# Module created by 'gafton' on 2012-01-19

NAME		:= kernel
VENDOR		:= GOBI

# these are specific for tis module and have to be defined before Makefile.common is ingested
SPECFILE	= template/kernel.spec
BUILDVAR	= buildid
CHANGELOGS	= $(SPECFILE) srpm

define find-makefile-common
$(firstword $(foreach d,. common ../common,$(if $(wildcard $(d)/Makefile.common),$(d)/Makefile.common)))
endef

include $(find-makefile-common)

ifeq ($(find-makefile-common),)
define get_common_url
$(shell git remote show -n origin | sed -re '/fetch\s+url/I!d;s,^.+\b(\w+://\S+)/[^/]+$$,\1/common.git,')
endef

common :
	git clone $(get_common_url) $@

sources : common
	$(MAKE) $@

else

include Makefile.kernel

reconfig reconfig-prep :: Makefile.reconfig sources
	make -f $< $@ SOURCEDIR=$(SOURCEDIR) SPECFILE=$(CUSTOMSPEC)

endif
