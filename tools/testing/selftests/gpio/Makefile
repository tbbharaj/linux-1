# SPDX-License-Identifier: GPL-2.0

TEST_PROGS := gpio-mockup.sh gpio-sim.sh
TEST_FILES := gpio-mockup-sysfs.sh
<<<<<<< HEAD
TEST_GEN_PROGS_EXTENDED := gpio-mockup-chardev

KSFT_KHDR_INSTALL := 1
include ../lib.mk

GPIODIR := $(realpath ../../../gpio)
GPIOOUT := $(OUTPUT)/tools-gpio/
GPIOOBJ := $(GPIOOUT)/gpio-utils.o

override define CLEAN
	$(RM) $(TEST_GEN_PROGS_EXTENDED)
	$(RM) -rf $(GPIOOUT)
endef

$(TEST_GEN_PROGS_EXTENDED): $(GPIOOBJ)

$(GPIOOUT):
	mkdir -p $@

$(GPIOOBJ): $(GPIOOUT)
	$(MAKE) OUTPUT=$(GPIOOUT) -C $(GPIODIR)
=======
TEST_GEN_PROGS_EXTENDED := gpio-mockup-cdev gpio-chip-info gpio-line-name
CFLAGS += -O2 -g -Wall -I../../../../usr/include/

include ../lib.mk
>>>>>>> 672c0c5173427e6b3e2a9bbb7be51ceeec78093a
