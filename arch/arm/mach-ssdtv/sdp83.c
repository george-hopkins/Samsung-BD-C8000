/*
 * linux/arch/arm/mach-ssdtv/sdp83.c
 *
 * Copyright (C) 2008 Samsung Electronics.co
 * Author : tukho.kim@samsung.com	06/19/2008
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

#include <asm/arch/sdp83.h>

static struct map_desc sdp83_io_desc[] __initdata = {
/* ------------------------------------------- */
/* ------ special function register ---------- */
/* ------------------------------------------- */
// 0x3000_0000 ~ 0x3000_FFFF, 64KByte
 { 
	.virtual = 0xf8000000,
	.pfn     = __phys_to_pfn(0x30000000),
	.length  = SZ_8M + SZ_4M,
	.type    = MT_DEVICE 
 },
// 0x3001_0000 ~ 0x3001_FFFF, 64KByte Don't access this region
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
                .end    = PA_EHCI0_BASE + 0x100,
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
                .end    = PA_EHCI1_BASE + 0x100,
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
                .end    = PA_OHCI0_BASE + 0x100,
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
                .end    = PA_OHCI1_BASE + 0x100,
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

static struct platform_device* sdp83_init_devs[] __initdata = {
	&ssdtv_uart0,
	&ssdtv_uart1,
	&ssdtv_uart2,
       	&ssdtv_ehci0,
        &ssdtv_ohci0,
        &ssdtv_ehci1,
        &ssdtv_ohci1,
};

void __init sdp83_iomap_init (void)
{
	iotable_init(sdp83_io_desc, ARRAY_SIZE(sdp83_io_desc));
}

void __init sdp83_init(void)
{
#if defined(CONFIG_SDP83_GPIO)
	sdp83_gpio_init();
#endif
	platform_add_devices(sdp83_init_devs, ARRAY_SIZE(sdp83_init_devs));
}

EXPORT_SYMBOL(sdp83_iomap_init);
EXPORT_SYMBOL(sdp83_init);

