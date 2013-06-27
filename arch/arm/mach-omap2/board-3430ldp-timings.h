/*
 * linux/arch/arm/mach-omap3/board-3430ldp-timings.h
 *
 * OMAP 34xx Power Reset and Clock Management (PRCM) functions
 *
 * Copyright (C) 2008 Texas Instruments, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 *
 */

/*
 * LDP/Zoom SDRC & GPMC Memory timings for OPPs & DVFS
 * FIXME: Half frequency values are not optimized here (66 & 83)MHz
 */
struct dvfs_config omap3_vdd2_config[2] = {
#ifdef CONFIG_OMAP3_CORE_166MHZ
	{
	/* SDRC CS0/CS1 values 83MHz, sdrc=not optimized values */
	{{0x00025801, 0x8a99b485, 0x00011413},
	 {0x00025801, 0x8a99b485, 0x00011413} },
	/* CS0 values are for NAND */
	/* CS1 values are for ETHERNET */
	/* CS2 values are for exp */
	/* CS3 values are for exp */
	{{0x00141400, 0x00141400, 0x0F010F01, 0x010c1414, 0x1f0f0a80},
	 {0x001f1f01, 0x00080803, 0x1c0b1c0a, 0x041f1f1f, 0x1f0f04c4},
	 {0, 0, 0, 0, 0},
	 {0, 0, 0, 0, 0} }
	},

	/* SDRC CS0/CS1 values 166MHz, sdrc=optimized values */
	{
	{{0x0003de01, 0x8a99b485, 0x00011413},
	 {0x0003de01, 0x8a99b485, 0x00011413} },
	/* GPMC CS0/1/2/3 values 166 MHZ, gpmc = check values */
	{{0x00141400, 0x00141400, 0x0F010F01, 0x010c1414, 0x1f0f0a80},
	 {0x001f1f01, 0x00080803, 0x1c0b1c0a, 0x041f1f1f, 0x1f0f04c4},
	 {0, 0, 0, 0, 0},
	 {0, 0, 0, 0, 0} }
	},
#elif defined(CONFIG_OMAP3_CORE_133MHZ)
	{
	/* SDRC CS0/CS1 values 66MHz, sdrc = not optimized values */
	{{0x0001ef01, 0x8a99b485, 0x00011412},
	 {0x0001ef01, 0x8a99b485, 0x00011412} },
	/* CS0 values are for NAND */
	/* CS1 values are for ETHERNET */
	/* CS2 values are for exp */
	/* CS3 values are for exp */
	{{0x00141400, 0x00141400, 0x0F010F01, 0x010c1414, 0x1f0f0a80},
	 {0x001f1f01, 0x00080803, 0x1c0b1c0a, 0x041f1f1f, 0x1f0f04c4},
	 {0, 0, 0, 0, 0},
	 {0, 0, 0, 0, 0} }
	},

	/* SDRC CS0/CS1 values 133MHz, sdrc = optimized values */
	{
	{{0x0003de01, 0x8a99b485, 0x00011412},
	 {0x0003de01, 0x8a99b485, 0x00011412} },
	/* GPMC CS0/1/2/3 values 133 MHz, gpmc = check values */
	{{0x00141400, 0x00141400, 0x0F010F01, 0x010c1414, 0x1f0f0a80},
	 {0x001f1f01, 0x00080803, 0x1c0b1c0a, 0x041f1f1f, 0x1f0f04c4},
	 {0, 0, 0, 0, 0},
	 {0, 0, 0, 0, 0} }
	},
#else
#error "No board memory timing tabled defined for CORE frequency"
#endif
};

