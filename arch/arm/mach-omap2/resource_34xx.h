/*
 * linux/arch/arm/mach-omap3/resource.h
 *
 * Copyright (C) 2006-2007 Texas Instruments, Inc.
 * Rajendra Nayak <rnayak@ti.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * THIS PACKAGE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED
 * WARRANTIES OF MERCHANTIBILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 *
 * History:
 *
 */

#ifndef __ARCH_ARM_MACH_OMAP3_RESOURCE_H
#define __ARCH_ARM_MACH_OMAP3_RESOURCE_H

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/device.h>
#include <asm/arch/resource.h>
#include <asm/arch/prcm.h>

/* Flags to denote Pool usage */
#define UNUSED			0x0
#define USED			0x1

#ifdef CONFIG_OMAP3430_ES2
#ifdef CONFIG_ARCH_OMAP3410
#define curr_arm_freq 	266
#define curr_dsp_freq 	266
#else
#define curr_arm_freq 	500
#define curr_dsp_freq 	360
#endif
#define curr_vdd1_opp	3
#define curr_vdd2_opp	3
#ifdef CONFIG_ARCH_OMAP3410
#define min_arm_freq 	66
#define max_arm_freq 	320
#define min_dsp_freq 	66
#define max_dsp_freq 	320
#else
#define min_arm_freq 	125
#define max_arm_freq 	600
#define min_dsp_freq 	90
#define max_dsp_freq 	430
#endif
#define min_vdd1_opp 	CO_VDD1_OPP1
#define max_vdd1_opp 	CO_VDD1_OPP5
#define min_vdd2_opp 	CO_VDD2_OPP2
#define max_vdd2_opp 	CO_VDD2_OPP3
#else
#define curr_arm_freq 	477
#define curr_dsp_freq 	313
#define curr_vdd1_opp	1
#define curr_vdd2_opp	1
#define min_arm_freq 	95
#define max_arm_freq 	477
#define min_dsp_freq 	62
#define max_dsp_freq 	313
#define min_vdd1_opp 	CO_VDD1_OPP1
#define max_vdd1_opp 	CO_VDD1_OPP4
#define min_vdd2_opp 	CO_VDD2_OPP1
#define max_vdd2_opp 	CO_VDD2_OPP2
#endif

extern u32 current_vdd1_opp;
extern u32 current_vdd2_opp;
extern struct clk *p_vdd1_clk;
extern struct clk *p_vdd2_clk;

extern unsigned int vdd1_opp_setting(u32 target_opp_no);
extern unsigned int vdd2_opp_setting(u32 target_opp_no);
extern int prcm_set_memory_resource_on_state(unsigned short state);
int set_memory_resource_state(unsigned short);
/* Activation/Validation functions for various shared resources */
int activate_power_res(struct shared_resource *resp,
		       unsigned short current_level,
		       unsigned short target_level);
int activate_logical_res(struct shared_resource *resp,
			 unsigned short current_level,
			 unsigned short target_level);
int activate_memory_res(unsigned long prcm_id, unsigned short current_level,
			unsigned short target_level);
int activate_dpll_res(unsigned long prcm_id, unsigned short current_level,
			unsigned short target_level);
int validate_power_res(struct shared_resource *res,
			unsigned short current_level,
			unsigned short target_level);
int validate_logical_res(struct shared_resource *res,
			unsigned short current_level,
			unsigned short target_level);
int validate_memory_res(struct shared_resource *res,
			unsigned short current_level,
			unsigned short target_level);
int validate_dpll_res(struct shared_resource *res, unsigned short current_level,
			unsigned short target_level);
int activate_constraint(struct shared_resource *resp,
			unsigned short current_value,
			unsigned short target_value);
int activate_pd_constraint(struct shared_resource *resp,
			   unsigned short current_value,
			   unsigned short target_value);
int validate_constraint(struct shared_resource *res,
			unsigned short current_level,
			unsigned short target_level);

int activate_memory_logic(struct shared_resource *resp,
			 unsigned short current_level,
			 unsigned short target_level);
int validate_memory_logic(struct shared_resource *resp,
			 unsigned short current_level,
			 unsigned short target_level);

int activate_autoidle_resource(struct shared_resource *resp,
			unsigned short current_level,
			unsigned short target_level);
int validate_autoidle_resource(struct shared_resource *resp,
			unsigned short current_level,
			unsigned short target_level);
static struct shared_resource dss = {
	.name = "dss",
	.prcm_id = DOM_DSS,
	.res_type = RES_POWER_DOMAIN,
	.no_of_users = 0,
	.curr_level = POWER_DOMAIN_OFF,
	.max_levels = POWER_DOMAIN_MAXLEVEL,
	.linked_res_num = 0,
	.action = activate_power_res,
	.validate = validate_power_res,
};

static struct shared_resource cam = {
	.name = "cam",
	.prcm_id = DOM_CAM,
	.res_type = RES_POWER_DOMAIN,
	.no_of_users = 0,
	.curr_level = POWER_DOMAIN_OFF,
	.max_levels = POWER_DOMAIN_MAXLEVEL,
	.linked_res_num = 0,
	.action = activate_power_res,
	.validate = validate_power_res,
};

static struct shared_resource iva2 = {
	.name = "iva2",
	.prcm_id = DOM_IVA2,
	.res_type = RES_POWER_DOMAIN,
	.no_of_users = 0,
	.curr_level = POWER_DOMAIN_OFF,
	.max_levels = POWER_DOMAIN_MAXLEVEL,
	.linked_res_num = 0,
	.action = activate_power_res,
	.validate = validate_power_res,
};

#ifndef CONFIG_ARCH_OMAP3410
#ifdef CONFIG_OMAP3430_ES2
static struct shared_resource sgx = {
	.name = "sgx",
	.prcm_id = DOM_SGX,
#else
static struct shared_resource gfx = {
	.name = "gfx",
	.prcm_id = DOM_GFX,
#endif
	.res_type = RES_POWER_DOMAIN,
	.no_of_users = 0,
	.curr_level = POWER_DOMAIN_OFF,
	.max_levels = POWER_DOMAIN_MAXLEVEL,
	.linked_res_num = 0,
	.action = activate_power_res,
	.validate = validate_power_res,
};
#endif

#ifdef CONFIG_OMAP3430_ES2
static struct shared_resource usbhost = {
	.name = "usb",
	.prcm_id = DOM_USBHOST,
	.res_type = RES_POWER_DOMAIN,
	.no_of_users = 0,
	.curr_level = POWER_DOMAIN_OFF,
	.max_levels = POWER_DOMAIN_MAXLEVEL,
	.linked_res_num = 0,
	.action = activate_power_res,
	.validate = validate_power_res,
};
#endif

static struct shared_resource per = {
	.name = "per",
	.prcm_id = DOM_PER,
	.res_type = RES_POWER_DOMAIN,
	.no_of_users = 0,
	.curr_level = POWER_DOMAIN_OFF,
	.max_levels = POWER_DOMAIN_MAXLEVEL,
	.linked_res_num = 0,
	.action = activate_power_res,
	.validate = validate_power_res,
};

#ifndef CONFIG_ARCH_OMAP3410
static struct shared_resource neon  = {
	.name = "neon",
	.prcm_id = DOM_NEON,
	.res_type = RES_POWER_DOMAIN,
	.no_of_users = 0,
	.curr_level = POWER_DOMAIN_OFF,
	.max_levels = POWER_DOMAIN_MAXLEVEL,
	.linked_res_num = 0,
	.action = activate_power_res,
	.validate = validate_power_res,
};
#endif

static struct shared_resource core = {
	.name = "core",
	.prcm_id = DOM_CORE1,
	.res_type = RES_LOGICAL,
	.no_of_users = 0,
	.curr_level = LOGICAL_UNUSED,
	.max_levels = LOGICAL_MAXLEVEL,
	.linked_res_num = 0,
	.action = activate_logical_res,
	.validate = validate_logical_res,
};

/* Constraint resources */
static struct shared_resource latency = {
	.name = "latency",
	.prcm_id = RES_LATENCY_CO,
	.res_type = RES_LATENCY_CO,
	.no_of_users = 0,
	.curr_level = CO_UNUSED,
	.max_levels = CO_MAXLEVEL,
	.linked_res_num = 0,
	.action = activate_constraint,
	.validate = validate_constraint,
};

static struct shared_resource arm_freq = {
	.name = "arm_freq",
	.prcm_id = PRCM_ARMFREQ_CONSTRAINT,
	.res_type = RES_FREQ_CO,
	.no_of_users = 0,
	.curr_level = curr_arm_freq,
	.max_levels = CO_MAXLEVEL,
	.linked_res_num = 0,
	.action = activate_constraint,
	.validate = validate_constraint,
};

static struct shared_resource dsp_freq = {
	.name = "dsp_freq",
	.prcm_id = PRCM_DSPFREQ_CONSTRAINT,
	.res_type = RES_FREQ_CO,
	.no_of_users = 0,
	.curr_level = curr_dsp_freq,
	.max_levels = CO_MAXLEVEL,
	.linked_res_num = 0,
	.action = activate_constraint,
	.validate = validate_constraint,
};

static struct shared_resource vdd1_opp = {
	.name = "vdd1_opp",
	.prcm_id = PRCM_VDD1_CONSTRAINT,
	.res_type = RES_OPP_CO,
	.no_of_users = 0,
	.curr_level = curr_vdd1_opp,
	.max_levels = max_vdd1_opp+1,
	.linked_res_num = 0,
	.action = activate_constraint,
	.validate = validate_constraint,
};

static struct shared_resource vdd2_opp = {
	.name = "vdd2_opp",
	.prcm_id = PRCM_VDD2_CONSTRAINT,
	.res_type = RES_OPP_CO,
	.no_of_users = 0,
	.curr_level = curr_vdd2_opp,
	.max_levels = max_vdd2_opp+1,
	.linked_res_num = 0,
	.action = activate_constraint,
	.validate = validate_constraint,
};

static char *lat_dss_linked_res[] = {"dss",};

static struct shared_resource lat_dss = {
	.name = "lat_dss",
	.prcm_id = PRCM_DSS_CONSTRAINT,
	.res_type = RES_CLOCK_RAMPUP_CO,
	.no_of_users = 0,
	.curr_level = CO_UNUSED,
	.max_levels = CO_MAXLEVEL,
	.linked_res_num = sizeof(lat_dss_linked_res) /
			  sizeof(lat_dss_linked_res[0]),
	.linked_res_names = lat_dss_linked_res,
	.action = activate_pd_constraint,
	.validate = validate_constraint,
};

static char *lat_cam_linked_res[] = {"cam",};

static struct shared_resource lat_cam = {
	.name = "lat_cam",
	.prcm_id = PRCM_CAM_CONSTRAINT,
	.res_type = RES_CLOCK_RAMPUP_CO,
	.no_of_users = 0,
	.curr_level = CO_UNUSED,
	.max_levels = CO_MAXLEVEL,
	.linked_res_num = sizeof(lat_cam_linked_res) /
			  sizeof(lat_cam_linked_res[0]),
	.linked_res_names = lat_cam_linked_res,
	.action = activate_pd_constraint,
	.validate = validate_constraint,
};

static char *lat_iva2_linked_res[] = {"iva2",};

static struct shared_resource lat_iva2 = {
	.name = "lat_iva2",
	.prcm_id = PRCM_IVA2_CONSTRAINT,
	.res_type = RES_CLOCK_RAMPUP_CO,
	.no_of_users = 0,
	.curr_level = CO_UNUSED,
	.max_levels = CO_MAXLEVEL,
	.linked_res_num = sizeof(lat_iva2_linked_res) /
			  sizeof(lat_iva2_linked_res[0]),
	.linked_res_names = lat_iva2_linked_res,
	.action = activate_pd_constraint,
	.validate = validate_constraint,
};

#ifndef CONFIG_ARCH_OMAP3410
#ifdef CONFIG_OMAP3430_ES2
#define ACCL_3D_NAME   "sgx"
#else
#define ACCL_3D_NAME    "gfx"
#endif

static char *lat_3d_linked_res[] = {ACCL_3D_NAME,};

static struct shared_resource lat_3d = {
	.name = "lat_3d",
	.prcm_id = PRCM_3D_CONSTRAINT,
	.res_type = RES_CLOCK_RAMPUP_CO,
	.no_of_users = 0,
	.curr_level = CO_UNUSED,
	.max_levels = CO_MAXLEVEL,
	.linked_res_num = sizeof(lat_3d_linked_res) /
			  sizeof(lat_3d_linked_res[0]),
	.linked_res_names = lat_3d_linked_res,
	.action = activate_pd_constraint,
	.validate = validate_constraint,
};
#endif

#ifdef CONFIG_OMAP3430_ES2
static char *lat_usbhost_linked_res[] = {"usb",};

static struct shared_resource lat_usbhost = {
	.name = "lat_usbhost",
	.prcm_id = PRCM_USBHOST_CONSTRAINT,
	.res_type = RES_CLOCK_RAMPUP_CO,
	.no_of_users = 0,
	.curr_level = CO_UNUSED,
	.max_levels = CO_MAXLEVEL,
	.linked_res_num = sizeof(lat_usbhost_linked_res) /
			  sizeof(lat_usbhost_linked_res[0]),
	.linked_res_names = lat_usbhost_linked_res,
	.action = activate_pd_constraint,
	.validate = validate_constraint,
};
#endif

static char *lat_per_linked_res[] = {"per",};

static struct shared_resource lat_per = {
	.name = "lat_per",
	.prcm_id = PRCM_PER_CONSTRAINT,
	.res_type = RES_CLOCK_RAMPUP_CO,
	.no_of_users = 0,
	.curr_level = CO_UNUSED,
	.max_levels = CO_MAXLEVEL,
	.linked_res_num = sizeof(lat_per_linked_res) /
			  sizeof(lat_per_linked_res[0]),
	.linked_res_names = lat_per_linked_res,
	.action = activate_pd_constraint,
	.validate = validate_constraint,
};

#ifndef CONFIG_ARCH_OMAP3410
static char *lat_neon_linked_res[] = {"neon",};

static struct shared_resource lat_neon = {
	.name = "lat_neon",
	.prcm_id = PRCM_NEON_CONSTRAINT,
	.res_type = RES_CLOCK_RAMPUP_CO,
	.no_of_users = 0,
	.curr_level = CO_UNUSED,
	.max_levels = CO_MAXLEVEL,
	.linked_res_num = sizeof(lat_neon_linked_res) /
			  sizeof(lat_neon_linked_res[0]),
	.linked_res_names = lat_neon_linked_res,
	.action = activate_pd_constraint,
	.validate = validate_constraint,
};
#endif

static char *lat_core1_linked_res[] = {"core",};

static struct shared_resource lat_core1 = {
	.name = "lat_core1",
	.prcm_id = PRCM_CORE1_CONSTRAINT,
	.res_type = RES_CLOCK_RAMPUP_CO,
	.no_of_users = 0,
	.curr_level = CO_UNUSED,
	.max_levels = CO_MAXLEVEL,
	.linked_res_num = sizeof(lat_core1_linked_res) /
			  sizeof(lat_core1_linked_res[0]),
	.linked_res_names = lat_core1_linked_res,
	.action = activate_pd_constraint,
	.validate = validate_constraint,
};

/* Domain logic and memory resource */
static struct shared_resource mpu_l2cacheon = {
	.name = "mpu_l2cacheon",
	.prcm_id = PRCM_MPU_L2CACHEON,
	.res_type = RES_MEMORY_LOGIC,
	.no_of_users = 0,
	.curr_level = MEMORY_OFF,
	.max_levels = MEMORY_MAXLEVEL_DOMAINON,
	.linked_res_num = 0,
	.action = activate_memory_logic,
	.validate = validate_memory_logic,
};

static struct shared_resource mpu_l2cacheret = {
	.name = "mpu_l2cacheret",
	.prcm_id = PRCM_MPU_L2CACHERET,
	.res_type = RES_MEMORY_LOGIC,
	.no_of_users = 0,
	.curr_level = MEMORY_OFF,
	.max_levels = MEMORY_MAXLEVEL_DOMAINRET,
	.linked_res_num = 0,
	.action = activate_memory_logic,
	.validate = validate_memory_logic,
};

static struct shared_resource mpu_logicl1cacheret = {
	.name = "mpu_logicl1cacheret",
	.prcm_id = PRCM_MPU_LOGICL1CACHERET,
	.res_type = RES_MEMORY_LOGIC,
	.no_of_users = 0,
	.curr_level = LOGIC_OFF,
	.max_levels = LOGIC_MAXLEVEL,
	.linked_res_num = 0,
	.action = activate_memory_logic,
	.validate = validate_memory_logic,
};

static struct shared_resource core_mem2on = {
	.name = "core_mem2on",
	.prcm_id = PRCM_CORE_MEM2ON,
	.res_type = RES_MEMORY_LOGIC,
	.no_of_users = 0,
	.curr_level = MEMORY_ON,
	.max_levels = MEMORY_MAXLEVEL_DOMAINON,
	.linked_res_num = 0,
	.action = activate_memory_logic,
	.validate = validate_memory_logic,
};

static struct shared_resource core_mem1on = {
	.name = "core_mem1on",
	.prcm_id = PRCM_CORE_MEM1ON,
	.res_type = RES_MEMORY_LOGIC,
	.no_of_users = 0,
	.curr_level = MEMORY_ON,
	.max_levels = MEMORY_MAXLEVEL_DOMAINON,
	.linked_res_num = 0,
	.action = activate_memory_logic,
	.validate = validate_memory_logic,
};

static struct shared_resource core_mem2ret = {
	.name = "core_mem2ret",
	.prcm_id = PRCM_CORE_MEM2RET,
	.res_type = RES_MEMORY_LOGIC,
	.no_of_users = 0,
	.curr_level = MEMORY_RET,
	.max_levels = MEMORY_MAXLEVEL_DOMAINRET,
	.linked_res_num = 0,
	.action = activate_memory_logic,
	.validate = validate_memory_logic,
};

static struct shared_resource core_mem1ret = {
	.name = "core_mem1ret",
	.prcm_id = PRCM_CORE_MEM1RET,
	.res_type = RES_MEMORY_LOGIC,
	.no_of_users = 0,
	.curr_level = MEMORY_RET,
	.max_levels = MEMORY_MAXLEVEL_DOMAINRET,
	.linked_res_num = 0,
	.action = activate_memory_logic,
	.validate = validate_memory_logic,
};

static struct shared_resource core_logicret = {
	.name = "core_logicret",
	.prcm_id = PRCM_CORE_LOGICRET,
	.res_type = RES_MEMORY_LOGIC,
	.no_of_users = 0,
	.curr_level = LOGIC_RET,
	.max_levels = LOGIC_MAXLEVEL,
	.linked_res_num = 0,
	.action = activate_memory_logic,
	.validate = validate_memory_logic,
};

static struct shared_resource per_logicret = {
	.name = "per_logicret",
	.prcm_id = PRCM_PER_LOGICRET,
	.res_type = RES_MEMORY_LOGIC,
	.no_of_users = 0,
	.curr_level = LOGIC_OFF,
	.max_levels = LOGIC_MAXLEVEL,
	.linked_res_num = 0,
	.action = activate_memory_logic,
	.validate = validate_memory_logic,
};

static struct shared_resource per_autoidle_res = {
	.name = "per_autoidle_res",
	.prcm_id = DPLL4_PER ,
	.res_type = RES_DPLL,
	.no_of_users = 0,
	.curr_level = DPLL_AUTOIDLE,
	.max_levels = DPLL_RES_MAXLEVEL,
	.linked_res_num = 0,
	.action = activate_autoidle_resource,
	.validate = validate_autoidle_resource,
};


static struct shared_resource mpu_autoidle_res = {
	.name = "mpu_autoidle_res",
	.prcm_id = DPLL1_MPU,
	.res_type = RES_DPLL,
	.no_of_users = 0,
	.curr_level = DPLL_AUTOIDLE,
	.max_levels = DPLL_RES_MAXLEVEL ,
	.linked_res_num = 0,
	.action = activate_autoidle_resource,
	.validate = validate_autoidle_resource,
};

static struct shared_resource core_autoidle_res = {
	.name = "core_autoidle_res",
	.prcm_id = DPLL3_CORE,
	.res_type = RES_DPLL,
	.no_of_users = 0,
	.curr_level = DPLL_AUTOIDLE,
	.max_levels = DPLL_RES_CORE_MAXLEVEL,
	.linked_res_num = 0,
	.action = activate_autoidle_resource,
	.validate = validate_autoidle_resource,
};

static struct shared_resource iva2_autoidle_res = {
	.name = "iva2_autoidle_res",
	.prcm_id = DPLL2_IVA2,
	.res_type = RES_DPLL,
	.no_of_users = 0,
	.curr_level = DPLL_AUTOIDLE,
	.max_levels = DPLL_RES_MAXLEVEL,
	.linked_res_num = 0,
	.action = activate_autoidle_resource,
	.validate = validate_autoidle_resource,
};
#ifdef CONFIG_OMAP3430_ES2
static struct shared_resource per2_autoidle_res = {
	.name = "per2_autoidle_res",
	.prcm_id = DPLL5_PER2,
	.res_type = RES_DPLL,
	.no_of_users = 0,
	.curr_level = DPLL_AUTOIDLE,
	.max_levels = DPLL_RES_MAXLEVEL,
	.linked_res_num = 0,
	.action = activate_autoidle_resource,
	.validate = validate_autoidle_resource,
};
#endif
static struct shared_resource *res_list[] = {
	/* Power domain resources */
	&dss,
	&cam,
	&iva2,
#ifdef CONFIG_OMAP3430_ES2
#ifndef CONFIG_ARCH_OMAP3410
	&sgx,
#endif
	&usbhost,
#else
	&gfx,
#endif
	&per,
#ifndef CONFIG_ARCH_OMAP3410
	&neon,
#endif
	/* Logical resources */
	&core,
	/* Constraints */
	&latency,
	&arm_freq,
	&dsp_freq,
	&vdd1_opp,
	&vdd2_opp,
	&lat_dss,
	&lat_cam,
	&lat_iva2,
#ifndef CONFIG_ARCH_OMAP3410
	&lat_3d,
#endif
#ifdef CONFIG_OMAP3430_ES2
	&lat_usbhost,
#endif
	&lat_per,
#ifndef CONFIG_ARCH_OMAP3410
	&lat_neon,
#endif
	&lat_core1,
	/*memory and logic resource */
	&mpu_l2cacheon,
	&mpu_l2cacheret,
	&mpu_logicl1cacheret,
	&core_mem2on,
	&core_mem1on,
	&core_mem2ret,
	&core_mem1ret,
	&core_logicret,
	&per_logicret,
	&per_autoidle_res,
	&mpu_autoidle_res,
	&core_autoidle_res,
	&iva2_autoidle_res,
#ifdef CONFIG_OMAP3430_ES2
	&per2_autoidle_res
#endif
};

#ifdef CONFIG_OMAP34XX_OFFMODE

struct domain_ctxsvres_status
{
	u32 context_saved;
	u32 context_restore;
};
#endif /* #ifdef CONFIG_OMAP34XX_OFFMODE */

#endif /* __ARCH_ARM_MACH_OMAP3_RESOURCE_H */
