/*
 * linux/arch/arm/mach-sdp/sdp_hrtime.c
 *
 * Copyright (C) 2008 Samsung Electronics.co
 * Author : tukho.kim@samsung.com
 *
 */
#include <linux/init.h>
#include <linux/dma-mapping.h>
#include <linux/sysdev.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/spinlock.h>

/* hrtimer headr file */
#include <linux/clocksource.h>
#include <linux/clockchips.h>
/* hrtimer headr file end */

#include <asm/mach/time.h>
#include <asm/arch/platform.h>


#ifndef R_TMDMASEL
#define R_TMDMASEL		rTMDMASEL

#define R_TMCON0                rTMCON0   
#define R_TMDATA0               rTMDATA0  
#define R_TMCNT0                rTMCNT0   
                                         
#define R_TMCON1                rTMCON1   
#define R_TMDATA1               rTMDATA1  
#define R_TMCNT1                rTMCNT1   

#  ifdef rTMCON2                                         
#define R_TMCON2                rTMCON2   
#define R_TMDATA2               rTMDATA2  
#define R_TMCNT2                rTMCNT2   
#  endif
                                         
#define R_TMSTATUS              rTMSTATUS 

#define R_SYSTMCNT		rSYSTMCNT
#define R_SYSTMCON		rSYSTMCON	
#define R_SYSTMDATA 		rSYSTMDATA 

#define R_CLKSRCTMCON		rCLKSRCTMCON	
#define R_CLKSRCTMDATA		rCLKSRCTMDATA	
#define R_CLKSRCTMCNT		rCLKSRCTMCNT	
#endif

static unsigned long clksrc_overflow = 0;
static unsigned char sdp_timer_prescaler = 1;
static spinlock_t sdp_hrtimer_lock;
enum clock_event_mode clkevent_mode = CLOCK_EVT_ONESHOT;

extern unsigned long SSDTV_GET_TIMERCLK(char);

static void sdp_clkevent_setmode(enum clock_event_mode mode,
				   struct clock_event_device *clk)
{
	if (mode == CLOCK_EVT_PERIODIC) {
		clkevent_mode = mode;
		R_SYSTMCON = (TMCON_MUX04 | TMCON_INT_DMA_EN | TMCON_RUN);
	} else if (mode == CLOCK_EVT_ONESHOT) {
		clkevent_mode = mode;
		R_SYSTMCON = (TMCON_MUX04 | TMCON_INT_DMA_EN | TMCON_RUN);
	} else { 
		R_SYSTMCON = 0;
	}
}

static void sdp_clkevent_nextevent(unsigned long evt,
				 struct clock_event_device *unused)
{
	BUG_ON(!evt);

	if(evt > 0xFFFF)
		printk(KERN_DEBUG "HRTIMER: evt overflow 0x%08x\n",(unsigned int)evt);

	R_SYSTMCON = 0;
	R_SYSTMDATA = (sdp_timer_prescaler << 16) | (evt & 0xFFFF);  
	R_SYSTMCON = (TMCON_MUX04 | TMCON_INT_DMA_EN | TMCON_RUN);
}

struct clock_event_device sdp_clockevent = {
	.name		= "SSDTV Timer clock event",
	.shift		= 32,		// nanosecond shift 
	.capabilities	= CLOCK_CAP_TICK | CLOCK_CAP_UPDATE | 
			  CLOCK_CAP_NEXTEVT | CLOCK_CAP_PROFILE,
	.set_mode	= sdp_clkevent_setmode,
	.set_next_event = sdp_clkevent_nextevent,
};

static irqreturn_t sdp_hrtimer_isr(int irq, void *dev_id, struct pt_regs *regs)
{
	unsigned int regVal = R_TMSTATUS;

	if(!regVal) return IRQ_NONE;

	if(regVal & CLKSRC_TIMER_BIT){
		R_TMSTATUS = CLKSRC_TIMER_BIT;  // clock source timer intr pend clear
		clksrc_overflow++;	       // clock source timer overflow 
	}
	
	if(regVal & SYS_TIMER_BIT){
		if (clkevent_mode == CLOCK_EVT_ONESHOT) R_SYSTMCON = 0;
		R_TMSTATUS = SYS_TIMER_BIT;  // clock event timer intr pend clear
		sdp_clockevent.event_handler(regs); 
	}

	return IRQ_HANDLED;
}

static struct irqaction sdp_hrtimer_event = {
	.name = "SSDTV Hrtimer interrupt handler",
	.flags = IRQF_DISABLED | IRQF_TIMER,
	.handler = sdp_hrtimer_isr,
};

cycle_t sdp_clksrc_read (void)
{
	unsigned long retVal;
	unsigned long flags;	
	static unsigned long oldVal = 0;	

	spin_lock_irqsave(&sdp_hrtimer_lock, flags);

__sdp_clksrc_read:
	if(unlikely(R_TMSTATUS & CLKSRC_TIMER_BIT))
		retVal = ((clksrc_overflow + 1) << 16) | (0xFFFF - (R_CLKSRCTMCNT & 0xFFFF));
	else
		retVal = (clksrc_overflow  << 16) | (0xFFFF - (R_CLKSRCTMCNT & 0xFFFF));

// correction
	if (unlikely(oldVal > retVal)) {
		printk(KERN_DEBUG "clksrc read is overflow value old: 0x%x, current: 0x%x\n", 
				(unsigned int)oldVal, (unsigned int)retVal);
		if((oldVal - retVal) < 0x10000)  // check clock source overlap
#if 0	// set += 0x10000
			retVal += 0x10000;  // simply add 1 clksrc_overflow
#else	// re-read
			goto __sdp_clksrc_read; // re-read clock source
#endif
	}
// correction end
	oldVal = retVal;		// to correct clock source to avoid time warp

	spin_unlock_irqrestore(&sdp_hrtimer_lock, flags);

	return retVal;
}

struct clocksource sdp_clocksource = {
	.name 		= "SSDTV Timer clock source",
	.rating 	= 200,
	.read 		= sdp_clksrc_read,
	.mask 		= 0xFFFFFFFF,
	.shift 		= 21,		
	.is_continuous 	= 1,
};

/* Initialize Timer */

/*  Polaris timer 
// source clock -> 105Mhz
// 3 x 5 x 7 Mhz
// Timer tick = source clock / prescaler / 4 or 8 or 16
// prescaler = 1 ~ 255
// Timer tick -> 500  khz
*/ 
static void __init sdp_hrtimer_init(void)
{
	unsigned long sysTimerClk;

	printk(KERN_INFO "Samsung DTV Linux System HRTimer initialize\n");
	
	R_TMDMASEL = 0;

// Timer reset & stop
#ifdef R_TMCON0
	R_TMCON0 = 0;
#endif
#ifdef R_TMCON1
	R_TMCON1 = 0;
#endif
#ifdef R_TMCON2
	R_TMCON2 = 0;
#endif

// get Timer source clock 
	sysTimerClk = SSDTV_GET_TIMERCLK(TIMER_CLOCK) >> 2;  //MUX04
	sdp_timer_prescaler = ((sysTimerClk / SSDTV_TIMER_TICK) - 1) & 0xFF;
	sysTimerClk = sysTimerClk / (sdp_timer_prescaler + 1);

	printk(KERN_INFO "HRTIMER: timer clock is %u Hz, prescaler %u \n", 
			(unsigned int)sysTimerClk, (unsigned int)sdp_timer_prescaler);
// init lock 
	spin_lock_init(&sdp_hrtimer_lock);

// register timer interrupt service routine
	setup_irq(SYS_TIMER_IRQ, &sdp_hrtimer_event);
	
// After Prescaler and divider
	R_SYSTMCON = TMCON_MUX04;
	R_SYSTMDATA = (sdp_timer_prescaler << 16) | (0xF);   // set minimum value
	R_CLKSRCTMCON = TMCON_MUX04;
	R_CLKSRCTMDATA = (sdp_timer_prescaler << 16) | (0xFFFF);  

// clock source init -> clock source 
	sdp_clocksource.mult = clocksource_khz2mult(sysTimerClk/1000, sdp_clocksource.shift);
// clock source multiplier and clock event device multiplier is not same !!!!
//   when apply this line, it dosen't work 
//	sdp_clocksource.mult = div_sc(sysTimerClk, NSEC_PER_SEC, sdp_clocksource.shift);
	clocksource_register(&sdp_clocksource);
	
	R_CLKSRCTMCON |= TMCON_INT_DMA_EN;
	R_CLKSRCTMCON |= TMCON_RUN;

// timer event init
	sdp_clockevent.mult = 
			div_sc(sysTimerClk, NSEC_PER_SEC, sdp_clockevent.shift);
	sdp_clockevent.max_delta_ns =
			clockevent_delta2ns(0xffff, &sdp_clockevent);
	sdp_clockevent.min_delta_ns =
			clockevent_delta2ns(0xf, &sdp_clockevent);
	register_global_clockevent(&sdp_clockevent);
}

struct sys_timer sdp_timer = {
	.init		= sdp_hrtimer_init,
};

