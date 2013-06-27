/*
 * linux/include/asm-arm/arch-omap/board-ldp.h
 *
 * Hardware definitions for TI OMAP3430 LDP board.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN
 * NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF
 * USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <linux/init.h>

#ifndef __ASM_ARCH_OMAP_LDP_H
#define __ASM_ARCH_OMAP_LDP_H
extern int twl4030_vaux1_ldo_use(void);
extern int twl4030_vaux1_ldo_unuse(void);
extern int twl4030_vaux3_ldo_use(void);
extern int twl4030_vaux3_ldo_unuse(void);
extern void setup_board_wakeup_source(u32);
extern int console_detect(char *str);
extern void setup_board_wakeup_source(u32);
void init_wakeupconfig(void);
void uart_padconf_control(void);

extern void __init ldp_usb_init(void);
extern void __init ldp3430_flash_init(void);
extern void omap_uart_restore_ctx(void);

#define DEBUG_BASE			0x08000000  /* debug board */

/* Placeholder for 3430Labrador specific defines */
#define	SDP3430_SMC91X_CS	1
#define OMAP34XX_ETHR_START		DEBUG_BASE
#define OMAP34XX_ETHR_GPIO_IRQ		152

#define DEBUG_BASE		0x08000000  /* debug board */
#define NAND_BASE		0x0C000000  /* NAND flash */

/* Board Wake up definations */
#define PRCM_WAKEUP_T2_KEYPAD   0x1   /* GPIO1.0 */
#define PRCM_WAKEUP_TOUCHSCREEN 0x2
#define PRCM_WAKEUP_DEBUG_UART  0x4   /* U1/U3 */
#define PRCM_WAKEUP_ETH_IRQ     0x8   /* GPIO5.152 func & io-pad */

#define BOARD_IDLE_WAKEUPS 	(PRCM_WAKEUP_T2_KEYPAD |\
				 PRCM_WAKEUP_DEBUG_UART |\
				 PRCM_WAKEUP_ETH_IRQ)

#define BOARD_SUSPEND_WAKEUPS 	PRCM_WAKEUP_T2_KEYPAD
#define BOARD_ALL_WAKEUPS 	(BOARD_IDLE_WAKEUPS | BOARD_SUSPEND_WAKEUPS)

/* T2 definations */
#ifdef CONFIG_TWL4030_CORE
#define TWL4030_IRQNUM INT_34XX_SYS_NIRQ

/* TWL4030 Primary Interrupt Handler (PIH) interrupts */
#define	IH_TWL4030_BASE		IH_BOARD_BASE
#define	IH_TWL4030_END		(IH_TWL4030_BASE+8)

#ifdef CONFIG_TWL4030_GPIO

/* TWL4030 GPIO Interrupts */
#define IH_TWL4030_GPIO_BASE	(IH_TWL4030_END)
#define IH_TWL4030_GPIO_END	(IH_TWL4030_BASE+18)
#define NR_IRQS			(IH_TWL4030_GPIO_END)
#else
#define NR_IRQS			(IH_TWL4030_END)
#endif /* CONFIG_I2C_TWL4030_GPIO */
#endif /* End of support for TWL4030 */
#endif /*  __ASM_ARCH_OMAP_LDP_H */

