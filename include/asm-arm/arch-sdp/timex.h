/*
 *  linux/include/asm-arm/arch-sdp/timex.h
 *
 *  s5h2111(Lake) architecture timex specifications
 *
 *  Copyright (C) 2003 ARM Limited
 *  Copyright 2006 Samsung Electronics co
 *  Author: tukho.kim@samsung.com
 *
 */

#ifndef __ASM_ARCH_SDP_TIMEX_H
#define __ASM_ARCH_SDP_TIMEX_H

#include <asm/arch/platform.h>

#define CLOCK_TICK_RATE		((PCLK >> 2) / (SYS_TIMER_PRESCALER + 1))

#ifndef CONFIG_HIGH_RES_TIMERS
extern unsigned long sdp_timer_read(void);
#define mach_read_cycles() sdp_timer_read()
#else
extern u64 sdp_clksrc_read(void);
#define mach_read_cycles() sdp_clksrc_read()
#endif


#endif /* __ASM_ARCH_SDP_TIMEX_H */
