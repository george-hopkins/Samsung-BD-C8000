/*
 * linux/arch/arm/mach-ssdtv/mach-sdp78fpga_board.c
 *
 * Copyright (C) 2006 Samsung Electronics.co
 * Author : seongkoo.cheong@samsung.com
 */

#include <linux/config.h>
#include <linux/init.h>
#include <linux/device.h>
#include <linux/dma-mapping.h>
#include <linux/mtd/mtd.h>
#include <linux/mtd/onenand.h>

#include <asm/mach-types.h>
#include <asm/mach/map.h>
#include <asm/arch/sdp78fpga_board.h>
#include <asm/arch/sdp78fpga.h>

#include <asm/hardware.h>
#include <asm/setup.h>
#include <asm/mach/arch.h>
#include <asm/io.h>

extern void sdp78fpga_iomap_init (void);
extern void ssdtv_init_irq(void); 
extern struct sys_timer ssdtv_timer; 
extern void sdp78fpga_init(void);

/* static I/O memory mapping */
static struct map_desc sdp78fpga_board_io_desc[] __initdata = {
	{  0xe0000000, __phys_to_pfn(0x62000000),DDR_BLCOK_A_SIZE, MT_DEVICE },
	{  0xe6000000, __phys_to_pfn(0x70000000),DDR_BLCOK_B_SIZE, MT_DEVICE },
		
#ifdef CONFIG_S4LX_CS8900
	/* CS8900 */
	{ ETH_MEM_START, __phys_to_pfn(PA_SMC_BANK2), SZ_1M, MT_DEVICE },
#endif
};

static void __init sdp78fpga_board_usb_init(void)
{
	volatile unsigned long val = __raw_readl(&rUSB_PCI_SET);
	
	/* using oscillator, 3.75% TX VREF, HS tx pre-emphasis enabled */
	val &= ~( (0x1 << 17) | (0xF << 19) | (0x3 << 25) );
	val |= (0x1 << 17) | (0xB << 19) | (0x1 << 25);

	__raw_writel (val, &rUSB_PCI_SET);
}

static void __init sdp78fpga_board_init(void)
{
	sdp78fpga_init();
	sdp78fpga_board_usb_init();
}

void __init sdp78fpga_board_map_io(void)
{
	//initialize iomap of special function register address
	sdp78fpga_iomap_init();

	//initialize iomap for peripheral device
	iotable_init(sdp78fpga_board_io_desc, ARRAY_SIZE(sdp78fpga_board_io_desc));
}

static void __init sdp78fpga_board_fixup(struct machine_desc *mdesc,
		struct tag *tags, char ** cmdline, struct meminfo *meminfo)
{
	meminfo->nr_banks = 1;
	meminfo->bank[0].start = PHYS_OFFSET;
	meminfo->bank[0].size = PHYS_MEM_SIZE;
	
}

/* BOOT_MEM (pram, pio, vio)*/

MACHINE_START(SDP78FPGA, "Samsung SDP78 FPGA Board")
	.phys_io	= PA_IO_BASE0,
	.io_pg_offst	= VA_IO_BASE0,
	.boot_params	= PA_SYSTEM_BASE + 0x100,
	.map_io 	= sdp78fpga_board_map_io,
	.init_irq	= ssdtv_init_irq,
	.init_machine	= sdp78fpga_board_init,
	.timer		= &ssdtv_timer,
	.fixup		= sdp78fpga_board_fixup,
MACHINE_END

