/*
 * linux/include/asm-arm/arch-ssdtv/sdp83.h
 *
 *  Copyright (C) 2003-2008 Samsung Electronics
 *  Author: tukho.kim@samsung.com - 06.16.2008
 *
 */
/* modifications
 * --------------------
 *  23.Oct.2008 expand system memory area (~0xe0000000)
 */

#ifndef __SDP83_H
#define __SDP83_H

#include "bitfield.h"

#define VA_IO_BASE0           (0xF8000000)
#define PA_IO_BASE0           (0x30000000)
#define DIFF_IO_BASE0	      (VA_IO_BASE0 - PA_IO_BASE0)

/* NAND Memory Controller Register */
#define PA_NAND_BASE           (0x30020000)
#define VA_NAND_BASE           (PA_NAND_BASE + DIFF_IO_BASE0)

/* Static Memory Controller Register */
#define PA_SMC_BASE           (0x30028000)
#define VA_SMC_BASE           (PA_SMC_BASE + DIFF_IO_BASE0)

/* PCI Host & Target controller register */
#define PA_PCI_BASE           (0x30030000)
#define VA_PCI_BASE           (PA_PCI_BASE + DIFF_IO_BASE0)

/* DMA controller register */
#define PA_DMA_BASE           (0x30040000)
#define VA_DMA_BASE           (PA_DMA_BASE + DIFF_IO_BASE0)

/* USB EHCI0 host controller register */
#define PA_EHCI0_BASE         (0x30070000)
#define VA_EHCI0_BASE         (PA_EHCI0_BASE + DIFF_IO_BASE0)

/* USB OHCI0 host controller register */
#define PA_OHCI0_BASE         (PA_EHCI0_BASE + 0x8000)
#define VA_OHCI0_BASE         (PA_OHCI0_BASE + DIFF_IO_BASE0)

/* USB EHCI1 host controller register */
#define PA_EHCI1_BASE         (0x30080000)
#define VA_EHCI1_BASE         (PA_EHCI1_BASE + DIFF_IO_BASE0)

/* USB OHCI1 host controller register */
#define PA_OHCI1_BASE         (PA_EHCI1_BASE + 0x8000)
#define VA_OHCI1_BASE         (PA_OHCI1_BASE + DIFF_IO_BASE0)

/* Timer Register */
#define PA_TIMER_BASE         (0x30090400)
#define VA_TIMER_BASE         (PA_TIMER_BASE + DIFF_IO_BASE0)

/* Watchdog Register */
#define PA_WDT_BASE           (0x30090600)
#define VA_WDT_BASE           (PA_WDT_BASE + DIFF_IO_BASE0)

/* Power Management unit & PLL Register */
#define PA_PMU_BASE           (0x30090800)
#define VA_PMU_BASE           (PA_PMU_BASE + DIFF_IO_BASE0)

/* UART Register */
#define PA_UART_BASE          (0x30090A00)
#define VA_UART_BASE          (PA_UART_BASE + DIFF_IO_BASE0)

/* Pad Control Register */
#define PA_PADCTRL_BASE       (0x30090C00)
#define VA_PADCTRL_BASE       (PA_GPIO_BASE + DIFF_IO_BASE0)

/* Interrupt controller register */
#define PA_INTC_BASE          (0x30090F00)
#define VA_INTC_BASE          (PA_INTC_BASE + DIFF_IO_BASE0)

/* SMC Register */
#define VA_SMC(offset)  (*(volatile unsigned *)(VA_SMC_BASE+(offset)))

#define O_SMC_BANK0	      	(0x48)
#define O_SMC_BANK1	      	(0x24)
#define O_SMC_BANK2	      	(0x00)
#define O_SMC_BANK3	      	(0x6C)

#define VA_SMC_BANK(bank, offset)  (*(volatile unsigned *)(VA_SMC_BASE+(bank)+(offset)))
#define VA_SMC_BANK0(offset)  	VA_SMC_BANK(O_SMC_BANK0, offset)
#define VA_SMC_BANK1(offset)  	VA_SMC_BANK(O_SMC_BANK1, offset)
#define VA_SMC_BANK2(offset)  	VA_SMC_BANK(O_SMC_BANK2, offset)
#define VA_SMC_BANK3(offset)  	VA_SMC_BANK(O_SMC_BANK3, offset)

#define O_SMC_SMBIDCYR			(0x00)
#define O_SMC_SMBWST1R			(0x04)
#define O_SMC_SMBWST2R			(0x08)
#define O_SMC_SMBWSTOENR		(0x0C)
#define O_SMC_SMBWSTWENR		(0x10)
#define O_SMC_SMBCR			(0x14)
#define O_SMC_SMBSR			(0x18)
#define O_SMC_CIWRCON			(0x1C)
#define O_SMC_CIRDCON			(0x20)

#define O_SMC_SMBEWS			(0x120)
#define O_SMC_CI_RESET			(0x128)
#define O_SMC_CI_ADDRSEL		(0x12C)
#define O_SMC_CI_CNTL			(0x130)
#define O_SMC_CI_REGADDR		(0x134)

#define O_SMC_CLKSTOP			(0x1E8)
#define O_SMC_SYNCEN			(0x1EC)
#define O_SMC_SMBWST3R_BK0		(0x140)
#define O_SMC_SMBWST3R_BK1		(0x144)
#define O_SMC_SMBWST3R_BK2		(0x148)
#define O_SMC_SMBWST3R_BK3		(0x14C)

#define O_SMC_PERIPHID0			(0xFE0)
#define O_SMC_PERIPHID1			(0xFE4)
#define O_SMC_PERIPHID2			(0xFE8)
#define O_SMC_PERIPHID3			(0xFEC)
#define O_SMC_PCELLID0			(0xFF0)
#define O_SMC_PCELLID1			(0xFF4)
#define O_SMC_PCELLID2			(0xFF8)
#define O_SMC_PCELLID3			(0xFFC)

#define R_SMC_SMBEWS			VA_SMC(O_SMC_SMBEWS)
#define R_SMC_CI_RESET			VA_SMC(O_SMC_CI_RESET)
#define R_SMC_CI_ADDRSEL		VA_SMC(O_SMC_CI_ADDRSEL)
#define R_SMC_CI_CNTL			VA_SMC(O_SMC_CI_CNTL)
#define R_SMC_CI_REGADDR		VA_SMC(O_SMC_CI_REGADDR)
                                        
#define R_SMC_CLKSTOP			VA_SMC(O_SMC_CLKSTOP)
#define R_SMC_SYNCEN			VA_SMC(O_SMC_SYNCEN)
                                        
#define R_SMC_PERIPHID0			VA_SMC(O_SMC_PERIPHID0)
#define R_SMC_PERIPHID1			VA_SMC(O_SMC_PERIPHID1)
#define R_SMC_PERIPHID2			VA_SMC(O_SMC_PERIPHID2)
#define R_SMC_PERIPHID3			VA_SMC(O_SMC_PERIPHID3)
#define R_SMC_PCELLID0			VA_SMC(O_SMC_PCELLID0)
#define R_SMC_PCELLID1			VA_SMC(O_SMC_PCELLID1)
#define R_SMC_PCELLID2			VA_SMC(O_SMC_PCELLID2)
#define R_SMC_PCELLID3			VA_SMC(O_SMC_PCELLID3)

#define R_SMC_SMBIDCYR_BK0		VA_SMC_BANK0(O_SMC_SMBIDCYR)
#define R_SMC_SMBWST1R_BK0		VA_SMC_BANK0(O_SMC_SMBWST1R)
#define R_SMC_SMBWST2R_BK0		VA_SMC_BANK0(O_SMC_SMBWST2R)
#define R_SMC_SMBWSTOENR_BK0		VA_SMC_BANK0(O_SMC_SMBWSTOENR)
#define R_SMC_SMBWSTWENR_BK0		VA_SMC_BANK0(O_SMC_SMBWSTWENR)
#define R_SMC_SMBCR_BK0			VA_SMC_BANK0(O_SMC_SMBCR)
#define R_SMC_SMBSR_BK0			VA_SMC_BANK0(O_SMC_SMBSR)
#define R_SMC_CIWRCON_BK0		VA_SMC_BANK0(O_SMC_CIWRCON)
#define R_SMC_CIRDCON_BK0		VA_SMC_BANK0(O_SMC_CIRDCON)
#define R_SMC_SMBWST3R_BK0		VA_SMC(O_SMC_SMBSWST3R_BK0)

#define R_SMC_SMBIDCYR_BK1		VA_SMC_BANK1(O_SMC_SMBIDCYR)
#define R_SMC_SMBWST1R_BK1		VA_SMC_BANK1(O_SMC_SMBWST1R)
#define R_SMC_SMBWST2R_BK1		VA_SMC_BANK1(O_SMC_SMBWST2R)
#define R_SMC_SMBWSTOENR_BK1		VA_SMC_BANK1(O_SMC_SMBWSTOENR)
#define R_SMC_SMBWSTWENR_BK1		VA_SMC_BANK1(O_SMC_SMBWSTWENR)
#define R_SMC_SMBCR_BK1			VA_SMC_BANK1(O_SMC_SMBCR)
#define R_SMC_SMBSR_BK1			VA_SMC_BANK1(O_SMC_SMBSR)
#define R_SMC_CIWRCON_BK1		VA_SMC_BANK1(O_SMC_CIWRCON)
#define R_SMC_CIRDCON_BK1		VA_SMC_BANK1(O_SMC_CIRDCON)
#define R_SMC_SMBWST3R_BK1		VA_SMC(O_SMC_SMBSWST3R_BK1)

#define R_SMC_SMBIDCYR_BK2		VA_SMC_BANK2(O_SMC_SMBIDCYR)
#define R_SMC_SMBWST1R_BK2		VA_SMC_BANK2(O_SMC_SMBWST1R)
#define R_SMC_SMBWST2R_BK2		VA_SMC_BANK2(O_SMC_SMBWST2R)
#define R_SMC_SMBWSTOENR_BK2		VA_SMC_BANK2(O_SMC_SMBWSTOENR)
#define R_SMC_SMBWSTWENR_BK2		VA_SMC_BANK2(O_SMC_SMBWSTWENR)
#define R_SMC_SMBCR_BK2			VA_SMC_BANK2(O_SMC_SMBCR)
#define R_SMC_SMBSR_BK2			VA_SMC_BANK2(O_SMC_SMBSR)
#define R_SMC_CIWRCON_BK2		VA_SMC_BANK2(O_SMC_CIWRCON)
#define R_SMC_CIRDCON_BK2		VA_SMC_BANK2(O_SMC_CIRDCON)
#define R_SMC_SMBWST3R_BK2		VA_SMC(O_SMC_SMBSWST3R_BK2)

#define R_SMC_SMBIDCYR_BK3		VA_SMC_BANK3(O_SMC_SMBIDCYR)
#define R_SMC_SMBWST1R_BK3		VA_SMC_BANK3(O_SMC_SMBWST1R)
#define R_SMC_SMBWST2R_BK3		VA_SMC_BANK3(O_SMC_SMBWST2R)
#define R_SMC_SMBWSTOENR_BK3		VA_SMC_BANK3(O_SMC_SMBWSTOENR)
#define R_SMC_SMBWSTWENR_BK3		VA_SMC_BANK3(O_SMC_SMBWSTWENR)
#define R_SMC_SMBCR_BK3			VA_SMC_BANK3(O_SMC_SMBCR)
#define R_SMC_SMBSR_BK3			VA_SMC_BANK3(O_SMC_SMBSR)
#define R_SMC_CIWRCON_BK3		VA_SMC_BANK3(O_SMC_CIWRCON)
#define R_SMC_CIRDCON_BK3		VA_SMC_BANK3(O_SMC_CIRDCON)
#define R_SMC_SMBWST3R_BK3		VA_SMC(O_SMC_SMBSWST3R_BK3)


/* interrupt controller */
#define VA_INTC(offset)  (*(volatile unsigned *)(VA_INTC_BASE+(offset)))

#define O_INTCON		(0x00)		/* R/W */
#define O_INTPND		(0x04)
#define O_INTMOD		(0x08)
#define O_INTMSK		(0x0C)
#define O_LEVEL			(0x10)
#define O_I_PSLV0		(0x14)
#define O_I_PSLV1 		(0x18)
#define O_I_PSLV2		(0x1C)
#define O_I_PSLV3		(0x20)
#define O_I_PMST 		(0x24)
#define O_I_CSLV0		(0x28)
#define O_I_CSLV1		(0x2C)
#define O_I_CSLV2		(0x30)
#define O_I_CSLV3		(0x34)
#define O_I_CMST 		(0x38)
#define O_I_ISPR 		(0x3C)
#define O_I_ISPC		(0x40)
#define O_F_PSLV0 		(0x44)
#define O_F_PSLV1 		(0x48)
#define O_F_PSLV2 		(0x4C)
#define O_F_PSLV3 		(0x50)
#define O_F_PMST  		(0x54)
#define O_F_CSLV0 		(0x58)
#define O_F_CSLV1 		(0x5C)
#define O_F_CSLV2 		(0x60)
#define O_F_CSLV3 		(0x64)
#define O_F_CMST  		(0x68)
#define O_F_ISPR  		(0x6C)
#define O_F_ISPC  		(0x70)
#define O_POLARITY		(0x74)
#define O_I_VECADDR 		(0x78)
#define O_F_VECADDR 		(0x7C)
#define O_TIC_QSRC		(0x80)
#define O_TIC_FIRQ		(0x84)
/* reserved           		(0x88) */
#define O_TIC_TESTEN		(0x8C)
#define O_SRCPND		(0x90)
#define O_SUBINT		(0x94)
#define O_INTSRCSEL0		(0x98)
#define O_INTSRCSEL1		(0x9C)
#define O_INTSRCSEL2		(0xA0)
#define O_INTSRCSEL3		(0xA4)
#define O_INTSRCSEL4		(0xA8)
#define O_INTSRCSEL5		(0xAC)
#define O_INTSRCSEL6		(0xB0)
#define O_INTSRCSEL7		(0xB4)

/* define for 'C' */
#define R_INTCON		VA_INTC(O_INTCON)
#define R_INTPND		VA_INTC(O_INTPND)
#define R_INTMOD		VA_INTC(O_INTMOD)
#define R_INTMSK		VA_INTC(O_INTMSK)
#define R_LEVEL			VA_INTC(O_LEVEL)
#define R_I_PSLV0		VA_INTC(O_I_PSLV0)
#define R_I_PSLV1 		VA_INTC(O_I_PSLV1)
#define R_I_PSLV2		VA_INTC(O_I_PSLV2)
#define R_I_PSLV3		VA_INTC(O_I_PSLV3)
#define R_I_PMST 		VA_INTC(O_I_PMST)
#define R_I_CSLV0		VA_INTC(O_I_CSLV0)
#define R_I_CSLV1		VA_INTC(O_I_CSLV1)
#define R_I_CSLV2		VA_INTC(O_I_CSLV2)
#define R_I_CSLV3		VA_INTC(O_I_CSLV3)
#define R_I_CMST 		VA_INTC(O_I_CMST)
#define R_I_ISPR 		VA_INTC(O_I_ISPR)
#define R_I_ISPC		VA_INTC(O_I_ISPC)
#define R_F_PSLV0 		VA_INTC(O_F_PSLV0)
#define R_F_PSLV1 		VA_INTC(O_F_PSLV1)
#define R_F_PSLV2 		VA_INTC(O_F_PSLV2)
#define R_F_PSLV3 		VA_INTC(O_F_PSLV3)
#define R_F_PMST  		VA_INTC(O_F_PMST)
#define R_F_CSLV0 		VA_INTC(O_F_CSLV0)
#define R_F_CSLV1 		VA_INTC(O_F_CSLV1)
#define R_F_CSLV2 		VA_INTC(O_F_CSLV2)
#define R_F_CSLV3 		VA_INTC(O_F_CSLV3)
#define R_F_CMST  		VA_INTC(O_F_CMST)
#define R_F_ISPR  		VA_INTC(O_F_ISPR)
#define R_F_ISPC  		VA_INTC(O_F_ISPC)
#define R_POLARITY		VA_INTC(O_POLARITY)
#define R_I_VECADDR 		VA_INTC(O_I_VECADDR)
#define R_F_VECADDR 		VA_INTC(O_F_VECADDR)
#define R_TIC_QSRC		VA_INTC(O_TIC_QSRC)
#define R_TIC_FIRQ		VA_INTC(O_TIC_FIRQ)
/* reserved           		(0x88) */
#define R_TIC_TESTEN		VA_INTC(O_TIC_TESTEN)
#define R_SRCPND		VA_INTC(O_SRCPND)
#define R_SUBINT		VA_INTC(O_SUBINT)
#define R_INTSRCSEL0		VA_INTC(O_INTSRCSEL0)
#define R_INTSRCSEL1		VA_INTC(O_INTSRCSEL1)
#define R_INTSRCSEL2		VA_INTC(O_INTSRCSEL2)
#define R_INTSRCSEL3		VA_INTC(O_INTSRCSEL3)
#define R_INTSRCSEL4		VA_INTC(O_INTSRCSEL4)
#define R_INTSRCSEL5		VA_INTC(O_INTSRCSEL5)
#define R_INTSRCSEL6		VA_INTC(O_INTSRCSEL6)
#define R_INTSRCSEL7		VA_INTC(O_INTSRCSEL7)

/* clock & power management */
#define VA_PMU(offset)    (*(volatile unsigned *)(VA_PMU_BASE+(offset)))

#define O_PMU_PLL0_PMS_CON		(0x00)
#define O_PMU_PLL1_PMS_CON		(0x04)
#define O_PMU_PLL2_PMS_CON		(0x08)
#define O_PMU_PLL3_PMS_CON		(0x0C)
#define O_PMU_PLL4_PMS_CON		(0x10)
#define O_PMU_PLL5_PMS_CON		(0x14)
#define O_PMU_PLL6_PMS_CON		(0x18)
/* Reseved 				(0x1C) */
#define O_PMU_PLL3_K_CON		(0x20)
#define O_PMU_PLL4_K_CON		(0x24)
#define O_PMU_PLL5_K_CON		(0x28)
#define O_PMU_PLL6_CTRL			(0x2C)
#define O_PMU_PLL0_LOCKCNT		(0x30)
#define O_PMU_PLL1_LOCKCNT		(0x34)
#define O_PMU_PLL2_LOCKCNT		(0x38)
#define O_PMU_PLL3_LOCKCNT		(0x3C)
#define O_PMU_PLL4_LOCKCNT		(0x40)
#define O_PMU_PLL5_LOCKCNT		(0x44)
#define O_PMU_PLL6_LOCKCNT		(0x48)
/* Reseved 				(0x4C) */
#define O_PMU_PLL_LOCK			(0x50)
#define O_PMU_PLL_PWD			(0x54)
#define O_PMU_PLL_BYPASS		(0x58)
#define O_PMU_MUX_AS_TEST_SEL		(0x5C)
#define O_PMU_MUX_SEL0			(0x60)
#define O_PMU_MUX_SEL1			(0x64)
#define O_PMU_MUX_SEL2			(0x68)
#define O_PMU_MUX_SEL3			(0x6C)
#define O_PMU_MUX_SEL4			(0x70)
#define O_PMU_MUX_SEL5			(0x74)
#define O_PMU_MUX_SEL6			(0x78)
/* Reseved 				(0x7C) */
#define O_PMU_MASK_CLKS0		(0x80)
#define O_PMU_MASK_CLKS1		(0x84)
#define O_PMU_MASK_CLKS2		(0x88)
#define O_PMU_MASK_CLKS3		(0x8C)
#define O_PMU_MASK_CLKS4		(0x90)
#define O_PMU_SW_RESET0			(0x94)
#define O_PMU_SW_RESET1			(0x98)
#define O_PMU_AE_SWRST			(0x9C)

/* define for 'C' */
#define R_PMU_PLL0_PMS_CON		VA_PMU(O_PMU_PLL0_PMS_CON)
#define R_PMU_PLL1_PMS_CON		VA_PMU(O_PMU_PLL1_PMS_CON)
#define R_PMU_PLL2_PMS_CON		VA_PMU(O_PMU_PLL2_PMS_CON)
#define R_PMU_PLL3_PMS_CON		VA_PMU(O_PMU_PLL3_PMS_CON)
#define R_PMU_PLL4_PMS_CON		VA_PMU(O_PMU_PLL4_PMS_CON)
#define R_PMU_PLL5_PMS_CON		VA_PMU(O_PMU_PLL5_PMS_CON)
#define R_PMU_PLL6_PMS_CON		VA_PMU(O_PMU_PLL6_PMS_CON)
/* Reseved 				VA_PMU( 	) */
#define R_PMU_PLL3_K_CON		VA_PMU(O_PMU_PLL3_K_CON)
#define R_PMU_PLL4_K_CON		VA_PMU(O_PMU_PLL4_K_CON)
#define R_PMU_PLL5_K_CON		VA_PMU(O_PMU_PLL5_K_CON)
#define R_PMU_PLL6_CTRL			VA_PMU(O_PMU_PLL6_CTRL)
#define R_PMU_PLL0_LOCKCNT		VA_PMU(O_PMU_PLL0_LOCKCNT)
#define R_PMU_PLL1_LOCKCNT		VA_PMU(O_PMU_PLL1_LOCKCNT)
#define R_PMU_PLL2_LOCKCNT		VA_PMU(O_PMU_PLL2_LOCKCNT)
#define R_PMU_PLL3_LOCKCNT		VA_PMU(O_PMU_PLL3_LOCKCNT)
#define R_PMU_PLL4_LOCKCNT		VA_PMU(O_PMU_PLL4_LOCKCNT)
#define R_PMU_PLL5_LOCKCNT		VA_PMU(O_PMU_PLL5_LOCKCNT)
#define R_PMU_PLL6_LOCKCNT		VA_PMU(O_PMU_PLL6_LOCKCNT)
/* Reseved 				VA_PMU( 	) */
#define R_PMU_PLL_LOCK			VA_PMU(O_PMU_PLL_LOCK)
#define R_PMU_PLL_PWD			VA_PMU(O_PMU_PLL_PWD)
#define R_PMU_PLL_BYPASS		VA_PMU(O_PMU_PLL_BYPASS)
#define R_PMU_MUX_AS_TEST_SEL		VA_PMU(O_PMU_MUX_AS_TEST_SEL)
#define R_PMU_MUX_SEL0			VA_PMU(O_PMU_MUX_SEL0)
#define R_PMU_MUX_SEL1			VA_PMU(O_PMU_MUX_SEL1)
#define R_PMU_MUX_SEL2			VA_PMU(O_PMU_MUX_SEL2)
#define R_PMU_MUX_SEL3			VA_PMU(O_PMU_MUX_SEL3)
#define R_PMU_MUX_SEL4			VA_PMU(O_PMU_MUX_SEL4)
#define R_PMU_MUX_SEL5			VA_PMU(O_PMU_MUX_SEL5)
#define R_PMU_MUX_SEL6			VA_PMU(O_PMU_MUX_SEL6)
/* Reseved 				VA_PMU( 	) */
#define R_PMU_MASK_CLKS0		VA_PMU(O_PMU_MASK_CLKS0)
#define R_PMU_MASK_CLKS1		VA_PMU(O_PMU_MASK_CLKS1)
#define R_PMU_MASK_CLKS2		VA_PMU(O_PMU_MASK_CLKS2)
#define R_PMU_MASK_CLKS3		VA_PMU(O_PMU_MASK_CLKS3)
#define R_PMU_MASK_CLKS4		VA_PMU(O_PMU_MASK_CLKS4)
#define R_PMU_SW_RESET0			VA_PMU(O_PMU_SW_RESET0)
#define R_PMU_SW_RESET1			VA_PMU(O_PMU_SW_RESET1)
#define R_PMU_AE_SWRST			VA_PMU(O_PMU_AE_SWRST)

#define PMU_PLL_P_VALUE(x)	((x >> 20) & 0x3F)
#define PMU_PLL_M_VALUE(x)	((x >> 8) & 0x3FF)
#define PMU_PLL_S_VALUE(x)	(x & 7)
#define PMU_PLL_K_VALUE(x)	(x & 0xFFFF)

#define GET_PLL_M(x)		PMU_PLL_M_VALUE(x)
#define GET_PLL_P(x)		PMU_PLL_P_VALUE(x)
#define GET_PLL_S(x)		PMU_PLL_S_VALUE(x)

#define REQ_FCLK	1		/* CPU Clock */
#define REQ_DCLK	2		/* DDR Clock */
#define REQ_HCLK	3		/* AHB Clock */
#define REQ_PCLK	REQ_HCLK	/* APB Clock */



/* UART */
/* Don't need register define */
/* Need Physical Base address */
#define VA_UART(dt,  n, offset)	\
    (*(volatile dt *)(VA_UART_BASE+(0x40*n)+(offset)))

#define VA_UART_0(offset)	VA_UART(unsigned, 0, offset)
#define VA_UART_1(offset)	VA_UART(unsigned, 1, offset)
#define VA_UART_2(offset)	VA_UART(unsigned, 2, offset)

#define VA_UART_0_B(offset)	VA_UART(char, 0, offset)
#define VA_UART_1_B(offset)	VA_UART(char, 1, offset)
#define VA_UART_2_B(offset)	VA_UART(char, 2, offset)

#define R_ULCON0    		VA_UART_0(0x00) /* UART 0 Line control */
#define R_UCON0     		VA_UART_0(0x04) /* UART 0 Control */
#define R_UFCON0    		VA_UART_0(0x08) /* UART 0 FIFO control */
#define R_UMCON0    		VA_UART_0(0x0c) /* UART 0 Modem control */
#define R_UTRSTAT0  		VA_UART_0(0x10) /* UART 0 Tx/Rx status */
#define R_UERSTAT0  		VA_UART_0(0x14) /* UART 0 Rx error status */
#define R_UFSTAT0   		VA_UART_0(0x18) /* UART 0 FIFO status */
#define R_UMSTAT0   		VA_UART_0(0x1c) /* UART 0 Modem status */
#define R_UBRDIV0   		VA_UART_0(0x28) /* UART 0 Baud rate divisor */

#define R_ULCON1    		VA_UART_1(0x00) /* UART 1 Line control */
#define R_UCON1     		VA_UART_1(0x04) /* UART 1 Control */
#define R_UFCON1    		VA_UART_1(0x08) /* UART 1 FIFO control */
#define R_UMCON1    		VA_UART_1(0x0c) /* UART 1 Modem control */
#define R_UTRSTAT1  		VA_UART_1(0x10) /* UART 1 Tx/Rx status */
#define R_UERSTAT1  		VA_UART_1(0x14) /* UART 1 Rx error status */
#define R_UFSTAT1   		VA_UART_1(0x18) /* UART 1 FIFO status */
#define R_UMSTAT1   		VA_UART_1(0x1c) /* UART 1 Modem status */
#define R_UBRDIV1   		VA_UART_1(0x28) /* UART 1 Baud rate divisor */

#define R_ULCON2    		VA_UART_2(0x00) /* UART 2 Line control */
#define R_UCON2     		VA_UART_2(0x04) /* UART 2 Control */
#define R_UFCON2    		VA_UART_2(0x08) /* UART 2 FIFO control */
#define R_UMCON2    		VA_UART_2(0x0c) /* UART 2 Modem control */
#define R_UTRSTAT2  		VA_UART_2(0x10) /* UART 2 Tx/Rx status */
#define R_UERSTAT2  		VA_UART_2(0x14) /* UART 2 Rx error status */
#define R_UFSTAT2   		VA_UART_2(0x18) /* UART 2 FIFO status */
#define R_UMSTAT2   		VA_UART_2(0x1c) /* UART 2 Modem status */
#define R_UBRDIV2   		VA_UART_2(0x28) /* UART 2 Baud rate divisor */

#ifdef __BIG_ENDIAN
#define R_UTXH0 		VA_UART_0_B(0x23) /* UART 0 Transmission Hold */
#define R_URXH0 		VA_UART_0_B(0x27) /* UART 0 Receive buffer */
#else /* Little Endian */
#define R_UTXH0			VA_UART_0_B(0x20) /* UART 0 Transmission Hold */
#define R_URXH0			VA_UART_0_B(0x24) /* UART 0 Receive buffer */
#endif

#ifdef __BIG_ENDIAN
#define R_UTXH1			VA_UART_1_B(0x23) /* UART 1 Transmission Hold */
#define R_URXH1			VA_UART_1_B(0x27) /* UART 1 Receive buffer */
#else /* Little Endian */
#define R_UTXH1			VA_UART_1_B(0x20) /* UART 1 Transmission Hold */
#define R_URXH1			VA_UART_1_B(0x24) /* UART 1 Receive buffer */
#endif

#ifdef __BIG_ENDIAN
#define R_UTXH2			VA_UART_2_B(0x23) /* UART 2 Transmission Hold */
#define R_URXH2			VA_UART_2_B(0x27) /* UART 2 Receive buffer */
#else /* Little Endian */
#define R_UTXH2			VA_UART_2_B(0x20) /* UART 2 Transmission Hold */
#define R_URXH2			VA_UART_2_B(0x24) /* UART 2 Receive buffer */
#endif

/* timer */
#define VA_TIMER(t, offset)	\
    (*(volatile unsigned *)(VA_TIMER_BASE+(0x10*t)+(offset)))

#define R_TMDMASEL		VA_TIMER(0, 0x0c) /* dma or interrupt mode selection */

#define VA_TIMER_0(offset)	VA_TIMER(0, offset)
#define VA_TIMER_1(offset)	VA_TIMER(1, offset)
#define VA_TIMER_2(offset)	VA_TIMER(2, offset)

#define R_TMCON0		VA_TIMER_0(0x00)
#define R_TMDATA0		VA_TIMER_0(0x04)
#define R_TMCNT0		VA_TIMER_0(0x08)

#define R_TMCON1		VA_TIMER_1(0x00)
#define R_TMDATA1		VA_TIMER_1(0x04)
#define R_TMCNT1		VA_TIMER_1(0x08)

#define R_TMCON2		VA_TIMER_2(0x00)
#define R_TMDATA2		VA_TIMER_2(0x04)
#define R_TMCNT2		VA_TIMER_2(0x08)

#define R_TMSTATUS		VA_TIMER_0(0x30)

#define TMCON_MUX04		(0x0 << 2)
#define TMCON_MUX08		(0x1 << 2)
#define TMCON_MUX16		(0x2 << 2)
#define TMCON_MUX32		(0x3 << 2)

#define TMCON_INT_DMA_EN	(0x1 << 1)
#define TMCON_RUN		(0x1)

#define TMDATA_PRES(x)	(x > 0) ? ((x & 0xFF) << 16) : 1


/* watch dog timer */
#define VA_WDT(offset) (*(volatile unsigned *)(VA_WDT_BASE+(offset)))

#define R_WTPSV			VA_WDT(0x00) /* Watch-dog timer prescaler value */
#define R_WTCON			VA_WDT(0x04) /* Watch-dog timer mode */
#define R_WTCNT			VA_WDT(0x08) /* Eatch-dog timer count */

/*-----------------------------------------------------*/
/* PAD Controller */
#define VA_PADCTRL_PORT(offset)  (*(volatile unsigned *)(VA_PADCTRL_BASE+(offset)))

/* PCI */
#define VA_PCI_CONF(offset)  (*(volatile unsigned *)(VA_PCI_BASE+(offset)))
#define R_PCIHID		VA_PCI_CONF(0x00)
#define R_PCIHSC		VA_PCI_CONF(0x04)
#define R_PCIHCODE		VA_PCI_CONF(0x08)
#define R_PCIHLINE		VA_PCI_CONF(0x0C)
#define R_PCIHBAR0		VA_PCI_CONF(0x10)
#define R_PCIHBAR1		VA_PCI_CONF(0x14)
#define R_PCIHBAR2		VA_PCI_CONF(0x18)
/*Reserved */
#define R_PCIHCISP		VA_PCI_CONF(0x28)
#define R_PCIHSSID		VA_PCI_CONF(0x2C)
/*Reserved */
#define R_PCIHCAP		VA_PCI_CONF(0x34)
/*Reserved */
#define R_PCIHLTIT		VA_PCI_CONF(0x3C)
#define R_PCIHTIMER		VA_PCI_CONF(0x40)
/*Reserved */
#define R_PCIHPMR0		VA_PCI_CONF(0xDC)
#define R_PCIHPMR1		VA_PCI_CONF(0xE0)

#define VA_PCI_BIF(offset)  (*(volatile unsigned *)(VA_PCI_BASE+(offset + 0x100)))
#define R_PCICON		VA_PCI_BIF(0x00)
#define R_PCISET		VA_PCI_BIF(0x04)
#define R_PCIINTEN		VA_PCI_BIF(0x08)
#define R_PCIINTST		VA_PCI_BIF(0x0C)
#define R_PCIINTAD		VA_PCI_BIF(0x10)
#define R_PCIBATAPM		VA_PCI_BIF(0x14)
#define R_PCIBATAPI		VA_PCI_BIF(0x18)
#define R_PCIRCC		VA_PCI_BIF(0x1C)
#define R_PCIDIAG0		VA_PCI_BIF(0x20)
#define R_PCIDIAG1		VA_PCI_BIF(0x24)
#define R_PCIBELAP		VA_PCI_BIF(0x28)
#define R_PCIBELPA		VA_PCI_BIF(0x2C)
#define R_PCIMAIL0		VA_PCI_BIF(0x30)
#define R_PCIMAIL1		VA_PCI_BIF(0x34)
#define R_PCIMAIL2		VA_PCI_BIF(0x38)
#define R_PCIMAIL3		VA_PCI_BIF(0x3C)
#define R_PCIBATPA0		VA_PCI_BIF(0x40)
#define R_PCIBAM0		VA_PCI_BIF(0x44)
#define R_PCIBATPA1		VA_PCI_BIF(0x48)
#define R_PCIBAM1		VA_PCI_BIF(0x4C)
#define R_PCIBATPA2		VA_PCI_BIF(0x50)
#define R_PCIBAM2		VA_PCI_BIF(0x54)
#define R_PCISWAP		VA_PCI_BIF(0x58)
/* Reserved */
#define R_PCIDMACON0		VA_PCI_BIF(0x80)
#define R_PCIDMASRC0		VA_PCI_BIF(0x84)
#define R_PCIDMADST0		VA_PCI_BIF(0x88)
#define R_PCIDMACNT0		VA_PCI_BIF(0x8C)
#define R_PCIDMARUN0		VA_PCI_BIF(0x90)
/* Reserved */
#define R_PCIDMACON1		VA_PCI_BIF(0xA0)
#define R_PCIDMASRC1		VA_PCI_BIF(0xA4)
#define R_PCIDMADST1		VA_PCI_BIF(0xA8)
#define R_PCIDMACNT1		VA_PCI_BIF(0xAC)
#define R_PCIDMARUN1		VA_PCI_BIF(0xB0)



#ifndef MHZ /* define MHZ */
#define MHZ		1000000
#endif

#endif /* __SDP83_H */
