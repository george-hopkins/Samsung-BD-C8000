/*
 * linux/arch/arm/mach-sdp/sdp_irq.h
 *
 * Copyright (C) 2006 Samsung Electronics.co
 * Author : tukho.kim@samsung.com
 *
 */
#ifndef __SDP_INTC_H
#define __SDP_INTC_H

#ifndef VA_INTC_BASE
#define SDP_INTC_BASE		VA_INT_BASE
#else
#define SDP_INTC_BASE		VA_INTC_BASE
#endif

#define INTCON		(0x00)
#define INTPND		(0x04)
#define INTMOD		(0x08)
#define INTMSK		(0x0C)
#define INT_LEVEL	(0x10)

#define I_PSLV0		(0x14)
#define I_PSLV1		(0x18)
#define I_PSLV2		(0x1C)
#define I_PSLV3		(0x20)
#define I_PMST		(0x24)
#define I_CSLV0		(0x28)
#define I_CSLV1		(0x2C)
#define I_CSLV2		(0x30)
#define I_CSLV3		(0x34)
// reserved		(0x38 ~ 3C)
#define I_ISPC		(0x40)

#define F_PSLV0		(0x44)
#define F_PSLV1		(0x48)
#define F_PSLV2		(0x4C)
#define F_PSLV3		(0x50)
#define F_PMST		(0x54)
#define F_CSLV0		(0x58)
#define F_CSLV1		(0x5C)
#define F_CSLV2		(0x60)
#define F_CSLV3		(0x64)
// reserved		(0x68 ~ 6C)
#define F_ISPC		(0x70)

#define INT_POLARITY	(0x74)
#define I_VECADDR	(0x78)
#define F_VECADDR	(0x7C)

#define INT_SRCPND	(0x90)

#ifdef SDP_USING_SUBINT
#define INT_SUBINT	(0x94)
#endif

#define INT_SRCSEL0	(0x98)
#define INT_SRCSEL1	(0x9C)
#define INT_SRCSEL2	(0xA0)
#define INT_SRCSEL3	(0xA4)
#define INT_SRCSEL4	(0xA8)
#define INT_SRCSEL5	(0xAC)
#define INT_SRCSEL6	(0xB0)
#define INT_SRCSEL7	(0xB4)

#define SDP_INTC_REG(x)	(SDP_INTC_BASE + x)
#define R_SDP_INTC(x)		(*(volatile unsigned int*)x)

#define SDP_INTC_INTCON	SDP_INTC_REG( INTCON	   )
#define SDP_INTC_INTPND	SDP_INTC_REG( INTPND	   )
#define SDP_INTC_INTMOD	SDP_INTC_REG( INTMOD	   )
#define SDP_INTC_INTMSK	SDP_INTC_REG( INTMSK	   )
#define SDP_INTC_LEVEL	SDP_INTC_REG( INT_LEVEL  )
                                                           
#define SDP_INTC_I_PSLV0	SDP_INTC_REG( I_PSLV0	   )
#define SDP_INTC_I_PSLV1	SDP_INTC_REG( I_PSLV1	   )
#define SDP_INTC_I_PSLV2	SDP_INTC_REG( I_PSLV2	   )
#define SDP_INTC_I_PSLV3	SDP_INTC_REG( I_PSLV3	   )
#define SDP_INTC_I_PMST	SDP_INTC_REG( I_PMST	   )
#define SDP_INTC_I_CSLV0	SDP_INTC_REG( I_CSLV0	   )
#define SDP_INTC_I_CSLV1	SDP_INTC_REG( I_CSLV1	   )
#define SDP_INTC_I_CSLV2	SDP_INTC_REG( I_CSLV2	   )
#define SDP_INTC_I_CSLV3	SDP_INTC_REG( I_CSLV3	   )
                                              
#define SDP_INTC_I_ISPC	SDP_INTC_REG( I_ISPC	   )
                                                           
#define SDP_INTC_F_PSLV0	SDP_INTC_REG( F_PSLV0	   )
#define SDP_INTC_F_PSLV1	SDP_INTC_REG( F_PSLV1	   )
#define SDP_INTC_F_PSLV2	SDP_INTC_REG( F_PSLV2	   )
#define SDP_INTC_F_PSLV3	SDP_INTC_REG( F_PSLV3	   )
#define SDP_INTC_F_PMST	SDP_INTC_REG( F_PMST	   )
#define SDP_INTC_F_CSLV0	SDP_INTC_REG( F_CSLV0	   )
#define SDP_INTC_F_CSLV1	SDP_INTC_REG( F_CSLV1	   )
#define SDP_INTC_F_CSLV2	SDP_INTC_REG( F_CSLV2	   )
#define SDP_INTC_F_CSLV3	SDP_INTC_REG( F_CSLV3	   )
                                               
#define SDP_INTC_F_ISPC	SDP_INTC_REG( F_ISPC	   )
                                                           
#define SDP_INTC_POLARITY	SDP_INTC_REG(INT_POLARITY)
#define SDP_INTC_I_VECADDR	SDP_INTC_REG( I_VECADDR  )
#define SDP_INTC_F_VECADDR	SDP_INTC_REG( F_VECADDR  )
                                                           
#define SDP_INTC_SRCPND	SDP_INTC_REG( INT_SRCPND )
                                                           
#ifdef SDP_USING_SUBINT	                
#define SDP_INTC_SUBINT	SDP_INTC_REG( INT_SUBINT )
#endif                                                     
                                                           
#define SDP_INTC_SRCSEL0	SDP_INTC_REG( INT_SRCSEL0)
#define SDP_INTC_SRCSEL1	SDP_INTC_REG( INT_SRCSEL1)
#define SDP_INTC_SRCSEL2	SDP_INTC_REG( INT_SRCSEL2)
#define SDP_INTC_SRCSEL3	SDP_INTC_REG( INT_SRCSEL3)
#define SDP_INTC_SRCSEL4	SDP_INTC_REG( INT_SRCSEL4)
#define SDP_INTC_SRCSEL5	SDP_INTC_REG( INT_SRCSEL5)
#define SDP_INTC_SRCSEL6	SDP_INTC_REG( INT_SRCSEL6)
#define SDP_INTC_SRCSEL7	SDP_INTC_REG( INT_SRCSEL7)

#define INTCON_FIQ_DIS		(0x1)
#define INTCON_IRQ_DIS		(0x1 << 1)
#define INTCON_VECTORED		(0x1 << 2)
#define INTCON_GMASK		(0x1 << 3)
#define INTCON_IRQ_FIQ_DIS	(0x3)

#define INTMSK_MASK(source) \
	R_SDP_INTC(SDP_INTC_INTMSK) |= ( 1 << source )

#define INTMSK_UNMASK(source) \
	R_SDP_INTC(SDP_INTC_INTMSK) &= ~( 1 << source )

#define INTPND_I_CLEAR(source) \
	R_SDP_INTC(SDP_INTC_I_ISPC) = ( 1 << source )
	
#define INTPND_F_CLEAR(source) \
	R_SDP_INTC(SDP_INTC_F_ISPC) = ( 1 << source )

#define POLARITY_LOW	1
#define POLARITY_HIGH	0

#define LEVEL_EDGE	0
#define LEVEL_LEVEL	1
//
typedef struct {
	unsigned irqSrc:8;
	unsigned qSrc:8;
	unsigned level:8;
	unsigned polarity:8;
	unsigned int prioMask;
	unsigned int subPrioMask;	
}SDP_INT_T;

#endif // __SDP_INTC_H
