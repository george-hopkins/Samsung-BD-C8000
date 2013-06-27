/*
 * drivers/media/video/omap/isp/omap_resizer.c
 *
 * Wrapper for Resizer module in TI's OMAP3430 ISP
 *
 * Copyright (C) 2007 Texas Instruments, Inc.
 *
 * This package is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * THIS PACKAGE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED
 * WARRANTIES OF MERCHANTIBILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 */

#include <linux/cdev.h>
#include <linux/delay.h>
#include <linux/device.h>
#include <linux/fs.h>
#include <linux/mm.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/time.h>
#include <linux/videodev.h>
#include <media/videobuf-dma-sg.h>
#include <media/v4l2-dev.h>
#include <asm/cacheflush.h>
#include <asm/uaccess.h>
#include <asm/io.h>
#include <asm/arch/io.h>
#include <asm/scatterlist.h>

#include "isp.h"
#include "ispmmu.h"
#include "ispreg.h"
#include "ispresizer.h"
#include "omap_resizer.h"

#define OMAP_REZR_NAME		"omap-resizer"

static struct device_params *device_config;
static struct device *rsz_device;

static struct rsz_mult multipass;
static u32 rsz_bufsize = 0;
static int rsz_major = -1;

static struct class *rsz_class = NULL;
static struct platform_driver omap_resizer_driver;


/*
 Function to set hardware configuration registers
*/

void rsz_hardware_setup(struct channel_config *rsz_conf_chan)
{
	/* Counter to set the value     of horizonatl and vertical coeff */
	int coeffcounter;
	/* for getting the coefficient offset */
	int coeffoffset = 0;
	
	/* setting the hardware register rszcnt */
	omap_writel(rsz_conf_chan->register_config.rsz_cnt, ISPRSZ_CNT);

	/* setting the hardware register instart */
	omap_writel(rsz_conf_chan->register_config.rsz_in_start,
		    ISPRSZ_IN_START);
	/* setting the hardware register insize */
	omap_writel(rsz_conf_chan->register_config.rsz_in_size, ISPRSZ_IN_SIZE);
	
	/* setting the hardware register outsize */
	omap_writel(rsz_conf_chan->register_config.rsz_out_size,
		    ISPRSZ_OUT_SIZE);
	/* setting the hardware register inaddress */
	omap_writel(rsz_conf_chan->register_config.rsz_sdr_inadd,
		    ISPRSZ_SDR_INADD);
	/* setting the hardware register in     offset */
	omap_writel(rsz_conf_chan->register_config.rsz_sdr_inoff,
		    ISPRSZ_SDR_INOFF);
	/* setting the hardware register out address */
	omap_writel(rsz_conf_chan->register_config.rsz_sdr_outadd,
		    ISPRSZ_SDR_OUTADD);
	/* setting the hardware register out offset */
	omap_writel(rsz_conf_chan->register_config.rsz_sdr_outoff,
		    ISPRSZ_SDR_OUTOFF);
	/* setting the hardware register yehn */
	omap_writel(rsz_conf_chan->register_config.rsz_yehn, ISPRSZ_YENH);
	
	/* setting the hardware registers     of coefficients */
	for (coeffcounter = 0; coeffcounter < MAX_COEF_COUNTER;
	     coeffcounter++) {
		omap_writel(rsz_conf_chan->register_config.
			    rsz_coeff_horz[coeffcounter],
			    ((ISPRSZ_HFILT10 + coeffoffset)));

		omap_writel(rsz_conf_chan->register_config.
			    rsz_coeff_vert[coeffcounter],
			    ((ISPRSZ_VFILT10 + coeffoffset)));
		coeffoffset = coeffoffset + COEFF_ADDRESS_OFFSET;
	}
}

/*
 This function enable the resize bit after doing the hardware register
 configuration after which resizing	will be	carried	on.
*/
int rsz_start(int *arg, struct device_params *device)
{
	struct channel_config *rsz_conf_chan = device->config;
	
	/* checking     the     configuartion status */
	if (rsz_conf_chan->config_state) {
		printk("State not configured \n");
		return -EINVAL;
	}

	/* Channel is busy */
	rsz_conf_chan->status = CHANNEL_BUSY;

	/*Function call to set up the hardware */
	rsz_hardware_setup(rsz_conf_chan);
	
	if (isp_set_callback(CBK_RESZ_DONE, rsz_isr, (void *)NULL, (void *)NULL)) {
	printk("No callback for RSZR\n");
	return -EINVAL;
	}
mult:
	/* Initialize the interrupt ISR to ZER0 */
	device_config->sem_isr.done = 0;

	/*Function call to enable resizer hardware */
	ispresizer_enable(1);
	
	/* Waiting for resizing to be complete */
	wait_for_completion_interruptible(&(device_config->sem_isr));

	if(multipass.active)
	{
		rsz_set_multipass(rsz_conf_chan);
		goto mult;
	}
	
	if (device->isp_addr_read) {
		ispmmu_unmap(device->isp_addr_read);
		device->isp_addr_read = 0;
	}
	if (device->isp_addr_write) {
		ispmmu_unmap(device->isp_addr_write);
		device->isp_addr_write = 0;
	}
	
	rsz_conf_chan->status = CHANNEL_FREE;

	isp_unset_callback(CBK_RESZ_DONE);

	return 0;

}



int rsz_set_multipass(struct channel_config *rsz_conf_chan)
{	
	multipass.in_hsize  = multipass.out_hsize;
	multipass.in_vsize  = multipass.out_vsize;
	multipass.out_hsize = multipass.end_hsize;
	multipass.out_vsize = multipass.end_vsize;
	
	multipass.out_pitch = ((multipass.inptyp)? 
				multipass.out_hsize: (multipass.out_hsize * 2));		
	multipass.in_pitch = ((multipass.inptyp)? 
				multipass.in_hsize: (multipass.in_hsize * 2));
			
	rsz_set_ratio(rsz_conf_chan);
	rsz_config_ratio(rsz_conf_chan);
	rsz_hardware_setup(rsz_conf_chan);
	return 0;
}

void rsz_copy_data(struct rsz_params *params)
{
	int i;
	multipass.in_hsize  = params->in_hsize;
	multipass.in_vsize  = params->in_vsize;
	multipass.out_hsize = params->out_hsize;
	multipass.out_vsize = params->out_vsize;
	multipass.end_hsize = params->out_hsize;
	multipass.end_vsize = params->out_vsize;
	multipass.in_pitch  = params->in_pitch;
	multipass.out_pitch = params->out_pitch;
	multipass.hstph     = params->hstph;
	multipass.vstph     = params->vstph;
	multipass.inptyp    = params->inptyp;
	multipass.pix_fmt   = params->pix_fmt;
	multipass.cbilin    = params->cbilin;

	for (i = 0; i < 32; i++) {
	multipass.tap4filt_coeffs[i] = params->tap4filt_coeffs[i];
	multipass.tap7filt_coeffs[i] = params->tap7filt_coeffs[i];
	}
}

int rsz_set_params(struct rsz_params *params,
		   struct channel_config *rsz_conf_chan)
{
	/*copy to local structure to be ready if multipass is required*/
	rsz_copy_data(params);

	if (0 != rsz_set_ratio(rsz_conf_chan))
	return -EINVAL;

	/* if input is from ram that vertical pixel should be zero */
	if (INPUT_RAM) {
		params->vert_starting_pixel = 0;
	}

	/* Configuring the starting pixel in vertical direction */
	rsz_conf_chan->register_config.rsz_in_start =
	    ((params->vert_starting_pixel << ISPRSZ_IN_SIZE_VERT_SHIFT)
	     & ISPRSZ_IN_SIZE_VERT_MASK);

	/* if input is 8 bit that start pixel should be <= to than 31 */
	if (params->inptyp == RSZ_INTYPE_PLANAR_8BIT) {
		if (params->horz_starting_pixel > MAX_HORZ_PIXEL_8BIT)
			return -EINVAL;
	}
	/* if input     is 16 bit that start pixel should be <= than 15 */
	if (params->inptyp == RSZ_INTYPE_YCBCR422_16BIT) {
		if (params->horz_starting_pixel > MAX_HORZ_PIXEL_16BIT)
			return -EINVAL;
	}

	/* Configuring the      starting pixel in horizontal direction */
	rsz_conf_chan->register_config.rsz_in_start |=
	    params->horz_starting_pixel & ISPRSZ_IN_START_HORZ_ST_MASK;

	/* Coefficinets of parameters for luma :- algo configuration */
	rsz_conf_chan->register_config.rsz_yehn =
	    ((params->yenh_params.type << ISPRSZ_YENH_ALGO_SHIFT) &
	     ISPRSZ_YENH_ALGO_MASK);

	/* Coefficinets of parameters for luma :- core configuration */
	if (params->yenh_params.type) {
		rsz_conf_chan->register_config.rsz_yehn |=
		    params->yenh_params.core & ISPRSZ_YENH_CORE_MASK;

		/* Coefficinets of parameters for luma :- gain configuration */

		rsz_conf_chan->register_config.rsz_yehn |=
		    ((params->yenh_params.gain << ISPRSZ_YENH_GAIN_SHIFT)
		     & ISPRSZ_YENH_GAIN_MASK);

		/* Coefficinets of parameters for luma :- gain configuration */
		rsz_conf_chan->register_config.rsz_yehn |=
		    ((params->yenh_params.slop << ISPRSZ_YENH_SLOP_SHIFT)
		     & ISPRSZ_YENH_SLOP_MASK);
	}

	rsz_config_ratio(rsz_conf_chan);

	/*Setting the configuration status */
	rsz_conf_chan->config_state = STATE_CONFIGURED;
			
	return 0;
}


int rsz_set_ratio (struct channel_config *rsz_conf_chan)
{
	int alignment = 0;

	rsz_conf_chan->register_config.rsz_cnt = 0;
	
	if ((multipass.out_hsize > MAX_IMAGE_WIDTH)||
			(multipass.out_vsize > MAX_IMAGE_WIDTH)){
		printk("Invalid output size!");
		return -EINVAL;
	}
	/* Configuring the chrominance algorithm */
	if (multipass.cbilin) {
		rsz_conf_chan->register_config.rsz_cnt =
		    BITSET(rsz_conf_chan->register_config.rsz_cnt,
			   SET_BIT_CBLIN);
	}
	/* Configuring the input source */
	if (INPUT_RAM) {
		rsz_conf_chan->register_config.rsz_cnt =
		    BITSET(rsz_conf_chan->register_config.rsz_cnt,
			   SET_BIT_INPUTRAM);
	}
	/* Configuring the input type */
	if (multipass.inptyp == RSZ_INTYPE_PLANAR_8BIT) {
		rsz_conf_chan->register_config.rsz_cnt =
		    BITSET(rsz_conf_chan->register_config.rsz_cnt,
			   SET_BIT_INPTYP);
	} else {
		rsz_conf_chan->register_config.rsz_cnt =
		    BITRESET(rsz_conf_chan->register_config.rsz_cnt,
			     SET_BIT_INPTYP);

		/* Configuring the chrominace position type */
		if (multipass.pix_fmt == RSZ_PIX_FMT_UYVY) {
			rsz_conf_chan->register_config.rsz_cnt =
			    BITRESET(rsz_conf_chan->register_config.rsz_cnt,
				     SET_BIT_YCPOS);
		} else if (multipass.pix_fmt == RSZ_PIX_FMT_YUYV) {
			rsz_conf_chan->register_config.rsz_cnt =
			    BITSET(rsz_conf_chan->register_config.rsz_cnt,
				   SET_BIT_YCPOS);
		}

	}
	multipass.vrsz = 
		(multipass.in_vsize * RATIO_MULTIPLIER)/multipass.out_vsize;
	multipass.hrsz =
		(multipass.in_hsize * RATIO_MULTIPLIER)/multipass.out_hsize;
	if (UP_RSZ_RATIO > multipass.vrsz || UP_RSZ_RATIO > multipass.hrsz){
		printk("Upscaling ratio not supported!");
		return -EINVAL;
	}
	/* calculating the horizontal and vertical ratio */
	multipass.vrsz = (multipass.in_vsize - NUM_D2TAPS) * RATIO_MULTIPLIER 
		/ (multipass.out_vsize - 1);
	multipass.hrsz = ((multipass.in_hsize - NUM_D2TAPS) * RATIO_MULTIPLIER)
	    	/ (multipass.out_hsize - 1);

	/* recalculating Horizontal     ratio */
	if (multipass.hrsz <= 512) {	/* 4-tap     8-phase filter */
		multipass.hrsz = (multipass.in_hsize - NUM_TAPS) * RATIO_MULTIPLIER
		    / (multipass.out_hsize - 1);
		if (multipass.hrsz < 64)
			multipass.hrsz = 64;
		if (multipass.hrsz > 512)
			multipass.hrsz = 512;
		if (multipass.hstph > NUM_PHASES)
			return -EINVAL;
		multipass.num_tap = 1;
	} else if (multipass.hrsz >= 513 && multipass.hrsz <= 1024) {
		/* 7-tap        4-phase filter */
		if (multipass.hstph > NUM_D2PH)
			return -EINVAL;
		multipass.num_tap = 0;
	}

	/* recalculating vertical ratio */
	if (multipass.vrsz <= 512) {	/* 4-tap     8-phase filter */
		multipass.vrsz = (multipass.in_vsize - NUM_TAPS) * RATIO_MULTIPLIER /
		    (multipass.out_vsize - 1);
		if (multipass.vrsz < 64)
			multipass.vrsz = 64;
		if (multipass.vrsz > 512)
			multipass.vrsz = 512;
		if (multipass.vstph > NUM_PHASES)
			return -EINVAL;
	} else if (multipass.vrsz >= 513 && multipass.vrsz <= 1024) {
		if (multipass.vstph > NUM_D2PH)
			return -EINVAL;
	}
	/* Filling the input pitch in the structure */
	if ((multipass.in_pitch) % ALIGN32) {
		printk("Invalid input pitch: %d \n", multipass.in_pitch);
		return -EINVAL;
	}
	if ((multipass.out_pitch) % ALIGN32) {
		printk("Invalid	output pitch %d \n", multipass.out_pitch);
		return -EINVAL;
	}

	/* If vertical upsizing then */
	if (multipass.vrsz < 256 && 
			(multipass.in_vsize < multipass.out_vsize)) {
		/* checking     for     both types of format */
		if (multipass.inptyp == RSZ_INTYPE_PLANAR_8BIT) {
			alignment = ALIGNMENT;
		} else if (multipass.inptyp == RSZ_INTYPE_YCBCR422_16BIT) {
			alignment = (ALIGNMENT / 2);
		} else {
			printk("Invalid input type	\n");
		}
		/* errror checking for output size */
		if (!(((multipass.out_hsize % PIXEL_EVEN) == 0)
		      && (multipass.out_hsize % alignment) == 0)) {
			printk("wrong hsize	\n");

			return -EINVAL;
		}
	}
	if (multipass.hrsz >= 64 && multipass.hrsz <= 1024) {
		if (multipass.out_hsize > MAX_IMAGE_WIDTH) {
			printk("wrong width	\n");
			return -EINVAL;
		}
		multipass.active = 0;
	
	} else if (multipass.hrsz > 1024){
		if (multipass.out_hsize > MAX_IMAGE_WIDTH) {
			printk("wrong width	\n");
			return -EINVAL;
		}
		if (multipass.hstph > NUM_D2PH)
			return -EINVAL;
		multipass.num_tap = 0;
		multipass.out_hsize = multipass.in_hsize *256/1024;
		if((multipass.out_hsize) % ALIGN32){
			multipass.out_hsize += 
				abs((multipass.out_hsize % ALIGN32) - ALIGN32);	
		}
		multipass.out_pitch = ((multipass.inptyp)? 
				multipass.out_hsize: (multipass.out_hsize * 2));
		multipass.hrsz = ((multipass.in_hsize - NUM_D2TAPS) * RATIO_MULTIPLIER)
	    	/ (multipass.out_hsize - 1);
		multipass.active = 1;
		
		
	}
	
	if (multipass.vrsz > 1024) {
		if (multipass.out_vsize > MAX_IMAGE_WIDTH_HIGH) {
		printk("wrong width	\n");
		return -EINVAL;
		}
		
		multipass.out_vsize = multipass.in_vsize *256 / 1024;
		multipass.vrsz = ((multipass.in_vsize - NUM_D2TAPS) * RATIO_MULTIPLIER)
	    	/ (multipass.out_vsize - 1);
		multipass.active = 1;
		multipass.num_tap = 0; //7tap
		
	}
	rsz_conf_chan->register_config.rsz_out_size =
	    (multipass.out_hsize & ISPRSZ_OUT_SIZE_HORZ_MASK);

	rsz_conf_chan->register_config.rsz_out_size |=
	    ((multipass.out_vsize << ISPRSZ_OUT_SIZE_VERT_SHIFT) &
	     ISPRSZ_OUT_SIZE_VERT_MASK);

	rsz_conf_chan->register_config.rsz_sdr_inoff =
	    ((multipass.in_pitch) & ISPRSZ_SDR_INOFF_OFFSET_MASK);
	    
	rsz_conf_chan->register_config.rsz_sdr_outoff =
	    multipass.out_pitch & ISPRSZ_SDR_OUTOFF_OFFSET_MASK;

	/* checking the validity of the horizontal phase value */
	if (multipass.hrsz >= 64 && multipass.hrsz <= 512) {
		if (multipass.hstph > NUM_PHASES)
			return -EINVAL;
	} else if (multipass.hrsz >= 64 && multipass.hrsz <= 512) {
		if (multipass.hstph > NUM_D2PH)
			return -EINVAL;
	}

	rsz_conf_chan->register_config.rsz_cnt |=
	    ((multipass.hstph << ISPRSZ_CNT_HSTPH_SHIFT) & ISPRSZ_CNT_HSTPH_MASK);

	/* checking     the     validity of     the     vertical phase value */
	if (multipass.vrsz >= 64 && multipass.hrsz <= 512) {
		if (multipass.vstph > NUM_PHASES)
			return -EINVAL;
	} else if (multipass.vrsz >= 64 && multipass.vrsz <= 512) {
		if (multipass.vstph > NUM_D2PH)
			return -EINVAL;
	}

	rsz_conf_chan->register_config.rsz_cnt |=
	    ((multipass.vstph << ISPRSZ_CNT_VSTPH_SHIFT) & ISPRSZ_CNT_VSTPH_MASK);

	/* Configuring the horizonatl ratio */
	rsz_conf_chan->register_config.rsz_cnt |=
	    ((multipass.hrsz - 1) & ISPRSZ_CNT_HRSZ_MASK);

	/* Configuring the vertical     ratio */
	rsz_conf_chan->register_config.rsz_cnt |=
	    (((multipass.vrsz - 1) << ISPRSZ_CNT_VRSZ_SHIFT) & ISPRSZ_CNT_VRSZ_MASK);

	return 0;
}



void rsz_config_ratio (struct channel_config *rsz_conf_chan)
{
	int hsize;
	int vsize;
	int coeffcounter;
	
	if (multipass.hrsz <= 512) {	/*4-tap filter */
		hsize =
		    ((32 * multipass.hstph + (multipass.out_hsize - 1) * multipass.hrsz +
		      16) >> 8) + 7;
	} else {
		hsize =
		    ((64 * multipass.hstph + (multipass.out_hsize - 1) * multipass.hrsz +
		      32) >> 8) + 7;
	}
	if (multipass.vrsz <= 512) {	/*4-tap filter */
		vsize =
		    ((32 * multipass.vstph + (multipass.out_vsize - 1) * multipass.vrsz +
		      16) >> 8) + 4;
	} else {
		vsize =
		    ((64 * multipass.vstph + (multipass.out_vsize - 1) * multipass.vrsz +
		      32) >> 8) + 7;
	}
	/* Configuring the Horizontal size of inputframn in MMR */
	rsz_conf_chan->register_config.rsz_in_size = hsize;
	
	rsz_conf_chan->register_config.rsz_in_size |=
	    ((vsize << ISPRSZ_IN_SIZE_VERT_SHIFT)
	     & ISPRSZ_IN_SIZE_VERT_MASK);
	
	for (coeffcounter = 0; coeffcounter < MAX_COEF_COUNTER;
	     coeffcounter++) {
		/* Configuration of     horizontal coefficients */
		if(multipass.num_tap){    //4tap
		rsz_conf_chan->register_config.
		    rsz_coeff_horz[coeffcounter] =
		    (multipass.tap4filt_coeffs[2 * coeffcounter]
		     & ISPRSZ_HFILT10_COEF0_MASK);
		
		rsz_conf_chan->register_config.
		    rsz_coeff_horz[coeffcounter] |=
		    ((multipass.tap4filt_coeffs[2 * coeffcounter + 1]
		      << ISPRSZ_HFILT10_COEF1_SHIFT) &
		     ISPRSZ_HFILT10_COEF1_MASK);
		
		} else {   //7tap
		
		rsz_conf_chan->register_config.
		    rsz_coeff_horz[coeffcounter] =
		    (multipass.tap7filt_coeffs[2 * coeffcounter]
		     & ISPRSZ_HFILT10_COEF0_MASK);
		
		rsz_conf_chan->register_config.
		    rsz_coeff_horz[coeffcounter] |=
		    ((multipass.tap7filt_coeffs[2 * coeffcounter + 1]
		      << ISPRSZ_HFILT10_COEF1_SHIFT) &
		     ISPRSZ_HFILT10_COEF1_MASK);
		}
		
		/* Configuration of     Vertical coefficients */
		if(multipass.num_tap){    //4tap
		rsz_conf_chan->register_config.
		    rsz_coeff_vert[coeffcounter] =
		    (multipass.tap4filt_coeffs[2 * coeffcounter] 
		     & ISPRSZ_VFILT10_COEF0_MASK);
		
		rsz_conf_chan->register_config.
		    rsz_coeff_vert[coeffcounter] |=
		    ((multipass.tap4filt_coeffs[2 * coeffcounter + 1] 
		      << ISPRSZ_VFILT10_COEF1_SHIFT) &
		     ISPRSZ_VFILT10_COEF1_MASK);
		} else {   //7tap
		
		rsz_conf_chan->register_config.
		    rsz_coeff_vert[coeffcounter] =
		    (multipass.tap7filt_coeffs[2 * coeffcounter] 
		     & ISPRSZ_VFILT10_COEF0_MASK);
		
		rsz_conf_chan->register_config.
		    rsz_coeff_vert[coeffcounter] |=
		    ((multipass.tap7filt_coeffs[2 * coeffcounter + 1] 
		       << ISPRSZ_VFILT10_COEF1_SHIFT) &
		     ISPRSZ_VFILT10_COEF1_MASK);
		}
	}
	
			
			
}
/*
 Function to get the parameters	values
*/
int rsz_get_params(struct rsz_params *params,
		   struct channel_config *rsz_conf_chan)
{
	int coeffcounter;

	if (rsz_conf_chan->config_state) {
		printk("	state not configured \n");
		return -EINVAL;
	}

	/* getting the horizontal size */
	params->in_hsize =
	    (rsz_conf_chan->register_config.rsz_in_size &
	    ISPRSZ_IN_SIZE_HORZ_MASK);
	/* getting the vertical size */
	params->in_vsize =
	    ((rsz_conf_chan->register_config.rsz_in_size
	      & ISPRSZ_IN_SIZE_VERT_MASK) >> ISPRSZ_IN_SIZE_VERT_SHIFT);

	/* getting the input pitch */
	params->in_pitch =
	    rsz_conf_chan->register_config.rsz_sdr_inoff
	    & ISPRSZ_SDR_INOFF_OFFSET_MASK;

	/* getting the output horizontal size */
	params->out_hsize =
	    rsz_conf_chan->register_config.rsz_out_size
	    & ISPRSZ_OUT_SIZE_HORZ_MASK;

	/* getting the vertical size   */
	params->out_vsize =
	    ((rsz_conf_chan->register_config.rsz_out_size
	      & ISPRSZ_OUT_SIZE_VERT_MASK) >> ISPRSZ_OUT_SIZE_VERT_SHIFT);

	/* getting the output pitch */
	params->out_pitch =
	    rsz_conf_chan->register_config.rsz_sdr_outoff
	    & ISPRSZ_SDR_OUTOFF_OFFSET_MASK;

	/* getting the chrominance algorithm  */
	params->cbilin =
	    ((rsz_conf_chan->register_config.rsz_cnt
	      & SET_BIT_CBLIN) >> SET_BIT_CBLIN);

	/* getting the input type */
	params->inptyp =
	    ((rsz_conf_chan->register_config.rsz_cnt
	      & ISPRSZ_CNT_INPTYP_MASK) >> SET_BIT_INPTYP);
	/* getting the  starting pixel in horizontal direction */
	params->horz_starting_pixel =
	    ((rsz_conf_chan->register_config.rsz_in_start
	      & ISPRSZ_IN_START_HORZ_ST_MASK));
	/* getting the  starting pixel in vertical direction */
	params->vert_starting_pixel =
	    ((rsz_conf_chan->register_config.rsz_in_start
	      & ISPRSZ_IN_START_VERT_ST_MASK) >> ISPRSZ_IN_START_VERT_ST_SHIFT);

	/* getting the horizontal starting phase */
	params->hstph =
	    ((rsz_conf_chan->register_config.rsz_cnt
	      & ISPRSZ_CNT_HSTPH_MASK >> ISPRSZ_CNT_HSTPH_SHIFT));
	/* getting the vertical starting phase */
	params->vstph =
	    ((rsz_conf_chan->register_config.rsz_cnt
	      & ISPRSZ_CNT_VSTPH_MASK >> ISPRSZ_CNT_VSTPH_SHIFT));

	for (coeffcounter = 0; coeffcounter < MAX_COEF_COUNTER;
	     coeffcounter++) {
		/* getting the horizontal coefficients 0 */
		params->tap4filt_coeffs[2 * coeffcounter] =
		    rsz_conf_chan->register_config.rsz_coeff_horz[coeffcounter]
		    & ISPRSZ_HFILT10_COEF0_MASK;

		/* getting the horizontal coefficients 1 */
		params->tap4filt_coeffs[2 * coeffcounter + 1] =
		    ((rsz_conf_chan->register_config.
		      rsz_coeff_horz[coeffcounter]
		      & ISPRSZ_HFILT10_COEF1_MASK) >>
		     ISPRSZ_HFILT10_COEF1_SHIFT);

		/* getting the vertical coefficients 0 */
		params->tap7filt_coeffs[2 * coeffcounter] =
		    rsz_conf_chan->register_config.rsz_coeff_vert[coeffcounter]
		    & ISPRSZ_VFILT10_COEF0_MASK;

		/* getting the vertical coefficients 1 */
		params->tap7filt_coeffs[2 * coeffcounter + 1] =
		    ((rsz_conf_chan->register_config.
		      rsz_coeff_vert[coeffcounter]
		      & ISPRSZ_VFILT10_COEF1_MASK) >>
		     ISPRSZ_VFILT10_COEF1_SHIFT);

	}

	/* getting the parameters for luma :- algo */
	params->yenh_params.type =
	    ((rsz_conf_chan->register_config.rsz_yehn
	      & ISPRSZ_YENH_ALGO_MASK) >> ISPRSZ_YENH_ALGO_SHIFT);

	/* getting the parameters for luma :- core      */
	params->yenh_params.core =
	    (rsz_conf_chan->register_config.rsz_yehn & ISPRSZ_YENH_CORE_MASK);

	/* Coefficinets of parameters for luma :- gain  */
	params->yenh_params.gain =
	    ((rsz_conf_chan->register_config.rsz_yehn
	      & ISPRSZ_YENH_GAIN_MASK) >> ISPRSZ_YENH_GAIN_SHIFT);

	/* Coefficinets of parameters for luma :- SLOP configuration */
	params->yenh_params.slop =
	    ((rsz_conf_chan->register_config.rsz_yehn
	      & ISPRSZ_YENH_SLOP_MASK) >> ISPRSZ_YENH_SLOP_SHIFT);

	/* getting the input type */
	params->pix_fmt =
	    ((rsz_conf_chan->register_config.rsz_cnt
	      & ISPRSZ_CNT_PIXFMT_MASK) >> SET_BIT_YCPOS);

	if (params->pix_fmt)
		params->pix_fmt = RSZ_PIX_FMT_UYVY;
	else
		params->pix_fmt = RSZ_PIX_FMT_YUYV;

	return 0;
}

void rsz_calculate_crop(struct channel_config *rsz_conf_chan,
			struct rsz_cropsize *cropsize)
{
	int luma_enable;

	cropsize->hcrop = 0;
	cropsize->vcrop = 0;

	luma_enable =
	    ((rsz_conf_chan->register_config.rsz_yehn
	      & ISPRSZ_YENH_ALGO_MASK) >> ISPRSZ_YENH_ALGO_SHIFT);

	/* Luma enhancement reduces image width 1 pixels from left,right */
	if (luma_enable) {
		cropsize->hcrop += 2;
	}
}


/*
 * Videobuffer queue release
 */ 
static void rsz_vbq_release(struct videobuf_queue *q,
				  struct videobuf_buffer *vb)
{
	struct rsz_fh *fh = q->priv_data;
	struct device_params *device = fh->device;

	ispmmu_unmap(device->isp_addr_read);
	ispmmu_unmap(device->isp_addr_write);
	device->isp_addr_read = 0;
	device->isp_addr_write = 0;
	spin_lock(&device->vbq_lock);
	vb->state = STATE_NEEDS_INIT;
	spin_unlock(&device->vbq_lock);

}

/*
 * Sets up the videobuffer size and validates count.
 */
static int rsz_vbq_setup(struct videobuf_queue *q,
			       unsigned int *cnt,
			       unsigned int *size)
{
	struct rsz_fh *fh = q->priv_data;
	struct device_params *device = fh->device;
	
	u32 bpp = 1;
	
	spin_lock(&device->vbq_lock);
	if (*cnt <= 0)
		*cnt = VIDEO_MAX_FRAME;	/* supply a default number of buffers */

	if (*cnt > VIDEO_MAX_FRAME)
		*cnt = VIDEO_MAX_FRAME;
	if (*cnt == 1 && (multipass.out_hsize > multipass.in_hsize)){
		printk("2 buffers are required for Upscaling mode\n");
		spin_unlock(&device->vbq_lock);
		return -EINVAL;
	}				
	if (!device->params->in_hsize ||
		!device->params->in_vsize) {
		printk("Can't setup buffer size\n");
		spin_unlock(&device->vbq_lock);
		return -EINVAL;
	} else {
		if (device->params->inptyp == RSZ_INTYPE_YCBCR422_16BIT)
			bpp = 2;

		if (*cnt == 2){
			*size = rsz_bufsize = (device->params->out_hsize
			 * device->params->out_vsize
			 * bpp);
		} else {
			*size = rsz_bufsize = (device->params->in_hsize
			 * device->params->in_vsize
			 * bpp);
		}			 
			 			 
					 
	}
	spin_unlock(&device->vbq_lock);
	
	return 0;
}

/*
 * Videobuffer is prepared and mmapped.
 */ 
static int rsz_vbq_prepare(struct videobuf_queue *q,
				 struct videobuf_buffer *vb,
				 enum v4l2_field field)
{
	struct rsz_fh *fh = q->priv_data;
	struct device_params *device = fh->device;
	struct channel_config *rsz_conf_chan = 
					fh->device->config;
	int err = -1;
	unsigned int isp_addr;

	spin_lock(&device->vbq_lock);
	if (vb->baddr) {
		vb->size = rsz_bufsize;
		/* video-buf uses bsize (buffer size) instead of
		 * size (image size) to generate sg slist. Work
		 * around this bug by making bsize the same as
		 * size.
		 */
		vb->bsize = rsz_bufsize;
	} else {
		spin_unlock(&device->vbq_lock);	
		printk("No user buffer allocated\n");
		return err;
	}
	if (vb->i){
	vb->width = device->params->out_hsize;
	vb->height = device->params->out_vsize;
	} else {
	vb->width = device->params->in_hsize;
	vb->height = device->params->in_vsize;
	}
	
	vb->field = field;
	spin_unlock(&device->vbq_lock);

	if (vb->state == STATE_NEEDS_INIT) {
		/* Need some time before mapping sg_list */
		
		err = videobuf_iolock(q, vb, NULL);
		if (!err) {
			struct videobuf_dmabuf *dma=videobuf_to_dma(vb);

			isp_addr = ispmmu_map_sg(dma->sglist, dma->sglen);
			if (!isp_addr)
				err = -EIO;
			else {
				if (vb->i){
					rsz_conf_chan->register_config.rsz_sdr_outadd 
					= isp_addr;
					device->isp_addr_write = isp_addr;
			  	}else{
			  		rsz_conf_chan->register_config.rsz_sdr_inadd 
				    		= isp_addr;
				    	if(multipass.out_hsize < multipass.in_hsize)
					rsz_conf_chan->register_config.rsz_sdr_outadd 
						= isp_addr;
					device->isp_addr_read = isp_addr;
			  	}
			}
		}
		
	}
	
	if (!err) {
		spin_lock(&device->vbq_lock);
		vb->state = STATE_PREPARED;
		spin_unlock(&device->vbq_lock);
		flush_cache_user_range(NULL, vb->baddr,
					(vb->baddr + vb->bsize));
	} else
		rsz_vbq_release(q, vb);

	return err;
}

/*
This function creates a channels.
*/
static int rsz_open(struct inode *inode, struct file *filp)
{
	struct channel_config *rsz_conf_chan;
	struct rsz_fh *fh;
	struct device_params *device = device_config;
	struct rsz_params *params;

	if (filp->f_flags == O_NONBLOCK)
		return -1;

	if (device->opened || filp->f_flags & O_NONBLOCK) {
		printk("resizer_open: device is already openend\n");
		return -EBUSY;
	}
	fh = kzalloc(sizeof (struct rsz_fh), GFP_KERNEL);
	if (NULL == fh)
		return -ENOMEM;

	isp_get();

	/* allocate     memory for a new configuration */
	rsz_conf_chan = kzalloc(sizeof(struct channel_config), GFP_KERNEL);

	if (rsz_conf_chan == NULL) {
		printk("\n cannot allocate memory to config");
		return -ENOMEM;
	}
	params = kzalloc(sizeof(struct rsz_params), GFP_KERNEL);

	if (params == NULL) {
		printk("\n cannot allocate memory to params");
		return -ENOMEM;
	}
	
	device->params = params;
	device->config = rsz_conf_chan;
	device->opened = 1;

	/*STATE_NOT_CONFIGURED*/
	rsz_conf_chan->config_state = STATE_NOT_CONFIGURED;
	
	filp->private_data = fh;
	fh->type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	fh->device = device;
	
	videobuf_queue_pci_init(&fh->vbq, &device->vbq_ops, NULL, &device->vbq_lock,
			    fh->type, V4L2_FIELD_NONE,
			    sizeof (struct videobuf_buffer), fh);
	init_completion(&device->sem_isr);

	/* Initializing of application mutex */
	device->sem_isr.done = 0;
	init_MUTEX(&device->array_sem);
	init_MUTEX(&(rsz_conf_chan->chanprotection_sem));

	return 0;

}

/*
 The Function	is used	to release the number of resources occupied
 by the channel
*/
static int rsz_release(struct inode *inode, struct file *filp)
{
	/* get the configuratin of this channel from private_date member of
	   file */
	int ret = 0;
	struct rsz_fh *fh = filp->private_data;
	struct device_params *device = fh->device;
	struct channel_config *rsz_conf_chan = device->config;
	struct rsz_params *params = device->params;			
	struct videobuf_queue *q = &fh->vbq;
			
	ret = down_trylock(&(rsz_conf_chan->chanprotection_sem));
	if (ret != 0) {

		printk("Channel in use\n");
		return -EBUSY;
	}
	device->opened = 0;
	device->params = NULL;
	device->config = NULL;
	
	videobuf_mmap_free(q);
	rsz_bufsize = 0;
	filp->private_data = NULL;
	
	kfree(rsz_conf_chan);
	kfree(fh);
	kfree(params);
	up(&(rsz_conf_chan->chanprotection_sem));

	isp_put();

	return 0;
}

/*
Function to map device memory into user	space
 */ 
static int rsz_mmap(struct file *file, struct vm_area_struct *vma)
{
	struct rsz_fh *fh = file->private_data;

	return videobuf_mmap_mapper(&fh->vbq, vma);
}

/*
This function	will process IOCTL commands sent by
the application	and
control the device IO operations.
*/
static int rsz_ioctl(struct inode *inode, struct file *file,
		     unsigned int cmd, unsigned long arg)
{
	int ret = 0;
	struct rsz_fh *fh = file->private_data;
	struct device_params *device = fh->device;
	struct channel_config *rsz_conf_chan = device->config;
	struct rsz_params *params = device->params;


	/* Create the structures of
	   different parameters passed by user */
	struct rsz_status *status;
	
	ret = down_trylock(&(rsz_conf_chan->chanprotection_sem));
	if (ret != 0) {
		printk("Channel in use\n");
		return -EBUSY;
	}

	/* Before decoding check for correctness of cmd */
	if (_IOC_TYPE(cmd) != RSZ_IOC_BASE) {
		printk("Bad	command	Value \n");
		return -1;
	}
	if (_IOC_NR(cmd) > RSZ_IOC_MAXNR) {
		printk("Bad	command	Value\n");
		return -1;
	}

	/*veryfying     access permission of commands */
	if (_IOC_DIR(cmd) & _IOC_READ)
		ret = !access_ok(VERIFY_WRITE, (void *)arg, _IOC_SIZE(cmd));
	else if (_IOC_DIR(cmd) & _IOC_WRITE)
		ret = !access_ok(VERIFY_READ, (void *)arg, _IOC_SIZE(cmd));
	if (ret) {
		printk("access denied\n");
		return -1;	/*error in access */
	}

	/* switch according     value of cmd */
	switch (cmd) {
	case RSZ_REQBUF:
		down_interruptible(&(device->array_sem));
		ret = videobuf_reqbufs(&fh->vbq, (void *)arg);
		up(&(device->array_sem));
		break;

	case RSZ_QUERYBUF:
		down_interruptible(&(device->array_sem));
		ret = videobuf_querybuf(&fh->vbq, (void *)arg);
		up(&(device->array_sem));
		break;

	case RSZ_QUEUEBUF:
		down_interruptible(&(device->array_sem));
		ret = videobuf_qbuf(&fh->vbq, (void *)arg);
		up(&(device->array_sem));
		break;

		/* This ioctl is used to set the parameters
		   of the Resizer hardware, including input and output
		   image size, horizontal    and     vertical poly-phase
		   filter coefficients,luma enchancement filter coefficients etc */
	case RSZ_S_PARAM:

		/* Function to set the hardware configuration */
		if (copy_from_user
		    (params, (struct rsz_params *)arg,
		     sizeof(struct rsz_params))) {
			up(&(device->array_sem));
			return -EFAULT;
		}
		ret = rsz_set_params(params, rsz_conf_chan);
		break;

		/*This ioctl is used to get the Resizer hardware settings
		   associated with the current logical channel represented
		   by fd. */
	case RSZ_G_PARAM:
		/* Function to get the hardware configuration */
		ret = rsz_get_params((struct rsz_params *)arg, rsz_conf_chan);
		break;

		/* This ioctl is used to check the current status
		   of the Resizer hardware */
	case RSZ_G_STATUS:
		status = (struct rsz_status *)arg;
		status->chan_busy = rsz_conf_chan->status;
		status->hw_busy = ispresizer_busy();
		status->src = INPUT_RAM;
		break;

		/*This ioctl submits a resizing task specified by the
		   rsz_resize structure.The call can either be blocked until
		   the task is completed or returned immediately based
		   on the value of the blocking argument in the rsz_resize
		   structure. If  it is blocking, the     status of the task
		   can be checked by calling ioctl   RSZ_G_STATUS. Only one task
		   can  be outstanding for each logical channel. */
	case RSZ_RESIZE:

		ret = rsz_start((int *)arg, device);
		break;

	case RSZ_GET_CROPSIZE:

		rsz_calculate_crop(rsz_conf_chan, (struct rsz_cropsize *)arg);
		break;

	default:
		printk("resizer_ioctl: Invalid Command Value");
		ret = -EINVAL;
	}

	up(&(rsz_conf_chan->chanprotection_sem));

	return ret;
}

static struct file_operations rsz_fops = {
	.owner = THIS_MODULE,
	.open = rsz_open,
	.release = rsz_release,
	.mmap = rsz_mmap,
	.ioctl = rsz_ioctl,
};

/*
Function to register the Resizer character device	driver
*/
void rsz_isr(unsigned long status, void *arg1, void *arg2)
{

	if ((RESZ_DONE & status) != RESZ_DONE)
		return;
	
	complete(&(device_config->sem_isr));
	
}


static void resizer_platform_release(struct device *device)
{
	/* This is called when the reference count goes to zero */
}

static int __init resizer_probe(struct platform_device *device)
{
	return 0;
}

static int resizer_remove(struct platform_device *omap_resizer_device)
{
	return 0;
}

static struct platform_device omap_resizer_device = {
	.name = OMAP_REZR_NAME,
	.id = 2,
	.dev = {
		.release = resizer_platform_release,}
};

static struct platform_driver omap_resizer_driver = {
	.probe = resizer_probe,
	.remove = resizer_remove,
	.driver = {
		   .bus = &platform_bus_type,
		   .name = OMAP_REZR_NAME,
		   },
};

/*
function to	register resizer character driver
*/
static int __init omap_rsz_init(void)
{

	int ret;
	struct device_params *device;
	device = kzalloc(sizeof (struct device_params), GFP_KERNEL);
	if (!device) {
		printk(KERN_ERR OMAP_REZR_NAME ": could not allocate memory\n");
		return -ENOMEM;
	}

	/* Register     the     driver in the kernel */
	rsz_major = register_chrdev(0, OMAP_REZR_NAME, &rsz_fops);

	if (rsz_major < 0) {
		printk(OMAP_REZR_NAME ": initialization "
		       "failed. could not register character device\n");
		return -ENODEV;
	}
	
	/* register driver as a platform driver */
	ret = platform_driver_register(&omap_resizer_driver);
	if (ret) {
		printk(OMAP_REZR_NAME
		       ": failed to register platform driver!\n");

		goto fail2;
	}

	/* Register the drive as a platform device */
	ret = platform_device_register(&omap_resizer_device);
	if (ret) {
		printk(OMAP_REZR_NAME
		       ": failed to register platform device!\n");
		goto fail3;
	}

	rsz_class = class_create(THIS_MODULE, OMAP_REZR_NAME);

	if (!rsz_class) 
		goto fail4;

	rsz_device = device_create(rsz_class, rsz_device, (MKDEV(rsz_major, 0)),
			       OMAP_REZR_NAME);
	printk(OMAP_REZR_NAME ": Registered Resizer Wrapper\n");
	device->opened = 0;

	/* make entry in the devfs */
	/* initialize the videobuf queue ops */
	device->vbq_ops.buf_setup = rsz_vbq_setup;
	device->vbq_ops.buf_prepare = rsz_vbq_prepare;
	device->vbq_ops.buf_release = rsz_vbq_release;
	spin_lock_init(&device->vbq_lock);

	device_config = device;
	return 0;

fail4:
	platform_device_unregister(&omap_resizer_device);
fail3:
	platform_driver_unregister(&omap_resizer_driver);
fail2:
	unregister_chrdev(rsz_major, OMAP_REZR_NAME);

	return ret;
}

/*
Function	is called by the kernel. It	unregister the device.
*/
void __exit omap_rsz_exit(void)
{
	platform_device_unregister(&omap_resizer_device);
	platform_driver_unregister(&omap_resizer_driver);
	unregister_chrdev(rsz_major, OMAP_REZR_NAME);
	isp_unset_callback(CBK_RESZ_DONE);
	kfree(device_config);
	rsz_major = -1;

}

module_init(omap_rsz_init)
module_exit(omap_rsz_exit)

MODULE_AUTHOR("Texas Instruments");
MODULE_DESCRIPTION("OMAP ISP Resizer");
MODULE_LICENSE("GPL");
