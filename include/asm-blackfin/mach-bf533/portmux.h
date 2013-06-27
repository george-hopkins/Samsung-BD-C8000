#ifndef _MACH_PORTMUX_H_
#define _MACH_PORTMUX_H_

#define P_PPI0_CLK	(P_DONTCARE)
#define P_PPI0_FS1	(P_DONTCARE)
#define P_PPI0_FS2	(P_DONTCARE)
#define P_PPI0_FS3	(P_DEFINED | P_IDENT(GPIO_PF3))
#define P_PPI0_D15	(P_DEFINED | P_IDENT(GPIO_PF4))
#define P_PPI0_D14	(P_DEFINED | P_IDENT(GPIO_PF5))
#define P_PPI0_D13	(P_DEFINED | P_IDENT(GPIO_PF6))
#define P_PPI0_D12	(P_DEFINED | P_IDENT(GPIO_PF7))
#define P_PPI0_D11	(P_DEFINED | P_IDENT(GPIO_PF8))
#define P_PPI0_D10	(P_DEFINED | P_IDENT(GPIO_PF9))
#define P_PPI0_D9	(P_DEFINED | P_IDENT(GPIO_PF10))
#define P_PPI0_D8	(P_DEFINED | P_IDENT(GPIO_PF11))
#define P_PPI0_D0	(P_DONTCARE)
#define P_PPI0_D1	(P_DONTCARE)
#define P_PPI0_D2	(P_DONTCARE)
#define P_PPI0_D3	(P_DONTCARE)
#define P_PPI0_D4	(P_DEFINED | P_IDENT(GPIO_PF15))
#define P_PPI0_D5	(P_DEFINED | P_IDENT(GPIO_PF14))
#define P_PPI0_D6	(P_DEFINED | P_IDENT(GPIO_PF13))
#define P_PPI0_D7	(P_DEFINED | P_IDENT(GPIO_PF12))

#define P_SPORT1_TSCLK	(P_DONTCARE)
#define P_SPORT1_RSCLK	(P_DONTCARE)
#define P_SPORT0_TSCLK	(P_DONTCARE)
#define P_SPORT0_RSCLK	(P_DONTCARE)
#define P_UART0_RX	(P_DONTCARE)
#define P_UART0_TX	(P_DONTCARE)
#define P_SPORT1_DRSEC	(P_DONTCARE)
#define P_SPORT1_RFS	(P_DONTCARE)
#define P_SPORT1_DTPRI	(P_DONTCARE)
#define P_SPORT1_DTSEC	(P_DONTCARE)
#define P_SPORT1_TFS	(P_DONTCARE)
#define P_SPORT1_DRPRI	(P_DONTCARE)
#define P_SPORT0_DRSEC	(P_DONTCARE)
#define P_SPORT0_RFS	(P_DONTCARE)
#define P_SPORT0_DTPRI	(P_DONTCARE)
#define P_SPORT0_DTSEC	(P_DONTCARE)
#define P_SPORT0_TFS	(P_DONTCARE)
#define P_SPORT0_DRPRI	(P_DONTCARE)

#define P_SPI0_MOSI	(P_DONTCARE)
#define P_SPI0_MISO	(P_DONTCARE)
#define P_SPI0_SCK	(P_DONTCARE)
#define P_SPI0_SSEL7	(P_DEFINED | P_IDENT(GPIO_PF7))
#define P_SPI0_SSEL6	(P_DEFINED | P_IDENT(GPIO_PF6))
#define P_SPI0_SSEL5	(P_DEFINED | P_IDENT(GPIO_PF5))
#define P_SPI0_SSEL4	(P_DEFINED | P_IDENT(GPIO_PF4))
#define P_SPI0_SSEL3	(P_DEFINED | P_IDENT(GPIO_PF3))
#define P_SPI0_SSEL2	(P_DEFINED | P_IDENT(GPIO_PF2))
#define P_SPI0_SSEL1	(P_DEFINED | P_IDENT(GPIO_PF1))
#define P_SPI0_SS	(P_DEFINED | P_IDENT(GPIO_PF0))

#define P_TMR2		(P_DONTCARE)
#define P_TMR1		(P_DONTCARE)
#define P_TMR0		(P_DONTCARE)
#define P_TMRCLK	(P_DEFINED | P_IDENT(GPIO_PF1))





#endif /* _MACH_PORTMUX_H_ */
