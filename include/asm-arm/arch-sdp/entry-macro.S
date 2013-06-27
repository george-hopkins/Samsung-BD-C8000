/* include/asm-arm/arch-ssdtv/entry-macro.S
 * Samsung DTV Soc interrupt Assembler
 * Copyright (C) 2003 - 2006 tukho.kim@samsung.com
 */
#include <asm/hardware.h>

#ifndef VA_INTC_BASE
#define VA_INTC_BASE VA_INT_BASE
#endif

.macro  disable_fiq
.endm

.macro arch_ret_to_user, tmp1, tmp2
.endm

.macro get_irqnr_preamble, base, tmp
.endm

/* Try to semi vectored interrupt.
 * if using semi vectored interrupt, must set semi-vectored
 * interrupt in INTCON register
 * tukho.kim@samsung.com 2006, 08 */

.macro get_irqnr_and_base, irqnr, irqstat, base, tmp

	mov     \base, #VA_INTC_BASE&0xff000000 @ IO Base add virtual
	add     \base, \base, #VA_INTC_BASE&0x00ff0000
	add     \base, \base, #VA_INTC_BASE&0x0000ff00

	@ read rI_ISPR register check, 
	@ if Pend regiser is 0, just out
	ldr	\irqstat, [\base, #0x3c]      @ get interrupts bits
	cmp	\irqstat, #0
	moveq	\irqnr, #0
	ldrne	\irqstat, [\base, #0x78]
	movne	\irqnr, \irqstat, LSR #0x2

	1001:
.endm


