/*
 *  linux/include/asm-arm/hardware/serial_s5h2x.h
 *
 *  Internal header file for Samsung S5H2X serial ports (UART0, 1)
 *
 *  Copyright (C) 2006 Samsung Electronics.
 *
 *  Adapted from:
 *
 */
#ifndef ASM_ARM_HARDWARE_SERIAL_S5H2X_H
#define ASM_ARM_HARDWARE_SERIAL_S5H2X_H

/* register offset */
#define S5H2X_UARTLCON_OFF      (0x00)
#define S5H2X_UARTCON_OFF       (0x04)
#define S5H2X_UARTFCON_OFF      (0x08)
#define S5H2X_UARTMCON_OFF      (0x0C)
#define S5H2X_UARTTRSTAT_OFF    (0x10)
#define S5H2X_UARTERSTAT_OFF    (0x14)
#define S5H2X_UARTFSTAT_OFF     (0x18)
#define S5H2X_UARTMSTAT_OFF     (0x1C)
#define S5H2X_UARTTXH0_OFF      (0x20)
#define S5H2X_UARTRXH0_OFF      (0x24)
#define S5H2X_UARTBRDIV_OFF     (0x28)

#define S5H2X_UART_GAP		(0x40)

#define S5H2X_UART0_OFF         (0x0)
#define S5H2X_UART1_OFF         (0x40)
#define S5H2X_UART2_OFF         (0x80)
#define S5H2X_UART3_OFF         (0xC0)

#define S5H2X_LCON_CFGMASK      (0x3F)

/* LCON Word length */
#define S5H2X_LCON_CS5          (0x0)
#define S5H2X_LCON_CS6          (0x1)
#define S5H2X_LCON_CS7          (0x2)
#define S5H2X_LCON_CS8          (0x3)

/* LCON Parity */
#define S5H2X_LCON_PNONE        (0x0)
#define S5H2X_LCON_PODD         ((0x4)<<3)
#define S5H2X_LCON_PEVEN        ((0x5)<<3)


#define S5H2X_UCON_RXIRQMODE		(0x1)
#define S5H2X_UCON_RXIRQMODE_MASK	(0x3)
#define S5H2X_UCON_TXIRQMODE		(0x1 << 2)
#define S5H2X_UCON_TXIRQMODE_MASK	(0x3 << 2)
#define S5H2X_UCON_SBREAK		(1<<4)
#define S5H2X_UCON_RXFIFO_TOI	(1<<7)
// [8:9] must be 0's
#define S5H2X_UCON_CLKSEL		(0<<10)
#define S5H2X_UCON_RXIE			(1<<12)
#define S5H2X_UCON_TXIE			(1<<13)
#define S5H2X_UCON_ERIE			(1<<14)

#define S5H2X_UCON_DEFAULT	(S5H2X_UCON_TXIE   \
				| S5H2X_UCON_RXIE		\
				| S5H2X_UCON_ERIE		\
				| S5H2X_UCON_CLKSEL  \
				| S5H2X_UCON_TXIRQMODE \
				| S5H2X_UCON_RXIRQMODE \
				| S5H2X_UCON_RXFIFO_TOI)

#define S5H2X_UFCON_FIFOMODE    (1<<0)
#define S5H2X_UFCON_TXTRIG0     (0<<6)
#define S5H2X_UFCON_TXTRIG1     (1<<6)
#define S5H2X_UFCON_TXTRIG12     (3<<6)
#define S5H2X_UFCON_RXTRIG8     (1<<4)
#define S5H2X_UFCON_RXTRIG12    (2<<4)
#define S5H2X_UFCON_RXTRIG16    (3<<4)
#define S5H2X_UFCON_RXTRIG4    (0<<4)
#define S5H2X_UFCON_TXFIFORESET    (1<<2)
#define S5H2X_UFCON_RXFIFORESET    (1<<1)

#define S5H2X_UFCON_DEFAULT   (S5H2X_UFCON_FIFOMODE | \
				 S5H2X_UFCON_TXTRIG0 | \
				 S5H2X_UFCON_RXTRIG4 | \
				 S5H2X_UFCON_TXFIFORESET | \
				 S5H2X_UFCON_RXFIFORESET )

#define S5H2X_UTRSTAT_CLEAR  (0xF0)
#define S5H2X_UTRSTAT_MI      (1<<7)
#define S5H2X_UTRSTAT_EI      (1<<6)
#define S5H2X_UTRSTAT_TXI      (1<<5)
#define S5H2X_UTRSTAT_RXI      (1<<4)
#define S5H2X_UTRSTAT_TXE	(1<<2)
#define S5H2X_UTRSTAT_TXFE      (1<<1)
#define S5H2X_UTRSTAT_RXDR      (1<<0)

#define S5H2X_UERSTAT_OE	(1 << 0)


#endif /* ASM_ARM_HARDWARE_SERIAL_S5H2X_H */

