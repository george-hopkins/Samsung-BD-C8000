/*
 * linux/arch/arm/mach-omap2/timer-gp.c
 *
 * OMAP2 GP timer support.
 *
 * Update to use new clocksource/clockevent layers
 * Author: Kevin Hilman, MontaVista Software, Inc. <source@mvista.com>
 * Copyright (C) 2007 MontaVista Software, Inc.
 *
 * Original driver:
 * Copyright (C) 2005 Nokia Corporation
 * Author: Paul Mundt <paul.mundt@nokia.com>
 *         Juha Yrjölä <juha.yrjola@nokia.com>
 * OMAP Dual-mode timer framework support by Timo Teras
 *
 * Some parts based off of TI's 24xx code:
 *
 *   Copyright (C) 2004 Texas Instruments, Inc.
 *
 * Roughly modelled after the OMAP1 MPU timer code.
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License. See the file "COPYING" in the main directory of this archive
 * for more details.
 */
#include <linux/init.h>
#include <linux/time.h>
#include <linux/interrupt.h>
#include <linux/err.h>
#include <linux/clk.h>
#include <linux/delay.h>
#include <linux/irq.h>
#include <linux/clocksource.h>
#include <linux/clockchips.h>

#include <asm/mach/time.h>
#include <asm/arch/dmtimer.h>

static struct omap_dm_timer *gptimer;
static struct clock_event_device clockevent_gpt;
static int ti_compat_oneshot;

static irqreturn_t omap2_gp_timer_interrupt(int irq, void *dev_id)
{
	struct omap_dm_timer *gpt = (struct omap_dm_timer *)dev_id;
	struct clock_event_device *evt = &clockevent_gpt;

	omap_dm_timer_write_status(gpt, OMAP_TIMER_INT_OVERFLOW);

	evt->event_handler(evt);
	return IRQ_HANDLED;
}

static struct irqaction omap2_gp_timer_irq = {
	.name		= "gp timer",
	.flags		= IRQF_DISABLED | IRQF_TIMER | IRQF_IRQPOLL,
	.handler	= omap2_gp_timer_interrupt,
};

static int omap2_gp_timer_set_next_event(unsigned long cycles,
					 struct clock_event_device *evt)
{
	omap_dm_timer_set_load(gptimer, 0, 0xffffffff - cycles);
	omap_dm_timer_start(gptimer);

	return 0;
}

static void omap2_gp_timer_set_mode(enum clock_event_mode mode,
				    struct clock_event_device *evt)
{
	u32 period;

	omap_dm_timer_stop(gptimer);
	ti_compat_oneshot = 0;

	switch (mode) {
	case CLOCK_EVT_MODE_PERIODIC:
		period = clk_get_rate(omap_dm_timer_get_fclk(gptimer)) / HZ;
		period -= 1;

		omap_dm_timer_set_load(gptimer, 1, 0xffffffff - period);
		omap_dm_timer_start(gptimer);
		break;
	case CLOCK_EVT_MODE_ONESHOT:
		ti_compat_oneshot = 1;
		break;
	case CLOCK_EVT_MODE_UNUSED:
	case CLOCK_EVT_MODE_SHUTDOWN:
	case CLOCK_EVT_MODE_RESUME:
		break;
	}
}

static struct clock_event_device clockevent_gpt = {
	.name		= "gp timer",
	.features       = CLOCK_EVT_FEAT_PERIODIC | CLOCK_EVT_FEAT_ONESHOT,
	.shift		= 32,
	.set_next_event	= omap2_gp_timer_set_next_event,
	.set_mode	= omap2_gp_timer_set_mode,
};

static void __init omap2_gp_clockevent_init(void)
{
	u32 tick_rate;

	gptimer = omap_dm_timer_request_specific(1);
	BUG_ON(gptimer == NULL);

#if defined(CONFIG_OMAP_32K_TIMER)
	omap_dm_timer_set_source(gptimer, OMAP_TIMER_SRC_32_KHZ);
#else
	omap_dm_timer_set_source(gptimer, OMAP_TIMER_SRC_SYS_CLK);
#endif
	tick_rate = clk_get_rate(omap_dm_timer_get_fclk(gptimer));

	omap2_gp_timer_irq.dev_id = (void *)gptimer;
	setup_irq(omap_dm_timer_get_irq(gptimer), &omap2_gp_timer_irq);
	omap_dm_timer_set_int_enable(gptimer, OMAP_TIMER_INT_OVERFLOW);

	clockevent_gpt.mult = div_sc(tick_rate, NSEC_PER_SEC,
				     clockevent_gpt.shift);
	clockevent_gpt.max_delta_ns =
		clockevent_delta2ns(0xffffffff, &clockevent_gpt);
	clockevent_gpt.min_delta_ns =
		clockevent_delta2ns(1, &clockevent_gpt);

	clockevent_gpt.cpumask = cpumask_of_cpu(0);
	clockevents_register_device(&clockevent_gpt);
}

#ifdef CONFIG_OMAP_32K_TIMER
/* 
 * When 32k-timer is enabled, don't use GPTimer for clocksource
 * instead, just leave default clocksource which uses the 32k
 * sync counter.  See clocksource setup in see plat-omap/common.c. 
 */

static inline void __init omap2_gp_clocksource_init(void) {}
#else
/*
 * clocksource
 */
static struct omap_dm_timer *gpt_clocksource;
static cycle_t clocksource_read_cycles(void)
{
	return (cycle_t)omap_dm_timer_read_counter(gpt_clocksource);
}

static struct clocksource clocksource_gpt = {
	.name		= "gp timer",
	.rating		= 300,
	.read		= clocksource_read_cycles,
	.mask		= CLOCKSOURCE_MASK(32),
	.shift		= 24,
	.flags		= CLOCK_SOURCE_IS_CONTINUOUS,
};

/* Setup free-running counter for clocksource */
static void __init omap2_gp_clocksource_init(void)
{
	static struct omap_dm_timer *gpt;
	u32 tick_rate, tick_period;
	static char err1[] __initdata = KERN_ERR
		"%s: failed to request dm-timer\n";
	static char err2[] __initdata = KERN_ERR
		"%s: can't register clocksource!\n";

	gpt = omap_dm_timer_request();
	if (!gpt)
		printk(err1, clocksource_gpt.name);
	gpt_clocksource = gpt;

	omap_dm_timer_set_source(gpt, OMAP_TIMER_SRC_SYS_CLK);
	tick_rate = clk_get_rate(omap_dm_timer_get_fclk(gpt));
	tick_period = (tick_rate / HZ) - 1;

	omap_dm_timer_set_load(gpt, 1, 0);
	omap_dm_timer_start(gpt);

	clocksource_gpt.mult =
		clocksource_khz2mult(tick_rate/1000, clocksource_gpt.shift);
	if (clocksource_register(&clocksource_gpt))
		printk(err2, clocksource_gpt.name);
}
#endif

static void __init omap2_gp_timer_init(void)
{
	omap_dm_timer_init();

	omap2_gp_clockevent_init();
	omap2_gp_clocksource_init();
}

struct sys_timer omap_timer = {
	.init	= omap2_gp_timer_init,
};

/* ============ TI kernel compat =================== */
static unsigned long long last_tick;

void omap_32k_timer_start(unsigned long load_val)
{
	int oneshot = ti_compat_oneshot;

	if (oneshot)
		omap_dm_timer_set_load(gptimer, 0, 0xffffffff - load_val);
	else
		omap_dm_timer_set_load(gptimer, 1, 0xffffffff - load_val);
	omap_dm_timer_start(gptimer);
}

unsigned long omap_32k_ticks_to_usecs(unsigned long ticks_32k)
{
	unsigned long nsecs = clockevent_delta2ns(ticks_32k, &clockevent_gpt);

	return nsecs/1000;
}

static inline void omap_32k_timer_ack_irq(void)
{
	u32 status = omap_dm_timer_read_status(gptimer);
	omap_dm_timer_write_status(gptimer, status);
}

#if defined(CONFIG_ARCH_OMAP24XX) || defined(CONFIG_ARCH_OMAP34XX)
#define TIMER_32K_SYNCHRONIZED		(OMAP2_32KSYNCT_BASE + 0x10)
#endif

#ifdef	TIMER_32K_SYNCHRONIZED
unsigned long omap_32k_sync_timer_read(void)
{
	return omap_readl(TIMER_32K_SYNCHRONIZED);
}
#endif

#ifdef CONFIG_OMAP_32K_TIMER
#define OMAP_32K_TICKS_PER_SEC		(32768)
#define OMAP_32K_TIMER_TICK_PERIOD	((OMAP_32K_TICKS_PER_SEC / HZ) - 1)

void omap_disable_system_timer(void)
{
	omap_dm_timer_set_int_enable(gptimer,0);
	omap_dm_timer_stop(gptimer);
/* 	while(omap_dm_timer_read_reg(gptimer, OMAP_TIMER_WRITE_PEND_REG)); */
	omap_32k_timer_ack_irq();
	omap_32k_timer_ack_irq();
	last_tick = omap_32k_sync_timer_read();
}

void omap_enable_system_timer(void)
{
	unsigned long long now = omap_32k_sync_timer_read();
	unsigned long long tick_cnt = 0;
	unsigned long rem;
	unsigned long long rem_u64;
	struct timespec sleeptime;
	unsigned long period;

	omap_dm_timer_set_int_enable(gptimer, OMAP_TIMER_INT_OVERFLOW);
	omap_dm_timer_start(gptimer);
	/* wait for all the writes to complete as we are running in 
	*posted mode */
/* 	while(omap_dm_timer_read_reg(gptimer, OMAP_TIMER_WRITE_PEND_REG)); */

	tick_cnt = (now - last_tick);
	do_div(tick_cnt, OMAP_32K_TIMER_TICK_PERIOD+1);

        /* Fix up the wall time */
	rem = do_div(tick_cnt,HZ);
	sleeptime.tv_sec = tick_cnt;
	rem_u64 = 1000000000UL;
	rem_u64 = rem_u64*rem;
	do_div(rem_u64,HZ);

	sleeptime.tv_nsec = rem_u64;

	if (!(in_atomic() || irqs_disabled()))
		write_seqlock_irq(&xtime_lock);

	xtime.tv_sec += sleeptime.tv_sec;
	xtime.tv_nsec += sleeptime.tv_nsec;

	/* nsec may have overflowed 1 sec */
	while (xtime.tv_nsec >= 1000000000) {
		xtime.tv_nsec -= 1000000000;
		xtime.tv_sec++;
	}

	if (!(in_atomic() || irqs_disabled()))
		write_sequnlock_irq(&xtime_lock);

	omap_32k_timer_ack_irq();
}
#endif
