/*
 * linux/arch/arm/mach-ssdtv/mach-ssa1fpga.c
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
extern void ssa1fpga_init(void);
extern void ssa1fpga_iomap_init(void);

static struct map_desc ssa1fpga_board_io_desc[] __initdata = {
 { 
	.virtual = 0xE0000000,
	.pfn     = __phys_to_pfn(0x60000000),  // ddrw
	.length  = SZ_128M,
	.type    = MT_DEVICE 
 },
 { 
	.virtual = 0xE8000000,
	.pfn     = __phys_to_pfn(0xD3100000),  // ddre
	.length  = (SZ_128M - 0x03100000),
	.type    = MT_DEVICE 
 },
 { 
	.virtual = 0xF0000000,
	.pfn     = __phys_to_pfn(0x50000000),
	.length  = SZ_128M,
	.type    = MT_DEVICE 
 },
 { 
	.virtual = 0xF8000000,
	.pfn     = __phys_to_pfn(0x5C000000),
	.length  = SZ_32M,
	.type    = MT_DEVICE 
 },
};

static void __init ssa1fpga_board_iomap_init(void)
{
	iotable_init(ssa1fpga_board_io_desc, ARRAY_SIZE(ssa1fpga_board_io_desc));
}

static void __init ssa1fpga_board_map_io(void)
{
	//initialize iomap of special function register address
	ssa1fpga_iomap_init();

	//initialize iomap for peripheral device
	ssa1fpga_board_iomap_init();
}

static void __init ssa1fpga_board_init(void)
{
	ssa1fpga_init();

}

static void __init 
ssa1fpga_board_fixup(struct machine_desc *mdesc, struct tag *tags, char ** cmdline, struct meminfo *meminfo)
{
	meminfo->nr_banks = 1;
	meminfo->bank[0].start = PHYS_OFFSET;
	meminfo->bank[0].size = MEM_SIZE;
}

MACHINE_START(SSA1FPGA_BOARD, "Samsung-A1 FPGA Board")
	.phys_io  = PA_IO_BASE0,
	.io_pg_offst = VA_IO_BASE0,
	.boot_params  = (PA_SYSTEM_BASE + 0x100),
	.map_io		= ssa1fpga_board_map_io,
	.init_irq	= ssdtv_init_irq,
	.timer		= &ssdtv_timer,
	.init_machine	= ssa1fpga_board_init,
	.fixup		= ssa1fpga_board_fixup,
MACHINE_END

