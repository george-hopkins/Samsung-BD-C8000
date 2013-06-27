/*
 * linux/arch/arm/mach-ssdtv/time.c
 *
 * Copyright (C) 2006 Samsung Electronics.co
 * Author : tukho.kim@samsung.com
 *
 */
#include <linux/config.h>
#include <linux/init.h>
#include <linux/dma-mapping.h>
#include <linux/sysdev.h>
#include <linux/interrupt.h>

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
                                         
#define R_TMCON2                rTMCON2   
#define R_TMDATA2               rTMDATA2  
#define R_TMCNT2                rTMCNT2   
                                         
#define R_TMSTATUS              rTMSTATUS 

#define R_SYSTMCNT		rSYSTMCNT
#define R_SYSTMCON		rSYSTMCON	
#define R_SYSTMDATA 		rSYSTMDATA 
#endif

extern void clk_init(void);
extern unsigned long SSDTV_GET_TIMERCLK(char mode);

struct cal_usec {
	unsigned long usec;
	unsigned long nsec;
};

static unsigned int last_SYSTMCNT = 0;
static struct cal_usec usecPerClk = {0, 0};

// Calcuration about usec per 1 tick for another parts
void ssdtv_get_time_tick(unsigned long* maxCount, unsigned long* pNsec)
{
	unsigned long timerSrcClk;
	unsigned long nsec;

	if(last_SYSTMCNT == 0){
		clk_init();

	 	timerSrcClk = SSDTV_GET_TIMERCLK(TIMER_CLOCK) >> 2;		

	/*timer scale is 1ns -> 1000000us * 1000 */
		if ((SYS_TICK > 0) && (SYS_TICK <= 1000)) {
			last_SYSTMCNT = (timerSrcClk/SYS_TICK/(SYS_TIMER_PRESCALER+1));
			nsec = 1000000000 / SYS_TICK / last_SYSTMCNT;
		}
		else {
			last_SYSTMCNT = (timerSrcClk/100/(SYS_TIMER_PRESCALER + 1));
			nsec = 1000000000 / 100 / last_SYSTMCNT;
		}

		last_SYSTMCNT--;
		usecPerClk.nsec = nsec % 1000;
		usecPerClk.usec = nsec / 1000;

		printk("Samsung DTV Linux Calcurate Timer Clock %d.%03d us per tick\n\tReset Value 0x%08x\n", 
				(int)usecPerClk.usec, (int)usecPerClk.nsec, last_SYSTMCNT);	
	}

	else {
		nsec = (usecPerClk.usec * 1000) + usecPerClk.nsec;
		printk("Samsung DTV Linux Calcurate Timer Clock %d.%03d us per tick\n\tReset Value 0x%08x\n", 
				(int)usecPerClk.usec, (int)usecPerClk.nsec, last_SYSTMCNT);	
	}

	*maxCount = last_SYSTMCNT;
	*pNsec = nsec;

	return;
}
EXPORT_SYMBOL(ssdtv_get_time_tick);

unsigned long ssdtv_timer_read(void)
{
	return R_SYSTMCNT;
}
EXPORT_SYMBOL(ssdtv_timer_read);

static unsigned long ssdtv_gettimeoffset (void)
{
	unsigned long usec;
	unsigned long nsec;
	unsigned int pastCNT;
	unsigned int currentCNT = R_SYSTMCNT;

// check timer interrupt 
	if (R_TMSTATUS & SYS_TIMER_BIT)
		return 1000000 / HZ;

// check timer reloading
	if (currentCNT > last_SYSTMCNT)
		return 0;
 
// calcurate usec delay
//	pastCNT = last_SYSTMCNT - R_SYSTMCNT;  // this code is wrong
	pastCNT = last_SYSTMCNT - currentCNT;

	usec = (unsigned long)usecPerClk.usec * pastCNT;
	nsec = (unsigned long)usecPerClk.nsec * pastCNT;
	usec += nsec / 1000;

	if ((nsec % 1000) > 499) usec++;

	return usec;
}

// interrupt service routine
static irqreturn_t
ssdtv_timer_interrupt(int irq, void *dev_id, struct pt_regs *regs)
{

	if (!(R_TMSTATUS & SYS_TIMER_BIT))
		return IRQ_NONE;

	R_TMSTATUS = SYS_TIMER_BIT;

	write_seqlock(&xtime_lock);

	timer_tick(regs);

	write_sequnlock(&xtime_lock);

	return IRQ_HANDLED;
}

// interrupt resource 
static struct irqaction ssdtv_timer_irq = {
	.name	= "SSDTV Timer tick",

	.flags	= IRQF_SHARED | IRQF_DISABLED,

	.handler = ssdtv_timer_interrupt
};


// Initailize the timer
static void __init ssdtv_timer_init(void)
{
	unsigned long timerData, nsec;

	printk(KERN_INFO"Samsung DTV Linux System timer initialize\n");

	R_TMDMASEL = 0;

	/* not have to do this */
#ifdef R_TMCON0
	R_TMCON0 = 0;
#endif
#ifdef R_TMCON1
	R_TMCON1 = 0;
#endif
#ifdef R_TMCON2
	R_TMCON2 = 0;
#endif

	R_SYSTMCON = TMCON_MUX04;
	
	ssdtv_get_time_tick(&timerData, &nsec);

	R_SYSTMDATA = timerData | (SYS_TIMER_PRESCALER << 16);

	setup_irq(SYS_TIMER_IRQ, &ssdtv_timer_irq);

	R_SYSTMCON |= TMCON_INT_DMA_EN;
	R_SYSTMCON |= TMCON_RUN;
}

struct sys_timer ssdtv_timer = {
	.init	= ssdtv_timer_init,
	.offset	= ssdtv_gettimeoffset,
};
