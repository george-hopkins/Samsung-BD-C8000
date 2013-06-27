/*
 * linux/arch/arm/mach-ssdtv/mach-leonid64256.c
 *
 * Copyright (C) 2008 Samsung Electronics.co
 * Author : tukho.kim@samsung.com  06/19/2008
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

static struct map_desc leonid64256_io_desc[] __initdata = {
#ifdef CONFIG_DISCONTIGMEM
{ 
	.virtual = 0xE0000000,
	.pfn     = __phys_to_pfn(MACH_MEM0_BASE + SYS_MEM0_SIZE),  
	.length  = MACH_MEM0_SIZE - SYS_MEM0_SIZE,
	.type    = MT_DEVICE 
},
{ 
	.virtual = 0xE6000000,
	.pfn     = __phys_to_pfn(MACH_MEM1_BASE + SYS_MEM1_SIZE),  
	.length  = MACH_MEM1_SIZE - SYS_MEM1_SIZE,
	.type    = MT_DEVICE 
},
#endif
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

static void __init leonid64256_iomap_init(void)
{
	iotable_init(leonid64256_io_desc, ARRAY_SIZE(leonid64256_io_desc));
}

static void __init leonid64256_map_io(void)
{
	//initialize iomap of special function register address
	sdp83_iomap_init();

	//initialize iomap for peripheral device
	leonid64256_iomap_init();
}

static void __init leonid64256_init(void)
{
	sdp83_init();

	// Set USB Register
	*(volatile unsigned int*)(VA_EHCI0_BASE + 0x94) = 0x00780020;
	*(volatile unsigned int*)(VA_EHCI0_BASE + 0x9C) = 0x00000000;

	*(volatile unsigned int*)(VA_EHCI1_BASE + 0x94) = 0x00780020;
	*(volatile unsigned int*)(VA_EHCI1_BASE + 0x9C) = 0x00000000;
}

static void __init 
leonid64256_fixup(struct machine_desc *mdesc, struct tag *tags, char ** cmdline, struct meminfo *meminfo)
{
	meminfo->nr_banks = 1;

/* SELP.arm.3.x support A1 2007-10-22 */
	meminfo->bank[0].start = MACH_MEM0_BASE;
	meminfo->bank[0].size =  SYS_MEM0_SIZE;
	meminfo->bank[0].node =  0;

#ifdef CONFIG_DISCONTIGMEM
	meminfo->nr_banks = N_MEM_CHANNEL;
	meminfo->bank[1].start = MACH_MEM1_BASE;
	meminfo->bank[1].size =  SYS_MEM1_SIZE;
	meminfo->bank[1].node =  1;
#endif
}

MACHINE_START(SDP83_EVAL_LEONID64256, "Samsung-SDP83 Eval. Board(64bit 256MB)")
	.phys_io  = PA_IO_BASE0,
	.io_pg_offst = VA_IO_BASE0,
	.boot_params  = (PHYS_OFFSET + 0x100),
	.map_io		= leonid64256_map_io,
	.init_irq	= ssdtv_init_irq,
	.timer		= &ssdtv_timer,
	.init_machine	= leonid64256_init,
	.fixup		= leonid64256_fixup,
MACHINE_END

