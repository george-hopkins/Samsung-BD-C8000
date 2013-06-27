/*
 * linux/arch/arm/mach-ssdtv/ssa1fpga_clock.c
 *
 * Copyright (C) 2006 Samsung Electronics.co
 * Author : tukho.kim@samsung.com
 *
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/list.h>
#include <linux/errno.h>
#include <linux/err.h>

#include <asm/semaphore.h>
#include <asm/hardware.h>

#include <asm/arch/platform.h>

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

};

struct clk*
clk_get(struct device *dev, const char *id)
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

void 	clk_put(struct clk *clk)
{
	module_put(clk->owner); 
}

int 	clk_enable(struct clk *clk)
{

	return 0;

}

void 	clk_disable(struct clk *clk)
{

}

int 	clk_use(struct clk *clk)
{
	atomic_inc(&clk->used);
	return 0;
}

void 	clk_unuse(struct clk *clk)
{
	if(clk->used.counter > 0) atomic_dec(&clk->used); 
}

unsigned long clk_get_rate(struct clk *clk)
{
	return (clk->rate) ? clk->rate : clk->source->rate;
}

long 	clk_round_rate(struct clk *clk, unsigned long rate)
{
	return rate;
}

int 	clk_set_rate(struct clk *clk, unsigned long rate)
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


unsigned long ssa1fpga_get_clock (char mode)
{
	switch(mode){
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

static void ssa1fpga_init_clock(void)
{
#if 0
	unsigned long pllVal = rPLLCON;
	unsigned long clkModVal = rCLKMOD;
#endif
	unsigned long srcClk;
	
	fclk.rate = A1FPGA_CLOCK;  // 6.75Mhz
	dclk.rate = A1FPGA_CLOCK;  // 6.75Mhz
	hclk.rate = A1FPGA_CLOCK;  // 6.75Mhz
	pclk.rate = A1FPGA_CLOCK;  // 6.75Mhz

	printk(KERN_INFO "A1 Core Clock: %d.%dMhz\n", 
			(unsigned int)fclk.rate/MHZ, (unsigned int)fclk.rate%MHZ/1000);
	printk(KERN_INFO "A1 DDR Clock: %d.%dMhz\n", 
			(unsigned int)dclk.rate/MHZ, (unsigned int)dclk.rate%MHZ/1000);
	printk(KERN_INFO "A1 Host Clock: %d.%dMhz\n", 
			(unsigned int)hclk.rate/MHZ, (unsigned int)hclk.rate%MHZ/1000);
	printk(KERN_INFO "A1 PCLK: %d.%dMhz\n", 
			(unsigned int)pclk.rate/MHZ, (unsigned int)pclk.rate%MHZ/1000);
}

int __init clk_init(void)
{

	struct clk* pClk = init_clocks;
	int ptr;
	int ret = 0;

	printk(KERN_INFO "ssa1fpga clock control (c) 2006 Samsung Electronics\n");
	ssa1fpga_init_clock();

	clk_register(&fclk);
	clk_register(&dclk);
	clk_register(&hclk);
	clk_register(&pclk);

	for(ptr = 0; ptr < ARRAY_SIZE(init_clocks);ptr++, pClk++)
		clk_register(pClk);

	return ret;
}

arch_initcall(clk_init);

EXPORT_SYMBOL(clk_init);
EXPORT_SYMBOL(clk_get_rate);
EXPORT_SYMBOL(clk_unuse);
EXPORT_SYMBOL(clk_use);
EXPORT_SYMBOL(clk_disable);
EXPORT_SYMBOL(clk_enable);
EXPORT_SYMBOL(clk_put);
EXPORT_SYMBOL(clk_get);
EXPORT_SYMBOL(clk_round_rate);
EXPORT_SYMBOL(clk_set_rate);
EXPORT_SYMBOL(clk_register);
EXPORT_SYMBOL(ssa1fpga_get_clock); 
