/*
 * linux/arch/arm/mach-ssdtvxx/ssdtv_irq.c
 *
 * Copyright (C) 2006 Samsung Electronics.co
 * Author : tukho.kim@samsung.com
 *
 */
/*
 *
 */

#include <linux/config.h>
#include <linux/init.h>
#include <linux/device.h>
#include <linux/sysdev.h>
#include <linux/interrupt.h>

#include <asm/system.h>
#include <asm/hardware.h>
#include <asm/io.h>
#include <asm/irq.h>
#include <asm/mach/arch.h>
#include <asm/mach/irq.h>

#include <asm/delay.h>

#include <asm/arch/irqs.h>
#include <asm/arch/platform.h>

#include "ssdtv_irq.h"


static spinlock_t lockIrq;
/* interrupt source is defined in machine header file */
const static SSDTV_INT_T ssdtvInt_init[] = SSDTV_INTERRUPT_RESOURCE;

#if defined(CONFIG_INTC_PRIORITY)
static unsigned long intrDisableFlag = 0xFFFFFFFF;
static unsigned long intrMaskSave[NR_IRQS];
#endif // CONFIG_INTC_PRIORITY

#if defined(CONFIG_CHECK_ISRTIME) && !defined(CONFIG_HIGH_RES_TIMERS)
static volatile unsigned long preTimerTick[NR_IRQS];
static unsigned long sysTimerCnt;
static unsigned long timerTickNsec;

extern void ssdtv_get_time_tick(unsigned long*, unsigned long*);
#endif // CONFIG_CHECK_ISRTIME


static void ssdtv_enable_irq(unsigned int irq)
{
#if defined(CONFIG_INTC_PRIORITY)
	unsigned long flags;

	spin_lock_irqsave(&lockIrq, flags);
	intrDisableFlag &= ~(1 << irq);
	R_SSDTV_INTC(SSDTV_INTC_INTMSK) &= ~(1 << irq);
	spin_unlock_irqrestore(&lockIrq, flags);
#else
	INTMSK_UNMASK(irq);
#endif
}

static void ssdtv_disable_irq(unsigned int irq)
{

#if defined(CONFIG_INTC_PRIORITY)
	unsigned long flags;

	spin_lock_irqsave(&lockIrq, flags);
	intrDisableFlag |= (1 << irq);
	intrMaskSave[irq] |= (1 << irq);
	R_SSDTV_INTC(SSDTV_INTC_INTMSK) |= (1 << irq);
	spin_unlock_irqrestore(&lockIrq, flags);
#else
	INTMSK_MASK(irq);	
#endif
}

static void ssdtv_ack_irq(unsigned int irq)
{
	INTPND_I_CLEAR(irq);
}

static void ssdtv_mask_irq(unsigned int irq)
{

#if defined(CONFIG_INTC_PRIORITY)
	intrMaskSave[irq] = R_SSDTV_INTC(SSDTV_INTC_INTMSK);				  // save previous interrupt mask
	R_SSDTV_INTC(SSDTV_INTC_INTMSK) = ssdtvInt_init[irq].prioMask | intrDisableFlag;   // prioriry mask
	R_SSDTV_INTC(SSDTV_INTC_INTPND) &= ~(ssdtvInt_init[irq].prioMask);  	          // clear interrupt pend register
#else 
#  if defined(CONFIG_INTC_NOT_NESTED)
	R_SSDTV_INTC(SSDTV_INTC_INTCON) |= INTCON_IRQ_DIS;
#  endif
	INTMSK_MASK(irq);		// masking
	INTPND_I_CLEAR(irq);    // avoid to nesting same interrupt when using semi vectored interrupt 
#endif

#ifdef CONFIG_CHECK_ISRTIME
	preTimerTick[irq] = rSYSTMCNT;
#endif
}

static void ssdtv_unmask_irq(unsigned int irq)
{
#ifdef CONFIG_CHECK_ISRTIME
	unsigned long postTimerTick = rSYSTMCNT;
	unsigned long takesTicks;

	takesTicks = (preTimerTick[irq] >= postTimerTick) ? 
			preTimerTick[irq] - postTimerTick : (sysTimerCnt - postTimerTick) + preTimerTick[irq];

	if((takesTicks * timerTickNsec) > 200000)
		printk(KERN_WARNING "[WARNING] %d ISR takes over 200 us\n", irq);
#endif

#if defined(CONFIG_INTC_PRIORITY)
	R_SSDTV_INTC(SSDTV_INTC_INTMSK) = intrMaskSave[irq] | intrDisableFlag;
#else
	INTMSK_UNMASK(irq);
#  if defined(CONFIG_INTC_NOT_NESTED)
	R_SSDTV_INTC(SSDTV_INTC_INTCON) &= ~(INTCON_IRQ_DIS);
#  endif
#endif

}

static struct irqchip ssdtv_intctl = {
	.enable = ssdtv_enable_irq,
	.disable = ssdtv_disable_irq,
	.ack	= ssdtv_ack_irq,
	.mask	= ssdtv_mask_irq,
	.unmask	= ssdtv_unmask_irq,
};

void __init ssdtv_init_irq(void)
{
	int i;
	unsigned int polarity = 0, level = 0;

	spin_lock_init(&lockIrq);

	/* initialize interrupt controller */
	R_SSDTV_INTC(SSDTV_INTC_INTCON) = INTCON_IRQ_FIQ_DIS;
	R_SSDTV_INTC(SSDTV_INTC_INTMOD) = 0x00000000; // all irq mode 
	R_SSDTV_INTC(SSDTV_INTC_INTMSK) = 0xFFFFFFFF; // all interrupt source mask
	R_SSDTV_INTC(SSDTV_INTC_I_ISPC) = 0xFFFFFFFF; // irq status clear
	R_SSDTV_INTC(SSDTV_INTC_F_ISPC) = 0xFFFFFFFF; // fiq status clear
	
	for (i = 0; i < NR_IRQS; i++) {
		polarity |= (ssdtvInt_init[i].polarity & 0x1) << i;
		level    |= (ssdtvInt_init[i].level & 0x1) << i;
	}
	R_SSDTV_INTC(SSDTV_INTC_POLARITY) = polarity; // Polarity high
	R_SSDTV_INTC(SSDTV_INTC_LEVEL) = level;  // Level configuration 

	/* register interrupt resource */
	for (i = 0; i < NR_IRQS; i++) {
		set_irq_chip(i, &ssdtv_intctl);
#if 1
		if (ssdtvInt_init[i].level == LEVEL_EDGE)
			set_irq_handler(i, do_edge_IRQ);
		else
#endif
			set_irq_handler(i, do_level_IRQ);

		set_irq_flags(i, IRQF_VALID | IRQF_PROBE);
	}

	R_SSDTV_INTC(SSDTV_INTC_INTCON) = INTCON_FIQ_DIS | INTCON_VECTORED;
#if defined(CONFIG_CHECK_ISRTIME) && !defined(CONFIG_HIGH_RES_TIMERS)
// add to check time of Isr
	ssdtv_get_time_tick(&sysTimerCnt, &timerTickNsec);
#endif
}

int ssdtv_setExtIrq(int extIrq, int type, int irqSrc)
{
        return 0;
}

EXPORT_SYMBOL(ssdtv_setExtIrq);

