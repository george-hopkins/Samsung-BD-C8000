/*
 * linux/include/asm-arm/arch-ssdtv/leonid.h
 *
 *  Copyright (C) 2003-2008 Samsung Electronics
 *  Author: tukho.kim@samsung.com 	06/19/2008
 *
 */

#ifndef __LEONID_H
#define __LEONID_H

//#include <asm/arch/param.h>
#include <asm/arch/irqs.h>

#include <asm/arch/sdp83.h>

/* clock parameters, see clocks.h */
#define INPUT_FREQ			27000000
#define FIN				INPUT_FREQ

#define FCLK				600000000 
#define DCLK				400000000 
#define BCLK				280000000
#define HCLK				(BCLK >> 1)
#define PCLK				(BCLK >> 1)  

/* system timer tick or high-resolution timer*/
#define SYS_TICK				HZ

#define SYS_TIMER				0

#define SYS_TIMER_BIT				(1 << SYS_TIMER)
#define SYS_TIMER_IRQ				(IRQ_TIMER)
#define ARCH_TIMER_IRQ				(IRQ_TIMER)

#define R_SYSTMCON				VA_TIMER(SYS_TIMER, 0x00)
#define R_SYSTMDATA				VA_TIMER(SYS_TIMER, 0x04)
#define R_SYSTMCNT				VA_TIMER(SYS_TIMER, 0x08)

// don't use this define when using hrtimer 
// 140Mhz -> 140,000,000
// 100Hz tick -> 1,400,000 * 100
// 1/4 timer -> 350,000
// 4 * 5 * 7 * 2500
#define SYS_TIMER_PRESCALER	 	6	

// 20080523 HRTimer (clock source) define
#if defined(CONFIG_GENERIC_TIME) || defined(CONFIG_GERNERIC_COCKEVENTS)
#define SDP_TIMER_TICK			500000	// 500khz
#define CLKSRC_TIMER				2

#define CLKSRC_TIMER_BIT			(1 << CLKSRC_TIMER)

#define R_CLKSRCTMCON				VA_TIMER(CLKSRC_TIMER, 0x00)
#define R_CLKSRCTMDATA				VA_TIMER(CLKSRC_TIMER, 0x04)
#define R_CLKSRCTMCNT				VA_TIMER(CLKSRC_TIMER, 0x08)
#endif
// 20080523 HRTimer (clock source) define end
/* system timer tick or high-resolution timer end */ 

/* Memory Define */
/* Move to include/asm-arm/arch-ssdtv/memory.h */
#include <asm/arch/memory.h>

/* System Kernel memory size */
#define KERNEL_MEM_SIZE			(SYS_MEM0_SIZE + SYS_MEM1_SIZE)	// 100MByte

/* flash chip locations(SMC banks) */
#define PA_FLASH_BANK0			(0x0)
#define PA_FLASH_BANK1			(0x04000000)
#define FLASH_MAXSIZE			(32 << 20)		

// UART definition 
#define SDP_UART_NR			(3)

#define SER_BAUD_9600			(9600)
#define SER_BAUD_19200			(19200)
#define SER_BAUD_38400			(38400)
#define SER_BAUD_57600			(57600)
#define SER_BAUD_115200			(115200)
#define CURRENT_BAUD_RATE		(SER_BAUD_115200)

#define UART_CLOCK			(REQ_PCLK)
#define SDP_GET_UARTCLK(x)		sdp83_get_clock(x)

// timer definition 
#define TIMER_CLOCK			(REQ_PCLK)
#define SDP_GET_TIMERCLK(x)		sdp83_get_clock(x)

#if 0
/* CS8900 external interrupt*/
#define ETH_BASE 				0xD1000300
#define ETH_INT_NUM				IRQ_EXT0	/* IRQ_EXT0 */
#define ETH_EXT_INT				0
#define ETH_INT_TYPE				0		/* EXT_INT_CON : low-level*/
#endif

// define interrupt resource definition 
#define SDP_INTERRUPT_RESOURCE \
/*irtSrc, qSrc,           level,       polarity,    priority,   sub priority        */ \
{{0,    IRQ_TSD,      	LEVEL_LEVEL, POLARITY_HIGH, 0xFFFFFFFF, 0xFFFFFFFF}, \
 {1,    IRQ_AIO,      	LEVEL_LEVEL, POLARITY_HIGH, 0xFFFFFFFE, 0xFFFFFFFF}, \
 {2,    IRQ_AE,       	LEVEL_LEVEL, POLARITY_HIGH, 0xFFFFFFFC, 0xFFFFFFFF}, \
 {3,    IRQ_MPGE0,    	LEVEL_LEVEL, POLARITY_HIGH, 0xFFFFFFF8, 0xFFFFFFFF}, \
 {4,    IRQ_MPEG1,    	LEVEL_LEVEL, POLARITY_HIGH, 0xFFFFFFF0, 0xFFFFFFFF}, \
 {5,    IRQ_DISP,     	LEVEL_LEVEL, POLARITY_HIGH, 0xFFFFFFE0, 0xFFFFFFFF}, \
 {6,    IRQ_GA2D,     	LEVEL_LEVEL, POLARITY_HIGH, 0xFFFFFFC0, 0xFFFFFFFF}, \
 {7,    IRQ_GA3D,     	LEVEL_LEVEL, POLARITY_HIGH, 0xFFFFFF80, 0xFFFFFFFF}, \
 {8,    IRQ_JPEG,     	LEVEL_LEVEL, POLARITY_HIGH, 0xFFFFFF00, 0xFFFFFFFF}, \
 {9,    IRQ_MFD,      	LEVEL_LEVEL, POLARITY_HIGH, 0xFFFFFE00, 0xFFFFFFFF}, \
 {10,   IRQ_AVD,      	LEVEL_LEVEL, POLARITY_HIGH, 0xFFFFFC00, 0xFFFFFFFF}, \
 {11,   IRQ_CROAD,    	LEVEL_LEVEL, POLARITY_HIGH, 0xFFFFF800, 0xFFFFFFFF}, \
 {12,   IRQ_PCI,      	LEVEL_LEVEL, POLARITY_HIGH, 0xFFFFF000, 0xFFFFFFFF}, \
 {13,   IRQ_DMA,      	LEVEL_LEVEL, POLARITY_HIGH, 0xFFFFE000, 0xFFFFFFFF}, \
 {14,   IRQ_USB_OHCI0,	LEVEL_LEVEL, POLARITY_HIGH, 0xFFFFC000, 0xFFFFFFFF}, \
 {15,   IRQ_USB_EHCI0,	LEVEL_LEVEL, POLARITY_HIGH, 0xFFFF8000, 0xFFFFFFFF}, \
 {16,   IRQ_USB_OHCI1,	LEVEL_LEVEL, POLARITY_HIGH, 0xFFFF0000, 0xFFFFFFFF}, \
 {17,   IRQ_USB_EHCI1,	LEVEL_LEVEL, POLARITY_HIGH, 0xFFFE0000, 0xFFFFFFFF}, \
 {18,   IRQ_IRB_IRR,  	LEVEL_LEVEL, POLARITY_HIGH, 0xFFFC0000, 0xFFFFFFFF}, \
 {19,   IRQ_UART0,    	LEVEL_LEVEL, POLARITY_HIGH, 0xFFF80000, 0xFFFFFFFF}, \
 {20,   IRQ_UART1,    	LEVEL_LEVEL, POLARITY_HIGH, 0xFFF00000, 0xFFFFFFFF}, \
 {21,   IRQ_UART2,    	LEVEL_LEVEL, POLARITY_HIGH, 0xFFE00000, 0xFFFFFFFF}, \
 {22,   IRQ_SPI,      	LEVEL_LEVEL, POLARITY_HIGH, 0xFFC00000, 0xFFFFFFFF}, \
 {23,   IRQ_TIMER,    	LEVEL_EDGE,  POLARITY_HIGH, 0xFF800000, 0xFFFFFFFF}, \
 {24,   IRQ_I2C,      	LEVEL_LEVEL, POLARITY_HIGH, 0xFF000000, 0xFFFFFFFF}, \
 {25,   IRQ_SCI,      	LEVEL_LEVEL, POLARITY_HIGH, 0xFE000000, 0xFFFFFFFF}, \
 {26,   IRQ_EXT7,     	LEVEL_EDGE, POLARITY_HIGH, 0xFC000000, 0xFFFFFFFF}, \
 {27,   IRQ_NAND,     	LEVEL_LEVEL, POLARITY_HIGH, 0xF8000000, 0xFFFFFFFF}, \
 {28,   IRQ_SE,       	LEVEL_LEVEL, POLARITY_HIGH, 0xF0000000, 0xFFFFFFFF}, \
 {29,   IRQ_EXT0,     	LEVEL_EDGE, POLARITY_HIGH, 0xE0000000, 0xFFFFFFFF}, \
 {30,   IRQ_EXT1,     	LEVEL_EDGE,  POLARITY_HIGH, 0xC0000000, 0xFFFFFFFF}, \
 {31,   IRQ_SUBINT,   	LEVEL_LEVEL,  POLARITY_HIGH, 0x80000000, 0xFFFFFFFF} }

#endif /*__LEONID_H*/
