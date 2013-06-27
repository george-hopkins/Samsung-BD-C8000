/*
 * sdp92.h : defined SoC SFRs
 *
 * Copyright (C) 2009-2013 Samsung Electronics
 * Ikjoon Jang <ij.jang@samsung.com>
 *
 */

#ifndef _SDP92_H_
#define _SDP92_H_

#define VA_IO_BASE0           (0xF8000000)
#define PA_IO_BASE0           (0x30000000)
#define DIFF_IO_BASE0	      (VA_IO_BASE0 - PA_IO_BASE0)

/* Static Memory Controller Register */
#define PA_SMC_BASE           (0x30028000)
#define VA_SMC_BASE           (PA_SMC_BASE + DIFF_IO_BASE0)

/* NAND Memory Controller Register */
#define PA_NAND_BASE           (0x30020000)
#define VA_NAND_BASE           (PA_NAND_BASE + DIFF_IO_BASE0)

/* Power Management unit & PLL Register */
#define PA_PMU_BASE           (0x30090800)
#define VA_PMU_BASE           (PA_PMU_BASE + DIFF_IO_BASE0)

/* Pad Control Register */
#define PA_PADCTRL_BASE       (0x30090C00)
#define VA_PADCTRL_BASE       (PA_GPIO_BASE + DIFF_IO_BASE0)

/* Timer Register */
#define PA_TIMER_BASE         (0x30090400)
#define VA_TIMER_BASE         (PA_TIMER_BASE + DIFF_IO_BASE0)

/* UART Register */
#define PA_UART_BASE          (0x30090A00)
#define VA_UART_BASE          (PA_UART_BASE + DIFF_IO_BASE0)

/* Interrupt controller register */
#define PA_INTC_BASE          (0x30090F00)
#define VA_INTC_BASE          (PA_INTC_BASE + DIFF_IO_BASE0)

/* Watchdog Register */
#define PA_WDT_BASE           (0x30090600)
#define VA_WDT_BASE           (PA_WDT_BASE + DIFF_IO_BASE0)

/* PCI PHY */
/* TODO */

/* USB EHCI0 host controller register */
#define PA_EHCI0_BASE         (0x30070000)
#define VA_EHCI0_BASE         (PA_EHCI0_BASE + DIFF_IO_BASE0)

/* USB EHCI1 host controller register */
#define PA_EHCI1_BASE         (0x30080000)
#define VA_EHCI1_BASE         (PA_EHCI1_BASE + DIFF_IO_BASE0)

/* USB OHCI0 host controller register */
#define PA_OHCI0_BASE         (0x30078000)
#define VA_OHCI0_BASE         (PA_OHCI0_BASE + DIFF_IO_BASE0)

/* USB OHCI1 host controller register */
#define PA_OHCI1_BASE         (0x30088000)
#define VA_OHCI1_BASE         (PA_OHCI1_BASE + DIFF_IO_BASE0)

/* end of SFR_BASE */

/* SMC Register */
#define VA_SMC(offset)  (*(volatile unsigned *)(VA_SMC_BASE+(offset)))

#define VA_SMC_BANK(bank, offset)  (*(volatile unsigned *)(VA_SMC_BASE+(bank)+(offset)))

#define O_SMC_BANK0		0x48
#define O_SMC_BANK1		0x24
#define O_SMC_BANK2		0x00
#define O_SMC_BANK3		0x6c

#define VA_SMC_BANK0(offset)  	VA_SMC_BANK(O_SMC_BANK0, offset)
#define VA_SMC_BANK1(offset)  	VA_SMC_BANK(O_SMC_BANK1, offset)
#define VA_SMC_BANK2(offset)  	VA_SMC_BANK(O_SMC_BANK2, offset)
#define VA_SMC_BANK3(offset)  	VA_SMC_BANK(O_SMC_BANK3, offset)

#define O_SMC_IDCYR		(0x00)
#define O_SMC_WST1		(0x04)
#define O_SMC_WST2		(0x08)
#define O_SMC_WSTOEN		(0x0C)
#define O_SMC_WSTWEN		(0x10)
#define O_SMC_CR		(0x14)
#define O_SMC_SR		(0x18)
#define O_SMC_CIWRCON		(0x1C)
#define O_SMC_CIRDCON		(0x20)

#define O_SMC_WST1_BK0		0x148
#define O_SMC_WST3_BK1		0x144
#define O_SMC_WST3_BK2		0x140
#define O_SMC_WST3_BK3		0x14c

#define R_SMC_WST3_BK0		VA_SMC(O_SMC_WST3_BK0)
#define R_SMC_WST3_BK1		VA_SMC(O_SMC_WST3_BK1)
#define R_SMC_WST3_BK2		VA_SMC(O_SMC_WST3_BK2)
#define R_SMC_WST3_BK3		VA_SMC(O_SMC_WST3_BK3)

/* bank 0 */
#define R_SMC_IDCY_BK0		VA_SMC_BANK0(O_SMC_SMBIDCYR)
#define R_SMC_WST1_BK0		VA_SMC_BANK0(O_SMC_SMBWST1R)
#define R_SMC_WST2_BK0		VA_SMC_BANK0(O_SMC_SMBWST2R)
#define R_SMC_WSTOEN_BK0	VA_SMC_BANK0(O_SMC_SMBWSTOENR)
#define R_SMC_WSTWEN_BK0	VA_SMC_BANK0(O_SMC_SMBWSTWENR)
#define R_SMC_CR_BK0		VA_SMC_BANK0(O_SMC_SMBCR)
#define R_SMC_SR_BK0		VA_SMC_BANK0(O_SMC_SMBSR)
#define R_SMC_CIWRCON_BK0	VA_SMC_BANK0(O_SMC_CIWRCON)
#define R_SMC_CIRDCON_BK0	VA_SMC_BANK0(O_SMC_CIRDCON)

/* bank 1 */
#define R_SMC_IDCY_BK1		VA_SMC_BANK1(O_SMC_SMBIDCYR)
#define R_SMC_WST1_BK1		VA_SMC_BANK1(O_SMC_SMBWST1R)
#define R_SMC_WST2_BK1		VA_SMC_BANK1(O_SMC_SMBWST2R)
#define R_SMC_WSTOEN_BK1	VA_SMC_BANK1(O_SMC_SMBWSTOENR)
#define R_SMC_WSTWEN_BK1	VA_SMC_BANK1(O_SMC_SMBWSTWENR)
#define R_SMC_CR_BK1		VA_SMC_BANK1(O_SMC_SMBCR)
#define R_SMC_SR_BK1		VA_SMC_BANK1(O_SMC_SMBSR)
#define R_SMC_CIWRCON_BK1	VA_SMC_BANK1(O_SMC_CIWRCON)
#define R_SMC_CIRDCON_BK1	VA_SMC_BANK1(O_SMC_CIRDCON)

/* bank 2 */
#define R_SMC_IDCY_BK2		VA_SMC_BANK2(O_SMC_SMBIDCYR)
#define R_SMC_WST1_BK2		VA_SMC_BANK2(O_SMC_SMBWST1R)
#define R_SMC_WST2_BK2		VA_SMC_BANK2(O_SMC_SMBWST2R)
#define R_SMC_WSTOEN_BK2	VA_SMC_BANK2(O_SMC_SMBWSTOENR)
#define R_SMC_WSTWEN_BK2	VA_SMC_BANK2(O_SMC_SMBWSTWENR)
#define R_SMC_CR_BK2		VA_SMC_BANK2(O_SMC_SMBCR)
#define R_SMC_SR_BK2		VA_SMC_BANK2(O_SMC_SMBSR)
#define R_SMC_CIWRCON_BK2	VA_SMC_BANK2(O_SMC_CIWRCON)
#define R_SMC_CIRDCON_BK2	VA_SMC_BANK2(O_SMC_CIRDCON)

/* bank 3 */
#define R_SMC_IDCY_BK3		VA_SMC_BANK3(O_SMC_SMBIDCYR)
#define R_SMC_WST1_BK3		VA_SMC_BANK3(O_SMC_SMBWST1R)
#define R_SMC_WST2_BK3		VA_SMC_BANK3(O_SMC_SMBWST2R)
#define R_SMC_WSTOEN_BK3	VA_SMC_BANK3(O_SMC_SMBWSTOENR)
#define R_SMC_WSTWEN_BK3	VA_SMC_BANK3(O_SMC_SMBWSTWENR)
#define R_SMC_CR_BK3		VA_SMC_BANK3(O_SMC_SMBCR)
#define R_SMC_SR_BK3		VA_SMC_BANK3(O_SMC_SMBSR)
#define R_SMC_CIWRCON_BK3	VA_SMC_BANK3(O_SMC_CIWRCON)
#define R_SMC_CIRDCON_BK3	VA_SMC_BANK3(O_SMC_CIRDCON)

#define R_SMC_SMBEWS		VA_SMC(0x120)
#define R_SMC_CI_RESET		VA_SMC(0x128)
#define R_SMC_CI_ADDRSEL	VA_SMC(0x12c)
#define R_SMC_CI_CNTL		VA_SMC(0x130)
#define R_SMC_CI_REGADDR	VA_SMC(0x134)
#define R_SMC_PERIPHID0		VA_SMC(0xFE0)
#define R_SMC_PERIPHID1		VA_SMC(0xFE4)
#define R_SMC_PERIPHID2		VA_SMC(0xFE8)
#define R_SMC_PERIPHID3		VA_SMC(0xFEC)
#define R_SMC_PCELLID0		VA_SMC(0xFF0)
#define R_SMC_PCELLID1		VA_SMC(0xFF4)
#define R_SMC_PCELLID2		VA_SMC(0xFF8)
#define R_SMC_PCELLID3		VA_SMC(0xFFC)
#define R_SMC_CLKSTOP		VA_SMC(0x1e8)
#define R_SMC_SYNCEN		VA_SMC(0x1ec)

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
#define O_PMU_PLL7_PMS_CON		(0x1c)

#define O_PMU_PLL2_K_CON		(0x20)
#define O_PMU_PLL3_K_CON		(0x24)
#define O_PMU_PLL4_K_CON		(0x28)
#define O_PMU_PLL5_K_CON		(0x2c)
#define O_PMU_PLL6_K_CON		(0x30)
#define O_PMU_PLL7_K_CON		(0x34)

#define O_PMU_PLL5_CTRL			(0x38)
#define O_PMU_PLL6_CTRL			(0x3C)
#define O_PMU_PLL7_CTRL			(0x40)

#define O_PMU_PLL0_ETC			(0x44)
#define O_PMU_PLL1_ETC			(0x48)

#define O_PMU_PLL0_LOCKCNT		(0x60)
#define O_PMU_PLL1_LOCKCNT		(0x64)
#define O_PMU_PLL2_LOCKCNT		(0x68)
#define O_PMU_PLL3_LOCKCNT		(0x6c)
#define O_PMU_PLL4_LOCKCNT		(0x70)
#define O_PMU_PLL5_LOCKCNT		(0x74)
#define O_PMU_PLL6_LOCKCNT		(0x78)
#define O_PMU_PLL7_LOCKCNT		(0x7c)

#define O_PMU_PLL_LOCK			(0x80)
#define O_PMU_PLL_PWD			(0x84)
#define O_PMU_PLL_BYPASS		(0x88)
#define O_PMU_PLL_PLLLOCK		(0x8c)

#define O_PMU_PRESCL_ON			(0xa0)
#define O_PMU_PRESCL_DIV		(0xa4)

#define O_PMU_MUX_SEL0			(0xa8)
#define O_PMU_MUX_SEL1			(0xac)
#define O_PMU_MUX_SEL2			(0xb0)
#define O_PMU_MUX_SEL3			(0xb4)
#define O_PMU_MUX_SEL4			(0xb8)
#define O_PMU_MUX_SEL5			(0xbc)
#define O_PMU_MUX_SEL6			(0xc0)
#define O_PMU_MUX_SEL7			(0xc4)

#define O_PMU_MASK_CLKS0		(0xd0)
#define O_PMU_MASK_CLKS1		(0xd4)
#define O_PMU_MASK_CLKS2		(0xd8)
#define O_PMU_MASK_CLKS3		(0xdc)

#define O_PMU_SW_RESET0			(0xe0)
#define O_PMU_SW_RESET1			(0xe4)
#define O_PMU_SYS_SET			(0xec)
#define O_PMU_QPI			(0xf0)

/* define for 'C' */
#define R_PMU_PLL0_PMS_CON	VA_PMU(O_PMU_PLL0_PMS_CON)
#define R_PMU_PLL1_PMS_CON	VA_PMU(O_PMU_PLL1_PMS_CON)
#define R_PMU_PLL2_PMS_CON	VA_PMU(O_PMU_PLL2_PMS_CON)
#define R_PMU_PLL3_PMS_CON	VA_PMU(O_PMU_PLL3_PMS_CON)
#define R_PMU_PLL4_PMS_CON	VA_PMU(O_PMU_PLL4_PMS_CON)
#define R_PMU_PLL5_PMS_CON	VA_PMU(O_PMU_PLL5_PMS_CON)
#define R_PMU_PLL6_PMS_CON	VA_PMU(O_PMU_PLL6_PMS_CON)
#define R_PMU_PLL7_PMS_CON	VA_PMU(O_PMU_PLL7_PMS_CON)

#define R_PMU_PLL2_K_CON	VA_PMU(O_PMU_PLL2_K_CON)
#define R_PMU_PLL3_K_CON	VA_PMU(O_PMU_PLL3_K_CON)
#define R_PMU_PLL4_K_CON	VA_PMU(O_PMU_PLL4_K_CON)
#define R_PMU_PLL5_K_CON	VA_PMU(O_PMU_PLL5_K_CON)
#define R_PMU_PLL6_K_CON	VA_PMU(O_PMU_PLL6_K_CON)
#define R_PMU_PLL7_K_CON	VA_PMU(O_PMU_PLL7_K_CON)

#define R_PMU_PLL5_CTRL		VA_PMU(O_PMU_PLL5_CTRL)
#define R_PMU_PLL6_CTRL		VA_PMU(O_PMU_PLL6_CTRL)
#define R_PMU_PLL7_CTRL		VA_PMU(O_PMU_PLL7_CTRL)

#define R_PMU_PLL0_ETC		VA_PMU(O_PMU_PLL0_ETC)
#define R_PMU_PLL1_ETC		VA_PMU(O_PMU_PLL1_ETC)

#define R_PMU_PLL0_LOCKCNT	VA_PMU(O_PMU_PLL0_LOCKCNT)
#define R_PMU_PLL1_LOCKCNT	VA_PMU(O_PMU_PLL1_LOCKCNT)
#define R_PMU_PLL2_LOCKCNT	VA_PMU(O_PMU_PLL2_LOCKCNT)
#define R_PMU_PLL3_LOCKCNT	VA_PMU(O_PMU_PLL3_LOCKCNT)
#define R_PMU_PLL4_LOCKCNT	VA_PMU(O_PMU_PLL4_LOCKCNT)
#define R_PMU_PLL5_LOCKCNT	VA_PMU(O_PMU_PLL5_LOCKCNT)
#define R_PMU_PLL6_LOCKCNT	VA_PMU(O_PMU_PLL6_LOCKCNT)
#define R_PMU_PLL7_LOCKCNT	VA_PMU(O_PMU_PLL7_LOCKCNT)

#define R_PMU_PLL_LOCK		VA_PMU(O_PMU_PLL_LOCK)
#define R_PMU_PLL_PWD		VA_PMU(O_PMU_PLL_PWD)
#define R_PMU_PLL_BYPASS	VA_PMU(O_PMU_PLL_BYPASS)
#define R_PMU_PLL_PLLLOCK	VA_PMU(O_PMU_PLL_PLLLOCK)

#define R_PMU_PRESCL_ON		VA_PMU(O_PMU_PRESCL_ON)
#define R_PMU_PRESCL_DIV	VA_PMU(O_PMU_PRESCL_DIV)

#define R_PMU_MUX_SEL0		VA_PMU(O_PMU_MUX_SEL0)
#define R_PMU_MUX_SEL1		VA_PMU(O_PMU_MUX_SEL1)
#define R_PMU_MUX_SEL2		VA_PMU(O_PMU_MUX_SEL2)
#define R_PMU_MUX_SEL3		VA_PMU(O_PMU_MUX_SEL3)
#define R_PMU_MUX_SEL4		VA_PMU(O_PMU_MUX_SEL4)
#define R_PMU_MUX_SEL5		VA_PMU(O_PMU_MUX_SEL5)
#define R_PMU_MUX_SEL6		VA_PMU(O_PMU_MUX_SEL6)
#define R_PMU_MUX_SEL7		VA_PMU(O_PMU_MUX_SEL7)

#define R_PMU_MASK_CLKS0	VA_PMU(O_PMU_MASK_CLKS0)
#define R_PMU_MASK_CLKS1	VA_PMU(O_PMU_MASK_CLKS1)
#define R_PMU_MASK_CLKS2	VA_PMU(O_PMU_MASK_CLKS2)
#define R_PMU_MASK_CLKS3	VA_PMU(O_PMU_MASK_CLKS3)

#define R_PMU_SW_RESET0		VA_PMU(O_PMU_SW_RESET0)
#define R_PMU_SW_RESET1		VA_PMU(O_PMU_SW_RESET1)
#define R_PMU_SYS_SET		VA_PMU(O_PMU_SYS_SET)
#define R_PMU_QPI		VA_PMU(O_PMU_QPI)

#define PMU_PLL_P_VALUE(x)	((x >> 20) & 0x3F)
#define PMU_PLL_M_VALUE(x)	((x >> 8) & 0x3FF)
#define PMU_PLL_S_VALUE(x)	(x & 7)
#define PMU_PLL_K_VALUE(x)	(x & 0xFFFF)

#define GET_PLL_M(x)		PMU_PLL_M_VALUE(x)
#define GET_PLL_P(x)		PMU_PLL_P_VALUE(x)
#define GET_PLL_S(x)		PMU_PLL_S_VALUE(x)

#define REQ_FCLK	1		/* CPU Clock */
#define REQ_DCLK	2		/* DDR Clock */
#define REQ_BUSCLK	3		/* BUS Clock */
#define REQ_HCLK	4		/* AHB Clock */
#define REQ_PCLK	5		/* APB Clock */

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

#define O_WTPSV			0x00
#define O_WTCON			0x04
#define O_WTCNT			0x08

#define R_WTPSV			VA_WDT(O_WTPSV)
#define R_WTCON			VA_WDT(O_WTCON)
#define R_WTCNT			VA_WDT(O_WTCNT)

#endif

