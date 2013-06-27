/*
 * linux/arch/arm/mach-omap2/board-3530evm-timings.h
 *
 * OMAP 35xx EVM Power Reset and Clock Management (PRCM) functions
 *
 * Copyright (C) 2007 Texas Instruments, Inc.
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
 * 3530EVM SDRC & GPMC Memory timings for OPPs & DVFS
 */
struct dvfs_config omap3_vdd2_config[2] = {
#ifdef CONFIG_OMAP3_CORE_166MHZ
	{
	/* SDRC CS0/CS1 values 83MHZ*/
	{{0x00025801, 0x629db4c6, 0x00012214},
	 {0x00025801, 0x31512283, 0x1110A} },
	/* GPMC CS0/1/2/3 values 83 MHZ*/
	/* CS0 values are for NOR */
	/* CS1 values are for NAND */
	/* CS2 values are for ONE-NAND */
	/* CS3 values are for FPGA */
	{{0x00101000, 0x00040401, 0x0E050E05, 0x010A1010, 0x100802C1},
	 {0x000A0A00, 0x000A0A00, 0x08010801, 0x01060A0A, 0x10080580},
	 {0x00080801, 0x00020201, 0x08020802, 0x01080808, 0x10030000},
	 {0x00101001, 0x00040402, 0x0E050E05, 0x020F1010, 0x0F0502C2} }
	},

	/* SDRC CS0/CS1 values 166MHZ*/
	{
	{{0x0004e201, 0x629db4c6, 0x00012214},
	 {0x0004e201, 0x629db4c6, 0x00012214} },
	/* GPMC CS0/1/2/3 values 166 MHZ*/
	{{0x001f1f00, 0x00080802, 0x1C091C09, 0x01131F1F, 0x1F0F03C2},
	 {0x00141400, 0x00141400, 0x0F010F01, 0x010C1414, 0x1F0F0A80},
	 {0x000F0F01, 0x00030301, 0x0F040F04, 0x010F1010, 0x1F060000},
	 {0x001F1F01, 0x00080803, 0x1D091D09, 0x041D1F1F, 0x1D0904C4} }
	},
#else
#error "No board memory timing tabled defined for CORE frequency"
#endif
};

