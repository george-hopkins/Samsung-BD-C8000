/*
 * mach-corona.c
 *
 * Copyright (C) 2009 Samsung Electronics.co
 * <tukho.kim@samsung.com>
 */

#include <linux/init.h>
#include <linux/device.h>
#include <linux/dma-mapping.h>

#include <asm/mach-types.h>
#include <asm/mach/map.h>

#include <asm/hardware.h>
#include <asm/setup.h>
#include <asm/mach/arch.h>

const unsigned int sdp_sys_mem0_size = SYS_MEM0_SIZE;
const unsigned int sdp_vmalloc_end = 0xe4000000;
EXPORT_SYMBOL(sdp_sys_mem0_size);
EXPORT_SYMBOL(sdp_vmalloc_end);

extern void sdp_init_irq(void); 
extern struct sys_timer sdp_timer; 
extern void sdp93_init(void);
extern void sdp93_iomap_init(void);


static struct map_desc corona_io_desc[] __initdata = {
{ 
	.virtual = MACH_BD_SUB0_BASE,
	.pfn     = __phys_to_pfn(MACH_MEM0_BASE + SYS_MEM0_SIZE),  
	.length  = MACH_MEM0_SIZE - SYS_MEM0_SIZE,
	.type    = MT_DEVICE 
},
{ 
	.virtual = MACH_BD_SUB1_BASE,
	.pfn     = __phys_to_pfn(MACH_MEM1_BASE + SYS_MEM1_SIZE),  
	.length  = MACH_MEM1_SIZE - SYS_MEM1_SIZE,
	.type    = MT_DEVICE 
},
#if 0
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
#endif
};

static void __init corona_iomap_init(void)
{
	iotable_init(corona_io_desc, ARRAY_SIZE(corona_io_desc));
}

static void __init corona_map_io(void)
{
	//initialize iomap of special function register address
	sdp93_iomap_init();

	//initialize iomap for peripheral device
	corona_iomap_init();
}

static void __init corona_init(void)
{
	sdp93_init();
}

static void __init 
corona_fixup(struct machine_desc *mdesc, struct tag *tags, char ** cmdline, struct meminfo *meminfo)
{
	meminfo->nr_banks = 1;

	meminfo->bank[0].start = MACH_MEM0_BASE;
	meminfo->bank[0].size =  sdp_sys_mem0_size;
	meminfo->bank[0].node =  0;
#if defined(CONFIG_DISCONTIGMEM)
	meminfo->nr_banks = 2;

	meminfo->bank[1].start = MACH_MEM1_BASE;
	meminfo->bank[1].size =  SYS_MEM1_SIZE;
	meminfo->bank[1].node =  1;
#endif
}

MACHINE_START(SDP93_EVAL_CORONA, "Samsung SDP93 Evaluation board CORONA")
	.phys_io  = PA_IO_BASE0,
	.io_pg_offst = VA_IO_BASE0,
	.boot_params  = (PHYS_OFFSET + 0x100),
	.map_io		= corona_map_io,
	.init_irq	= sdp_init_irq,
	.timer		= &sdp_timer,
	.init_machine	= corona_init,
	.fixup		= corona_fixup,
MACHINE_END

