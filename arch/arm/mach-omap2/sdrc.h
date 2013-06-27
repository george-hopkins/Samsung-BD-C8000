#ifndef __ARCH_ARM_MACH_OMAP2_SDRC_H
#define __ARCH_ARM_MACH_OMAP2_SDRC_H

/*
 * OMAP2 SDRC register definitions
 *
 * Copyright (C) 2007 Texas Instruments, Inc.
 * Copyright (C) 2007 Nokia Corporation
 *
 * Written by Paul Walmsley
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#undef DEBUG

#include <linux/kernel.h>
#include <asm/arch/sdrc.h>

extern unsigned long omap2_sdrc_base;
extern unsigned long omap2_sms_base;

#define OMAP_SDRC_REGADDR(reg)	(void __iomem *)IO_ADDRESS(omap2_sdrc_base + reg)
#define OMAP_SMS_REGADDR(reg)	(void __iomem *)IO_ADDRESS(omap2_sms_base + reg)


/* SDRC global register get/set */

static void __attribute__((unused)) sdrc_write_reg(u32 val, u16 reg)
{
	pr_debug("sdrc_write_reg: writing 0x%0x to 0x%0x\n", val,
		 (u32)OMAP_SDRC_REGADDR(reg));

	__raw_writel(val, OMAP_SDRC_REGADDR(reg));
}

static u32 __attribute__((unused)) sdrc_read_reg(u16 reg)
{
	return __raw_readl(OMAP_SDRC_REGADDR(reg));
}

/* SMS global register get/set */

static void __attribute__((unused)) sms_write_reg(u32 val, u16 reg)
{
	pr_debug("sms_write_reg: writing 0x%0x to 0x%0x\n", val,
		 (u32)OMAP_SMS_REGADDR(reg));

	__raw_writel(val, OMAP_SMS_REGADDR(reg));
}

static u32 __attribute__((unused)) sms_read_reg(u16 reg)
{
	return __raw_readl(OMAP_SMS_REGADDR(reg));
}



#endif
