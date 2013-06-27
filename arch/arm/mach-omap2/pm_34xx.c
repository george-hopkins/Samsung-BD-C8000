/*
 * linux/arch/arm/mach-omap3/pm.c
 *
 * OMAP3 Power Management Routines
 *
 * Copyright (C) 2007 Texas Instruments, Inc.
 * Karthik Dasu <karthik-dp@ti.com>
 *
 * Copyright (C) 2006 Nokia Corporation
 * Tony Lindgren <tony@atomide.com>
 *
 * Copyright (C) 2005 Texas Instruments, Inc.
 * Richard Woodruff <r-woodruff2@ti.com>
 *
 * Based on pm.c for omap1
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/suspend.h>
#include <linux/sched.h>
#include <linux/proc_fs.h>
#include <linux/interrupt.h>
#include <linux/sysfs.h>
#include <linux/module.h>
#include <linux/delay.h>
#include <linux/cpuidle.h>

#include <asm/io.h>
#include <asm/irq.h>
#include <asm/atomic.h>
#include <asm/mach/time.h>
#include <asm/mach/irq.h>
#include <asm/mach-types.h>
#include <linux/mm.h>
#include <asm/mmu.h>
#include <asm/tlbflush.h>

#include <asm/arch/irqs.h>
#include <asm/arch/clock.h>
#include <asm/arch/sram.h>
#include <asm/arch/pm.h>
#include <linux/tick.h>
#ifdef CONFIG_OMAP34XX_OFFMODE
#include <asm/arch/io.h>
#endif /* #ifdef CONFIG_OMAP34XX_OFFMODE */
#define SR1_ID 1
#define SR2_ID 2

#include "prcm-regs.h"
void prcm_save_core_context(u32 target_core_state);

static u32 context_mem[128];

/* HACK HACK: TI diplay driver exports a global symbol (yuck!).  Use
 *            a weak symbol here to compile without TI display driver. */
int lpr_enabled __attribute__((weak));

#if 0
#define DEBUG_HW_SUP 1
#endif

#define S600M   600000000
#define S550M   550000000
#define S500M   500000000
#define S250M   250000000
#define S125M   125000000
#define S19M    19200000
#define S120M   120000000
#define S477M   476000000
#define S381M   381000000
#define S191M   190000000
#define S96M    96000000
#define S166M	166000000
#define S83M	83000000

#define PRCM_MPU_IRQ	0xB /* MPU IRQ 11 */
#define CORE_FCLK_MASK	0x3FF9E29 /* Mask of all functional clocks*/
						/*in core except UART*/
#ifdef CONFIG_OMAP3430_ES2
#define CORE3_FCLK_MASK	0x7 /* Mask of all functional clocks*/
#define USBHOST_FCLK_MASK	0x3 /* Mask of all functional clocks in USB*/
#ifndef CONFIG_ARCH_OMAP3410
#define SGX_FCLK_MASK	0x2 /*Mask of all functional clocks in SGX*/
#endif
#else
#define GFX_FCLK_MASK	0x6 /*Mask of all functional clocks in GFX*/
#endif

#define DSS_FCLK_MASK	0x7 /*Mask of all functional clocks in DSS*/
#define CAM_FCLK_MASK	0x1 /*Mask of all functional clocks in CAM*/
#define PER_FCLK_MASK	0x17FF /*Mask of all functional clocks in PER */
						/*except UART and GPIO*/
#define CORE1_ICLK_VALID	0x3FFFFFF9 /*Ignore SDRC_ICLK*/
#define CORE2_ICLK_VALID	0x1F
#ifdef CONFIG_OMAP3430_ES2
#define CORE3_ICLK_VALID	0x4
#define USBHOST_ICLK_VALID	0x1
#ifndef CONFIG_ARCH_OMAP3410
#define	SGX_ICLK_VALID	0x1
#endif
#else
#define	GFX_ICLK_VALID	0x1
#endif

#define DSS_ICLK_VALID	0x1
#define CAM_ICLK_VALID	0x1
#define PER_ICLK_VALID 0x1
#define WKUP_ICLK_VALID 0x3E /* Ignore GPT1 ICLK as it is hanlded explicitly*/

#define OMAP3_MAX_STATES 7
#define OMAP3_STATE_C0 0 /* MPU ACTIVE + CORE ACTIVE(only WFI)*/
#define OMAP3_STATE_C1 1 /* MPU WFI + Dynamic tick + CORE ACTIVE*/
#define OMAP3_STATE_C2 2 /* MPU CSWR + CORE ACTIVE*/
#define OMAP3_STATE_C3 3 /* MPU OFF + CORE ACTIVE*/
#define OMAP3_STATE_C4 4 /* MPU RET + CORE CSWR */
#define OMAP3_STATE_C5 5 /* MPU OFF + CORE CSWR */
#define OMAP3_STATE_C6 6 /* MPU OFF + CORE OFF */
#ifdef CONFIG_CORE_OFF_CPUIDLE
#define MAX_SUPPORTED_STATES 7
#else
/*Currently, we support only upto C4*/
#define MAX_SUPPORTED_STATES 5
#endif

#define IOPAD_WKUP 1

/* #define DEBUG_CPUIDLE 1 */
#ifdef DEBUG_CPUIDLE
#  define DPRINTK(fmt, args...) printk(KERN_ERR "%s: " fmt, __FUNCTION__ ,\
									## args)
#else
#  define DPRINTK(fmt, args...)
#endif

#ifdef CONFIG_PREVENT_MPU_RET
int  cpuidle_deepest_st = 1;
#elif defined(CONFIG_PREVENT_CORE_RET)
int  cpuidle_deepest_st = 2;
#else
int cpuidle_deepest_st = MAX_SUPPORTED_STATES - 1;/*Currently only states */
						/*upto C3 are supported*/
#endif

/* Macro for debugging */
#define RESTORE_TABLE_ENTRY 1

/* Defines the default value of frame buffer timeout*/
int fb_timeout_val = 30; /*30 seconds*/
u32 target_opp_no;
int enable_off = 1;
u32 debug_domain = DOM_CORE1;

struct res_handle  *memret1;
struct res_handle  *memret2;
struct res_handle  *logret1;
int res1_level = -1, res2_level = -1, res3_level = -1;

static struct constraint_handle *vdd1_opp_co;
static struct constraint_handle *vdd2_opp_co;
static struct constraint_id cnstr_id1 = {
	.type = RES_FREQ_CO,
	.data = (void *)"vdd1_opp",
};

static struct constraint_id cnstr_id2 = {
	.type = RES_FREQ_CO,
	.data = (void *)"vdd2_opp",
};


extern void omap_uart_save_ctx(void);
extern void omap_uart_restore_ctx(void);
extern void set_blank_interval(int fb_timeout_val);
#define UART_TIME_OUT 6000 /* ms before cutting clock */

#define SCRATCHPAD_ROM	0x48002860
#define SCRATCHPAD	0x48002910
#define SCRATHPAD_ROM_OFFSET	0x19C
#define TABLE_ADDRESS_OFFSET	0x31
#define TABLE_VALUE_OFFSET	0x30
#define CONTROL_REG_VALUE_OFFSET	0x32

struct system_power_state target_state;

static u32 *scratchpad_restore_addr;
static u32 restore_pointer_address;

static u32 mpu_ret_cnt;
static u32 mpu_off_cnt;
static u32 core_ret_cnt;
static u32 core_off_cnt;
struct omap3_processor_cx {
	u8 valid;
	u8 type;
	u32 sleep_latency;
	u32 wakeup_latency;
	u32 mpu_state;
	u32 core_state;
	u32 threshold;
	u32 flags;
};

#define LAST_IDLE_STATE_ARR_SIZE 10

struct idle_state {
	u32 mpu_state;
	u32 core_state;
	u32 fclks;
	u32 iclks;
	u8 iva_state;
};

static struct idle_state last_idle_state[LAST_IDLE_STATE_ARR_SIZE];
u32 arr_wrptr;

struct omap3_processor_cx omap3_power_states[OMAP3_MAX_STATES];

struct omap3_processor_cx current_cx_state;

static void (*saved_idle)(void);
#ifdef CONFIG_OMAP3430_ES2
u32 current_vdd1_opp = PRCM_VDD1_OPP3;
u32 current_vdd2_opp = PRCM_VDD2_OPP3;
#else
u32 current_vdd1_opp = PRCM_VDD1_OPP1;
u32 current_vdd2_opp = PRCM_VDD2_OPP1;
#endif
struct clk *p_vdd1_clk;
struct clk *p_vdd2_clk;
u32 vdd1_opp_no;
u32 vdd2_opp_no;
static unsigned long awake_time_end;
#ifdef CONFIG_ENABLE_VOLTSCALE_IN_SUSPEND
static u32 valid_rate;
#endif

struct mpu_core_vdd_state target_sleep_state;

#ifdef CONFIG_CPU_IDLE
static void store_prepwst(void)
{
	last_idle_state[arr_wrptr].mpu_state = PM_PREPWSTST_MPU;
	last_idle_state[arr_wrptr].core_state = PM_PREPWSTST_CORE;

	if ((PM_PREPWSTST_MPU & 0x3) == 0x1)
		mpu_ret_cnt++;
	if ((PM_PREPWSTST_MPU & 0x3) == 0x0)
		mpu_off_cnt++;
	if ((PM_PREPWSTST_CORE & 0x3) == 0x1)
		core_ret_cnt++;
	if ((PM_PREPWSTST_CORE & 0x3) == 0x0)
		core_off_cnt++;
	last_idle_state[arr_wrptr].fclks =
		((CM_FCLKEN1_CORE & CORE_FCLK_MASK) |
#ifdef CONFIG_OMAP3430_ES2
#ifndef CONFIG_ARCH_OMAP3410
		(CM_FCLKEN_SGX & SGX_FCLK_MASK) |
		(CM_FCLKEN_DSS & DSS_FCLK_MASK) |
#else
	(CM_FCLKEN_DSS & DSS_FCLK_MASK) |
#endif
	(CM_FCLKEN_USBHOST & USBHOST_FCLK_MASK) |
	(CM_FCLKEN3_CORE & CORE3_FCLK_MASK)
#else
		(CM_FCLKEN_GFX & GFX_FCLK_MASK) |
		(CM_FCLKEN_DSS & DSS_FCLK_MASK)
#endif
		| (CM_FCLKEN_CAM & CAM_FCLK_MASK) |
		(CM_FCLKEN_PER & PER_FCLK_MASK));
	last_idle_state[arr_wrptr].iclks =
		(CORE1_ICLK_VALID &
		(CM_ICLKEN1_CORE & ~CM_AUTOIDLE1_CORE)) |
		(CORE2_ICLK_VALID & (CM_ICLKEN2_CORE & ~CM_AUTOIDLE2_CORE)) |
#ifdef CONFIG_OMAP3430_ES2
		(CORE3_ICLK_VALID & (CM_ICLKEN3_CORE & ~CM_AUTOIDLE3_CORE)) |
#ifndef CONFIG_ARCH_OMAP3410
		(SGX_ICLK_VALID & (CM_ICLKEN_SGX)) |
#endif
		(USBHOST_ICLK_VALID & (CM_ICLKEN_USBHOST)) |
#else
		(GFX_ICLK_VALID & (CM_ICLKEN_GFX)) |
#endif
		(DSS_ICLK_VALID & (CM_ICLKEN_DSS & ~CM_AUTOIDLE_DSS)) |
		(CAM_ICLK_VALID & (CM_ICLKEN_CAM & ~CM_AUTOIDLE_CAM)) |
		(PER_ICLK_VALID & (CM_ICLKEN_PER & ~CM_AUTOIDLE_PER)) |
		(WKUP_ICLK_VALID & (CM_ICLKEN_WKUP & ~CM_AUTOIDLE_WKUP));
	prcm_get_power_domain_state(DOM_IVA2,
				&(last_idle_state[arr_wrptr].iva_state));
	arr_wrptr++;

	if (arr_wrptr == LAST_IDLE_STATE_ARR_SIZE)
		arr_wrptr = 0;
}
#endif

int omap_get_system_sleep_time(int ticks)
{
	ticks -= (current_cx_state.wakeup_latency)/10000;
	return ticks;
}

static void restore_control_register(u32 val)
{
	__asm__ __volatile__ ("mcr p15, 0, %0, c1, c0, 0" : : "r" (val));
}

/* Function to restore the table entry that was modified for enabling MMU*/
static void restore_table_entry(void)
{
	u32 *scratchpad_address;
	u32 previous_value, control_reg_value;
	u32 *address;
	/* Get virtual address of SCRATCHPAD */
	scratchpad_address = (u32 *) io_p2v(SCRATCHPAD);
	/* Get address of entry that was modified */
	address = (u32 *) *(scratchpad_address + TABLE_ADDRESS_OFFSET);
	/* Get the previous value which needs to be restored */
	previous_value = *(scratchpad_address + TABLE_VALUE_OFFSET);
#ifdef RESTORE_TABLE_ENTRY
	/* Do nothing if no modified entries found */
	/* Convert address to virtual address */
	address = __va(address);
	/* Restore table entry */
	*address = previous_value;
	/* Flush TLB */
	flush_tlb_all();
#endif
	control_reg_value = *(scratchpad_address + CONTROL_REG_VALUE_OFFSET);
	/* Restore control register*/
	/* This will enable caches and prediction */
	restore_control_register(control_reg_value);
}

static void (*_omap_sram_idle)(u32 *addr, int save_state);

void omap_sram_idle(void)
{
	/* Variable to tell what needs to be saved and restored
	 * in omap_sram_idle*/
	/* save_state = 0 => Nothing to save and restored */
	/* save_state = 1 => Only L1 and logic lost */
	/* save_state = 2 => Only L2 lost */
	/* save_state = 3 => L1, L2 and logic lost */
	int save_state = 0;
	if (!_omap_sram_idle)
		return;
	switch (target_state.mpu_state) {
	case PRCM_MPU_ACTIVE:
	case PRCM_MPU_INACTIVE:
	case PRCM_MPU_CSWR_L2RET:
		/* No need to save context */
		save_state = 0;
		break;
	case PRCM_MPU_OSWR_L2RET:
		/* L1 and Logic lost */
		save_state = 1;
		break;
	case PRCM_MPU_CSWR_L2OFF:
		/* Only L2 lost */
		save_state = 2;
		break;
	case PRCM_MPU_OSWR_L2OFF:
	case PRCM_MPU_OFF:
		/* L1, L2 and logic lost */
		save_state = 3;
		break;
	default:
		/* Invalid state */
		printk(KERN_ERR "Invalid mpu state in sram_idle\n");
		return;
	}
	_omap_sram_idle(context_mem, save_state);
	/* Restore table entry modified during MMU restoration */
	if ((PM_PREPWSTST_MPU & 0x3) == 0x0)
		restore_table_entry();
}

#ifdef CONFIG_CPU_IDLE
static int pre_uart_inactivity(void)
{
	int driver8250_managed = 0;

	if (are_driver8250_uarts_active(&driver8250_managed)) {
		awake_time_end = jiffies + msecs_to_jiffies(UART_TIME_OUT);
		return 0;
	}

	if (time_before(jiffies, awake_time_end))
		return 0;

	return 1;
}

static void post_uart_inactivity(void)
{
	if (awake_time_end == 0)
		awake_time_end = jiffies;
}
#endif /* CONFIG_CPU_IDLE */

static suspend_state_t cur_suspend_state;
static int omap3_pm_set_target(suspend_state_t state)
{
	cur_suspend_state = state;
	return 0;
}

static int omap3_pm_prepare(void)
{
	int error = 0;
	suspend_state_t state = cur_suspend_state;

	/* We cannot sleep in idle until we have resumed */
	saved_idle = pm_idle;
	pm_idle = NULL;

	switch (state) {
	case PM_SUSPEND_STANDBY:
	case PM_SUSPEND_MEM:
		break;

	/*TODO: Need to fix this*/
	/*case PM_SUSPEND_DISK:
		return -ENOTSUPP;*/

	default:
		return -EINVAL;
	}
#ifdef CONFIG_ENABLE_VOLTSCALE_IN_SUSPEND
#ifdef CONFIG_OMAP3430_ES2
	if (current_vdd1_opp != PRCM_VDD1_OPP1) {
		valid_rate = clk_round_rate(p_vdd1_clk, S125M);
		p_vdd1_clk->set_rate(p_vdd1_clk, valid_rate);
		prcm_do_voltage_scaling(PRCM_VDD1_OPP1, current_vdd1_opp);
	}
	if (current_vdd2_opp != PRCM_VDD2_OPP2) {
		valid_rate = clk_round_rate(p_vdd2_clk, S83M);
		p_vdd1_clk->set_rate(p_vdd2_clk, valid_rate);
		prcm_do_voltage_scaling(PRCM_VDD2_OPP2, current_vdd2_opp);
	}
#else
	/* Scale to lowest OPP for VDD1 before executing suspend */
	if (current_vdd1_opp != PRCM_VDD1_OPP4) {
		valid_rate = clk_round_rate(p_vdd1_clk, S96M);
		p_vdd1_clk->set_rate(p_vdd1_clk, valid_rate);
		prcm_do_voltage_scaling(PRCM_VDD1_OPP4, current_vdd1_opp);
	}
	if (current_vdd2_opp != PRCM_VDD2_OPP2) {
		valid_rate = clk_round_rate(p_vdd2_clk, S83M);
		p_vdd1_clk->set_rate(p_vdd2_clk, valid_rate);
		prcm_do_voltage_scaling(PRCM_VDD2_OPP2, current_vdd2_opp);
	}
#endif /* CONFIG_OMAP3430_ES2 */
#endif /* CONFIG_ENABLE_VOLTSCALE_IN_SUSPEND */
	return error;
}

#ifndef CONFIG_ARCH_OMAP3410
/* Save and retore global configuration in NEON domain */
static void omap3_save_neon_context(void)
{
	/*Nothing to do here */
	return;
}

static void omap3_restore_neon_context(void)
{
#ifdef CONFIG_VFP
	vfp_enable();
#endif
}
#endif

/* Save and restore global configuration in PER domain */
static void omap3_save_per_context(void)
{
	/* Only GPIO save is done here */
	omap_gpio_save();
}

static void omap3_restore_per_context(void)
{
	/* Only GPIO restore is done here */
	omap_gpio_restore();
}

#if defined(CONFIG_CPU_IDLE) || defined(CONFIG_CORE_OFF)
/* Configuration that is OS specific is done here */
static void omap3_restore_core_settings(void)
{
	prcm_lock_iva_dpll(current_vdd1_opp);
	restore_sram_functions();
	_omap_sram_idle = omap_sram_push(omap34xx_suspend,
				omap34xx_suspend_sz);
	save_scratchpad_contents();
}
#endif
  
#if defined(CONFIG_CPU_IDLE) || !defined(CONFIG_CORE_OFF)
static void memory_logic_res_seting(void)
{


	if (res3_level == LOGIC_RET) {
		if ((res1_level == MEMORY_RET) && (res2_level == MEMORY_RET))
			target_state.core_state = PRCM_CORE_CSWR_MEMRET;
		else if (res1_level == MEMORY_OFF && res2_level == MEMORY_RET)
			target_state.core_state = PRCM_CORE_CSWR_MEM1OFF;
		else if (res1_level == MEMORY_RET &&  res2_level == MEMORY_OFF)
			target_state.core_state = PRCM_CORE_CSWR_MEM2OFF;
		else
			target_state.core_state = PRCM_CORE_CSWR_MEMOFF;
	} else if (res3_level == LOGIC_OFF) {
		if ((res1_level && res2_level) == MEMORY_RET)
			target_state.core_state = PRCM_CORE_OSWR_MEMRET;
		else if (res1_level == MEMORY_OFF && res2_level == MEMORY_RET)
			target_state.core_state = PRCM_CORE_OSWR_MEM1OFF;
		else if (res1_level == MEMORY_RET &&  res2_level == MEMORY_OFF)
			target_state.core_state = PRCM_CORE_OSWR_MEM2OFF;
		else
			target_state.core_state = PRCM_CORE_OSWR_MEMOFF;
	} else
		DPRINTK("Current State not supported in Retention");

}
#endif /* CONFIG_CPU_IDLE */

static int omap3_pm_suspend(void)
{
	int ret;
#ifdef CONFIG_OMAP3430_ES2
	disable_smartreflex(SR1_ID);
	disable_smartreflex(SR2_ID);
#endif /* #ifdef CONFIG_OMAP3430_ES2 */

	local_irq_disable();
	local_fiq_disable();

#ifdef CONFIG_CORE_OFF
	if (enable_off)
		omap_uart_save_ctx();
#endif

#ifdef CONFIG_OMAP34XX_OFFMODE
	context_restore_update(DOM_PER);
	context_restore_update(DOM_CORE1);

	target_state.iva2_state = PRCM_OFF;
#ifndef CONFIG_ARCH_OMAP3410
	target_state.gfx_state = PRCM_OFF;
#endif
	target_state.dss_state = PRCM_OFF;
	target_state.cam_state = PRCM_OFF;
	target_state.per_state = PRCM_OFF;
#ifdef CONFIG_OMAP3430_ES2
	target_state.usbhost_state = PRCM_OFF;
#ifndef CONFIG_ARCH_OMAP3410
	target_state.neon_state = PRCM_OFF;
#endif
#ifdef CONFIG_MPU_OFF
	/* On ES 2.0, if scrathpad is populated with valid
	* pointer, warm reset does not work
	* So populate scrathpad restore address only in
	* cpuidle and suspend calls
	*/
	*(scratchpad_restore_addr) = restore_pointer_address;
#endif
#else
	/* Neon cannot put to OFF on ES1.0 - Errata 1.46 */
	target_state.neon_state = PRCM_RET;
#endif
#else
	target_state.iva2_state = PRCM_RET;
#ifndef CONFIG_ARCH_OMAP3410
	target_state.gfx_state = PRCM_RET;
#endif
	target_state.dss_state = PRCM_RET;
	target_state.cam_state = PRCM_RET;
	target_state.per_state = PRCM_RET;
#ifndef CONFIG_ARCH_OMAP3410
	target_state.neon_state = PRCM_RET;
#endif
#ifdef CONFIG_OMAP3430_ES2
	target_state.usbhost_state = PRCM_RET;
#endif
#endif /* #ifdef CONFIG_OMAP34XX_OFFMODE */

#ifdef CONFIG_MPU_OFF
	if (enable_off)
		target_state.mpu_state = PRCM_MPU_OFF;
	else
		target_state.mpu_state = PRCM_MPU_CSWR_L2RET;
#else
	target_state.mpu_state = PRCM_MPU_CSWR_L2RET;
#endif
#ifndef CONFIG_ARCH_OMAP3410
	if (target_state.neon_state == PRCM_OFF)
		omap3_save_neon_context();
#endif
	if (target_state.per_state == PRCM_OFF)
		omap3_save_per_context();


#ifdef CONFIG_CORE_OFF
	if (enable_off)
		target_state.core_state = PRCM_CORE_OFF;
	else
		target_state.core_state = PRCM_CORE_CSWR_MEMRET;
#else
	target_state.core_state = PRCM_CORE_CSWR_MEMRET;
	if (target_state.core_state == PRCM_CORE_CSWR_MEMRET) {
		res1_level = resource_get_level(memret1);
		res2_level = resource_get_level(memret2);
		res3_level = resource_get_level(logret1);
		memory_logic_res_seting();
	}
	if (target_state.core_state >=  PRCM_CORE_OSWR_MEMRET) {
		prcm_save_core_context(target_state.core_state);
		omap_uart_save_ctx();
	}

#endif



#ifdef CONFIG_OMAP_32K_TIMER
	omap_disable_system_timer();
#endif

	ret = prcm_set_chip_power_mode(&target_state, PRCM_WAKEUP_T2_KEYPAD |
	PRCM_WAKEUP_TOUCHSCREEN);
	if (ret != 0)
		printk(KERN_ERR "Could not enter target state in pm_suspend\n");
	else {
		printk(KERN_INFO "Successfully put chip to target state "
					"in suspend\n");
#ifdef CONFIG_MPU_OFF
#ifdef CONFIG_CORE_OFF
		if (enable_off)
			printk(KERN_INFO "Target MPU state: OFF, "
					"Target CORE state: OFF\n");
		else
			printk(KERN_INFO "Target MPU state: CSWR_L2RET, "
					"Target CORE state: CSWR_MEMRET\n");
#else
		printk(KERN_INFO "Target MPU state: OFF, "
					"Target CORE state: CSWR_MEMRET\n");
#endif
#else
		printk(KERN_INFO "Target MPU state: CSWR_L2RET, "
					"Target CORE state: CSWR_MEMRET\n");
#endif
	}
#ifdef CONFIG_MPU_OFF
	*(scratchpad_restore_addr) = 0;
#endif
#ifdef CONFIG_OMAP_32K_TIMER
	omap_enable_system_timer();
#endif
#ifndef CONFIG_ARCH_OMAP3410
	if (target_state.neon_state == PRCM_OFF)
		omap3_restore_neon_context();
#endif
	if (target_state.per_state == PRCM_OFF)
		omap3_restore_per_context();
#ifdef CONFIG_CORE_OFF
	if (enable_off) {
		omap3_restore_core_settings();
		omap_uart_restore_ctx();
	}
#else
	if (target_state.core_state >= PRCM_CORE_OSWR_MEMRET) {
#ifdef CONFIG_OMAP34XX_OFFMODE
	context_restore_update(DOM_CORE1);
#endif
		prcm_restore_core_context(target_state.core_state);
		omap_uart_restore_ctx();
	}

	if ((target_state.core_state > PRCM_CORE_CSWR_MEMRET) &&
			(target_state.core_state != PRCM_CORE_OSWR_MEMRET)) {
			restore_sram_functions();
			_omap_sram_idle = omap_sram_push(omap34xx_suspend,
							omap34xx_suspend_sz);
		}
#endif
	local_fiq_enable();
	local_irq_enable();
#ifdef CONFIG_OMAP3430_ES2
	enable_smartreflex(SR1_ID);
	enable_smartreflex(SR2_ID);
#endif /* #ifdef CONFIG_OMAP3430_ES2 */

	return 0;
}

static int omap3_pm_enter(suspend_state_t unused)
{
	int ret = 0;
	suspend_state_t state;

	/* state passed to 'set_target' overrides the passed in one */
	state = cur_suspend_state;

	switch (state) {
	case PM_SUSPEND_STANDBY:
	case PM_SUSPEND_MEM:
		ret = omap3_pm_suspend();
		break;
	/*case PM_SUSPEND_DISK:
		ret = -ENOTSUPP;
		break;*/
	default:
		ret = -EINVAL;
	}

	return ret;
}

static int omap3_pm_valid(suspend_state_t state)
{
	int ret = 0;

	switch (state) {
	case PM_SUSPEND_STANDBY:
	case PM_SUSPEND_MEM:
		ret = 1;
		break;
	default:
		ret = -EINVAL;
	}

	return ret;
}

static void omap3_pm_finish(void)
{
#ifdef CONFIG_ENABLE_VOLTSCALE_IN_SUSPEND
#ifdef CONFIG_OMAP3430_ES2
	/* Scale back to previous OPP for VDD2*/
	if (current_vdd2_opp != PRCM_VDD2_OPP2) {
		prcm_do_voltage_scaling(PRCM_VDD2_OPP3, PRCM_VDD2_OPP2);
		valid_rate = clk_round_rate(p_vdd2_clk, S166M);
		p_vdd2_clk->set_rate(p_vdd2_clk, valid_rate);
	}
	/* Scale back to previous OPP for VDD1 */
	prcm_do_voltage_scaling(current_vdd1_opp, PRCM_VDD1_OPP1);
	if (current_vdd1_opp != PRCM_VDD1_OPP1) {
		switch (current_vdd1_opp) {
		case PRCM_VDD1_OPP2:
			valid_rate = clk_round_rate(p_vdd1_clk, S250M);
			break;
		case PRCM_VDD1_OPP3:
			valid_rate = clk_round_rate(p_vdd1_clk, S500M);
			break;
		case PRCM_VDD1_OPP4:
			valid_rate = clk_round_rate(p_vdd1_clk, S550M);
			break;
		case PRCM_VDD1_OPP5:
			valid_rate = clk_round_rate(p_vdd1_clk, S600M);
			break;
		}
		p_vdd1_clk->set_rate(p_vdd1_clk, valid_rate);
	}
#else
	/* Scale back to previous OPP for VDD2*/
	if (current_vdd2_opp != PRCM_VDD2_OPP2) {
		prcm_do_voltage_scaling(PRCM_VDD2_OPP1, PRCM_VDD2_OPP2);
		valid_rate = clk_round_rate(p_vdd2_clk, S166M);
		p_vdd2_clk->set_rate(p_vdd2_clk, valid_rate);
	}

	/* Scale back to previous OPP for VDD1 */
	prcm_do_voltage_scaling(current_vdd1_opp, PRCM_VDD1_OPP4);
	if (current_vdd1_opp != PRCM_VDD1_OPP4) {
		switch (current_vdd1_opp) {
		case PRCM_VDD1_OPP1:
			valid_rate = clk_round_rate(p_vdd1_clk, S477M);
			break;
		case PRCM_VDD1_OPP2:
			valid_rate = clk_round_rate(p_vdd1_clk, S381M);
			break;
		case PRCM_VDD1_OPP3:
			valid_rate = clk_round_rate(p_vdd1_clk, S191M);
			break;
		}
		p_vdd1_clk->set_rate(p_vdd1_clk, valid_rate);
	}
#endif /* CONFIG_OMAP3430_ES2 */
#endif /* CONFIG_ENABLE_VOLTSCALE_IN_SUSPEND */
	pm_idle = saved_idle;

	/* 'state' given to us by 'set_target' is now invalid */
	cur_suspend_state = PM_SUSPEND_ON;

	return;
}

static struct platform_suspend_ops omap_pm_ops = {
	.valid		= omap3_pm_valid,
	.set_target     = omap3_pm_set_target,
	.prepare	= omap3_pm_prepare,
	.enter		= omap3_pm_enter,
	.finish		= omap3_pm_finish,
};

#ifdef CONFIG_CPU_IDLE

static int omap3_idle_bm_check(void)
{
	u32 core_dep;
	u8 state;
	/* Check if any modules other than debug uart and gpios are active*/
#ifdef CONFIG_OMAP3430_ES2
	core_dep = (CM_FCLKEN1_CORE & CORE_FCLK_MASK) |
#ifndef CONFIG_ARCH_OMAP3410
		(CM_FCLKEN_SGX & SGX_FCLK_MASK) |
#endif
		(CM_FCLKEN_CAM & CAM_FCLK_MASK) | (CM_FCLKEN_PER &
								PER_FCLK_MASK) |
		(CM_FCLKEN_USBHOST & USBHOST_FCLK_MASK) |
		(CM_FCLKEN3_CORE & CORE3_FCLK_MASK);
	/* To allow core retention during LPR scenario */
	if (!lpr_enabled)
		core_dep |= (CM_FCLKEN_DSS & DSS_FCLK_MASK);

	DPRINTK("FCLKS: %x,%x,%x,%x,%x,%x,%x\n",
	(CM_FCLKEN1_CORE & CORE_FCLK_MASK),
	(CM_FCLKEN3_CORE & CORE3_FCLK_MASK),
	(CM_FCLKEN_SGX & SGX_FCLK_MASK), (CM_FCLKEN_DSS & DSS_FCLK_MASK),
	(CM_FCLKEN_CAM & CAM_FCLK_MASK), (CM_FCLKEN_PER & PER_FCLK_MASK),
	(CM_FCLKEN_USBHOST & USBHOST_FCLK_MASK));
#else
	core_dep = (CM_FCLKEN1_CORE & CORE_FCLK_MASK) |
	(CM_FCLKEN_GFX & GFX_FCLK_MASK) | (CM_FCLKEN_DSS & DSS_FCLK_MASK) |
	(CM_FCLKEN_CAM & CAM_FCLK_MASK) | (CM_FCLKEN_PER & PER_FCLK_MASK);

	DPRINTK("FCLKS: %x,%x,%x,%x,%x\n", (CM_FCLKEN1_CORE & CORE_FCLK_MASK),
	(CM_FCLKEN_GFX & GFX_FCLK_MASK), (CM_FCLKEN_DSS & DSS_FCLK_MASK),
	(CM_FCLKEN_CAM & CAM_FCLK_MASK), (CM_FCLKEN_PER & PER_FCLK_MASK));
#endif

	/* Check if any modules have ICLK bit enabled and interface clock */
	/* autoidle disabled						 */
	core_dep |= (CORE1_ICLK_VALID & (CM_ICLKEN1_CORE & ~CM_AUTOIDLE1_CORE));
	/* Check for secure modules which have only ICLK */
	/* Do not check for rng module.It has been ensured that
	 * if rng is active cpu idle will never be entered
	 */
	core_dep |= (CORE2_ICLK_VALID & CM_ICLKEN2_CORE & ~4);
	/* Enabling SGX ICLK will prevent CORE ret*/
#ifdef CONFIG_OMAP3430_ES2
#ifndef CONFIG_ARCH_OMAP3410
	core_dep |= SGX_ICLK_VALID & (CM_ICLKEN_SGX);
#endif
	core_dep |= (CORE3_ICLK_VALID & (CM_ICLKEN3_CORE & ~CM_AUTOIDLE3_CORE));
	core_dep |= (USBHOST_ICLK_VALID & (CM_ICLKEN_USBHOST &
			~CM_AUTOIDLE_USBHOST));
#else
	core_dep |= (GFX_ICLK_VALID & (CM_ICLKEN_GFX));
#endif
	core_dep |= (DSS_ICLK_VALID & (CM_ICLKEN_DSS & ~CM_AUTOIDLE_DSS));
	core_dep |= (CAM_ICLK_VALID & (CM_ICLKEN_CAM & ~CM_AUTOIDLE_CAM));
	core_dep |= (PER_ICLK_VALID & (CM_ICLKEN_PER & ~CM_AUTOIDLE_PER));
	core_dep |= (WKUP_ICLK_VALID & (CM_ICLKEN_WKUP & ~CM_AUTOIDLE_WKUP));
	DPRINTK("ICLKS: %x,%x,%x,%x%x,%x,%x\n", (CORE1_ICLK_VALID &
		(CM_ICLKEN1_CORE & ~CM_AUTOIDLE1_CORE)),
		(CORE2_ICLK_VALID & (CM_ICLKEN2_CORE & ~CM_AUTOIDLE2_CORE)),
#ifdef CONFIG_OMAP3430_ES2
		(CORE3_ICLK_VALID & (CM_ICLKEN3_CORE & ~CM_AUTOIDLE3_CORE)),
#ifndef CONFIG_ARCH_OMAP3410
		(SGX_ICLK_VALID & (CM_ICLKEN_SGX)),
#endif
		(USBHOST_ICLK_VALID & (CM_ICLKEN_USBHOST &
			~CM_AUTOIDLE_USBHOST)),
#else
		(GFX_ICLK_VALID & (CM_ICLKEN_GFX)),
#endif
		(DSS_ICLK_VALID & (CM_ICLKEN_DSS & ~CM_AUTOIDLE_DSS)),
		(CAM_ICLK_VALID & (CM_ICLKEN_CAM & ~CM_AUTOIDLE_CAM)),
		(PER_ICLK_VALID & (CM_ICLKEN_PER & ~CM_AUTOIDLE_PER)),
		(WKUP_ICLK_VALID & (CM_ICLKEN_WKUP & ~CM_AUTOIDLE_WKUP)));

	prcm_get_power_domain_state(DOM_IVA2, &state);

	DPRINTK("IVA:%d\n", state);

	if (state == PRCM_ON)
		core_dep |= 1;
	if (core_dep)
		return 1;
	else
		return 0;
}

/* Correct target state based on inactivity timer expiry, etc */
static void correct_target_state(void)
{
	switch (target_state.mpu_state) {
	case PRCM_MPU_ACTIVE:
	case PRCM_MPU_INACTIVE:
#ifndef CONFIG_ARCH_OMAP3410
		target_state.neon_state = PRCM_ON;
#endif
		break;
	case PRCM_MPU_CSWR_L2RET:
	case PRCM_MPU_OSWR_L2RET:
	case PRCM_MPU_CSWR_L2OFF:
	case PRCM_MPU_OSWR_L2OFF:
#ifndef CONFIG_ARCH_OMAP3410
		target_state.neon_state = PRCM_RET;
#endif
		break;
	case PRCM_MPU_OFF:
#ifndef CONFIG_ARCH_OMAP3410
		target_state.neon_state = PRCM_OFF;
#endif
		if (!enable_off) {
			target_state.mpu_state = PRCM_MPU_CSWR_L2RET;
#ifndef CONFIG_ARCH_OMAP3410
			target_state.neon_state = PRCM_RET;
#endif
		}
		break;
	}


	if (target_state.core_state > PRCM_CORE_INACTIVE) {
#ifdef CONFIG_OMAP34XX_OFFMODE
		/* Core can be put to RET/OFF - This means
		 * PER can be put to off if its inactivity timer
		 * has expired
		 */
		if (perdomain_timer_pending())
			target_state.per_state = PRCM_RET;
		else
			target_state.per_state = PRCM_OFF;

		if (target_state.core_state == PRCM_CORE_OFF) {
			if (coredomain_timer_pending())
				target_state.core_state = PRCM_CORE_CSWR_MEMRET;
			if (CM_FCLKEN_DSS & DSS_FCLK_MASK)
				target_state.core_state = PRCM_CORE_CSWR_MEMRET;
			if (!enable_off) {
				target_state.core_state = PRCM_CORE_CSWR_MEMRET;
				target_state.per_state = PRCM_RET;
			}
		}
#else
		target_state.per_state = PRCM_RET;
#endif /* #ifdef CONFIG_OMAP34XX_OFFMODE */
	} else
		target_state.per_state = PRCM_ON;
		res1_level = resource_get_level(memret1);
		res2_level = resource_get_level(memret2);
		res3_level = resource_get_level(logret1);

		if (target_state.core_state == PRCM_CORE_CSWR_MEMRET)
			memory_logic_res_seting();

}

static int omap3_enter_idle(struct cpuidle_device *dev,
			struct cpuidle_state *state)
{
	struct omap3_processor_cx *cx = cpuidle_get_statedata(state);
#ifndef CONFIG_ARCH_OMAP3410
	u8 cur_per_state, cur_neon_state, pre_neon_state, pre_per_state;
#else
	u8 cur_per_state, pre_per_state;
#endif
	unsigned long timer_32k_sleep_ticks;
	unsigned long expected_sleep_usecs, sleep_usecs;
	u32 fclken_core, iclken_core, fclken_per, iclken_per;
#if 0
	u32 reprogram_ns;
	int reprogram_us;
	unsigned long long reprogram_cycles;
#endif
	int wakeup_latency;
#ifdef CONFIG_HW_SUP_TRANS
	u32 sleepdep_per, wakedep_per;
#endif /* #ifdef CONFIG_HW_SUP_TRANS */

	/* Reset previous power state registers for mpu and core*/
	PM_PREPWSTST_MPU = 0xFF;
	PM_PREPWSTST_CORE = 0xFF;
#ifndef CONFIG_ARCH_OMAP3410
	PM_PREPWSTST_NEON = 0xFF;
#endif
	PM_PREPWSTST_PER = 0xFF;
	current_cx_state = *cx;

	target_state.mpu_state = cx->mpu_state;
	target_state.core_state = cx->core_state;

	/* Check for update of 'deepest_state' (takes effect next time) */
	if (dev->state_count != cpuidle_deepest_st)
		dev->state_count = cpuidle_deepest_st;

	if (INTCPS_PENDING_IRQ0 | INTCPS_PENDING_IRQ1 | INTCPS_PENDING_IRQ2) {
		store_prepwst();
		return 0;
	}

	expected_sleep_usecs = (u32)ktime_to_ns(tick_nohz_get_sleep_length());
	expected_sleep_usecs /= 1000;

	timer_32k_sleep_ticks = omap_32k_sync_timer_read();

	if (cx->type == OMAP3_STATE_C0) {
		omap_sram_idle();
		goto out;
	}

	correct_target_state();
	wakeup_latency = cx->wakeup_latency;
	if (target_state.core_state != cx->core_state) {
		/* Currently, this can happen only for core_off */
		/* Adjust wakeup latency to that of core_cswr state */
		/* Hard coded now and needs to be made more generic */
		/* omap3_power_states[4] is CSWR for core */
		wakeup_latency = omap3_power_states[4].wakeup_latency;
	}

#if 0
	/* Reprogram the tick timer */
	reprogram_us = (s32) ktime_to_ns(tick_nohz_get_sleep_length())/1000;
	if (reprogram_us > wakeup_latency) {
		reprogram_us = reprogram_us - wakeup_latency;
		reprogram_ns = reprogram_us * 1000;
		reprogram_cycles = (unsigned long long) reprogram_ns *
						div_sc(32768, NSEC_PER_SEC, 32);
		reprogram_cycles >>= 32;
		omap_32k_timer_start((unsigned long) reprogram_cycles);
	}
#endif

	/* Check for pending interrupts. If there is an interrupt, return */
	if (INTCPS_PENDING_IRQ0 | INTCPS_PENDING_IRQ1 | INTCPS_PENDING_IRQ2) {
		store_prepwst();
		return 0;
	}

	prcm_get_power_domain_state(DOM_PER, &cur_per_state);
#ifndef CONFIG_ARCH_OMAP3410
	prcm_get_power_domain_state(DOM_NEON, &cur_neon_state);
#endif
	fclken_core = CM_FCLKEN1_CORE;
	iclken_core = CM_ICLKEN1_CORE;
	fclken_per = CM_FCLKEN_PER;
	iclken_per = CM_ICLKEN_PER;

#ifdef CONFIG_HW_SUP_TRANS
	/* Facilitating SWSUP RET, from HWSUP mode */
	sleepdep_per = CM_SLEEPDEP_PER;
	wakedep_per = PM_WKDEP_PER;
#endif /* #ifdef CONFIG_HW_SUP_TRANS */

	/* If target state if core_off, save registers
	 * before changing anything
	 */
	if (target_state.core_state >= PRCM_CORE_OSWR_MEMRET) {
		prcm_save_registers();
		omap_uart_save_ctx();
	}

	/* Check for pending interrupts. If there is an interrupt, return */
	if (INTCPS_PENDING_IRQ0 | INTCPS_PENDING_IRQ1 | INTCPS_PENDING_IRQ2) {
		store_prepwst();
		return 0;
	}

	/* Program MPU and NEON to target state */
	if (target_state.mpu_state > PRCM_MPU_ACTIVE) {
#ifndef CONFIG_ARCH_OMAP3410
		if ((cur_neon_state == PRCM_ON) &&
			(target_state.neon_state != PRCM_ON)) {
			if (target_state.neon_state == PRCM_OFF)
				omap3_save_neon_context();;
#ifdef CONFIG_HW_SUP_TRANS
			/* Facilitating SWSUP RET, from HWSUP mode */
			prcm_set_clock_domain_state(DOM_NEON, PRCM_NO_AUTO,
							       PRCM_FALSE);
			prcm_set_power_domain_state(DOM_NEON, PRCM_ON,
								PRCM_FORCE);
#endif /* #ifdef CONFIG_HW_SUP_TRANS */
			prcm_force_power_domain_state(DOM_NEON,
				target_state.neon_state);
		}
#endif
#ifdef CONFIG_MPU_OFF
		/* Populate scrathpad restore address */
		*(scratchpad_restore_addr) = restore_pointer_address;
#endif
		prcm_set_mpu_domain_state(target_state.mpu_state);
	}

	/* Check for pending interrupts. If there is an interrupt, return */
	if (INTCPS_PENDING_IRQ0 | INTCPS_PENDING_IRQ1 | INTCPS_PENDING_IRQ2)
		goto restore;

	/* Program CORE and PER to target state */
	if ((target_state.core_state > PRCM_CORE_ACTIVE) &&
			(pre_uart_inactivity())) {
		if ((cur_per_state == PRCM_ON) &&
				(target_state.per_state != PRCM_ON)) {
			if (target_state.per_state == PRCM_OFF)
				omap3_save_per_context();;
			CM_FCLKEN_PER = 0;
			CM_ICLKEN_PER = 0;
#ifdef CONFIG_HW_SUP_TRANS
			/* Facilitating SWSUP RET, from HWSUP mode */
			prcm_set_clock_domain_state(DOM_PER, PRCM_NO_AUTO,
								PRCM_FALSE);
			prcm_clear_sleep_dependency(DOM_PER,
							PRCM_SLEEPDEP_EN_MPU);
			prcm_clear_wkup_dependency(DOM_PER, PRCM_WKDEP_EN_MPU);

			prcm_set_power_domain_state(DOM_PER, PRCM_ON,
								PRCM_FORCE);
#endif /* #ifdef CONFIG_HW_SUP_TRANS */
			prcm_force_power_domain_state(DOM_PER,
							target_state.per_state);
		}
#ifdef CONFIG_OMAP3430_ES2
		disable_smartreflex(SR1_ID);
		disable_smartreflex(SR2_ID);
#endif /* #ifdef CONFIG_OMAP3430_ES2 */

	/* WORKAROUND FOR SILICON ERRATA 1.64*/
	if (is_sil_rev_equal_to(OMAP3430_REV_ES1_0)) {
		if ((CM_CLKOUT_CTRL & (0x1 << 7)) == 0x80)
			CM_CLKOUT_CTRL &= ~(0x1 << 7);
	}

		prcm_set_core_domain_state(target_state.core_state);
		/* Enable Autoidle for GPT1 explicitly - Errata 1.4 */
		CM_AUTOIDLE_WKUP |= 0x1;
		CM_FCLKEN1_CORE &= ~0x6000;
		/* Disable HSUSB OTG ICLK explicitly*/
		CM_ICLKEN1_CORE &= ~0x10;
#ifdef IOPAD_WKUP
		/* Enabling IO_PAD capabilities */
		PM_WKEN_WKUP |= 0x100;
#endif /* #ifdef IOPAD_WKUP */
	}

	/* Check for pending interrupts. If there is an interrupt, return */
	if (INTCPS_PENDING_IRQ0 | INTCPS_PENDING_IRQ1 | INTCPS_PENDING_IRQ2)
		goto restore;


	omap_sram_idle();

restore:
#ifdef IOPAD_WKUP
		/* Disabling IO_PAD capabilities */
		PM_WKEN_WKUP &= ~(0x100);
#endif /* #ifdef IOPAD_WKUP */

	CM_FCLKEN1_CORE = fclken_core;
	CM_ICLKEN1_CORE = iclken_core;
	post_uart_inactivity();
	if (target_state.mpu_state > PRCM_MPU_ACTIVE) {
#ifdef CONFIG_MPU_OFF
		/* On ES 2.0, if scrathpad is populated with valid
		* pointer, warm reset does not work
		* So populate scrathpad restore address only in
		* cpuidle and suspend calls
		*/
		*(scratchpad_restore_addr) = 0x0;
#endif
		prcm_set_mpu_domain_state(PRCM_MPU_ACTIVE);
#ifndef CONFIG_ARCH_OMAP3410
		if ((cur_neon_state == PRCM_ON) &&
			(target_state.mpu_state > PRCM_MPU_INACTIVE)) {
			prcm_force_power_domain_state(DOM_NEON, cur_neon_state);
			prcm_get_pre_power_domain_state(DOM_NEON,
					&pre_neon_state);
			if (pre_neon_state == PRCM_OFF)
				omap3_restore_neon_context();
#ifdef CONFIG_HW_SUP_TRANS
			prcm_set_power_domain_state(DOM_NEON, POWER_DOMAIN_ON,
								PRCM_AUTO);
#endif /* #ifdef CONFIG_HW_SUP_TRANS */
		}
#endif
	}
	if (target_state.core_state > PRCM_CORE_ACTIVE) {
		prcm_set_core_domain_state(PRCM_CORE_ACTIVE);

#ifdef CONFIG_OMAP3430_ES2
		enable_smartreflex(SR1_ID);
		enable_smartreflex(SR2_ID);
#endif /* #ifdef CONFIG_OMAP3430_ES2 */

		if (cur_per_state == PRCM_ON) {
			prcm_force_power_domain_state(DOM_PER, cur_per_state);
			prcm_get_pre_power_domain_state(DOM_PER,
					&pre_per_state);
			CM_FCLKEN_PER = fclken_per;
			CM_ICLKEN_PER = iclken_per;
			if (pre_per_state == PRCM_OFF) {
				omap3_restore_per_context();
#ifdef CONFIG_OMAP34XX_OFFMODE
				context_restore_update(DOM_PER);
#endif
			}
#ifdef CONFIG_HW_SUP_TRANS
		/* Facilitating SWSUP RET, from HWSUP mode */
			CM_SLEEPDEP_PER = sleepdep_per;
			PM_WKDEP_PER = wakedep_per;
			prcm_set_power_domain_state(DOM_PER, POWER_DOMAIN_ON,
								PRCM_AUTO);
#endif /* #ifdef CONFIG_HW_SUP_TRANS */
		}
		if (target_state.core_state >= PRCM_CORE_OSWR_MEMRET) {
#ifdef CONFIG_OMAP34XX_OFFMODE
			context_restore_update(DOM_CORE1);
#endif
			prcm_restore_registers();
			prcm_restore_core_context(target_state.core_state);
			omap3_restore_core_settings();
			}
		/* Errata 1.4
		* if the timer device gets idled which is when we
		* are cutting the timer ICLK which is when we try
		* to put Core to RET.
		* Wait Period = 2 timer interface clock cycles +
		* 1 timer functional clock cycle
		* Interface clock = L4 clock. For the computation L4
		* clock is assumed at 50MHz (worst case).
		* Functional clock = 32KHz
		* Wait Period = 2*10^-6/50 + 1/32768 = 0.000030557 = 30.557uSec
		* Roundingoff the delay value to a safer 50uSec
		*/
		omap_udelay(GPTIMER_WAIT_DELAY);
		CM_AUTOIDLE_WKUP &= ~(0x1);
	}

	DPRINTK("MPU state:%x,CORE state:%x\n", PM_PREPWSTST_MPU,
							PM_PREPWSTST_CORE);
out:
	store_prepwst();

	timer_32k_sleep_ticks = omap_32k_sync_timer_read() -
							timer_32k_sleep_ticks;
	if (target_state.core_state >= PRCM_CORE_OSWR_MEMRET)
			omap_uart_restore_ctx();

	if ((target_state.core_state > PRCM_CORE_CSWR_MEMRET) &&
			(target_state.core_state != PRCM_CORE_OSWR_MEMRET)) {
		restore_sram_functions(); /*restore sram data */
			_omap_sram_idle = omap_sram_push(omap34xx_suspend,
				omap34xx_suspend_sz);
		}

	sleep_usecs = omap_32k_ticks_to_usecs(timer_32k_sleep_ticks);
#if 0
	if (cx && cx->type != OMAP3_STATE_C0) {
		printk("%s: slept %6lu (%5lu), "
		       "expected %8lu, over %8d\n", state->name,
		       sleep_usecs, timer_32k_sleep_ticks, 
		       expected_sleep_usecs,
		       (int)(sleep_usecs - expected_sleep_usecs));
	}
#endif
	return sleep_usecs;
}

static int omap3_enter_idle_bm(struct cpuidle_device *dev,
			       struct cpuidle_state *state)
{
	struct cpuidle_state *new_state = NULL;
	int i, j;
	
	if ((state->flags & CPUIDLE_FLAG_CHECK_BM) && omap3_idle_bm_check()) {

		/* Find current state in list */
		for(i = 0; i < OMAP3_MAX_STATES; i++)
			if (state == &dev->states[i])
				break;
		BUG_ON(i == OMAP3_MAX_STATES);

		/* Back up to non 'CHECK_BM' state */
		for(j = i - 1;  j > 0; j--) {
			struct cpuidle_state *s = &dev->states[j];

			if (!(s->flags & CPUIDLE_FLAG_CHECK_BM)) {
				new_state = s;
				break;
			}
		}
		
		DPRINTK("%s: Bus activity: Entering %s (instead of %s)\n",
			__FUNCTION__, new_state->name, state->name);
	}

	return omap3_enter_idle(dev, new_state ? : state);
}

DEFINE_PER_CPU(struct cpuidle_device, omap3_idle_dev);

static int omap3_idle_init(void)
{
	int i, count = 0;
	struct omap3_processor_cx *cx;
	struct cpuidle_state *state;
	struct cpuidle_device *dev;

	dev = &per_cpu(omap3_idle_dev, smp_processor_id());

	for (i = 0; i < OMAP3_MAX_STATES; i++) {
		cx = &omap3_power_states[i];
		state = &dev->states[count];

		if (!cx->valid)
			continue;
		cpuidle_set_statedata(state, cx);
		state->exit_latency = cx->sleep_latency + cx->wakeup_latency;
		state->target_residency = cx->threshold;
		state->flags = cx->flags;
		state->enter = (state->flags & CPUIDLE_FLAG_CHECK_BM) ? 
			omap3_enter_idle_bm : omap3_enter_idle;
		sprintf(state->name, "C%d", count+1);
		count++;
	}

	if (!count)
		return -EINVAL;
	dev->state_count = count;

	if (cpuidle_register_device(dev)) {
		printk(KERN_ERR "%s: CPUidle register device failed\n",
		       __FUNCTION__);
		return -EIO;
	}

	return 0;
}

void omap_init_power_states(void)
{
	omap3_power_states[0].valid = 1;
	omap3_power_states[0].type = OMAP3_STATE_C0;
	omap3_power_states[0].sleep_latency = 0;
	omap3_power_states[0].wakeup_latency = 0;
	omap3_power_states[0].threshold = 0;
	omap3_power_states[0].mpu_state = PRCM_MPU_ACTIVE;
	omap3_power_states[0].core_state = PRCM_CORE_ACTIVE;
	omap3_power_states[0].flags = CPUIDLE_FLAG_TIME_VALID;

	omap3_power_states[1].valid = 1;
	omap3_power_states[1].type = OMAP3_STATE_C1;
	omap3_power_states[1].sleep_latency = 10;
	omap3_power_states[1].wakeup_latency = 10;
	omap3_power_states[1].threshold = 30;
	omap3_power_states[1].mpu_state = PRCM_MPU_ACTIVE;
	omap3_power_states[1].core_state = PRCM_CORE_ACTIVE;
	omap3_power_states[1].flags = CPUIDLE_FLAG_TIME_VALID;

	omap3_power_states[2].valid = 1;
	omap3_power_states[2].type = OMAP3_STATE_C2;
	omap3_power_states[2].sleep_latency = 50;
	omap3_power_states[2].wakeup_latency = 50;
	omap3_power_states[2].threshold = 300;
	omap3_power_states[2].mpu_state = PRCM_MPU_CSWR_L2RET;
	omap3_power_states[2].core_state = PRCM_CORE_ACTIVE;
	omap3_power_states[2].flags = CPUIDLE_FLAG_TIME_VALID;
	omap3_power_states[2].flags |= CPUIDLE_FLAG_CHECK_BM;

	omap3_power_states[3].valid = 1;
	omap3_power_states[3].type = OMAP3_STATE_C3;
	omap3_power_states[3].sleep_latency = 1500;
	omap3_power_states[3].wakeup_latency = 1800;
	omap3_power_states[3].threshold = 4000;
#ifdef CONFIG_MPU_OFF
	omap3_power_states[3].mpu_state = PRCM_MPU_OFF;
#else
	omap3_power_states[3].mpu_state = PRCM_MPU_CSWR_L2RET;
#endif
	omap3_power_states[3].core_state = PRCM_CORE_ACTIVE;
	omap3_power_states[3].flags = CPUIDLE_FLAG_TIME_VALID;
	omap3_power_states[3].flags |= CPUIDLE_FLAG_CHECK_BM;

	omap3_power_states[4].valid = 1;
	omap3_power_states[4].type = OMAP3_STATE_C4;
	omap3_power_states[4].sleep_latency = 2500;
	omap3_power_states[4].wakeup_latency = 7500;
	omap3_power_states[4].threshold = 12000;
	omap3_power_states[4].mpu_state = PRCM_MPU_CSWR_L2RET;
	omap3_power_states[4].core_state = PRCM_CORE_CSWR_MEMRET;
	omap3_power_states[4].flags = CPUIDLE_FLAG_TIME_VALID;
	omap3_power_states[4].flags |= CPUIDLE_FLAG_CHECK_BM;

	omap3_power_states[5].valid = 1;
	omap3_power_states[5].type = OMAP3_STATE_C5;
	omap3_power_states[5].sleep_latency = 3000;
	omap3_power_states[5].wakeup_latency = 8500;
	omap3_power_states[5].threshold = 15000;
#ifdef CONFIG_MPU_OFF
	omap3_power_states[5].mpu_state = PRCM_MPU_OFF;
#else
	omap3_power_states[5].mpu_state = PRCM_MPU_CSWR_L2RET;
#endif
	omap3_power_states[5].core_state = PRCM_CORE_CSWR_MEMRET;
	omap3_power_states[5].flags = CPUIDLE_FLAG_TIME_VALID;
	omap3_power_states[5].flags |= CPUIDLE_FLAG_CHECK_BM;

#ifdef CONFIG_CORE_OFF_CPUIDLE
	omap3_power_states[6].valid = 1;
#else
	omap3_power_states[6].valid = 0;
#endif
	omap3_power_states[6].type = OMAP3_STATE_C6;
	omap3_power_states[6].sleep_latency = 10000;
	omap3_power_states[6].wakeup_latency = 30000;
	omap3_power_states[6].threshold = 300000;
	omap3_power_states[6].mpu_state = PRCM_MPU_OFF;
	omap3_power_states[6].core_state = PRCM_CORE_OFF;
	omap3_power_states[6].flags = CPUIDLE_FLAG_TIME_VALID | CPUIDLE_FLAG_DEEP ;
	omap3_power_states[6].flags |= CPUIDLE_FLAG_CHECK_BM;
}
struct cpuidle_driver omap3_idle_driver = {
	.name = 	"omap3_idle",
	.owner = 	THIS_MODULE,
};

/* Sysfs interface to select the system sleep state */
static ssize_t omap_pm_sleep_idle_state_show(struct kset *subsys, char *buf)
{
	return sprintf(buf, "%hu\n", cpuidle_deepest_st);
}

static ssize_t omap_pm_sleep_idle_state_store(struct kset *subsys,
				const char *buf, size_t n)
{
	unsigned short value;
	if (sscanf(buf, "%hu", &value) != 1 ||
		(value > (MAX_SUPPORTED_STATES - 1))) {
		printk(KERN_ERR "idle_sleep_store: Invalid value\n");
		return -EINVAL;
	}
	cpuidle_deepest_st = value;
	return n;
}

static struct subsys_attribute sleep_idle_state = {
	.attr = {
	.name = __stringify(cpuidle_deepest_state),
	.mode = 0644,
	},
	.show = omap_pm_sleep_idle_state_show,
	.store = omap_pm_sleep_idle_state_store,
};
#endif /* CONFIG_CPU_IDLE */

static ssize_t omap_pm_off_show(struct kset *subsys, char *buf)
{
	return sprintf(buf, "%hu\n", enable_off);
}

static ssize_t omap_pm_off_store(struct kset *subsys,
				const char *buf, size_t n)
{
	unsigned short value;
	if (sscanf(buf, "%hu", &value) != 1 ||
		((value != 0) && (value != 1))) {
		printk(KERN_ERR "off_enable: Invalid value\n");
		return -EINVAL;
	}
	enable_off = value;
	return n;
}


static struct subsys_attribute off_enable = {
	.attr = {
	.name = __stringify(enable_mpucoreoff),
	.mode = 0644,
	},
	.show = omap_pm_off_show,
	.store = omap_pm_off_store,
};

static ssize_t omap_pm_domain_show(struct kset *subsys, char *buf)
{
	prcm_printreg(debug_domain);
	return 0;
}

static ssize_t omap_pm_domain_store(struct kset *subsys,
				const char *buf, size_t n)
{
	unsigned short value;
	if (sscanf(buf, "%hu", &value) != 1 ||
		(value > PRCM_NUM_DOMAINS)
		 || (value == DOM_CORE2) || (value == 12)) {
		printk(KERN_ERR "dump_domain: Invalid value\n");
		return -EINVAL;
	}
	debug_domain = value;
	return n;
}


static struct subsys_attribute domain_print = {
	.attr = {
	.name = __stringify(debug_domain),
	.mode = 0644,
	},
	.show = omap_pm_domain_show,
	.store = omap_pm_domain_store,
};

/*sysfs interface to chg the frame buffer timer*/
static ssize_t omap_pm_fb_timeout_show(struct kset *subsys, char *buf)
{
		return sprintf(buf, "%hu\n", fb_timeout_val);
}

static ssize_t omap_pm_fb_timeout_store(struct kset *subsys,
					const char *buf, size_t n)
{
	unsigned short value;
	if (sscanf(buf, "%hu", &value) != 1) {
		printk(KERN_ERR "fb_timeout_store: Invalid value\n");
		return -EINVAL;
	}
	fb_timeout_val = value;
	set_blank_interval(fb_timeout_val);
	return n;
}

static struct subsys_attribute fb_timeout = {
	.attr = {
	.name = __stringify(fb_timeout_value),
	.mode = 0644,
	},
	.show = omap_pm_fb_timeout_show,
	.store = omap_pm_fb_timeout_store,
};


/* Sysfs interface to set the opp for vdd1 and vdd2 */
static ssize_t omap_pm_vdd1_opp_show(struct kset *subsys, char *buf)
{
	return sprintf(buf, "%x\n", (unsigned int)get_opp_no(
							current_vdd1_opp));
}

static ssize_t omap_pm_vdd1_opp_store(struct kset *subsys,
						const char *buf, size_t n)
{
	sscanf(buf, "%u", &target_opp_no);
#ifdef CONFIG_OMAP3430_ES2
	if ((target_opp_no < 1) || (target_opp_no > 5)) {
		printk(KERN_ERR "opp_store for vdd1 ES 2.0:Invalid value\n");
		return -EINVAL;
	}
#else
	if ((target_opp_no < 1) || (target_opp_no > 4)) {
		printk(KERN_ERR "opp_store for vdd1 ES 1.0:Invalid value\n");
		return -EINVAL;
	}
#endif
	if (target_opp_no == get_opp_no(current_vdd1_opp)) {
		DPRINTK("Target and current opp values are same\n");
	} else {
		if (target_opp_no != 1) {
			if (vdd1_opp_co == NULL)
				vdd1_opp_co = constraint_get("pm_fwk",
								&cnstr_id1);
			constraint_set(vdd1_opp_co, target_opp_no);
		} else {
			if (vdd1_opp_co != NULL) {
				constraint_remove(vdd1_opp_co);
				constraint_put(vdd1_opp_co);
				vdd1_opp_co = NULL;
			}
		}
	}
	return n;
}

static struct subsys_attribute vdd1_opp = {
	.attr = {
	.name = __stringify(vdd1_opp_value),
	.mode = 0644,
	},
	.show = omap_pm_vdd1_opp_show,
	.store = omap_pm_vdd1_opp_store,
};

static ssize_t omap_pm_vdd2_opp_show(struct kset *subsys, char *buf)
{
	return sprintf(buf, "%x\n", (unsigned int)get_opp_no(current_vdd2_opp));
}

static ssize_t omap_pm_vdd2_opp_store(struct kset *subsys,
						const char *buf, size_t n)
{
	sscanf(buf, "%u", &target_opp_no);
#ifdef CONFIG_OMAP3430_ES2
	if ((target_opp_no < 1) || (target_opp_no > 3)) {
		printk(KERN_ERR "opp_store for vdd2 ES 2.0:Invalid value\n");
		return -EINVAL;
	}
#else
	if ((target_opp_no < 1) || (target_opp_no > 2)) {
		printk(KERN_ERR "opp_store for vdd2 ES 1.0:Invalid value\n");
		return -EINVAL;
	}
#endif
	if (target_opp_no == get_opp_no(current_vdd2_opp)) {
		DPRINTK("Target and current opp values are same\n");
	} else {
		if (target_opp_no != 1) {
			if (vdd2_opp_co == NULL)
				vdd2_opp_co = constraint_get("pm_fwk",
								&cnstr_id2);
			constraint_set(vdd2_opp_co, target_opp_no);
		} else {
			if (vdd2_opp_co != NULL) {
				constraint_remove(vdd2_opp_co);
				constraint_put(vdd2_opp_co);
				vdd2_opp_co = NULL;
			}
		}
	}

	return n;
}
static struct subsys_attribute vdd2_opp = {
	.attr = {
	.name = __stringify(vdd2_opp_value),
	.mode = 0644,
	},
	.show = omap_pm_vdd2_opp_show,
	.store = omap_pm_vdd2_opp_store,
};

/* PRCM Interrupt Handler */
irqreturn_t prcm_interrupt_handler (int irq, void *dev_id)
{
	u32 wkst_wkup = PM_WKST_WKUP;
	u32 wkst1_core = PM_WKST1_CORE;
#ifdef CONFIG_OMAP3430_ES2
	u32 wkst3_core = PM_WKST3_CORE;
	u32 wkst_usbhost = PM_WKST_USBHOST;
#endif
	u32 wkst_per = PM_WKST_PER;
	u32 errst_vc = PRM_VC_TIMEOUTERR_ST | PRM_VC_RAERR_ST |
							PRM_VC_SAERR_EN;
	u32 fclk = 0;
	u32 iclk = 0;

	if (PM_WKST_WKUP & PM_MPUGRPSEL_WKUP) {
#ifdef IOPAD_WKUP
		/* Resetting UART1 inactivity timeout, during IO_PAD wakeup */
		if (wkst_wkup & 0x100)
			awake_time_end = jiffies +
					 msecs_to_jiffies(UART_TIME_OUT);

#endif /* #ifdef IOPAD_WKUP */

		iclk = CM_ICLKEN_WKUP;
		fclk = CM_FCLKEN_WKUP;
		CM_ICLKEN_WKUP |= wkst_wkup & PM_MPUGRPSEL_WKUP;
		CM_FCLKEN_WKUP |= wkst_wkup & PM_MPUGRPSEL_WKUP;
		PM_WKST_WKUP = wkst_wkup & PM_MPUGRPSEL_WKUP;
		while (PM_WKST_WKUP & PM_MPUGRPSEL_WKUP);
		CM_ICLKEN_WKUP = iclk;
		CM_FCLKEN_WKUP = fclk;
	}
	if (PM_WKST1_CORE & PM_MPUGRPSEL1_CORE) {
		iclk = CM_ICLKEN1_CORE;
		fclk = CM_FCLKEN1_CORE;
		CM_ICLKEN1_CORE |= wkst1_core & PM_MPUGRPSEL1_CORE;
		CM_FCLKEN1_CORE |= wkst1_core & PM_MPUGRPSEL1_CORE;
		PM_WKST1_CORE = wkst1_core & PM_MPUGRPSEL1_CORE;
		while (PM_WKST1_CORE & PM_MPUGRPSEL1_CORE);
		CM_ICLKEN1_CORE = iclk;
		CM_FCLKEN1_CORE = fclk;
	}
#ifdef CONFIG_OMAP3430_ES2
	if (PM_WKST3_CORE & PM_MPUGRPSEL3_CORE) {
		iclk = CM_ICLKEN3_CORE;
		fclk = CM_FCLKEN3_CORE;
		CM_ICLKEN3_CORE |= wkst3_core & PM_MPUGRPSEL3_CORE;
		CM_FCLKEN3_CORE |= wkst3_core & PM_MPUGRPSEL3_CORE;
		PM_WKST3_CORE = wkst3_core & PM_MPUGRPSEL3_CORE;
		while (PM_WKST3_CORE & PM_MPUGRPSEL3_CORE);
		CM_ICLKEN3_CORE = iclk;
		CM_FCLKEN3_CORE = fclk;
	}
	if (PM_WKST_USBHOST & PM_MPUGRPSEL_USBHOST) {

		iclk = CM_ICLKEN_USBHOST;
		fclk = CM_FCLKEN_USBHOST;
		CM_ICLKEN_USBHOST |= wkst_usbhost & PM_MPUGRPSEL_USBHOST;
		CM_FCLKEN_USBHOST |= wkst_usbhost & PM_MPUGRPSEL_USBHOST;
		PM_WKST_USBHOST = wkst_usbhost & PM_MPUGRPSEL_USBHOST;
		while (PM_WKST_USBHOST & PM_MPUGRPSEL_USBHOST);
		CM_ICLKEN_USBHOST = iclk;
		CM_FCLKEN_USBHOST = fclk;
	}
#endif
	if (PM_WKST_PER & PM_MPUGRPSEL_PER) {
		iclk = CM_ICLKEN_PER;
		fclk = CM_FCLKEN_PER;
		CM_ICLKEN_PER |= wkst_per & PM_MPUGRPSEL_PER;
		CM_FCLKEN_PER |= wkst_per & PM_MPUGRPSEL_PER;
		PM_WKST_PER = wkst_per & PM_MPUGRPSEL_PER;
		while (PM_WKST_PER & PM_MPUGRPSEL_PER);
		CM_ICLKEN_PER = iclk;
		CM_FCLKEN_PER = fclk;
	}
#ifdef CONFIG_OMAP3430_ES2
	if (!(wkst_wkup | wkst1_core | wkst3_core | wkst_usbhost | wkst_per)) {
#else
	if (!(wkst_wkup | wkst1_core | wkst_per)) {
#endif
		if (!(PRM_IRQSTATUS_MPU & errst_vc)) {
			printk(KERN_ERR "%x,%x,%x,%x\n", PRM_IRQSTATUS_MPU,
					wkst_wkup, wkst1_core, wkst_per);
			printk(KERN_ERR "Spurious PRCM interrupt\n");
		}
	}

#ifdef CONFIG_OMAP_VOLT_SR_BYPASS
	if (PRM_IRQSTATUS_MPU & PRM_VC_TIMEOUTERR_ST)
		printk(KERN_ERR "PRCM : Voltage Controller timeout\n");
	if (PRM_IRQSTATUS_MPU & PRM_VC_RAERR_ST)
		printk(KERN_ERR "PRCM : Voltage Controller register address "
							"acknowledge error\n");
	if (PRM_IRQSTATUS_MPU & PRM_VC_SAERR_ST)
		printk(KERN_ERR "PRCM : Voltage Controller slave address "
							"acknowledge error\n");
#endif /* #ifdef CONFIG_OMAP_VOLT_SR_BYPASS */

	if (PRM_IRQSTATUS_MPU) {
		PRM_IRQSTATUS_MPU |= 0x3;
		while (PRM_IRQSTATUS_MPU);
	}
	return IRQ_HANDLED;
}

#ifdef CONFIG_PROC_FS
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
static void *omap_pm_prepwst_start(struct seq_file *m, loff_t *pos)
{
	return *pos < 1 ? (void *)1 : NULL;
}

static void *omap_pm_prepwst_next(struct seq_file *m, void *v, loff_t *pos)
{
	++*pos;
	return NULL;
}

static void omap_pm_prepwst_stop(struct seq_file *m, void *v)
{
}

char *pstates [4] =	{
			"OFF",
			"RET",
			"INACT",
			"ON"
		};

int omap_pm_prepwst_show(struct seq_file *m, void *v)
{
	u32 arr_rdptr, arr_cnt;

	seq_printf(m, "Previous power state for MPU +CORE\n");

	arr_rdptr = arr_wrptr;
	arr_cnt = LAST_IDLE_STATE_ARR_SIZE;

#ifdef DEBUG_HW_SUP
	seq_printf(m, "PM_PWSTST_CORE %x\n", PM_PWSTST_CORE);
	seq_printf(m, "PM_PREPWSTST_CORE %x\n", PM_PREPWSTST_CORE);
	seq_printf(m, "CM_CLKSTCTRL_CORE %x\n", CM_CLKSTCTRL_CORE);
	seq_printf(m, "CM_CLKSTST_CORE %x\n\n", CM_CLKSTST_CORE);

	seq_printf(m, "PM_PWSTST_IVA2 %x\n", PM_PWSTST_IVA2);
	seq_printf(m, "PM_PREPWSTST_IVA2 %x\n", PM_PREPWSTST_IVA2);
	seq_printf(m, "CM_CLKSTCTRL_IVA2 %x\n", CM_CLKSTCTRL_IVA2);
	seq_printf(m, "CM_CLKSTST_IVA2 %x\n\n", CM_CLKSTST_IVA2);

#ifndef CONFIG_ARCH_OMAP3410
	seq_printf(m, "PM_PWSTST_NEON %x\n", PM_PWSTST_NEON);
	seq_printf(m, "PM_PREPWSTST_NEON %x\n", PM_PREPWSTST_NEON);
	seq_printf(m, "CM_CLKSTCTRL_NEON %x\n\n", CM_CLKSTCTRL_NEON);
#endif
	seq_printf(m, "PM_PWSTST_PER %x\n", PM_PWSTST_PER);
	seq_printf(m, "PM_PREPWSTST_PER %x\n", PM_PREPWSTST_PER);
	seq_printf(m, "CM_CLKSTCTRL_PER %x\n", CM_CLKSTCTRL_PER);
	seq_printf(m, "CM_CLKSTST_PER %x\n\n", CM_CLKSTST_PER);

	seq_printf(m, "PM_PWSTST_DSS %x\n", PM_PWSTST_DSS);
	seq_printf(m, "PM_PREPWSTST_DSS %x\n", PM_PREPWSTST_DSS);
	seq_printf(m, "CM_CLKSTCTRL_DSS %x\n", CM_CLKSTCTRL_DSS);
	seq_printf(m, "CM_CLKSTST_DSS %x\n\n", CM_CLKSTST_DSS);

#ifdef CONFIG_OMAP3430_ES2
	seq_printf(m, "PM_PWSTST_USBHOST %x\n", PM_PWSTST_USBHOST);
	seq_printf(m, "PM_PREPWSTST_USBHOST %x\n", PM_PREPWSTST_USBHOST);
	seq_printf(m, "CM_CLKSTCTRL_USBHOST %x\n", CM_CLKSTCTRL_USBHOST);
	seq_printf(m, "CM_CLKSTST_USBHOST %x\n\n", CM_CLKSTST_USBHOST);

#ifndef CONFIG_ARCH_OMAP3410
	seq_printf(m, "PM_PWSTST_SGX %x\n", PM_PWSTST_SGX);
	seq_printf(m, "PM_PREPWSTST_SGX %x\n", PM_PREPWSTST_SGX);
	seq_printf(m, "CM_CLKSTCTRL_SGX %x\n", CM_CLKSTCTRL_SGX);
	seq_printf(m, "CM_CLKSTST_SGX %x\n\n", CM_CLKSTST_SGX);
#endif
#else
	seq_printf(m, "PM_PWSTST_GFX %x\n", PM_PWSTST_GFX);
	seq_printf(m, "PM_PREPWSTST_GFX %x\n", PM_PREPWSTST_GFX);
	seq_printf(m, "CM_CLKSTCTRL_GFX %x\n", CM_CLKSTCTRL_GFX);
	seq_printf(m, "CM_CLKSTST_GFX %x\n\n", CM_CLKSTST_GFX);
#endif /* #ifdef CONFIG_OMAP3430_ES2 */

	seq_printf(m, "PM_PWSTST_CAM %x\n", PM_PWSTST_CAM);
	seq_printf(m, "PM_PREPWSTST_CAM %x\n", PM_PREPWSTST_CAM);
	seq_printf(m, "CM_CLKSTCTRL_CAM %x\n", CM_CLKSTCTRL_CAM);
	seq_printf(m, "CM_CLKSTST_CAM %x\n\n", CM_CLKSTST_CAM);
#endif /* #ifdef DEBUG_HW_SUP */

	while (arr_cnt--) {
		if (arr_rdptr == 0)
			arr_rdptr = LAST_IDLE_STATE_ARR_SIZE;

		arr_rdptr--;
		seq_printf(m, "MPU = %x - %s,  CORE = %x - %s, IVA = %x - %s,"
						"fclks: %x, iclks: %x\n",
				last_idle_state[arr_wrptr].mpu_state,
				pstates [last_idle_state[arr_wrptr].mpu_state &
				0x3], last_idle_state[arr_wrptr].core_state,
				pstates [last_idle_state[arr_wrptr].core_state &
				0x3], last_idle_state[arr_wrptr].iva_state,
				pstates [last_idle_state[arr_wrptr].iva_state &
				0x3], last_idle_state[arr_wrptr].fclks,
				last_idle_state[arr_wrptr].iclks
		);
	}
	seq_printf(m, "\nMPU RET CNT = %d, MPU OFF CNT = %d, CORE RET CNT= %d,"
			" CORE OFF CNT= %d\n\n",
			mpu_ret_cnt, mpu_off_cnt, core_ret_cnt, core_off_cnt);

	return 0;
}

static struct seq_operations omap_pm_prepwst_op = {
	.start = omap_pm_prepwst_start,
	.next  = omap_pm_prepwst_next,
	.stop  = omap_pm_prepwst_stop,
	.show  = omap_pm_prepwst_show
};

static int omap_pm_prepwst_open(struct inode *inode, struct file *file)
{
	return seq_open(file, &omap_pm_prepwst_op);
}

static struct file_operations proc_pm_prepwst_ops = {
	.open		= omap_pm_prepwst_open,
	.read		= seq_read,
	.llseek		= seq_lseek,
	.release	= seq_release,
};

/* This API creates a proc entry for shared resources. */
int create_pmproc_entry(void)
{
	struct proc_dir_entry *entry;

	/* Create a proc entry for shared resources */
	entry = create_proc_entry("pm_prepwst", 0, NULL);
	if (entry) {
		entry->proc_fops = &proc_pm_prepwst_ops;
		printk(KERN_ERR "create_proc_entry succeeded\n");
		}
	else
		printk(KERN_ERR "create_proc_entry failed\n");

	return 0;
}

#endif  /* #ifdef CONFIG_PROC_FS */


int __init omap3_pm_init(void)
{
	int m, ret;

	printk(KERN_ERR "Power Management for TI OMAP.\n");

	_omap_sram_idle = omap_sram_push(omap34xx_suspend,
					omap34xx_suspend_sz);

	suspend_set_ops(&omap_pm_ops);

	/* In case of cold boot, clear scratchpad */
	if (RM_RSTST_CORE & 0x1)
		clear_scratchpad_contents();
#ifdef CONFIG_MPU_OFF
	save_scratchpad_contents();
#endif
	prcm_init();

	memret1 = (struct res_handle *)resource_get("corememresret1",
							"core_mem1ret");
	memret2 = (struct res_handle *)resource_get("corememresret2",
							"core_mem2ret");
	logret1 = (struct res_handle *)resource_get("corelogicret",
							"core_logicret");

	/* Registering PRCM Interrupts to MPU, for Wakeup events */
	ret = request_irq(PRCM_MPU_IRQ, (irq_handler_t)prcm_interrupt_handler,
				IRQF_DISABLED, "prcm", NULL);
	if (ret) {
		printk(KERN_ERR "request_irq failed to register for 0x%x\n",
			PRCM_MPU_IRQ);
		return -1;
	}

	/* Adjust the system OPP based during bootup using
	* KConfig option
	*/
	if (p_vdd1_clk == NULL) {
		p_vdd1_clk = clk_get(NULL, "virt_vdd1_prcm_set");
		if (p_vdd1_clk == NULL) {
			printk(KERN_ERR "Unable to get the VDD1 clk\n");
			return -1;
		}
	}

	if (p_vdd2_clk == NULL) {
		p_vdd2_clk = clk_get(NULL, "virt_vdd2_prcm_set");
		if (p_vdd2_clk == NULL) {
			printk(KERN_ERR "Unable to get the VDD2 clk\n");
			return -1;
		}
	}

	/* Configure OPP for VDD1 based on compile time setting */
#ifdef CONFIG_OMAP3ES1_VDD1_OPP1
	vdd1_opp_no = PRCM_VDD1_OPP1;
#elif defined(CONFIG_OMAP3ES2_VDD1_OPP1) \
	|| defined(CONFIG_OMAP3ES2_3410_VDD1_OPP1)
	vdd1_opp_no = PRCM_VDD1_OPP1;
#elif defined(CONFIG_OMAP3ES1_VDD1_OPP2)
	vdd1_opp_no = PRCM_VDD1_OPP2;
#elif defined(CONFIG_OMAP3ES2_VDD1_OPP2) \
	|| defined(CONFIG_OMAP3ES2_3410_VDD1_OPP2)
	vdd1_opp_no = PRCM_VDD1_OPP2;
#elif defined(CONFIG_OMAP3ES1_VDD1_OPP3)
	vdd1_opp_no = PRCM_VDD1_OPP3;
#elif defined(CONFIG_OMAP3ES2_VDD1_OPP3) \
	|| defined(CONFIG_OMAP3ES2_3410_VDD1_OPP3)
	vdd1_opp_no = PRCM_VDD1_OPP3;
#elif defined(CONFIG_OMAP3ES1_VDD1_OPP4)
	vdd1_opp_no = PRCM_VDD1_OPP4;
#elif defined(CONFIG_OMAP3ES2_VDD1_OPP4) \
	|| defined(CONFIG_OMAP3ES2_3410_VDD1_OPP4)
	vdd1_opp_no = PRCM_VDD1_OPP4;
#elif defined(CONFIG_OMAP3ES2_VDD1_OPP5) \
	|| defined(CONFIG_OMAP3ES2_3410_VDD1_OPP5)
	vdd1_opp_no = PRCM_VDD1_OPP5;
#endif

#ifdef CONFIG_OMAP3ES2_VDD2_OPP2
	vdd2_opp_no = PRCM_VDD2_OPP2;
#elif defined(CONFIG_OMAP3ES2_VDD2_OPP3)
	vdd2_opp_no = PRCM_VDD2_OPP3;
	prcm_do_voltage_scaling(PRCM_VDD2_OPP3, PRCM_VDD2_OPP2);
#endif

	/* Request for VDD1 and VDD2 OPP constraints */
	/* Currently by default VDD2 OPP level 3 is requested */
	if (vdd1_opp_no != PRCM_VDD1_OPP1) {
		vdd1_opp_co = constraint_get("pm_fwk", &cnstr_id1);
		constraint_set(vdd1_opp_co, get_opp_no(vdd1_opp_no));
	}

	if (vdd2_opp_no != PRCM_VDD2_OPP1) {
		vdd2_opp_co = constraint_get("pm_fwk", &cnstr_id2);
		constraint_set(vdd2_opp_co, get_opp_no(vdd2_opp_no));
	}

#ifdef CONFIG_CPU_IDLE
	omap_init_power_states();
	cpuidle_register_driver(&omap3_idle_driver);
	omap3_idle_init();
	PRM_IRQSTATUS_MPU = 0x3FFFFFD;
	PRM_IRQENABLE_MPU = 0x1;
#ifdef IOPAD_WKUP
	/* Enabling the IO_PAD PRCM interrupts */
	PRM_IRQENABLE_MPU |= 0x200;
#endif /* #ifdef IOPAD_WKUP */

#ifdef CONFIG_OMAP_VOLT_SR_BYPASS
	/* Enabling the VOLTAGE CONTROLLER PRCM interrupts */
	PRM_IRQENABLE_MPU |= PRM_VC_TIMEOUTERR_EN | PRM_VC_RAERR_EN|
							PRM_VC_SAERR_EN;
#endif /* #ifdef CONFIG_OMAP_VOLT_SR_BYPASS */
#endif /* CPU_IDLE */

#if defined(CONFIG_PM) && defined(CONFIG_CPU_IDLE)
	m = subsys_create_file(&power_subsys, &sleep_idle_state);
	if (m)
		printk(KERN_ERR "subsys_create_file failed: %d\n", m);
	m = subsys_create_file(&power_subsys, &off_enable);
	if (m)
		printk(KERN_ERR "subsys_create_file failed for off: %d\n", m);
	/*For register dump*/
	m = subsys_create_file(&power_subsys, &domain_print);
	if (m)
		printk(KERN_ERR "subsys_create_file failed for print: %d\n", m);

	/* Sysfs entry for setting Display timeout*/
	m = subsys_create_file(&power_subsys, &fb_timeout);
	if (m)
		printk(KERN_ERR "subsys_create_file failed: %d\n", m);
	 /* Sysfs entry for setting opp for vdd1 */
		m = subsys_create_file(&power_subsys, &vdd1_opp);
	if (m)
		printk(KERN_ERR "subsys_create_file failed: %d\n", m);
	 /* Sysfs entry for setting opp for vdd2 */
	m = subsys_create_file(&power_subsys, &vdd2_opp);
	if (m)
		printk(KERN_ERR "subsys_create_file failed: %d\n", m);
#endif

	/* Set the default value of frame buffer timeout*/
	set_blank_interval(fb_timeout_val);
#ifdef CONFIG_PROC_FS
	create_pmproc_entry();
#endif  /* #ifdef CONFIG_PROC_FS */

#ifdef CONFIG_MPU_OFF
	clear_scratchpad_contents();
	save_scratchpad_contents();
#endif
	return 0;
}

/* Clears the scratchpad contents in case of cold boot- called during bootup*/
void clear_scratchpad_contents(void)
{
	u32 max_offset = SCRATHPAD_ROM_OFFSET;
	u32 offset = 0;
	u32 v_addr = io_p2v(SCRATCHPAD_ROM);
	/* Check if it is a cold reboot */
	if (PRM_RSTST & 0x1) {
		for ( ; offset <= max_offset; offset += 0x4)
			__raw_writel(0x0, (v_addr + offset));
		PRM_RSTST |= 0x1;
	}
}

/* Populate the scratchpad structure with restore structure */
void save_scratchpad_contents(void)
{
	u32 *scratchpad_address;
	u32 *restore_address;
	u32 *sdram_context_address;
	/* Get virtual address of SCRATCHPAD */
	scratchpad_address = (u32 *) io_p2v(SCRATCHPAD);
	/* Get Restore pointer to jump to while waking up from OFF */
	restore_address = get_restore_pointer();
	/* Convert it to physical address */
	restore_address = (u32 *) io_v2p(restore_address);
	/* Get address where registers are saved in SDRAM */
	sdram_context_address = (u32 *) io_v2p(context_mem);
	/* Booting configuration pointer*/
	*(scratchpad_address++) = 0x0;
	/* Public restore pointer */
	/* On ES 2.0, if scrathpad is populated with valid
	* pointer, warm reset does not work
	* So populate scrathpad restore address only in
	* cpuidle and suspend calls
	*/
	scratchpad_restore_addr = scratchpad_address;
	restore_pointer_address = (u32) restore_address;
	*(scratchpad_address++) = 0x0;
	/* Secure ram restore pointer */
	*(scratchpad_address++) = 0x0;
	/* SDRC Module semaphore */
	*(scratchpad_address++) = 0x0;
	/* PRCM Block Offset */
	*(scratchpad_address++) = 0x2C;
	/* SDRC Block Offset */
	*(scratchpad_address++) = 0x64;
	/* Empty */
	/* Offset 0x8*/
	*(scratchpad_address++) = 0x0;
	*(scratchpad_address++) = 0x0;
	/* Offset 0xC*/
	*(scratchpad_address++) = 0x0;
	/* Offset 0x10*/
	*(scratchpad_address++) = 0x0;
	/* Offset 0x14*/
	*(scratchpad_address++) = 0x0;
	/* Offset 0x18*/
	/* PRCM Block */
	*(scratchpad_address++) = PRM_CLKSRC_CTRL;
	*(scratchpad_address++) = PRM_CLKSEL;
	*(scratchpad_address++) = CM_CLKSEL_CORE;
	*(scratchpad_address++) = CM_CLKSEL_WKUP;
	*(scratchpad_address++) = CM_CLKEN_PLL;
	*(scratchpad_address++) = CM_AUTOIDLE_PLL;
	*(scratchpad_address++) = CM_CLKSEL1_PLL;
	*(scratchpad_address++) = CM_CLKSEL2_PLL;
	*(scratchpad_address++) = CM_CLKSEL3_PLL;
	*(scratchpad_address++) = CM_CLKEN_PLL_MPU;
	*(scratchpad_address++) = CM_AUTOIDLE_PLL_MPU;
	*(scratchpad_address++) = CM_CLKSEL1_PLL_MPU;
	*(scratchpad_address++) = CM_CLKSEL2_PLL_MPU;
	*(scratchpad_address++) = 0x0;
	/* SDRC Block */
	*(scratchpad_address++) = ((SDRC_CS_CFG & 0xFFFF) << 16) |
					(SDRC_SYS_CONFIG & 0xFFFF);
	*(scratchpad_address++) = ((SDRC_ERR_TYPE & 0xFFFF) << 16) |
					(SDRC_SHARING & 0xFFFF);
	*(scratchpad_address++) = SDRC_DLL_A_CTRL;
	*(scratchpad_address++) = 0x0;
	*(scratchpad_address++) = SDRC_PWR;
	*(scratchpad_address++) = 0x0;
	*(scratchpad_address++) = SDRC_MCFG_0;
	*(scratchpad_address++) = SDRC_MR0 & 0xFFFF;
	*(scratchpad_address++) = 0x0;
	*(scratchpad_address++) = SDRC_ACTIM_CTRL_A_0;
	*(scratchpad_address++) = SDRC_ACTIM_CTRL_B_0;
	*(scratchpad_address++) = SDRC_RFR_CTRL_0;
	*(scratchpad_address++) = 0x0;
	*(scratchpad_address++) = SDRC_MCFG_1;
	*(scratchpad_address++) = SDRC_MR1 & 0xFFFF;
	*(scratchpad_address++) = 0x0;
	*(scratchpad_address++) = SDRC_ACTIM_CTRL_A_1;
	*(scratchpad_address++) = SDRC_ACTIM_CTRL_B_1;
	*(scratchpad_address++) = SDRC_RFR_CTRL_1;
	*(scratchpad_address++) = 0x0;
	*(scratchpad_address++) = 0x0;
	*(scratchpad_address++) = 0x0;
	*(scratchpad_address++) = (u32) sdram_context_address;
}

__initcall(omap3_pm_init);
