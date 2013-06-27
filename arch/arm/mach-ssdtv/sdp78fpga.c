/*
 * linux/arch/arm/mach-ssdtv/mach-sdp78fpga.c
 *
 * Copyright (C) 2006 Samsung Electronics.co
 * Author : seongkoo.cheong@samsung.com
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

#include <asm/arch/sdp78fpga.h>
#include <asm/arch/irqs.h>

static struct map_desc sdp78fpga_io_desc[] __initdata = {
	{ 0xD0000000, __phys_to_pfn(0x30000000), 0x10000000, MT_DEVICE },
#ifdef CONFIG_PCI
	/* PCI Configurations type 0 & 1 */
	{ 0xF0000000, __phys_to_pfn(0x50000000), 0x08000000, MT_DEVICE },
	/* PCI I/O Cycle */
	{ 0xF8000000, __phys_to_pfn(0x5C000000), 0x03000000, MT_DEVICE },
#endif
};

/* resources */
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

/* USB HCs resources */
static u64 ssdtv_ohci0_dmamask = (u32)0xFFFFFFFFUL;
static u64 ssdtv_ehci0_dmamask = (u32)0xFFFFFFFFUL;
static u64 ssdtv_ohci1_dmamask = (u32)0xFFFFFFFFUL;

/* standalone OHCI HC */
static struct resource ssdtv_ohci1_resource[] = {
	[0] = {
		.start	= PA_OHCI1_BASE,
		.end	= PA_OHCI1_BASE + 0x5B,
		.flags	= IORESOURCE_MEM,
	},
	[1] = {
		.start	= IRQ_USB_OHCI1,
		.end	= IRQ_USB_OHCI1,
		.flags	= IORESOURCE_IRQ,
	},
};

/* USB 2.0 EHCI HC */
static struct resource ssdtv_ehci0_resource[] = {
	[0] = {
		.start	= PA_EHCI0_BASE,
		.end	= PA_EHCI0_BASE + 0x5B,
		.flags	= IORESOURCE_MEM,
	},
	[1] = {
		.start	= IRQ_USB_EHCI0,
		.end	= IRQ_USB_EHCI0,
		.flags	= IORESOURCE_IRQ,
	},
};

/* USB 2.0 companion OHCI HC */
static struct resource ssdtv_ohci0_resource[] = {
	[0] = {
		.start	= PA_OHCI0_BASE,
		.end	= PA_OHCI0_BASE + 0x5B,
		.flags	= IORESOURCE_MEM,
	},
	[1] = {
		.start	= IRQ_USB_OHCI0,
		.end	= IRQ_USB_OHCI0,
		.flags	= IORESOURCE_IRQ,
	},
};

static struct platform_device ssdtv_uart0 = {
	.name		= "ssdtv-uart",
	.id		= 0,
	.num_resources	= ARRAY_SIZE(ssdtv_uart0_resource),
	.resource	= ssdtv_uart0_resource,
};

static struct platform_device ssdtv_uart1 = {
	.name		= "ssdtv-uart",
	.id		= 1,
	.num_resources	= ARRAY_SIZE(ssdtv_uart1_resource),
	.resource	= ssdtv_uart1_resource,
};

static struct platform_device ssdtv_uart2 = {
	.name		= "ssdtv-uart",
	.id		= 2,
	.num_resources	= ARRAY_SIZE(ssdtv_uart2_resource),
	.resource	= ssdtv_uart2_resource,
};

/* USB HCD platform_devices */
/* EHCI0 */
static struct platform_device ssdtv_ehci0= {
	.name		= "ehci-s5h2x",
	.id		= 0,
	.dev = {
		.dma_mask		= &ssdtv_ehci0_dmamask,
		.coherent_dma_mask	= 0xFFFFFFFFUL,
	},
	.num_resources	= ARRAY_SIZE(ssdtv_ehci0_resource),
	.resource	= ssdtv_ehci0_resource,
};

/* OHCI0 */
static struct platform_device ssdtv_ohci0= {
	.name		= "ohci-s5h2x",
	.id		= 0,
	.dev = {
		.dma_mask		= &ssdtv_ohci0_dmamask,
		.coherent_dma_mask	= 0xFFFFFFFFUL,
	},
	.num_resources	= ARRAY_SIZE(ssdtv_ohci0_resource),
	.resource	= ssdtv_ohci0_resource,
};

/* OHCI1 */
static struct platform_device ssdtv_ohci1= {
	.name		= "ohci-s5h2x",
	.id		= 1,
	.dev = {
		.dma_mask		= &ssdtv_ohci1_dmamask,
		.coherent_dma_mask	= 0xFFFFFFFFUL,
	},
	.num_resources	= ARRAY_SIZE(ssdtv_ohci1_resource),
	.resource	= ssdtv_ohci1_resource,
};

static struct platform_device* sdp78fpga_init_devs[] __initdata = {
	&ssdtv_uart0,
	&ssdtv_uart1,
	&ssdtv_uart2,
	&ssdtv_ehci0,
	&ssdtv_ohci0,
/*	&ssdtv_ohci1,	*/
};

void __init sdp78fpga_iomap_init (void)
{
	iotable_init(sdp78fpga_io_desc, ARRAY_SIZE(sdp78fpga_io_desc));
}

void __init sdp78fpga_init(void)
{
	platform_add_devices(sdp78fpga_init_devs, ARRAY_SIZE(sdp78fpga_init_devs));
}

EXPORT_SYMBOL(sdp78fpga_iomap_init);
EXPORT_SYMBOL(sdp78fpga_init);
