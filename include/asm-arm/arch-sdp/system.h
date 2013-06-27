/*
 * linux/include/asm-arm/arch-sdp/system.h
 *
 * Copyright 2006 Samsung Electronics co.
 * Author: tukho.kim@samsung.com
 */
#ifndef __ASM_ARCH_SYSTEM_H
#define __ASM_ARCH_SYSTEM_H

#include <asm/hardware.h>
#include <asm/io.h>
#include <asm/arch/platform.h>

extern int intrTestCnt;

static inline void arch_idle(void)
{
        /*
         * This should do all the clock switching
         * and wait for interrupt ticks, 
	 *  using linux library 
         */
#ifndef CONFIG_POLL_INTR_PEND
	cpu_do_idle();
#else
	do {
		if(R_INTPND) break;
	} while(1);


#  ifndef INTCON_IRQ_DIS
#  	define SDP_INTC_REG(x)	(VA_INTC_BASE + x)
#	define R_SDP_INTC(x)		(*(volatile unsigned int*)x)
#	define SDP_INTC_INTCON	SDP_INTC_REG(0)
#	define INTCON_IRQ_DIS (1 << 1)
#  endif

	if(intrTestCnt){
		if(R_SDP_INTC(SDP_INTC_INTCON) & INTCON_IRQ_DIS)
			R_SDP_INTC(SDP_INTC_INTCON) &= ~(INTCON_IRQ_DIS);
		printk("@@@@@@@@@@@@@@@@@@@@@@@@@@@\n");
		printk("intrTestCnt is not zero %d\n", intrTestCnt);
		printk("@@@@@@@@@@@@@@@@@@@@@@@@@@@\n");
		intrTestCnt = 0;
	}
#endif
}

static inline void arch_reset(char mode)
{
        /* use the watchdog timer reset to reset the processor */

        /* (at this point, MMU is shut down, so we use physical addrs) */
        volatile unsigned long *prWTCON = (unsigned long*) (PA_WDT_BASE + 0x00);
        volatile unsigned long *prWTDAT = (unsigned long*) (PA_WDT_BASE + 0x04);
        volatile unsigned long *prWTCNT = (unsigned long*) (PA_WDT_BASE + 0x08);

        /* set the countdown timer to a small value before enableing WDT */
        *prWTDAT = 0x00000100;
        *prWTCNT = 0x00000100;

        /* enable the watchdog timer */
        *prWTCON = 0x00008021;

        /* machine should reboot..... */
        mdelay(5000);
        panic("Watchdog timer reset failed!\n");
        printk(" Jump to address 0 \n");
        cpu_reset(0);
}

#endif

