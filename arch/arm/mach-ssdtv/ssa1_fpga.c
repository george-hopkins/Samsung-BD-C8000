/*
 * linux/arch/arm/mach-ssdtv/ssa1fpga.c
 *
 * Copyright (C) 2006 Samsung Electronics.co
 * Author : tukho.kim@samsung.com
 *
 */

#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/interrupt.h>
#include <linux/list.h>
#include <linux/timer.h>
#include <linux/init.h>
#include <linux/device.h>
#include <linux/platform_device.h>

#include <asm/mach/arch.h>
#include <asm/mach/map.h>
#include <asm/mach/irq.h>

#include <asm/hardware.h>
#include <asm/io.h>
#include <asm/irq.h>

#include <asm/arch/ssa1_fpga.h>

static struct map_desc ssa1fpga_io_desc[] __initdata = {
/*  virtual address,	physical address,   	size,      type*/
// special function register
 { 
	.virtual = 0xD0000000,
	.pfn     = __phys_to_pfn(0x30000000),
	.length  = SZ_8M,
	.type    = MT_DEVICE 
 },

 { 
	.virtual = 0xD0940000,
	.pfn     = __phys_to_pfn(0xB0140000),
	.length  = (SZ_8M - 0x00140000),
	.type    = MT_DEVICE 
 },

 { 
	.virtual = 0xD0804000,
	.pfn     = __phys_to_pfn(0xB0C04000),
	.length  = SZ_4K,
	.type    = MT_DEVICE 
 },

};

/**** resource
struct resource {
        const char *name;
        unsigned long start, end;
        unsigned long flags;
        struct resource *parent, *sibling, *child;
};
*/

static struct resource ssdtv_uart0_resource[] = {
	[0] = {
		.start 	= PA_UART_BASE,
		.end	= PA_UART_BASE + 0x30,
		.flags	= IORESOURCE_MEM,
	},
	[1] = {
		.start 	= IRQ_UART0,
		.end	= IRQ_UART0,
		.flags	= IORESOURCE_IRQ,
	},
};

static struct resource ssdtv_uart1_resource[] = {
	[0] = {
		.start 	= PA_UART_BASE + 0x40,
		.end	= PA_UART_BASE + 0x40 + 0x30,
		.flags	= IORESOURCE_MEM,
	},
	[1] = {
		.start 	= IRQ_UART1,
		.end	= IRQ_UART1,
		.flags	= IORESOURCE_IRQ,
	},
};

static struct resource ssdtv_uart2_resource[] = {
	[0] = {
		.start 	= PA_UART_BASE + 0x80,
		.end	= PA_UART_BASE + 0x80 + 0x30,
		.flags	= IORESOURCE_MEM,
	},
	[1] = {
		.start 	= IRQ_UART2,
		.end	= IRQ_UART2,
		.flags	= IORESOURCE_IRQ,
	},
};


/**** platform_device 
struct platform_device {
        char            * name;
         u32             id;
         struct device   dev;
         u32             num_resources;
         struct resource * resource;
};
*/
static struct platform_device ssdtv_uart0 = {
	.name		= "s5h2x-uart",
	.id		= 0,
	.num_resources	= ARRAY_SIZE(ssdtv_uart0_resource),
	.resource	= ssdtv_uart0_resource,
};

static struct platform_device ssdtv_uart1 = {
	.name		= "s5h2x-uart",
	.id		= 1,
	.num_resources	= ARRAY_SIZE(ssdtv_uart1_resource),
	.resource	= ssdtv_uart1_resource,
};

static struct platform_device ssdtv_uart2 = {
	.name		= "s5h2x-uart",
	.id		= 2,
	.num_resources	= ARRAY_SIZE(ssdtv_uart2_resource),
	.resource	= ssdtv_uart2_resource,
};

static struct platform_device* ssa1fpga_init_devs[] __initdata = {
	&ssdtv_uart0,
	&ssdtv_uart1,
	&ssdtv_uart2,
};

void __init ssa1fpga_iomap_init (void)
{
	iotable_init(ssa1fpga_io_desc, ARRAY_SIZE(ssa1fpga_io_desc));
}

void __init ssa1fpga_init(void)
{
	platform_add_devices(ssa1fpga_init_devs, ARRAY_SIZE(ssa1fpga_init_devs));
}

EXPORT_SYMBOL(ssa1fpga_iomap_init);
EXPORT_SYMBOL(ssa1fpga_init);

