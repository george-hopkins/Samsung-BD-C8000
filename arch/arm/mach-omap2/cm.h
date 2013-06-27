#ifndef __ARCH_ASM_MACH_OMAP2_CM_H
#define __ARCH_ASM_MACH_OMAP2_CM_H

/*
 * OMAP2/3 Clock Management (CM) register definitions
 *
 * Copyright (C) 2007-2008 Texas Instruments, Inc.
 * Copyright (C) 2007-2008 Nokia Corporation
 *
 * Written by Paul Walmsley
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/kernel.h>
#include <asm/io.h>
#include "prcm_common.h"


#define OMAP_CM_REGADDR(module, reg)	 (void __iomem *)IO_ADDRESS(OMAP2_CM_BASE + module + reg)

/*
 * Architecture-specific global CM registers
 * Use cm_{read,write}_reg() with these registers.
 */

#define OMAP3430_CM_REVISION		OMAP_CM_REGADDR(OCP_MOD, 0x0000)
#define OMAP3430_CM_SYSCONFIG		OMAP_CM_REGADDR(OCP_MOD, 0x0010)
#define OMAP3430_CM_POLCTRL		OMAP_CM_REGADDR(OCP_MOD, 0x009c)

#define OMAP3430_CM_CLKOUT_CTRL		OMAP_CM_REGADDR(OMAP3430_CCR_MOD, 0x0070)


/* Clock management global register get/set */

static void __attribute__((unused)) cm_write_reg(u32 val, void __iomem *addr)
{
	pr_debug("cm_write_reg: writing 0x%0x to 0x%0x\n", val, (u32)addr);

	__raw_writel(val, addr);
}

static u32 __attribute__((unused)) cm_read_reg(void __iomem *addr)
{
	return __raw_readl(addr);
}


/*
 * Module specific CM registers from CM_BASE + domain offset
 * Use cm_{read,write}_mod_reg() with these registers.
 */

/* Common between 24xx and 34xx */

#define CM_FCLKEN1					0x0000
#define CM_FCLKEN					CM_FCLKEN1
#define CM_CLKEN					CM_FCLKEN1
#define CM_ICLKEN1					0x0010
#define CM_ICLKEN					CM_ICLKEN1
#define CM_ICLKEN2					0x0014
#define CM_ICLKEN3					0x0018
#define CM_IDLEST1					0x0020
#define CM_IDLEST                                       CM_IDLEST1
#define CM_IDLEST2					0x0024
#define CM_AUTOIDLE					0x0030
#define CM_AUTOIDLE1					0x0030
#define CM_AUTOIDLE2					0x0034
#define CM_CLKSEL					0x0040
#define CM_CLKSEL1					CM_CLKSEL
#define CM_CLKSEL2					0x0044
#define CM_CLKSTCTRL					0x0048


/* Architecture-specific registers */

#define OMAP24XX_CM_FCLKEN2				0x0004
#define OMAP24XX_CM_ICLKEN4				0x001c
#define OMAP24XX_CM_AUTOIDLE3				0x0038
#define OMAP24XX_CM_AUTOIDLE4				0x003c

#define OMAP2430_CM_IDLEST3				0x0028


/* Clock management domain register get/set */

static void __attribute__((unused)) cm_write_mod_reg(u32 val, s16 module, s16 idx)
{
	cm_write_reg(val, OMAP_CM_REGADDR(module, idx));
}

static u32 __attribute__((unused)) cm_read_mod_reg(s16 module, s16 idx)
{
	return cm_read_reg(OMAP_CM_REGADDR(module, idx));
}

/* CM register bits shared between 24XX and 3430 */

/* CM_CLKSEL_GFX */
#define OMAP_CLKSEL_GFX_SHIFT				0
#define OMAP_CLKSEL_GFX_MASK				(0x7 << 0)

/* CM_ICLKEN_GFX */
#define OMAP_EN_GFX_SHIFT				0
#define OMAP_EN_GFX					(1 << 0)

/* CM_IDLEST_GFX */
#define OMAP_ST_GFX					(1 << 0)

#define OMAP3430_CM_CLKEN_PLL				0x0004
#define OMAP3430ES2_CM_CLKEN2				0x0004
#define OMAP3430ES2_CM_FCLKEN3				0x0008
#define OMAP3430_CM_IDLEST_PLL				CM_IDLEST2
#define OMAP3430_CM_AUTOIDLE_PLL			CM_AUTOIDLE2
#define OMAP3430_CM_CLKSEL1				CM_CLKSEL
#define OMAP3430_CM_CLKSEL1_PLL				CM_CLKSEL
#define OMAP3430_CM_CLKSEL2_PLL				CM_CLKSEL2
#define OMAP3430_CM_SLEEPDEP				CM_CLKSEL2
#define OMAP3430_CM_CLKSEL3				CM_CLKSTCTRL
#define OMAP3430_CM_CLKSTST				0x004c
#define OMAP3430ES2_CM_CLKSEL4				0x004c
#define OMAP3430ES2_CM_CLKSEL5				0x0050
#define OMAP3430_CM_CLKSEL2_EMU				0x0050
#define OMAP3430_CM_CLKSEL3_EMU				0x0054
#define OMAP3430_CM_IDLEST3_CORE			0x0028
#define OMAP3430_CM_AUTOIDLE3_CORE			0x0038



#endif
