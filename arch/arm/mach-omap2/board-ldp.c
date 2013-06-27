/*
 * linux/arch/arm/mach-omap2/board-ldp.c
 *
 * Copyright (C) 2007 Texas Instruments
 *
 * Modified from mach-omap2/board-generic.c
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/delay.h>
#include <linux/input.h>
#include <linux/workqueue.h>
#include <linux/err.h>
#include <linux/clk.h>
#include <linux/spi/spi.h>
#include <linux/spi/ads7846.h>

#include <asm/hardware.h>
#include <asm/mach-types.h>
#include <asm/mach/arch.h>
#include <asm/mach/map.h>

#include <asm/arch/gpio.h>
#include <asm/arch/mux.h>
#include <asm/arch/board.h>
#include <asm/arch/common.h>
#include <asm/arch/keypad.h>
#include <asm/arch/dma.h>
#include <asm/arch/gpmc.h>
#include <asm/arch/twl4030-rtc.h>
#include <asm/arch/mcspi.h>

#include <asm/io.h>
#include <asm/delay.h>
#include <asm/arch/control.h>

#define CONTROL_SYSC_SMARTIDLE	(0x2 << 3)
#define CONTROL_SYSC_AUTOIDLE	(0x1)

#define PRCM_INTERRUPT_MASK	(1 << 11)
#define UART1_INTERRUPT_MASK	(1 << 8)
#define UART2_INTERRUPT_MASK    (1 << 9)
#define UART3_INTERRUPT_MASK    (1 << 10)
#define TWL4030_MSECURE_GPIO    22
#include "prcm-regs.h"

#define CONFIG_I2C_OMAP34XX_HS_BUS1	100
#define CONFIG_I2C_OMAP34XX_HS_BUS2	100
#define CONFIG_I2C_OMAP34XX_HS_BUS3	100

/* GPIO used for TSC2046 (touchscreen)
 *
 * Also note that the tsc2046 is the same silicon as the ads7846, so
 * that driver is used for the touchscreen. */
#define TS_GPIO			54

unsigned int uart_interrupt_mask_value;

static struct resource ldp3430_smc91x_resources[] = {
	[0] = {
		.start	= OMAP34XX_ETHR_START,
		.end	= OMAP34XX_ETHR_START + SZ_4K,
		.flags	= IORESOURCE_MEM,
	},
	[1] = {
		.start	= 0,
		.end	= 0,
		.flags	= IORESOURCE_IRQ,
	},
};

static struct platform_device ldp3430_smc91x_device = {
	.name		= "smc911x",
	.id		= -1,
	.num_resources	= ARRAY_SIZE(ldp3430_smc91x_resources),
	.resource	= ldp3430_smc91x_resources,
};
static u32 *uart_detect(void){
	char str[7];
	u32 *temp_ptr = 0;
	if (console_detect(str))
		printk(KERN_INFO"Invalid console paramter....\n");

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
		printk(KERN_INFO
		"!!!!!!!!! Unable to recongnize Console UART........\n");
	return (u32 *)(temp_ptr);
}
void  init_wakeupconfig(void){
	u32 *ptr;
	/* init UARTs IO pads as wakeup capable in OFF mode */
	ptr = (u32 *)uart_detect();
	*ptr |= (u32)((IO_PAD_WAKEUPENABLE | IO_PAD_OFFPULLUDENABLE |
			IO_PAD_OFFOUTENABLE | IO_PAD_OFFENABLE |
			IO_PAD_INPUTENABLE | IO_PAD_PULLUDENABLE)
			<<  IO_PAD_HIGH_SHIFT);

	/* init GPIO-ethernet-irq pad as wakeup capable in OFF modes */
	/* bits[15:0]-- GPIO 152 */
	ptr = (u32 *)(&CONTROL_PADCONF_MCBSP4_CLKX);
	/* clear OFF state bits */
	*ptr &= (u32)~0x0000FE00;
	*ptr |= (u32)((IO_PAD_WAKEUPENABLE | IO_PAD_OFFPULLUDENABLE |
			IO_PAD_OFFOUTENABLE | IO_PAD_OFFENABLE |
			IO_PAD_INPUTENABLE));
}

void uart_padconf_control(void){
	u32 *ptr;
	ptr = (u32 *)uart_detect();
		*ptr |= (u32)((IO_PAD_WAKEUPENABLE)
			 << IO_PAD_HIGH_SHIFT);
}
/* currently only called on suspend path */
void setup_board_wakeup_source(u32 wakeup_source)
{
	if (wakeup_source & PRCM_WAKEUP_T2_KEYPAD) {
		PRCM_GPIO1_SYSCONFIG = 0x15;
		PM_WKEN_WKUP |= 0x8;     /* en gpio 1 block wakeup */
		PM_MPUGRPSEL_WKUP = 0x8; /* route gpio 1 block wake to MPU */
		INTC_MIR_0 = ~((1<<29)); /* unmask GPIO block 1 at MPU pic */
		/* mode switch from nIRQ to GPIO-block1-source0 */
		CONTROL_PADCONF_SYS_NIRQ &= 0xFFFFFFF8;
		CONTROL_PADCONF_SYS_NIRQ |= 0x4;
		/* Enable wake up trigger in GPIO1 */
		GPIO1_SETIRQENABLE1 |= 0x1;
		GPIO1_SETWKUENA |= 0x1;
		GPIO1_FALLINGDETECT |= 0x1;
	}
	/* Unmasking the PRCM interrupts */
	INTC_MIR_0 &= ~PRCM_INTERRUPT_MASK;
	 /* Unmasking the UART interrupts */
	INTC_MIR_2 = ~uart_interrupt_mask_value;
}

static void scm_clk_init(void)
{
	struct clk *p_omap_ctrl_clk = NULL;

	p_omap_ctrl_clk = clk_get(NULL, "omapctrl_ick");
	if (p_omap_ctrl_clk != NULL) {
		if (clk_enable(p_omap_ctrl_clk)	!= 0) {
			printk(KERN_ERR "failed to enable scm clks\n");
			clk_put(p_omap_ctrl_clk);
		}
	}
	/* Sysconfig set to SMARTIDLE and AUTOIDLE */
	CONTROL_SYSCONFIG = (CONTROL_SYSC_SMARTIDLE | CONTROL_SYSC_AUTOIDLE);
}

static void ads7846_dev_init(void)
{
	if (omap_request_gpio(TS_GPIO) < 0)
		printk(KERN_ERR "can't get ads746 pen down GPIO\n");

	omap_set_gpio_direction(TS_GPIO, 1);

	omap_set_gpio_debounce(TS_GPIO, 1);
	omap_set_gpio_debounce_time(TS_GPIO, 0xa);
}

static int ads7846_get_pendown_state(void)
{
	return !omap_get_gpio_datain(TS_GPIO);
}

/* This enable(1)/disable(0) the voltage for TS: uses twl4030 calls */
static int ads7846_vaux_control(int vaux_cntrl)
{
	int ret = 0;

	/* check for return value of ldo_use: if success it returns 0*/
	if (vaux_cntrl == VAUX_ENABLE)
		ret = twl4030_vaux1_ldo_use();
	else if (vaux_cntrl == VAUX_DISABLE)
		ret = twl4030_vaux1_ldo_unuse();

	return ret;
}

static struct ads7846_platform_data tsc2046_config = {
	.x_max		    = 0x0fff,
	.y_max		    = 0x0fff,
	.x_plate_ohms	    = 90,
	.pressure_max	    = 255,
	.debounce_max	    = 10,
	.debounce_tol	    = 10,
	.debounce_rep	    = 1,
	.get_pendown_state = ads7846_get_pendown_state,
	.keep_vref_on	   = 1,
	.vaux_control	   = ads7846_vaux_control,
	.settle_delay_usecs = 150,
};


static struct omap2_mcspi_device_config tsc2046_mcspi_config = {
	.turbo_mode	= 0,
	.single_channel = 1,  /* 0: slave, 1: master */
};

#ifdef CONFIG_SPI_TI_OMAP_TEST
static struct omap2_mcspi_device_config sublcd_mcspi_config = {
	.turbo_mode	= 0,
	.single_channel = 1,  /* 0: slave, 1: master */
};

static struct omap2_mcspi_device_config dummy1_mcspi_config = {
	.turbo_mode	= 0,
	.single_channel = 1,  /* 0: slave, 1: master */
};
static struct omap2_mcspi_device_config dummy2_mcspi_config = {
	.turbo_mode	= 0,
	.single_channel = 0,  /* 0: slave, 1: master */
};
#endif

static struct spi_board_info ldp3430_spi_board_info[] __initdata = {
	[0] = {
		/* TSC2046 operates at a max freqency of 2MHz, so
		 * operate slightly below at 1.5MHz */
		.modalias	= "ads7846",
		.bus_num	= 1,
		.chip_select	= 0,
		.max_speed_hz   = 1500000,
		.controller_data = &tsc2046_mcspi_config,
		.irq		 = OMAP_GPIO_IRQ(TS_GPIO),
		.platform_data  = &tsc2046_config,
	},
#ifdef CONFIG_SPI_TI_OMAP_TEST
/* below info is only for test case */
	[1] = {
		.modalias	= "dummydevice1",
		.bus_num	= 2,
		.chip_select	= 0,
		.max_speed_hz   = 1500000,
		.controller_data = &dummy1_mcspi_config,
	},

	[2] = {
		.modalias	= "dummydevice2",
		.bus_num	= 3,
		.chip_select	= 0,
		.max_speed_hz   = 6000000,
		.controller_data = &dummy2_mcspi_config,
	},
	[3] = {
		.modalias	= "sublcd",
		.bus_num	= 1,
		.chip_select	= 2,
		.max_speed_hz   = 1500000,
		.controller_data = &sublcd_mcspi_config,
	},
#endif
};

/*
 * Key mapping for 3430 Labrador board
 */
static int lab_twl4030_keymap[] = {
	KEY(0, 0, KEY_1),
	KEY(1, 0, KEY_2),
	KEY(2, 0, KEY_3),
	KEY(3, 0, KEY_ENTER),
	KEY(4, 0, KEY_F1),
	KEY(5, 0, KEY_F2),
	KEY(0, 1, KEY_4),
	KEY(1, 1, KEY_5),
	KEY(2, 1, KEY_6),
	KEY(3, 1, KEY_F5),
	KEY(4, 1, KEY_F3),
	KEY(5, 1, KEY_F4),
	KEY(0, 2, KEY_7),
	KEY(1, 2, KEY_8),
	KEY(2, 2, KEY_9),
	KEY(3, 2, KEY_F6),
	KEY(0, 3, KEY_F7),
	KEY(1, 3, KEY_0),
	KEY(2, 3, KEY_F8),
	KEY(0, 4, KEY_RIGHT),
	KEY(1, 4, KEY_UP),
	KEY(2, 4, KEY_DOWN),
	KEY(3, 4, KEY_LEFT),
	KEY(5, 4, KEY_MUTE),
	KEY(4, 4, KEY_VOLUMEUP),
	KEY(5, 5, KEY_VOLUMEDOWN),
	0
};

#define GPIO_KEY(gpio, key)  ((gpio<<16) | (key))

static unsigned int lab_omap_gpio_keymap[] = {
	GPIO_KEY(101, KEY_ENTER), /*select*/
	GPIO_KEY(102, KEY_F1),    /*S7*/
	GPIO_KEY(103, KEY_F2),    /*S3*/
	GPIO_KEY(104, KEY_F3),    /*S1*/
	GPIO_KEY(105, KEY_F4),    /*S2*/
	GPIO_KEY(106, KEY_LEFT),  /*left*/
	GPIO_KEY(107, KEY_RIGHT), /*right*/
	GPIO_KEY(108, KEY_UP),    /*up*/
	GPIO_KEY(109, KEY_DOWN),  /*down*/
	0
};

/* Labrador Keymaps:
 * Labrador uses both TWL4030 GPIO's and OMAP GPIO's to get key presses.
 * This is why there are two keymaps:
 *  - lab_twl4030_keymap (TWL4030)
 *  - lab_omap_gpio_keymap (OMAP GPIO's)
 * Unfortunately the input subsystem requires all the keymaps to be
 * listed in one place (.keymap) in order for a key to be a valid input.
 * This is why some keys appear in both keymaps.
 * If a key does appear in both keymaps then its entry in
 * lab_twl4030_keymap is purely to keep the input subsystem happy and
 * its row/col values have no meaning.
 */
static struct omap_kp_platform_data ldp3430_kp_data = {
	.rows		= 6,
	.cols		= 6,
	.keymap 	= lab_twl4030_keymap,
	.keymapsize 	= ARRAY_SIZE(lab_twl4030_keymap),
	.rep		= 1,
	/*Use row_gpios as a way to pass the OMAP GPIO keymap pointer*/
	.row_gpios	= lab_omap_gpio_keymap,
};

static struct platform_device ldp3430_kp_device = {
	.name		= "omap_twl4030keypad",
	.id		= -1,
	.dev		= {
		.platform_data = &ldp3430_kp_data,
	},
};

#if defined(CONFIG_RTC_DRV_TWL4030) || defined(CONFIG_RTC_DRV_TWL4030_MODULE)
static int twl4030_rtc_init(void)
{
	int ret = 0;

	/* 3430ES2.0 doesn't have msecure/gpio-22 line connected to T2 */
	if (is_device_type_gp() && is_sil_rev_less_than(OMAP3430_REV_ES2_0)) {
		u32 msecure_pad_config_reg = omap_ctrl_base_get() + 0xA3C;
		int mux_mask = 0x04;
		u16 tmp;

		ret = omap_request_gpio(TWL4030_MSECURE_GPIO);
		if (ret < 0) {
			printk(KERN_ERR "twl4030_rtc_init: can't"
				"reserve GPIO:%d !\n", TWL4030_MSECURE_GPIO);
			goto out;
		}
		/*
		 * TWL4030 will be in secure mode if msecure line from OMAP
		 * is low. Make msecure line high in order to change the
		 * TWL4030 RTC time and calender registers.
		 */
		omap_set_gpio_direction(TWL4030_MSECURE_GPIO, 0);

		tmp = omap_readw(msecure_pad_config_reg);
		tmp &= 0xF8;	/* To enable mux mode 03/04 = GPIO_RTC */
		tmp |= mux_mask;/* To enable mux mode 03/04 = GPIO_RTC */
		omap_writew(tmp, msecure_pad_config_reg);

		omap_set_gpio_dataout(TWL4030_MSECURE_GPIO, 1);
	}
out:
	return ret;
}

static void twl4030_rtc_exit(void)
{
	if (is_device_type_gp() && is_sil_rev_less_than(OMAP3430_REV_ES2_0))
		omap_free_gpio(TWL4030_MSECURE_GPIO);
}

static struct twl4030rtc_platform_data ldp3430_twl4030rtc_data = {
	.init = &twl4030_rtc_init,
	.exit = &twl4030_rtc_exit,
};

static struct platform_device ldp3430_twl4030rtc_device = {
	.name		= "twl4030_rtc",
	.id		= -1,
	.dev		= {
		.platform_data	= &ldp3430_twl4030rtc_data,
	},
};
#endif

static struct platform_device *ldp3430_devices[] __initdata = {
	&ldp3430_smc91x_device,
	&ldp3430_kp_device,
#if defined(CONFIG_RTC_DRV_TWL4030) || defined(CONFIG_RTC_DRV_TWL4030_MODULE)
	&ldp3430_twl4030rtc_device,
#endif
};

static void __init ldp3430_init_smc91x(void)
{
	unsigned long cs_mem_base;
	unsigned int rate;
	struct clk *l3ck;
	int eth_gpio = 0;

	l3ck = clk_get(NULL, "l3_ck");
	if (IS_ERR(l3ck))
		rate = 100000000;
	else
		rate = clk_get_rate(l3ck);

	if (gpmc_cs_request(SDP3430_SMC91X_CS, SZ_16M, &cs_mem_base) < 0) {
		printk(KERN_ERR "Failed to request GPMC mem for smc91x\n");
		return;
	}

	ldp3430_smc91x_resources[0].start = cs_mem_base + 0x0;
	ldp3430_smc91x_resources[0].end   = cs_mem_base + 0xf;
	udelay(100);

	eth_gpio = OMAP34XX_ETHR_GPIO_IRQ;

	ldp3430_smc91x_resources[1].start = OMAP_GPIO_IRQ(eth_gpio);

	if (omap_request_gpio(eth_gpio) < 0) {
		printk(KERN_ERR "Failed to request GPIO%d for smc91x IRQ\n",
			eth_gpio);
		return;
	}
	omap_set_gpio_direction(eth_gpio, 1);
}

static void __init omap_3430ldp_init_irq(void)
{
	omap2_init_common_hw();
	omap_init_irq();
	omap_gpio_init();
	ldp3430_init_smc91x();
}

static struct omap_uart_config ldp3430_uart_config __initdata = {
	.enabled_uarts	= ((1 << 0) | (1 << 1) | (1 << 2)),
};
static int __init omap3430_i2c_init(void)
{
	omap_register_i2c_bus(1, CONFIG_I2C_OMAP34XX_HS_BUS1, NULL, 0);
	omap_register_i2c_bus(2, CONFIG_I2C_OMAP34XX_HS_BUS2, NULL, 0);
	omap_register_i2c_bus(3, CONFIG_I2C_OMAP34XX_HS_BUS3, NULL, 0);
	return 0;
}

static struct omap_mmc_config ldp3430_mmc_config __initdata = {
	.mmc [0] = {
		.enabled        = 1,
		.wire4          = 1,
		.wp_pin         = -1,
		.power_pin      = -1,
		.switch_pin     = 0,
	},
	.mmc [1] = {
		.enabled        = 1,
		.wire4          = 1,
		.wp_pin         = -1,
		.power_pin      = -1,
		.switch_pin     = 1,
	},
};

static struct omap_board_config_kernel ldp3430_config[] __initdata = {
	{ OMAP_TAG_UART,	&ldp3430_uart_config },
	{ OMAP_TAG_MMC,         &ldp3430_mmc_config },
};

static void __init omap_3430ldp_init(void)
{
	/* System Control module clock initialization */
	scm_clk_init();
	platform_add_devices(ldp3430_devices, ARRAY_SIZE(ldp3430_devices));
	omap_board_config = ldp3430_config;
	omap_board_config_size = ARRAY_SIZE(ldp3430_config);
	spi_register_board_info(ldp3430_spi_board_info,
				ARRAY_SIZE(ldp3430_spi_board_info));
	ldp3430_flash_init();
	ads7846_dev_init();
	omap_serial_init();
	ldp_usb_init();
}
arch_initcall(omap3430_i2c_init);

static void __init omap_3430ldp_map_io(void)
{
	omap2_set_globals_343x();
	omap2_map_common_io();
}

MACHINE_START(OMAP_LDP, "OMAP3430 LDP/Zoom")
	/* Maintainer: Texas Instruments Inc */
	.phys_io	= 0x48000000,
	.io_pg_offst	= ((0xd8000000) >> 18) & 0xfffc,
	.boot_params	= 0x80000100,
	.map_io		= omap_3430ldp_map_io,
	.init_irq	= omap_3430ldp_init_irq,
	.init_machine	= omap_3430ldp_init,
	.timer		= &omap_timer,
MACHINE_END
