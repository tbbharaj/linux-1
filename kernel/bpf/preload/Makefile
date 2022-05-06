# SPDX-License-Identifier: GPL-2.0

<<<<<<< HEAD
LIBBPF_SRCS = $(srctree)/tools/lib/bpf/
LIBBPF_A = $(obj)/libbpf.a
LIBBPF_OUT = $(abspath $(obj))

# Although not in use by libbpf's Makefile, set $(O) so that the "dummy" test
# in tools/scripts/Makefile.include always succeeds when building the kernel
# with $(O) pointing to a relative path, as in "make O=build bindeb-pkg".
$(LIBBPF_A):
	$(Q)$(MAKE) -C $(LIBBPF_SRCS) O=$(LIBBPF_OUT)/ OUTPUT=$(LIBBPF_OUT)/ $(LIBBPF_OUT)/libbpf.a

userccflags += -I $(srctree)/tools/include/ -I $(srctree)/tools/include/uapi \
	-I $(srctree)/tools/lib/ -Wno-unused-result

userprogs := bpf_preload_umd

clean-files := $(userprogs) bpf_helper_defs.h FEATURE-DUMP.libbpf staticobjs/ feature/

bpf_preload_umd-objs := iterators/iterators.o
bpf_preload_umd-userldlibs := $(LIBBPF_A) -lelf -lz

$(obj)/bpf_preload_umd: $(LIBBPF_A)

$(obj)/bpf_preload_umd_blob.o: $(obj)/bpf_preload_umd
=======
LIBBPF_INCLUDE = $(srctree)/tools/lib
>>>>>>> 672c0c5173427e6b3e2a9bbb7be51ceeec78093a

obj-$(CONFIG_BPF_PRELOAD_UMD) += bpf_preload.o
CFLAGS_bpf_preload_kern.o += -I$(LIBBPF_INCLUDE)
bpf_preload-objs += bpf_preload_kern.o
