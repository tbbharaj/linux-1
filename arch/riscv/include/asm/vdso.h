/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (C) 2012 ARM Limited
 * Copyright (C) 2014 Regents of the University of California
 * Copyright (C) 2017 SiFive
 */

#ifndef _ASM_RISCV_VDSO_H
#define _ASM_RISCV_VDSO_H

<<<<<<< HEAD
#include <linux/types.h>

#ifndef CONFIG_GENERIC_TIME_VSYSCALL
struct vdso_data {
};
#endif

=======
>>>>>>> 672c0c5173427e6b3e2a9bbb7be51ceeec78093a
/*
 * All systems with an MMU have a VDSO, but systems without an MMU don't
 * support shared libraries and therefor don't have one.
 */
#ifdef CONFIG_MMU

#define __VVAR_PAGES    2

#ifndef __ASSEMBLY__
#include <generated/vdso-offsets.h>

#define VDSO_SYMBOL(base, name)							\
	(void __user *)((unsigned long)(base) + __vdso_##name##_offset)
#endif /* !__ASSEMBLY__ */

#endif /* CONFIG_MMU */

#endif /* _ASM_RISCV_VDSO_H */
