/*
 *  sata_aspen.h - Aspen SATA
 *
 *  Maintained by:  Rudrajit Sengupta <r.sengupta@samsung.com>
 				Sunil M <sunilm@samsung.com>
 *
 * Copyright 2009 Samsung Electronics, Inc.,
 
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  libata documentation is available via 'make {ps|pdf}docs',
 *  as Documentation/DocBook/libata.*
 *
 *  Other errata and documentation available under NDA.
 *
 */
#if 0
#define PHY2VIR(addr)	\
				((addr&0x0FFFFFFF)|(0xD0000000)) */
#endif
#define NR_DEVS 						2
#define INT_SATA0					10 	/*RAPTOR*/
#define INT_SATA1					9	/*NEC*/
#define SATA_DMA_BASE_HOST_0		(0x30014000 + DIFF_IO_BASE0)
#define SATA_DMA_BASE_HOST_1		(0x3001C000 + DIFF_IO_BASE0)
#define SATA_REG_BASE_HOST_0		(0x30010000 + DIFF_IO_BASE0)
#define SATA_REG_BASE_HOST_1		(0x30018000 + DIFF_IO_BASE0)

#define REG_SATA0_CDR0      ((void __iomem*)(SATA_REG_BASE_HOST_0 + 0x00))
#define REG_SATA0_CLR0      ((void __iomem*)(SATA_REG_BASE_HOST_0 + 0x20))
#define REG_SATA0_SCR0      ((void __iomem*)(SATA_REG_BASE_HOST_0 + 0x24))

#define REG_SATA1_CDR0      ((void __iomem*)(SATA_REG_BASE_HOST_1 + 0x00))
#define REG_SATA1_CLR0      ((void __iomem*)(SATA_REG_BASE_HOST_1 + 0x20))
#define REG_SATA1_SCR0      ((void __iomem*)(SATA_REG_BASE_HOST_1 + 0x24))

#define REG_INTCON_MASK     ((void __iomem*)(0x30090f0c + DIFF_IO_BASE0))

/* vendor specific command */
#define CMD_VENDER_LOG		(0xFE)

