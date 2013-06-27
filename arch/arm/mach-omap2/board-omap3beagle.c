/*
 * linux/arch/arm/mach-omap2/board-omap3beagle.c
 *
 * Copyright (C) 2008 Texas Instruments
 *
 * Modified from mach-omap2/board-3430sdp.c
 *
 * Initial code: Syed Mohammed Khasim
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/delay.h>
#include <linux/err.h>
#include <linux/clk.h>
#include <linux/io.h>

#include <asm/hardware.h>
#include <asm/mach-types.h>
#include <asm/mach/arch.h>
#include <asm/mach/map.h>

#include <asm/arch/gpio.h>
#include <asm/arch/board.h>
#include <asm/arch/usb-musb.h>
#include <asm/arch/usb-ehci.h>
#include <asm/arch/hsmmc.h>
#include <asm/arch/common.h>
#include <asm/arch/prcm.h>


#include "ti-compat.h"
#include "prcm-regs.h"

#define CONTROL_SYSC_SMARTIDLE	(0x2 << 3)
#define CONTROL_SYSC_AUTOIDLE	(0x1)

#define PRCM_INTERRUPT_MASK	(1 << 11)
#define UART1_INTERRUPT_MASK	(1 << 8)
#define UART2_INTERRUPT_MASK	(1 << 9)
#define UART3_INTERRUPT_MASK	(1 << 10) 
 
unsigned int uart_interrupt_mask_value;

static struct omap_uart_config omap3_beagle_uart_config __initdata = {
	.enabled_uarts	= ((1 << 0) | (1 << 1) | (1 << 2)),
};

extern int console_detect(char *str) ;          /* See: omap24xx-uart.c */
extern void __init omap3beagle_flash_init(void);

static u32 *uart_detect(void)
{
	char str[7];
	u32 *temp_ptr = 0;

	if (console_detect(str))
		printk(KERN_INFO "Invalid console paramter....\n");

	if (!strcmp(str, "ttyS0")) {
		temp_ptr = (u32 *)(&CONTROL_PADCONF_UART1_CTS);
		uart_interrupt_mask_value = UART1_INTERRUPT_MASK;
	}
	else if (!strcmp(str, "ttyS1")) {
		temp_ptr = (u32 *)(&CONTROL_PADCONF_UART2_TX);
		uart_interrupt_mask_value = UART2_INTERRUPT_MASK;
	}
	else if (!strcmp(str, "ttyS2")) {  
		temp_ptr = (u32 *)(&CONTROL_PADCONF_UART3_RTS_SD);
		uart_interrupt_mask_value = UART3_INTERRUPT_MASK;
	}
	else
		printk(KERN_INFO "ERROR: Unable to recongnize Console UART!\n");

	return (u32 *)(temp_ptr); 
}

void uart_padconf_control(void)
{
	u32 *ptr;

	ptr = (u32 *)uart_detect();

	*ptr |= (u32)((IO_PAD_WAKEUPENABLE) << IO_PAD_HIGH_SHIFT);
}


static int __init omap3_beagle_i2c_init(void)
{
	omap_register_i2c_bus(1, 2600, NULL, 0);
	omap_register_i2c_bus(2, 400, NULL, 0);
	omap_register_i2c_bus(3, 400, NULL, 0);
	return 0;
}

static void __init omap3_beagle_init_irq(void)
{
	omap2_init_common_hw();
	omap_init_irq();
	omap_gpio_init();
}

static void scm_clk_init(void)
{
	struct clk *p_omap_ctrl_clk = NULL;

	p_omap_ctrl_clk = clk_get (NULL, "omapctrl_ick");
	if (p_omap_ctrl_clk != NULL) {
		if (clk_enable(p_omap_ctrl_clk) != 0) {
			printk(KERN_ERR "failed to enable scm clks\n");
			clk_put(p_omap_ctrl_clk);
		}
	}
	/* Sysconfig set to SMARTIDLE and AUTOIDLE */
	CONTROL_SYSCONFIG = (CONTROL_SYSC_SMARTIDLE | CONTROL_SYSC_AUTOIDLE);
}

/* =============================================================================
 * PRCM
 * =============================================================================
 */     
void setup_board_wakeup_source(u32 wakeup_source)
{ 
	if ((wakeup_source & PRCM_WAKEUP_T2_KEYPAD) ||
		(wakeup_source & PRCM_WAKEUP_TOUCHSCREEN)) {
        
		PRCM_GPIO1_SYSCONFIG	= 0x15;
		PM_WKEN_WKUP		|= 0x8;
		PM_MPUGRPSEL_WKUP	= 0x8;
        
		/* Unmask GPIO interrupt */
		INTC_MIR_0 = ~((1<<29));
	}
 
	if (wakeup_source & PRCM_WAKEUP_T2_KEYPAD) {
		CONTROL_PADCONF_SYS_NIRQ	&= 0xFFFFFFF8;
		CONTROL_PADCONF_SYS_NIRQ	|= 0x4;
		GPIO1_SETIRQENABLE1		|= 0x1;
		GPIO1_SETWKUENA			|= 0x1;
		GPIO1_FALLINGDETECT		|= 0x1;
	}

	/* Unmasking the PRCM interrupts */
	INTC_MIR_0 &= ~PRCM_INTERRUPT_MASK;

	/* Unmask GPIO module 6 for Touchscreen */
	INTC_MIR_1 = ~((1<<3));

	/* Unmasking the UART interrupts */
	INTC_MIR_2 = ~uart_interrupt_mask_value;
}

void  init_wakeupconfig(void)
{
	u32 *ptr;

	ptr = (u32 *)uart_detect();

	*ptr |= (u32) ((IO_PAD_WAKEUPENABLE |
			IO_PAD_OFFPULLUDENABLE |
			IO_PAD_OFFOUTENABLE |
			IO_PAD_OFFENABLE |
			IO_PAD_INPUTENABLE |
			IO_PAD_PULLUDENABLE) << IO_PAD_HIGH_SHIFT);

	ptr = (u32 *)(&CONTROL_PADCONF_MCSPI1_CS1);

	*ptr |= (u32)(IO_PAD_WAKEUPENABLE |
			IO_PAD_OFFPULLUDENABLE |	/* Pull up enable */
			IO_PAD_OFFOUTENABLE |		/* Input enable   */
			IO_PAD_OFFENABLE |
			IO_PAD_INPUTENABLE |
			IO_PAD_PULLUDENABLE);
}


static struct omap_mmc_config omap3beagle_mmc_config __initdata = {
	.mmc [0] = {
		.enabled	= 1,
		.wire4		= 1,
	},
};

static struct omap_board_config_kernel omap3_beagle_config[] __initdata = {
	{ OMAP_TAG_UART,	&omap3_beagle_uart_config },
	{ OMAP_TAG_MMC,		&omap3beagle_mmc_config },
};

static void __init omap3_beagle_init(void)
{
	scm_clk_init();
	omap_board_config = omap3_beagle_config;
	omap_board_config_size = ARRAY_SIZE(omap3_beagle_config);
	omap3beagle_flash_init();
	omap_serial_init();
	hsmmc_init();
	usb_musb_init();
	usb_ehci_init();
}

arch_initcall(omap3_beagle_i2c_init);

static void __init omap3_beagle_map_io(void)
{
	omap2_set_globals_343x();
	omap2_map_common_io();
}

MACHINE_START(OMAP3_BEAGLE, "OMAP3 Beagle Board")
	/* Maintainer: Syed Mohammed Khasim - http://beagleboard.org */
	.phys_io	= 0x48000000,
	.io_pg_offst	= ((0xd8000000) >> 18) & 0xfffc,
	.boot_params	= 0x80000100,
	.map_io		= omap3_beagle_map_io,
	.init_irq	= omap3_beagle_init_irq,
	.init_machine	= omap3_beagle_init,
	.timer		= &omap_timer,
MACHINE_END
