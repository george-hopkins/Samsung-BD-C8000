/*
 * linux/include/asm-arm/arch-ssdtv/irqs-sdp.h
 *
 * Copyright (C) 2008 SAMSUNG ELECTRONICS  
 * Author : tukho.kim@samsung.com 06/19/2008
 *
 */

#ifndef __SDP83_IRQS_H
#define __SDP83_IRQS_H

/* bits in interrupt register */
#define BIT_SHIFT_TSD      		0
#define BIT_SHIFT_AIO      		1
#define BIT_SHIFT_AE       		2
#define BIT_SHIFT_MPGE0    		3
#define BIT_SHIFT_MPEG1    		4
#define BIT_SHIFT_DISP     		5
#define BIT_SHIFT_GA2D     		6
#define BIT_SHIFT_GA3D     		7
#define BIT_SHIFT_JPEG 	   		8
#define BIT_SHIFT_MFD     		9
#define BIT_SHIFT_AVD     		10
#define BIT_SHIFT_CROAD	   		11
#define BIT_SHIFT_PCI 	   		12
#define BIT_SHIFT_DMA      		13
#define BIT_SHIFT_USB_OHCI0		14
#define BIT_SHIFT_USB_EHCI0		15
#define BIT_SHIFT_USB_OHCI1		16
#define BIT_SHIFT_USB_EHCI1		17
#define BIT_SHIFT_IRB_IRR   		18
#define BIT_SHIFT_UART0    		19
#define BIT_SHIFT_UART1    		20
#define BIT_SHIFT_UART2    		21
#define BIT_SHIFT_SPI	   		22
#define BIT_SHIFT_TIMER    		23
#define BIT_SHIFT_I2C     		24
#define BIT_SHIFT_SCI     		25
#define BIT_SHIFT_EXT7     		26
#define BIT_SHIFT_NAND      		27
#define BIT_SHIFT_SE       		28
#define BIT_SHIFT_EXT0      		29
#define BIT_SHIFT_EXT1     		30
#define BIT_SHIFT_SUBINT   		31

#define IRQ_TSD      			BIT_SHIFT_TSD      
#define IRQ_AIO                         BIT_SHIFT_AIO      
#define IRQ_AE                          BIT_SHIFT_AE       
#define IRQ_MPGE0                       BIT_SHIFT_MPGE0    
#define IRQ_MPEG1                       BIT_SHIFT_MPEG1    
#define IRQ_DISP                        BIT_SHIFT_DISP     
#define IRQ_GA2D                        BIT_SHIFT_GA2D     
#define IRQ_GA3D                        BIT_SHIFT_GA3D     
#define IRQ_JPEG      	                BIT_SHIFT_JPEG 	   
#define IRQ_MFD                         BIT_SHIFT_MFD      
#define IRQ_AVD                         BIT_SHIFT_AVD      
#define IRQ_CROAD    	                BIT_SHIFT_CROAD	   
#define IRQ_PCI       	                BIT_SHIFT_PCI 	   
#define IRQ_DMA                         BIT_SHIFT_DMA      
#define IRQ_USB_OHCI0                   BIT_SHIFT_USB_OHCI0
#define IRQ_USB_EHCI0                   BIT_SHIFT_USB_EHCI0
#define IRQ_USB_OHCI1                   BIT_SHIFT_USB_OHCI1
#define IRQ_USB_EHCI1                   BIT_SHIFT_USB_EHCI1
#define IRQ_IRB_IRR                     BIT_SHIFT_IRB_IRR  
#define IRQ_UART0                       BIT_SHIFT_UART0    
#define IRQ_UART1                       BIT_SHIFT_UART1    
#define IRQ_UART2                       BIT_SHIFT_UART2    
#define IRQ_SPI	                        BIT_SHIFT_SPI	   
#define IRQ_TIMER                       BIT_SHIFT_TIMER    
#define IRQ_I2C                         BIT_SHIFT_I2C      
#define IRQ_SCI                         BIT_SHIFT_SCI      
#define IRQ_EXT7                        BIT_SHIFT_EXT7     
#define IRQ_NAND                        BIT_SHIFT_NAND     
#define IRQ_SE                          BIT_SHIFT_SE       
#define IRQ_EXT0                        BIT_SHIFT_EXT0     
#define IRQ_EXT1                        BIT_SHIFT_EXT1     
#define IRQ_SUBINT                      BIT_SHIFT_SUBINT   

#define NR_IRQS            		(IRQ_SUBINT+1)

#endif /* __SDP83_IRQS_H */

