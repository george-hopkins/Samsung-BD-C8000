/*
 * mach-aquila.c
 *
 * Copyright (C) 2009 Samsung Electronics.co
 * Ikjoon Jang <ij.jang@samsung.com>
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
EXPORT_SYMBOL(sdp_sys_mem0_size);

#if defined(CONFIG_MEMORYMAP_768)
const unsigned int sdp_vmalloc_end = 0xe4000000;
#elif defined(CONFIG_MEMORYMAP_512)
const unsigned int sdp_vmalloc_end = 0xd8000000;
#endif

EXPORT_SYMBOL(sdp_vmalloc_end);

extern void sdp_init_irq(void); 
extern struct sys_timer sdp_timer; 
extern void sdp92_init(void);
extern void sdp92_iomap_init(void);


static struct map_desc aquila_io_desc[] __initdata = {
{ 
	.virtual = MACH_DTVSUB0_BASE,
	.pfn     = __phys_to_pfn(MACH_MEM0_BASE + SYS_MEM0_SIZE),  
	.length  = MACH_MEM0_SIZE - SYS_MEM0_SIZE,
	.type    = MT_DEVICE 
},
{ 
	.virtual = MACH_DTVSUB1_BASE,
	.pfn     = __phys_to_pfn(MACH_MEM1_BASE + SYS_MEM1_SIZE),  
	.length  = MACH_MEM1_SIZE - SYS_MEM1_SIZE,
	.type    = MT_DEVICE 
},
};

static void __init aquila_iomap_init(void)
{
	iotable_init(aquila_io_desc, ARRAY_SIZE(aquila_io_desc));
}

static void __init aquila_map_io(void)
{
	//initialize iomap of special function register address
	sdp92_iomap_init();

	//initialize iomap for peripheral device
	aquila_iomap_init();
}

static void __init aquila_init(void)
{
	sdp92_init();
}

static void __init 
aquila_fixup(struct machine_desc *mdesc, struct tag *tags, char ** cmdline, struct meminfo *meminfo)
{
#if defined(CONFIG_DISCONTIGMEM)
	meminfo->nr_banks = 2;

	meminfo->bank[0].start = MACH_MEM0_BASE;
	meminfo->bank[0].size =  sdp_sys_mem0_size;
	meminfo->bank[0].node =  0;

	meminfo->bank[1].start = MACH_MEM1_BASE;
	meminfo->bank[1].size =  SYS_MEM1_SIZE;
	meminfo->bank[1].node =  1;
#elif defined(CONFIG_SDP_SINGLE_DDR_A)
	meminfo->nr_banks = 1;
	meminfo->bank[0].start = MACH_MEM0_BASE;
	meminfo->bank[0].size =  sdp_sys_mem0_size;
	meminfo->bank[0].node =  0;
#elif defined(CONFIG_SDP_SINGLE_DDR_B)
	meminfo->nr_banks = 1;
	meminfo->bank[0].start = MACH_MEM1_BASE;
	meminfo->bank[0].size =  SYS_MEM1_SIZE;
	meminfo->bank[0].node =  0;
#endif
}

MACHINE_START(SDP92_EVAL_AQUILA, "Samsung SDP92 Evaluation board")
	.phys_io  = PA_IO_BASE0,
	.io_pg_offst = VA_IO_BASE0,
	.boot_params  = (PHYS_OFFSET + 0x100),
	.map_io		= aquila_map_io,
	.init_irq	= sdp_init_irq,
	.timer		= &sdp_timer,
	.init_machine	= aquila_init,
	.fixup		= aquila_fixup,
MACHINE_END

