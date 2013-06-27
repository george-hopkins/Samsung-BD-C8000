/*
 * linux/include/asm-arm/arch-ssdtv/ssdtv_pci.h
 *
 * Author: Samsung Electronics
 *
 */

#ifndef __SSDTV_PCI_H

#define __SSDTV_PCI_H

#define PCI_MEM_ADDR			0x40000000
#define PCI_IO_ADDR			0x00000000
#define PCI_MEM_BAR0			0xC0000000
#define PCI_MEM_BAR1			0xD0000000 + 0x0
#define PCI_MEM_BAR2			0xD0000000 + 0x12000000
                                                                                
#define PCI_MEM_SIZE			0x10000000
#define PCI_IO_SIZE			0x04000000
                                                                                
#define AHB_PCI_MEM_BASE		PCI_MEM_ADDR
#define AHB_PCI_CONFIG_TYPE0_BASE 	0x50000000
#define AHB_PCI_CONFIG_TYPE1_BASE 	0x54000000
#define AHB_PCI_IO_BASE			0x5C000000
                                                                                
/* XXX: see arch/arm/mach-ssdtv/mach-xxx.c */
#define PCI_CONFIG_TYPE0_VADDR  	0xF0000000
#define PCI_CONFIG_TYPE1_VADDR  	0xF4000000
                                                                                
#define  AHB_ADDR_PCI_CFG0( device, functn, offset )            \
			(PCI_CONFIG_TYPE0_VADDR | ((device)<<11) | \
			((    (functn)    &0x07)<< 8) | \
			((    (offset)    &0xFF)    )   )
#define  AHB_ADDR_PCI_CFG1( bus, device, functn, offset )       \
			(PCI_CONFIG_TYPE1_VADDR | (((bus)&0xFF)<<16) | \
			((    (device)    &0x1F)<<11) |\
			((    (functn)    &0x07)<< 8) | \
			((    (offset)    &0xFF)    )   )


/* SELP.arm.3.x support A1 2007-10-22 */
/* PCI Interrupt source mask */
#define PINTEN_BAP  	(1 << 31)		/* Agent Only */
/* Reserved [30:17]*/
#define PINTEN_AER    	(1 << 16)
#define PINTEN_DE1    	(1 << 15)
#define PINTEN_DM1    	(1 << 14)
#define PINTEN_DE0    	(1 << 13)
#define PINTEN_DM0    	(1 << 12)
/* Reserved [11] */
#define PINTEN_INA    	(1 << 10)
#define PINTEN_SER    	(1 << 9)		/* Host Only */
#define PINTEN_BPA    	(1 << 8)
#define PINTEN_PSC    	(1 << 7)		/* Agent Only */
#define PINTEN_PMC    	(1 << 6)		/* Agent Only */
#define PINTEN_PME    	(1 << 5)
#define PINTEN_TPE    	(1 << 4)
#define PINTEN_MPE    	(1 << 3)
#define PINTEN_MFE    	(1 << 2)
#define PINTEN_PRA    	(1 << 1)
#define PINTEN_PRD    	(1 << 0)

#define PINTEN_ALL_HOST  (PINTEN_AER | PINTEN_INA | PINTEN_SER | PINTEN_BPA | PINTEN_PME | \
		                  PINTEN_TPE | PINTEN_MPE | PINTEN_MFE | PINTEN_PRA | PINTEN_PRD )

/* PCI Interrupt Status Register */
#define PINTST_WDE		(1 << 31)
#define PINTST_RDE		(1 << 30)
#define PINTST_DME		(1 << 29)
/* Reserved [28:17]*/
#define PINTST_AER		(1 << 16)
#define PINTST_DE1		(1 << 15)
#define PINTST_DM1		(1 << 14)
#define PINTST_DE0		(1 << 13)
#define PINTST_DM0		(1 << 12)
/* Reserved [11] */
#define PINTST_INA		(1 << 10)
#define PINTST_SER		(1 << 9)
#define PINTST_BPA		(1 << 8)
#define PINTST_PSC		(1 << 7)
#define PINTST_PMC		(1 << 6)
#define PINTST_PME		(1 << 5)
#define PINTST_TPE		(1 << 4)
#define PINTST_MPE		(1 << 3)
#define PINTST_MFE		(1 << 2)
#define PINTST_PRA		(1 << 1)
#define PINTST_PRD		(1 << 0)

#define PINTST_CLEAR		(0x7FF | (0x1F << 12) | (0x7 << 29))

/* PCI register bit */
#define P_API       (25)
#define P_APM       (27)

/*PCI configuration*/
#define PCON_HST (1<<0)
#define PCON_ARB (1<<1)
#define PCON_ATS (1<<4)
#define PCON_SPL (1<<5)
#define PCON_IOP (1<<6)
#define PCON_MMP (1<<7)
#define PCON_CFD (1<<8)
#define PCON_RDY (1<<9)
                                                                                
#ifndef R_PCIHID
#define R_PCIHID		rPCIHID	
#define R_PCIHSC		rPCIHSC	
#define R_PCIHCODE		rPCIHCODE	
#define R_PCIHLINE		rPCIHLINE	
#define R_PCIHBAR0		rPCIHBAR0	
#define R_PCIHBAR1		rPCIHBAR1	
#define R_PCIHBAR2		rPCIHBAR2	
/*Reserved */ 
#define R_PCIHCISP		rPCIHCISP	
#define R_PCIHSSID		rPCIHSSID	
/*Reserved */
#define R_PCIHCAP		rPCIHCAP	
/*Reserved */
#define R_PCIHLTIT		rPCIHLTIT	
#define R_PCIHTIMER		rPCIHTIMER	
/*Reserved */
#define R_PCIHPMR0		rPCIHPMR0	
#define R_PCIHPMR1		rPCIHPMR1	

#define R_PCICON		rPCICON	
#define R_PCISET		rPCISET	
#define R_PCIINTEN		rPCIINTEN	
#define R_PCIINTST		rPCIINTST	
#define R_PCIINTAD		rPCIINTAD	
#define R_PCIBATAPM		rPCIBATAPM	
#define R_PCIBATAPI		rPCIBATAPI	
#define R_PCIRCC		rPCIRCC	
#define R_PCIDIAG0		rPCIDIAG0	
#define R_PCIDIAG1		rPCIDIAG1	
#define R_PCIBELAP		rPCIBELAP	
#define R_PCIBELPA		rPCIBELPA	
#define R_PCIMAIL0		rPCIMAIL0	
#define R_PCIMAIL1		rPCIMAIL1	
#define R_PCIMAIL2		rPCIMAIL2	
#define R_PCIMAIL3		rPCIMAIL3	
#define R_PCIBATPA0		rPCIBATPA0	
#define R_PCIBAM0		rPCIBAM0	
#define R_PCIBATPA1		rPCIBATPA1	
#define R_PCIBAM1		rPCIBAM1	
#define R_PCIBATPA2		rPCIBATPA2	
#define R_PCIBAM2		rPCIBAM2	
#define R_PCISWAP		rPCISWAP	
/* Reserved */
#define R_PCIDMACON0		rPCIDMACON0	
#define R_PCIDMASRC0		rPCIDMASRC0	
#define R_PCIDMADST0		rPCIDMADST0	
#define R_PCIDMACNT0		rPCIDMACNT0	
#define R_PCIDMARUN0		rPCIDMARUN0	
/* Reserved */
#define R_PCIDMACON1		rPCIDMACON1	
#define R_PCIDMASRC1		rPCIDMASRC1	
#define R_PCIDMADST1		rPCIDMADST1	
#define R_PCIDMACNT1		rPCIDMACNT1	
#define R_PCIDMARUN1		rPCIDMARUN1	
#endif /*R_PCIHID*/

#endif /* __PCI_SSDTV_H */
