/*
 * linux/arch/arm/mach-ssdtv/sdp78fpga_clock.c
 *
 * Copyright (C) 2006 Samsung Electronics.co
 * Author : seongkoo.cheong@samsung.com
 *
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/list.h>
#include <linux/errno.h>
#include <linux/err.h>
#include <linux/device.h>

#include <asm/semaphore.h>
#include <asm/hardware.h>

struct module;

struct clk {
	struct list_head	list;
	struct module*		owner;
	struct clk*		source;
	const char*		name;

	atomic_t		used;
	unsigned long		rate;
	int			ctrlbit;
};

static LIST_HEAD(clocks);
static DECLARE_MUTEX(clocks_sem);

/* Linux kernel -2.6 clock API */
static struct clk fclk = {
	.source = NULL,
	.name	= "FCLK",
	.rate	= 0,
	.ctrlbit = -1,
};

static struct clk dclk = {
	.source = NULL,
	.name	= "DCLK",
	.rate	= 0,
	.ctrlbit = -1,
};

static struct clk hclk = {
	.source = NULL,
	.name	= "HCLK",
	.rate	= 0,
	.ctrlbit = -1,
};

static struct clk pclk = {
	.source = NULL,
	.name	= "PCLK",
	.rate	= 0,
	.ctrlbit = -1,
};

static struct clk init_clocks[]={
	/* HCLK based */
	{ .source = &hclk,
	  .name   = "smc",
	  .rate   = 0,
	  .ctrlbit = CLK_SMC,
	},

	{ .source = &hclk,
	  .name   = "dma",
	  .rate   = 0,
	  .ctrlbit = CLK_DMA,
	},

	{ .source = &hclk,
	  .name   = "usb",
	  .rate   = 0,
	  .ctrlbit = CLK_USB,
	},

	{ .source = &hclk,
	  .name   = "pci",
	  .rate   = 0,
	  .ctrlbit = CLK_PCI,
	},

	/* PCLK based */
	{ .source = &pclk,
	  .name   = "i2c",
	  .rate   = 0,
	  .ctrlbit = CLK_I2C,
	},

	{ .source = &pclk,
	  .name   = "ssp",
	  .rate   = 0,
	  .ctrlbit = CLK_SSP,
	},

	{ .source = &pclk,
	  .name   = "irr",
	  .rate   = 0,
	  .ctrlbit = CLK_IRR,
	},

	{ .source = &pclk,
	  .name   = "timer",
	  .rate   = 0,
	  .ctrlbit = CLK_TIMER,
	},

	{ .source = &pclk,
	  .name   = "rtc",
	  .rate   = 0,
	  .ctrlbit = CLK_RTC,
	},

	{ .source = &pclk,
	  .name   = "wdt",
	  .rate   = 0,
	  .ctrlbit = CLK_WDT,
	},

	{ .source = &pclk,
	  .name   = "idle",
	  .rate   = 0,
	  .ctrlbit = CLK_IDLE,
	},

	{ .source = &pclk,
	  .name   = "uart",
	  .rate   = 0,
	  .ctrlbit = CLK_UART,
	},

	{ .source = &pclk,
	  .name   = "intc",
	  .rate   = 0,
	  .ctrlbit = CLK_INTC,
	},

	{ .source = &pclk,
	  .name   = "irb",
	  .rate   = 0,
	  .ctrlbit = CLK_IRB,
	},
};

struct clk* clk_get(struct device *dev, const char *id)
{
        struct clk *p, *pclk = ERR_PTR(-ENOENT);

        down(&clocks_sem);
        list_for_each_entry(p, &clocks, list) {
                if (strcmp(id, p->name) == 0 && try_module_get(p->owner)) {
                        pclk = p;
                        break;
                }
        }
        up(&clocks_sem);

        return pclk;
}

void clk_put(struct clk *clk)
{
	module_put(clk->owner); 
}

int clk_enable(struct clk *clk)
{
	if (clk->ctrlbit > 0) {
		if (clk->source == &hclk)
			rHCLKCON |= (1 << clk->ctrlbit);
		else if (clk->source == &pclk)
			rPCLKCON |= (1 << clk->ctrlbit);
	}

	return 0;

}

void clk_disable(struct clk *clk)
{
	if(clk->ctrlbit > 0){
		if (clk->source == &hclk)
			rHCLKCON &= ~(1 << clk->ctrlbit);
		else if (clk->source == &pclk)
			rPCLKCON &= ~(1 << clk->ctrlbit);
	}
}

int clk_use(struct clk *clk)
{
	atomic_inc(&clk->used);
	return 0;
}

void clk_unuse(struct clk *clk)
{
	if(clk->used.counter > 0) atomic_dec(&clk->used); 
}

unsigned long clk_get_rate(struct clk *clk)
{
	return (clk->rate) ? clk->rate : clk->source->rate;
}

long clk_round_rate(struct clk *clk, unsigned long rate)
{
	return rate;
}

int clk_set_rate(struct clk *clk, unsigned long rate)
{
	printk(KERN_WARNING "Can't support to chagne rate\n");
	return 0;
}

int clk_register(struct clk* clk)
{
	clk->owner = THIS_MODULE;
	atomic_set(&clk->used, 0);

	down(&clocks_sem);
	list_add(&clk->list, &clocks);
	up(&clocks_sem);

	return 0;
}


unsigned long sdp78fpga_get_clock (char mode)
{
	switch (mode) {
	case (REQ_DCLK):
		return dclk.rate;
		break;
	case (REQ_HCLK):
		return hclk.rate;
		break;	
	case (REQ_PCLK):
		return pclk.rate;
		break;
	case (REQ_FCLK):
	default:
		return fclk.rate;
		break;
	}
}

#define GET_ARM_PLL_P(x)			((x>>3)&0x3F)
#define GET_ARM_PLL_M(x)			((x>>9)&0xFF)
#define GET_ARM_PLL_S(x)			((x>>0)&0x7)

#define GET_DDR_PLL_P(x)			((x>>3)& 0x3F)
#define GET_DDR_PLL_M(x)			((x>>9) & 0x1FF)
#define GET_DDR_PLL_S(x)			((x>>0) & 0x7)


static __init void sdp78fpga_init_clock(void)
{
	fclk.rate=CONFIG_SYSTEM_CLOCK;
	dclk.rate=CONFIG_SYSTEM_CLOCK;
	hclk.rate=CONFIG_SYSTEM_CLOCK;
	pclk.rate=CONFIG_SYSTEM_CLOCK;
	printk (KERN_INFO "SDP78 FPGA FCLK: %u.%uMhz\n",
			(unsigned int)(fclk.rate/MHZ),
			(unsigned int)(fclk.rate%MHZ/1000));

	printk (KERN_INFO "SDP78 FPGA DCLK: %u.%uMhz\n",
			(unsigned int)(dclk.rate/MHZ),
			(unsigned int)(dclk.rate%MHZ/1000));

	printk (KERN_INFO "SDP78 FPGA HCLK: %u.%uMhz\n",
			(unsigned int)(hclk.rate/MHZ),
			(unsigned int)(hclk.rate%MHZ/1000));

	printk (KERN_INFO "SDP78 FPGA PCLK: %u.%uMhz\n",
			(unsigned int)(pclk.rate/MHZ),
			(unsigned int)(pclk.rate%MHZ/1000));
}

int __init clk_init(void)
{
	int i;

	printk (KERN_INFO "S4LJ003X Clock control (c) 2007 Samsung Electronics\n");
	sdp78fpga_init_clock();

	clk_register(&fclk);
	clk_register(&dclk);
	clk_register(&hclk);
	clk_register(&pclk);

	for (i=0; i<ARRAY_SIZE(init_clocks); i++) {
		clk_register (init_clocks + i);
	}

	return 0;
}

EXPORT_SYMBOL(clk_init);
EXPORT_SYMBOL(clk_get);
EXPORT_SYMBOL(clk_enable);
EXPORT_SYMBOL(clk_disable);
EXPORT_SYMBOL(clk_use);
EXPORT_SYMBOL(clk_unuse);
EXPORT_SYMBOL(clk_get_rate);
EXPORT_SYMBOL(clk_put);
EXPORT_SYMBOL(clk_round_rate);
EXPORT_SYMBOL(clk_set_rate);
EXPORT_SYMBOL(clk_register);
EXPORT_SYMBOL(sdp78fpga_get_clock); 
