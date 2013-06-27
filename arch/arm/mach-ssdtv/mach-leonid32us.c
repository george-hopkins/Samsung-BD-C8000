/*
 * linux/arch/arm/mach-ssdtv/mach-leonid32us.c
 *
 * Copyright (C) 2008 Samsung Electronics.co
 * Author : tukho.kim@samsung.com  06/19/2008
 * ij.jang 080807 : create for leonid 32bit 58MB kernel (US)
 * ij.jang 080731 : create for leonid 48bit 70MB kernel (US)
 * ij.jang 080731 : modify to 67mb, rename to leonid48eu
 * ij.jang 080721 : create leonid48eu 68mb version
 */

#include <linux/config.h>
#include <linux/init.h>
#include <linux/device.h>
#include <linux/dma-mapping.h>

#include <asm/mach-types.h>
#include <asm/mach/map.h>

#include <asm/hardware.h>
#include <asm/setup.h>
#include <asm/mach/arch.h>

extern void ssdtv_init_irq(void); 
extern struct sys_timer ssdtv_timer; 
extern void sdp83_init(void);
extern void sdp83_iomap_init(void);

static struct map_desc leonid32us_io_desc[] __initdata = {
{ 
	.virtual = 0xE0000000,
	.pfn     = __phys_to_pfn(MACH_MEM0_BASE + SYS_MEM0_SIZE),  
	.length  = MACH_MEM0_SIZE - SYS_MEM0_SIZE,
	.type    = MT_DEVICE 
},
{	/* PCI configuration */
	.virtual = 0xF0000000,
	.pfn     = __phys_to_pfn(0x50000000),
	.length  = SZ_64M,
	.type    = MT_DEVICE 
},
{ 
	/* PCI I/O */
	.virtual = 0xF4000000,
	.pfn     = __phys_to_pfn(0x5C000000),
	.length  = SZ_32M,
	.type    = MT_DEVICE 
 },
};

static void __init leonid32us_iomap_init(void)
{
	iotable_init(leonid32us_io_desc, ARRAY_SIZE(leonid32us_io_desc));
}

static void __init leonid32us_map_io(void)
{
	//initialize iomap of special function register address
	sdp83_iomap_init();

	//initialize iomap for peripheral device
	leonid32us_iomap_init();
}

static void __init leonid32us_init(void)
{
	sdp83_init();

	// Set USB Register
	*(volatile unsigned int*)(VA_EHCI0_BASE + 0x94) = 0x00780020;
	*(volatile unsigned int*)(VA_EHCI0_BASE + 0x9C) = 0x00000000;

	*(volatile unsigned int*)(VA_EHCI1_BASE + 0x94) = 0x00780020;
	*(volatile unsigned int*)(VA_EHCI1_BASE + 0x9C) = 0x00000000;
}

static void __init 
leonid32us_fixup(struct machine_desc *mdesc, struct tag *tags, char ** cmdline, struct meminfo *meminfo)
{
	meminfo->nr_banks = 1;
	meminfo->bank[0].start = MACH_MEM0_BASE;
	meminfo->bank[0].size =  SYS_MEM0_SIZE;
	meminfo->bank[0].node =  0;
}

MACHINE_START(SDP83_EVAL_LEONID32US, "Samsung-SDP83 Eval Board(32bit US)")
	.phys_io  = PA_IO_BASE0,
	.io_pg_offst = VA_IO_BASE0,
	.boot_params  = (PHYS_OFFSET + 0x100),
	.map_io		= leonid32us_map_io,
	.init_irq	= ssdtv_init_irq,
	.timer		= &ssdtv_timer,
	.init_machine	= leonid32us_init,
	.fixup		= leonid32us_fixup,
MACHINE_END

