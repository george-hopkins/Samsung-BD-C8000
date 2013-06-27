/*
 *  linux/arch/arm/plat-sdp/sdp_pcie.c
 *
 *  PCIE functions for sdp PCIE Root Complex 
 *
 *  Copyright (C) 1999 ARM Limited
 *  Copyright (C) 2009 Samsung Electronics.co
 *
 *  Created by tukho.kim@samsung.com
 */

#include <linux/kernel.h>
#include <linux/pci.h>
#include <linux/ptrace.h>
#include <linux/slab.h>
#include <linux/ioport.h>
#include <linux/interrupt.h>
#include <linux/spinlock.h>
#include <linux/init.h>

#include <asm/types.h>
#include <asm/hardware.h>
#include <asm/io.h>
#include <asm/irq.h>
#include <asm/system.h>
#include <asm/mach/pci.h>
#include <asm/delay.h>

#include <asm/arch/platform.h>

#include "sdp_pcie.h"

#undef DEBUG

#ifdef DEBUG
#define DBG(format, args...)	printk("%s[%d]\t" format, __FUNCTION__, __LINE__, ##args)
#else
#define DBG(format, args...)
#endif

#define DEBUG_BOARD

#define MAX_SLOTS 21 /* AD11 ~ AD31 */

static DEFINE_SPINLOCK(sdp_lock);

static u8 gPhyCmu_table[] = 
{
	/* 0     1     2     3     4     5     6     7     8     9 */
/* 00 */  0x06 ,0x00 ,0x81 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00
/* 01 */ ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00
/* 02 */ ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00
/* 03 */ ,0x00 ,0x00 ,0x00 ,0x00 ,0xa0 ,0x64 ,0x00 ,0x00 ,0x00 ,0x00
/* 04 */ ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x04 ,0x50 ,0x70 ,0x02
/* 05 */ ,0x25 ,0x40 ,0x01 ,0x40 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00
/* 06 */ ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00
/* 07 */ ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00
/* 08 */ ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00
/* 09 */ ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x2e ,0x00 ,0x5e
/* 10 */ ,0x00 ,0x42 ,0x91 ,0x10 ,0x48 ,0x90 ,0x44 ,0xce ,0x0b ,0x00
/* 11 */ ,0x00 ,0x00 ,0x00 ,0x00 ,0x00
};

static u8 gPhyLane0_table[] =
{
	/* 0     1     2     3     4     5     6     7     8     9 */
/* 00 */  0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x10 ,0x84 ,0x04 ,0xe0 ,0x00
/* 01 */ ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x23 ,0x00 ,0x00 ,0x01 
/* 02 */ ,0x38 ,0x04 ,0x00 ,0x68 ,0xa2 ,0x1e ,0x18 ,0x05 ,0x0d ,0x00 
/* 03 */ ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 
/* 04 */ ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 
/* 05 */ ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 
/* 06 */ ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 
/* 07 */ ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 
/* 08 */ ,0x60 ,0x0f
};


static u8 gPhyCmnLaneBlock_table[] =
{
	/* 0     1     2     3     4     5     6     7     8     9 */
/* 00 */  0xc0 ,0x90 ,0x02 ,0x40 ,0x3c ,0x00 ,0x00 ,0x00 ,0x00 ,0x63
/* 01 */ ,0x49 ,0xc6 ,0x01 ,0x03 ,0x28 ,0x98 ,0x19 ,0x28 ,0x78 ,0xe1
/* 02 */ ,0xf0 ,0x10 ,0xf4 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00
/* 03 */ ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 
/* 04 */ ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00
/* 05 */ ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0xa0 ,0xa0 ,0xa0
/* 06 */ ,0xa0 ,0xa0 ,0xa0 ,0xa0 ,0x64 ,0x00 ,0xc0 ,0x9f ,0x01 ,0x00
/* 07 */ ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x30 ,0x41 ,0x7e ,0xd0
/* 08 */ ,0xcc ,0x85 ,0x52 ,0x93 ,0xe0 ,0x49 ,0xdd ,0xb0 ,0x0b ,0x02
/* 09 */ ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00
/* 10 */ ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00
/* 11 */ ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00
/* 12 */ ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0xd8 ,0x1a ,0xff
/* 13 */ ,0x01 ,0x00 ,0x00 ,0x00 ,0x00 ,0xf0 ,0xff ,0xff ,0xff ,0xff
/* 14 */ ,0x1c ,0xc2 ,0xc3 ,0x3f ,0x0a ,0x00 ,0x00 ,0x00 ,0x00 ,0x00
/* 15 */ ,0xf8
};


/* PCI compatilbe header ~0xFF*/
static PCI_HEADER_T *gpPciHeader;
static PCI_PWM_T *gpPciPwm;
static PCI_MSI_T *gpPciMsi;
static PCI_PCIE_T *gpPciPcie;
static PCI_MSI_X_T *gpPciMsiX;
static PCI_SLOT_ID_T *gpPciSlotId;
static PCI_VPD_T *gpPciVpd;
/* PCIE Extended register ~0xFFF */
static PCIE_EXT_T *gpPcieExt;
static PCIE_PORT_LOGIC_T *gpPciePortLogic;

/* PCIE ELBI Register */
static PCIE_ELBI_T *gpPcieElbi;

/* PCIE PHY Register */
static PCIE_PHY_CMU_T *gpPciePhyCmu;
static PCIE_PHY_LANE0_T *gpPciePhyLane0;
static PCIE_PHY_CMN_LANE_BLOCK_T *gpPciePhyCmnLaneBlock;


static unsigned long 
sdp_open_config_window(struct pci_bus *bus, unsigned int devfn, int offset)
{
	unsigned int address, busnr;
	u32 slot, func;

	slot = PCI_SLOT(devfn);
	func = PCI_FUNC(devfn);

	busnr = bus->number;

	/*
	 * Trap out illegal values
	 */
	if (offset > 255)
		BUG();
	if (busnr > 255)
		BUG();
	if (devfn > 255)
		BUG();

	if(busnr == 0){
		address = AHB_ADDR_PCI_CFG0(slot, func, offset);
	} else {
		address = AHB_ADDR_PCI_CFG1(busnr, slot, func, offset);
	}
	return address;
}


static void 
sdp_close_config_window(void)
{
	/*
	 * Reassign base1 for use by prefetchable PCI memory
	 */

	/*
	 * And shrink base0 back to a 256M window (NOTE: MAP0 already correct)
	 */
	mb();
}

static int 
sdp_read_config (struct pci_bus *bus, unsigned int devfn, int where, int size, u32 *val)
{
	unsigned long addr;
	unsigned long flags;
	u32 regVal;

	spin_lock_irqsave(&sdp_lock, flags);	// È®ÀÎ interrupt disable !!!

	if(bus->number) 
		gpPcieElbi->slvARCtrl = 0x5; // configuation 1 transction 
	else
		gpPcieElbi->slvARCtrl = 0x4; // configuation 0 transction 
	

	addr = sdp_open_config_window(bus, devfn, where);

	switch (size) {
	case 1:
		regVal = ioread8(addr);
		break;

	case 2:
		regVal = ioread16(addr);
		break;

	default:
		regVal = ioread32(addr);
		break;
	}

	sdp_close_config_window();

	gpPcieElbi->slvARCtrl = 0; // memory transction 

	spin_unlock_irqrestore(&sdp_lock, flags);

	*val = regVal;

	return PCIBIOS_SUCCESSFUL;
}

static int 
sdp_write_config (struct pci_bus *bus, unsigned int devfn, int where, int size, u32 val)
{
	unsigned long addr;
	unsigned long flags;

	spin_lock_irqsave(&sdp_lock, flags);

	if(bus->number){
		gpPcieElbi->slvARCtrl = 0x5; // configuation 1 transction 
		gpPcieElbi->slvAWCtrl = 0x5; // configuation 1 transction 
	}
	else {
		gpPcieElbi->slvARCtrl = 0x4; // configuation 0 transction 
		gpPcieElbi->slvAWCtrl = 0x4; // configuation 0 transction 
	}

	addr = sdp_open_config_window(bus, devfn, where);

	switch (size) {
	case 1:
		iowrite8((u8)val, addr);
		ioread8(addr);
		break;

	case 2:
		iowrite16((u16)val, addr);
		ioread16(addr);
		break;

	case 4:
		iowrite32(val, addr);
		ioread32(addr);
		break;
	}

	sdp_close_config_window();

	gpPcieElbi->slvARCtrl = 0; // memory transction 
	gpPcieElbi->slvAWCtrl = 0; // memory transction 

	spin_unlock_irqrestore(&sdp_lock, flags);

	return PCIBIOS_SUCCESSFUL;
}


static struct pci_ops pci_sdp_ops = {
	.read	= sdp_read_config,
	.write	= sdp_write_config,
};


static int sdp_fault(unsigned long addr, unsigned int fsr, struct pt_regs *regs)
{
	unsigned long pc = instruction_pointer(regs);
	unsigned long instr = *(unsigned long *)pc;

	/*
	 * If the instruction being executed was a read,
	 * make it look like it read all-ones.
	 */
	if ((instr & 0x0c100000) == 0x04100000) {
		int reg = (instr >> 12) & 15;
		unsigned long val;

		if (instr & 0x00400000)
			val = 255;
		else
			val = -1;

		regs->uregs[reg] = val;
		regs->ARM_pc += 4;
		return 0;
	}

	if ((instr & 0x0e100090) == 0x00100090) {
		int reg = (instr >> 12) & 15;

		regs->uregs[reg] = -1;
		regs->ARM_pc += 4;
		return 0;
	}

	return 1;
}


static struct resource io_mem = {
        .name   = "PCI I/O apeture",
        .start  = AHB_PCI_IO_BASE,
        .end    = AHB_PCI_IO_BASE + PCI_IO_SIZE -1,
        .flags  = IORESOURCE_MEM,
};
static struct resource non_mem = {
        .name   = "PCI MEM apeture",
        .start  = AHB_PCI_MEM_BASE,
        .end    = AHB_PCI_MEM_BASE + PCI_MEM_SIZE -1,
        .flags  = IORESOURCE_MEM,
};

int __init pci_sdp_setup_resources(struct resource **resource)
{
    /*
     * Hook in our fault handler for PCI errors
     */

         if (request_resource(&iomem_resource, &non_mem)) {
                DBG(KERN_ERR "PCI: unable to allocate non-prefetchable "
                       "memory region\n");
                return -EBUSY;
        }
	
        //if (request_resource(&ioport_resource, &io_mem)) {
        if (request_resource(&iomem_resource, &io_mem)) {
                release_resource(&non_mem);
                DBG(KERN_ERR "PCI: unable to allocate I/O "
                       "memory region\n");
                return -EBUSY;
        }
        /*
         * bus->resource[0] is the IO resource for this bus
         * bus->resource[1] is the mem resource for this bus
         * bus->resource[2] is the prefetch mem resource for this bus
         */
        resource[0] = &ioport_resource;
        resource[1] = &non_mem;
        resource[2] = NULL;
                                                                                
        return 1;

}

/*
 * These don't seem to be implemented on the Integrator I have, which
 * means I can't get additional information on the reason for the pm2fb
 * problems.  I suppose I'll just have to mind-meld with the machine. ;)
 */

int __init sdp_setup(int nr, struct pci_sys_data *sys)
{
	int ret = 0;

	if (nr == 0) {
		sys->mem_offset = AHB_PCI_MEM_BASE;
		ret = pci_sdp_setup_resources(sys->resource);
	}

	return ret;
}

struct pci_bus *sdp_scan_bus(int nr, struct pci_sys_data *sys)
{
	return pci_scan_bus(sys->busnr, &pci_sdp_ops, sys);
}

int __init sdp_map_irq(struct pci_dev *dev, u8 slot, u8 pin)
{
	return IRQ_PCI;
}

irqreturn_t sdp_pci_irq(int irq, void *devid)

{
	u32 regVal0 = gpPcieElbi->userConfig8 ;
	u32 regVal1 = gpPcieElbi->userOnePulseConfig0;

// user config8 check 	
	if(regVal0 & 0x1FFFF) {
		printk("User Config0 Error 0x%08x\n", regVal0);
	}

// user one pulse config 0 check
	if(regVal1 & 0x1FFFF) {
		printk("User One Pulse Config0 Error 0x%08x\n", regVal1);
	}


	gpPcieElbi->userConfig8 = regVal0;		// clear
	gpPcieElbi->userOnePulseConfig0 = regVal1;	// clear

	return IRQ_HANDLED;
}

extern struct pci_ops pci_sdp_ops;


static void __init pcie_root_complex_init(void)
{
		// did a62c, vid 144d
	printk("SDP PCI Express RC VID %04xh PID %04xh\n",
		gpPciHeader->vendorId, gpPciHeader->deviceId);

	gpPciHeader->command = (u16)0x7; //Bus master, memory space, I/o space;
	gpPciHeader->status = gpPciHeader->status; // Clear status 

#if 0
	gpPciHeader->baseAddress0 = 0xF0000000;	// MEMORY SPACE | 32BIT ADDR | NON Prefetch
	gpPciHeader->baseAddress1 = 0x20200001;	// IO_SPACE

	gpPciHeader->expansionRomBase = 0x000FFFF1; // EPROM BAR Enable
	gpPciHeader->expansionRomBase = 0x60000000; // EPROM BAR Enable ????
#endif

	/* AHB -> PCIe OutBound */
	/* Set AHB->PCIe Configuration0 region on AHB*/
        gpPcieElbi->inCfg0AddrStartLow = AHB_PCI_CONFIG_TYPE0_BASE;  		//0x50000000; 
        gpPcieElbi->inCfg0AddrStartHigh = 0x00000000;
        gpPcieElbi->inCfg0AddrLimitLow = AHB_PCI_CONFIG_TYPE0_BASE + (16 << 20); //0x50FFFFFF; 
	gpPcieElbi->inCfg0AddrLimitHigh = 0x00000000;
	/* Set AHB->PCIe Configuration0 region on PCI*/
        gpPcieElbi->pom0Cfg0AddrStartLow = 0x00000000;  
        gpPcieElbi->pom0Cfg0AddrStartHigh = 0x00000000; 

	/* Set AHB->PCIe Configuration1 region on AHB*/
        gpPcieElbi->inCfg1AddrStartLow = AHB_PCI_CONFIG_TYPE1_BASE; // 0x54000000; 
        gpPcieElbi->inCfg1AddrStartHigh = 0x00000000;
        gpPcieElbi->inCfg1AddrLimitLow = AHB_PCI_CONFIG_TYPE1_BASE + (16 << 20); //0x54FFFFFF; 
	gpPcieElbi->inCfg1AddrLimitHigh = 0x00000000;
	/* Set AHB->PCIe Configuration1 region on PCI*/
        gpPcieElbi->pom1Cfg1AddrStartLow = 0x00000000; 		// ??? 
        gpPcieElbi->pom1Cfg1AddrStartHigh = 0x00000000; 

	/* Set AHB->PCIe Memory0 region on AHB*/
	gpPcieElbi->in0MemAddrStartLow = AHB_PCI_MEM_BASE; 		//0x40000000;
	gpPcieElbi->in0MemAddrStartHigh = 0x00000000;
	gpPcieElbi->in0MemAddrLimitLow = AHB_PCI_MEM_BASE + 0x07FFFFFF;
	gpPcieElbi->in0MemAddrLimitHigh = 0x00000000;
	/* Set AHB->PCIe Memory0 region on PCI*/
        gpPcieElbi->pom0MemAddrStartLow = 0x00000000;   
        gpPcieElbi->pom0MemAddrStartHigh = 0x00000000;  
	
	/* Set AHB->PCIe Memory1 region on AHB*/
	gpPcieElbi->in1MemAddrStartLow = AHB_PCI_MEM_BASE + 0x08000000; // 0x48000000; 
        gpPcieElbi->in1MemAddrStartHigh = 0x00000000;
        gpPcieElbi->in1MemAddrLimitLow = AHB_PCI_MEM_BASE + 0x0FFFFFFF; // 0x4FFFFFFF; 
        gpPcieElbi->in1MemAddrLimitHigh = 0x00000000;
	/* Set AHB->PCIe Memory1 region on PCI*/
        gpPcieElbi->pom1MemAddrStartLow = 0x08000000;   
        gpPcieElbi->pom1MemAddrStartHigh = 0x00000000;  

	/* Set AHB->PCIe I/O region on AHB*/
        gpPcieElbi->inIoAddrStartLow = AHB_PCI_IO_BASE; // 0x5C000000;   
        gpPcieElbi->inIoAddrStartHigh = 0x00000000;  
        gpPcieElbi->inIoAddrLimitLow = AHB_PCI_IO_BASE + 0x0FFFFFFF; // 0x5FFFFFFF;   
        gpPcieElbi->inIoAddrLimitHigh = 0x00000000;  
	/* Set AHB->PCIe I/O region on PCI*/
        gpPcieElbi->pomIoAddrStartLow = AHB_PCI_IO_BASE;     
        gpPcieElbi->pomIoAddrStartHigh = 0x00000000;    

	/* PCIe->AHB Inbound */
	/* Set PCIe->AHB Memory0 region on PCI*/
        gpPcieElbi->pin0MemAddrStartLow = 0x60000000; 	// DDR-A channel
        gpPcieElbi->pin0MemAddrStartHigh = 0x00000000;
        gpPcieElbi->pin0MemAddrLimitLow = 0x7FFFFFFF; 
        gpPcieElbi->pin0MemAddrLimitHigh = 0x00000000;
	/* Set PCIe->AHB Memory0 region on AHB*/
        gpPcieElbi->pim0MemAddrStartLow = 0x60000000; 
        gpPcieElbi->pim0MemAddrStartHigh = 0x00000000;

	/* Set PCIe->AHB Memory1 region on PCI*/
        gpPcieElbi->pin1MemAddrStartLow = 0x80000000; 	// DDR-B Channel 
        gpPcieElbi->pin1MemAddrStartHigh = 0x00000000;
        gpPcieElbi->pin1MemAddrLimitLow = 0x8FFFFFFF; 
        gpPcieElbi->pin1MemAddrLimitHigh = 0x00000000;
	/* Set PCIe->AHB Memory1 region on AHB*/
        gpPcieElbi->pim1MemAddrStartLow = 0x80000000; 
        gpPcieElbi->pim1MemAddrStartHigh = 0x00000000;

	/* Set PCIe->AHB Memory2 region on PCI*/
        gpPcieElbi->pin2MemAddrStartLow = 0x90000000;  	// DDR-B Channel
        gpPcieElbi->pin2MemAddrStartHigh = 0x00000000;
        gpPcieElbi->pin2MemAddrLimitLow = 0x9FFFFFFF;  
        gpPcieElbi->pin2MemAddrLimitHigh = 0x00000000;
	/* Set PCIe->AHB Memory2 region on AHB*/
        gpPcieElbi->pim2MemAddrStartLow = 0x90000000; 
        gpPcieElbi->pim2MemAddrStartHigh = 0x00000000;

#if 0
	/* Set PCIe->AHB Memory3 region on AHB*/
	gpPcieElbi->pim3MemAddrStartLow; 
	gpPcieElbi->pim3MemAddrStartHigh;

	/* Set PCIe->AHB I/O region on PCI*/
	gpPcieElbi->pinIoAddrStartLow;   
	gpPcieElbi->pinIoAddrStartHigh;  
        gpPcieElbi->pinIoAddrLimitLow;   
        gpPcieElbi->pinIoAddrLimitHigh;  
	/* Set PCIe->AHB I/O region on AHB*/
        gpPcieElbi->pimIoAddrStartLow;   
        gpPcieElbi->pimIoAddrStartHigh;  

	/* Set PCIe->AHB Rom region on AHB*/
        gpPcieElbi->pimRomAddrStartLow;  
        gpPcieElbi->pimRomAddrStartHigh; 
#endif

}

#define RESET_RELEASE(sw_reset_nr, bit_nr)
#define RESET_SET(sw_reset_nr, bit_nr)

static void __init pcie_phy_init (void)
{
	volatile u8 regVal;
	int nr = 0;

	// Initialize phy cmu 
	for (nr = 0; nr < 115; nr++) 
		gpPciePhyCmu->reg[nr] = (u32) gPhyCmu_table[nr];

	// Initialize phy Lane0
	for (nr = 0; nr < 9; nr++) // 0 ~ 8
		gpPciePhyLane0->reg[nr] = (u32) gPhyLane0_table[nr];
	for (nr = 16; nr < 82; nr++) // 16 ~ 81
		gpPciePhyLane0->reg[nr] = (u32) gPhyLane0_table[nr];

	// Initialize phy Common Lane block
	for (nr = 0; nr < 24; nr++)  // 0 ~ 23
		gpPciePhyCmnLaneBlock->reg[nr] = (u32) gPhyCmnLaneBlock_table[nr];
	for (nr = 48; nr < 151; nr++) // 48 ~ 150
		gpPciePhyCmnLaneBlock->reg[nr] = (u32) gPhyCmnLaneBlock_table[nr];

	regVal = (u8)gpPciePhyCmu->reg[0];
	gpPciePhyCmu->reg[0] = 0x07;

	//Un-reset PHY CMU ->  PMU 300908E0 , [18] set
	RESET_RELEASE(0, 18);
	
	// wait to phy cmu pll to lock
	do{
		if(gpPciePortLogic->phyStatus & 0x1) break;	
	}while(1);

/* for test 
	regVal = (u8)gpPciePhyCmnLaneBlock.reg[0];	
	gpPciePhyCmnLaneBlock.reg[0] = (u32) 0xC0;	
	regVal = (u8)gpPciePhyCmnLaneBlock.reg[0];	
	regVal = (u8)gpPciePhyCmnLaneBlock.reg[0];	
	gpPciePhyCmnLaneBlock.reg[0] = (u32) 0xC2;	
for test end */

	//Un-reset PHY Lane0 -> PMU 300908E0 , [24] set
	RESET_RELEASE(0, 24);

	do{
		if(gpPcieElbi->userConfig0 & (1 << 21)) break;	
	}while(1);

}

void __init pci_sdp_pre_init(void)
{
	unsigned long flags;
	u32 regVal = 0xFFFFFFFF;
	u8 * pTemp;

// power on reset PCIE 300908E0 (1 << 27) CMU Reset !!!!!!
// Auto un-reset power on reset PCIE

	spin_lock_irqsave(&sdp_lock, flags);

	hook_fault_code(4, sdp_fault, SIGBUS, "external abort on linefetch");
	hook_fault_code(6, sdp_fault, SIGBUS, "external abort on linefetch");
	hook_fault_code(8, sdp_fault, SIGBUS, "external abort on non-linefetch");
	hook_fault_code(10, sdp_fault, SIGBUS, "external abort on non-linefetch");

	/* PCI compatilbe header ~0xFF*/
	pTemp = (u8 *)ioremap_nocache(PA_PCIE_CDM_BASE, 0x1000);

	gpPciHeader =(PCI_HEADER_T*) pTemp;
	gpPciPwm = (PCI_PWM_T*)(pTemp + OFFSET_PCI_PWM);
	gpPciMsi = (PCI_MSI_T*)(pTemp + OFFSET_PCI_MSI);
	gpPciPcie = (PCI_PCIE_T*)(pTemp + OFFSET_PCI_PCIE);
	gpPciMsiX = (PCI_MSI_X_T*)(pTemp + OFFSET_PCI_MSI_X);
	gpPciSlotId = (PCI_SLOT_ID_T*)(pTemp + OFFSET_PCI_SLOT_ID);
	gpPciVpd = (PCI_VPD_T*)(pTemp + OFFSET_PCI_VPD);
	/* PCIE Extended register 0x100 ~ */
	gpPcieExt = (PCIE_EXT_T*)(pTemp + OFFSET_PCIE_EXT);
	/* PCIE Port logic register 0x700 ~ 0xFFF */
	gpPciePortLogic = (PCIE_PORT_LOGIC_T*)(pTemp + OFFSET_PCIE_PORT_LOGIC);

	/* PCIE ELBI Register */
	gpPcieElbi = (PCIE_ELBI_T *)ioremap_nocache(PA_PCIE_ELBI_BASE, 0x200);

	/* PCIE PHY Register */
	pTemp = (u8 *)ioremap_nocache(PA_PCIE_PHY_BASE, 0x1000);
	gpPciePhyCmu = (PCIE_PHY_CMU_T *) pTemp;
	gpPciePhyLane0 = (PCIE_PHY_LANE0_T *)(pTemp + OFFSET_PCIE_PHY_LANE0);
	gpPciePhyCmnLaneBlock = (PCIE_PHY_CMN_LANE_BLOCK_T *) 
		ioremap_nocache(PA_PCIE_PHY_CMN_LANE_BLK_BASE, 0x300);

	// Enable LTSSM
	gpPcieElbi->userCtrl0 = gpPcieElbi->userCtrl0 | PCIE_ELBI_UCTRL0_APP_LTSSM_ENABLE;

	// phy initialize 
	pcie_phy_init();
	// PCIe Root Complex initialize
	pcie_root_complex_init();

	// Clear and un-mask user one pulse config0 interrupt source 
	regVal = 0xFFFFFFFF;

	gpPcieElbi->userOnePulseConfig1 = 0;	// interrupt mask
	gpPcieElbi->userConfig9 = 0;		// interrupt mask
	gpPcieElbi->userOnePulseConfig0 = regVal;	// interrupt status clear
	gpPcieElbi->userConfig8 = regVal;		// interrupt status clear

	// Set PCIe Memory Transaction 
	gpPcieElbi->slvAWCtrl = 0;	// Memory Write Transaction 
	gpPcieElbi->slvARCtrl = 0;	// Memory Write Transaction 

	request_irq(IRQ_PCI, sdp_pci_irq, IRQF_SHARED,"PCI irq", (void *)0xa700);

	spin_unlock_irqrestore(&sdp_lock, flags);
	
}


void __init pci_sdp_post_init(void)
{
	unsigned long flags;
	const u32 regVal = 0xFFFFFFFF;

	spin_lock_irqsave(&sdp_lock, flags);

	gpPcieElbi->userOnePulseConfig1 = regVal;	// interrupt unmask
	gpPcieElbi->userConfig9 = regVal;		// interrupt unmask

	spin_unlock_irqrestore(&sdp_lock, flags);
}

