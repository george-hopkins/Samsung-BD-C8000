/*
 * linux/arch/arm/mach-sdp/sdp93_clock.c
 *
 * Copyright (C) 2009 Samsung Electronics.co
 * Author : tukho.kim@samsung.com 	09/11/2009
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
#include <asm/arch/platform.h>

#if !defined(MHZ)
#define MHZ	1000000
#endif

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

static void sdp93_init_clock(void);
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

static struct clk dspclk = {
	.source 	= NULL,
	.name		= "DSPCLK",
	.rate		= 0,
	.ctrlbit	= -1,
};

static struct clk hclk = {
	.source = &dclk,
	.name	= "HCLK",
	.rate	= 0,
	.ctrlbit = -1,
};

static struct clk pclk = {
	.source = &dclk,
	.name	= "PCLK",
	.rate	= 0,
	.ctrlbit = -1,
};

static struct clk *init_clocks[]={
	&fclk,
	&dclk,
	&dspclk,
	&hclk,
	&pclk,
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


unsigned long sdp93_get_clock (char mode)
{
// 20080524 by tukho.kim 
// apply HRTIMER 
	if(!fclk.rate) sdp93_init_clock();

	switch(mode){
		case (REQ_DCLK):
			return dclk.rate;
			break;

		case (REQ_DSPCLK):
			return dspclk.rate;
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

static unsigned int sdp93_calc_cpu_pms(unsigned int pms)
{
	unsigned int ret;
	ret = (INPUT_FREQ >> (GET_PLL_S(pms)-1));
	ret *= GET_PLL_M(pms); 
	ret /= GET_PLL_P(pms);
	return ret;
}

static unsigned int sdp93_calc_ddr_pmsk(unsigned int pms, unsigned int k)
{
	unsigned int ret, tmp;
	// (M + (K/1024)) * FIN / P / 2^S
	tmp = (INPUT_FREQ >> GET_PLL_S(pms)) / GET_PLL_P(pms);
	ret = GET_PLL_M(pms) + (k >> 10);
	return tmp * ret;
}

static unsigned int sdp93_calc_dsp_pms(unsigned int pms)
{
	unsigned int ret;
	ret = (INPUT_FREQ >> GET_PLL_S(pms));
	ret *= GET_PLL_M(pms); 
	ret /= GET_PLL_P(pms);
	return ret;
}

static void sdp93_init_clock(void)
{
	unsigned int pms_core = R_PMU_PLL1_PMS_CON;
	unsigned int pms_ddr = R_PMU_PLL2_PMS_CON;
	unsigned int pms_dsp = R_PMU_PLL3_PMS_CON;
	unsigned int k_ddr = R_PMU_PLL2_K_CON;
	unsigned int bypass = R_PMU_PLL_BYPASS;
	unsigned int muxsel0 = R_PMU_MUX_SEL0;

// 20080524 by tukho.kim 
// apply HRTIMER 
	if(fclk.rate) return;

    	if(bypass & 2)
		fclk.rate = INPUT_FREQ;
	else {
		fclk.rate = sdp93_calc_cpu_pms(pms_core);
	}
	
    	if(bypass & 4) {
		dclk.rate = INPUT_FREQ;
	} else {
		k_ddr = PMU_PLL_K_VALUE(k_ddr);
		dclk.rate = sdp93_calc_ddr_pmsk(pms_ddr, k_ddr);
	}

	if (bypass & 8) {
		dspclk.rate = INPUT_FREQ;
	} else {
		dspclk.rate = sdp93_calc_dsp_pms(pms_dsp);
	}

	if (muxsel0 & (1<<22)) {
		/* AHB,APB = 1/4 of DSP PLL3 */
		hclk.rate = dspclk.rate >> 2;
		hclk.source = &dspclk;
		pclk.rate = dspclk.rate >> 2;
		pclk.source = &dspclk;
		
	} else {
		/* AHB,APB = 1/4 of DDR PLL2 */
		hclk.rate = dclk.rate >> 2;
		pclk.rate = dclk.rate >> 2;
	}
}

int __init clk_init(void)
{
	struct clk *clk;
	int i;

	printk(KERN_INFO "sdp93 clock control (c) 2008 Samsung Electronics\n");
	sdp93_init_clock();

	for(i= 0; i< ARRAY_SIZE(init_clocks);i++) {
		clk = init_clocks[i];
		clk_register(clk);
		printk (KERN_INFO "SDP93 %s Clock: %d.%03dMhz\n",
				clk->name,
				(unsigned int)clk->rate / MHZ,
				(unsigned int)clk->rate % MHZ / 1000);
	}

	return 0;
}

arch_initcall(clk_init);

#if 0
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
EXPORT_SYMBOL(sdp92_get_clock); 
#endif

