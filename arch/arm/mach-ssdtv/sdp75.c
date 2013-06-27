/*
 * linux/arch/arm/mach-ssdtv/sdp75.c
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

#include <asm/arch/sdp75.h>

static struct map_desc sdp75_io_desc[] __initdata = {
/* ------------------------------------------- */
/* ------ special function register ---------- */
/* ------------------------------------------- */
// 0x3000_0000 ~ 0x3000_FFFF, 64KByte
 { 
	.virtual = 0xD0000000,
	.pfn     = __phys_to_pfn(0x30000000),
	.length  = SZ_64K,
	.type    = MT_DEVICE 
 },
// 0x3001_0000 ~ 0x3001_FFFF, 64KByte Don't access this region
// 0x3002_0000 ~ 0x3005_FFFF, 256KByte
 { 
	.virtual = 0xD0020000,
	.pfn     = __phys_to_pfn(0x30020000),
	.length  = SZ_256K,
	.type    = MT_DEVICE 
 },
// 0x3006_0000 ~ 0x3006_FFFF, 64KByte Don't access this region
// 0x3007_0000 ~ 0x3009_FFFF, 192KByte
 { 
	.virtual = 0xD0070000,
	.pfn     = __phys_to_pfn(0x30070000),
	.length  = SZ_128K + SZ_64K,
	.type    = MT_DEVICE 
 },
// 0x300A_0000 ~ 0x300D_FFFF, 256KByte Don't access this region
// 0x300E_0000 ~ 0x300E_FFFF, 64KByte
 { 
	.virtual = 0xD00E0000,
	.pfn     = __phys_to_pfn(0x300E0000),
	.length  = SZ_64K,
	.type    = MT_DEVICE 
 },
// 0x300F_0000 ~ 0x300F_FFFF, 64KByte Don't access this region
// 0x3010_0000 ~ 0x307F_FFFF, 7MByte
 { 
	.virtual = 0xD0100000,
	.pfn     = __phys_to_pfn(0x30100000),
	.length  = SZ_4M + SZ_2M + SZ_1M,
	.type    = MT_DEVICE 
 },

 { 
	/* SELP.arm.3.x support A1 2007-10-22 */
	.virtual = 0xD0804000,		// ddr west control register
	.pfn     = __phys_to_pfn(0xB0404000),
	.length  = SZ_4K,
	.type    = MT_DEVICE 
 },

 { 
	.virtual = 0xD0940000,		// mpeg, disp, bgp	
	.pfn     = __phys_to_pfn(0xB0140000),
	.length  = (SZ_4M - 0x00140000),
	.type    = MT_DEVICE 
 },
// 0xE000_0000 ~ 0xE00A_FFFF, 704KByte
 {
   	.virtual = 0xD0C00000,		// divx
   	.pfn     = __phys_to_pfn(0xE0000000),
   	.length  = SZ_512K + SZ_128K + SZ_64K,
   	.type    = MT_DEVICE
 },
// 0xE00B_0000 ~ 0xE00B_FFFF, 64KByte Don't access this region
// 0xE00C_0000 ~ 0xE00F_FFFF, 256KByte
 {
   	.virtual = 0xD0CC0000,		// divx
   	.pfn     = __phys_to_pfn(0xE00C0000),
   	.length  = SZ_256K,
   	.type    = MT_DEVICE
 },
 { 
	.virtual = 0xD0E00000,		// mpeg1, h.264, divx
	.pfn     = __phys_to_pfn(0xB0600000),
	.length  = SZ_1M,
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

/* EHCI host controller */
static struct resource ssdtv_ehci0_resource[] = {
        [0] = {
                .start  = PA_EHCI0_BASE,
                .end    = PA_EHCI0_BASE + 0x5B,
                .flags  = IORESOURCE_MEM,
        },
        [1] = {
                .start  = IRQ_USB_EHCI0,
                .end    = IRQ_USB_EHCI0,
                .flags  = IORESOURCE_IRQ,
        },
};

static struct resource ssdtv_ehci1_resource[] = {
        [0] = {
                .start  = PA_EHCI1_BASE,
                .end    = PA_EHCI1_BASE + 0x5B,
                .flags  = IORESOURCE_MEM,
        },
        [1] = {
                .start  = IRQ_USB_EHCI1,
                .end    = IRQ_USB_EHCI1,
                .flags  = IORESOURCE_IRQ,
        },
};

/* USB 2.0 companion OHCI */
static struct resource ssdtv_ohci0_resource[] = {
        [0] = {
                .start  = PA_OHCI0_BASE,
                .end    = PA_OHCI0_BASE + 0x5B,
                .flags  = IORESOURCE_MEM,
        },
        [1] = {
                .start  = IRQ_USB_OHCI0,
                .end    = IRQ_USB_OHCI0,
                .flags  = IORESOURCE_IRQ,
        },
};

static struct resource ssdtv_ohci1_resource[] = {
        [0] = {
                .start  = PA_OHCI1_BASE,
                .end    = PA_OHCI1_BASE + 0x5B,
                .flags  = IORESOURCE_MEM,
        },
        [1] = {
                .start  = IRQ_USB_OHCI1,
                .end    = IRQ_USB_OHCI1,
                .flags  = IORESOURCE_IRQ,
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
static u64 ssdtv_ehci0_dmamask = (u32)0xFFFFFFFFUL;
static u64 ssdtv_ehci1_dmamask = (u32)0xFFFFFFFFUL;
static u64 ssdtv_ohci0_dmamask = (u32)0xFFFFFFFFUL;
static u64 ssdtv_ohci1_dmamask = (u32)0xFFFFFFFFUL;

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

/* USB Host controllers */
static struct platform_device ssdtv_ehci0 = {
        .name           = "ehci-s5h2x",
        .id             = 0,
        .dev = {
                .dma_mask               = &ssdtv_ehci0_dmamask,
                .coherent_dma_mask      = 0xFFFFFFFFUL,
        },
        .num_resources  = ARRAY_SIZE(ssdtv_ehci0_resource),
        .resource       = ssdtv_ehci0_resource,
};

static struct platform_device ssdtv_ehci1 = {
        .name           = "ehci-s5h2x",
        .id             = 1,
        .dev = {
                .dma_mask               = &ssdtv_ehci1_dmamask,
                .coherent_dma_mask      = 0xFFFFFFFFUL,
        },
        .num_resources  = ARRAY_SIZE(ssdtv_ehci1_resource),
        .resource       = ssdtv_ehci1_resource,
};

static struct platform_device ssdtv_ohci0 = {
        .name           = "ohci-s5h2x",
        .id             = 0,
        .dev = {
                .dma_mask               = &ssdtv_ohci0_dmamask,
                .coherent_dma_mask      = 0xFFFFFFFFUL,
        },
        .num_resources  = ARRAY_SIZE(ssdtv_ohci0_resource),
        .resource       = ssdtv_ohci0_resource,
};

static struct platform_device ssdtv_ohci1 = {
        .name           = "ohci-s5h2x",
        .id             = 1,
        .dev = {
                .dma_mask               = &ssdtv_ohci1_dmamask,
                .coherent_dma_mask      = 0xFFFFFFFFUL,
        },
        .num_resources  = ARRAY_SIZE(ssdtv_ohci1_resource),
        .resource       = ssdtv_ohci1_resource,
};

static struct platform_device* sdp75_init_devs[] __initdata = {
	&ssdtv_uart0,
	&ssdtv_uart1,
	&ssdtv_uart2,
       	&ssdtv_ehci0,
        &ssdtv_ehci1,
        &ssdtv_ohci0,
        &ssdtv_ohci1,
};

void __init sdp75_iomap_init (void)
{
	iotable_init(sdp75_io_desc, ARRAY_SIZE(sdp75_io_desc));
}

void __init sdp75_init(void)
{
	platform_add_devices(sdp75_init_devs, ARRAY_SIZE(sdp75_init_devs));
}

EXPORT_SYMBOL(sdp75_iomap_init);
EXPORT_SYMBOL(sdp75_init);

