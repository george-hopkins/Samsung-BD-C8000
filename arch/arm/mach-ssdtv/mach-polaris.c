/*
 * linux/arch/arm/mach-ssdtv/mach-polaris.c
 *
 * Copyright (C) 2006 Samsung Electronics.co
 * Author : tukho.kim@samsung.com
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
extern void sdp75_init(void);
extern void sdp75_iomap_init(void);

static struct map_desc polaris_io_desc[] __initdata = {
/* SELP.arm.3.x support A1 2007-10-22 */
#ifndef CONFIG_DISCONTIGMEM
 { 
	.virtual = 0xE0000000,
	.pfn     = __phys_to_pfn(PA_MEMORY2_BASE),  
	.length  = PA_MEMORY2_SIZE,
	.type    = MT_DEVICE 
 },
 { 
	.virtual = 0xE8000000,
	.pfn     = __phys_to_pfn(PA_SYSTEM_BASE+KERNEL_MEM_SIZE),  
	.length  = POLARIS_IO_SIZE,
	.type    = MT_DEVICE 
 },
#else 
 { 
	.virtual = 0xE0000000,
	.pfn     = __phys_to_pfn(MACH_MEM0_BASE + SYS_MEM0_SIZE),  
	.length  = MACH_MEM0_SIZE - SYS_MEM0_SIZE,
	.type    = MT_DEVICE 
 },
 { 
	.virtual = 0xE8000000,
	.pfn     = __phys_to_pfn(MACH_MEM1_BASE + SYS_MEM1_SIZE),  
	.length  = MACH_MEM1_SIZE - SYS_MEM1_SIZE,
	.type    = MT_DEVICE 
 },
#endif

// pci memory configuration
 { 
	.virtual = 0xF0000000,			// pci configuration
	.pfn     = __phys_to_pfn(0x50000000),
	.length  = SZ_128M,
	.type    = MT_DEVICE 
 },

#ifndef CONFIG_SDP74
 { 
	.virtual = 0xD8000000,			// pci io region
	.pfn     = __phys_to_pfn(0x5C000000),
	.length  = SZ_64M,
	.type    = MT_DEVICE 
 },
#else
 { 
	.virtual = 0xD8000000,			// pci io region
	.pfn     = __phys_to_pfn(0x5C000000),
	.length  = SZ_64M - SZ_8M,
	.type    = MT_DEVICE 
 },

 { 
 	.virtual = 0xD8000000 + (SZ_64M - SZ_8M) , // sdp74 register 
	.pfn     = __phys_to_pfn(0x46000000),
	.length  = SZ_8M,
	.type    = MT_DEVICE 
 },

 { 
	.virtual = 0xDC000000,			// sdp74 memory  
	.pfn     = __phys_to_pfn(0x40000000),
	.length  = SZ_64M,
	.type    = MT_DEVICE 
 },
#endif
};

static void __init polaris_iomap_init(void)
{
	iotable_init(polaris_io_desc, ARRAY_SIZE(polaris_io_desc));
}

static void __init polaris_map_io(void)
{
	//initialize iomap of special function register address
	sdp75_iomap_init();

	//initialize iomap for peripheral device
	polaris_iomap_init();
}

static void __init polaris_init(void)
{
	sdp75_init();

// Set USB Register
	*(volatile unsigned int*)(VA_EHCI0_BASE + 0x94) = 0x00700020;
	*(volatile unsigned int*)(VA_EHCI0_BASE + 0x9C) = 0x00000000;

	*(volatile unsigned int*)(VA_EHCI1_BASE + 0x94) = 0x00700020;
	*(volatile unsigned int*)(VA_EHCI1_BASE + 0x9C) = 0x00000000;

}

static void __init 
polaris_fixup(struct machine_desc *mdesc, struct tag *tags, char ** cmdline, struct meminfo *meminfo)
{
	meminfo->nr_banks = 1;

/* SELP.arm.3.x support A1 2007-10-22 */
	meminfo->bank[0].start = MACH_MEM0_BASE;
	meminfo->bank[0].size =  SYS_MEM0_SIZE;
	meminfo->bank[0].node =  0;

#ifdef CONFIG_DISCONTIGMEM
#  ifdef CONFIG_MACH_POLARIS
	meminfo->nr_banks = N_MEM_CHANNEL;
/* Continous Offset Mapping */
#if 0
	meminfo->bank[1].start = MACH_MEM1_BASE + SYS_MEM0_SIZE;
	meminfo->bank[1].size =  MACH_MEM1_SIZE - SYS_MEM0_SIZE;
	meminfo->bank[1].node =  1;

/* Bank MemBase Mapping */
#else
	meminfo->bank[1].start = MACH_MEM1_BASE;
	meminfo->bank[1].size =  SYS_MEM1_SIZE;
	meminfo->bank[1].node =  1;
#endif
#  endif /*CONFIG_MACH_POLARIS*/
#endif
}

MACHINE_START(SDP75_EVAL_POLARIS, "Samsung-SDP75 Evaluation Board")
	.phys_io  = PA_IO_BASE0,
	.io_pg_offst = VA_IO_BASE0,
	.boot_params  = (PHYS_OFFSET + 0x100),
	.map_io		= polaris_map_io,
	.init_irq	= ssdtv_init_irq,
	.timer		= &ssdtv_timer,
	.init_machine	= polaris_init,
	.fixup		= polaris_fixup,
MACHINE_END

