/*
 *  linux/arch/arm/mach-s5h2x/ssdtv_pci.c
 *
 *  PCI functions for ssdtv host PCI bridge
 *
 *  Copyright (C) 1999 ARM Limited
 *  Copyright (C) 2007 Samsung Electronics.co
 *
 */
/* SELP.arm.3.x support A1 2007-10-22 */

#include <linux/kernel.h>
#include <linux/pci.h>
#include <linux/ptrace.h>
#include <linux/slab.h>
#include <linux/ioport.h>
#include <linux/interrupt.h>
#include <linux/spinlock.h>
#include <linux/init.h>

#include <asm/hardware.h>
#include <asm/io.h>
#include <asm/irq.h>
#include <asm/system.h>
#include <asm/mach/pci.h>
#include <asm/delay.h>

#include <asm/arch/platform.h>

#include "ssdtv_pci.h"

#undef DEBUG

#ifdef DEBUG
#define DBG(format, args...)	printk("%s[%d]\t" format, __FUNCTION__, __LINE__, ##args)
#else
#define DBG(format, args...)
#endif

#define DEBUG_BOARD

#define MAX_SLOTS 21 /* AD11 ~ AD31 */

static DEFINE_SPINLOCK(ssdtv_lock);

static unsigned long ssdtv_open_config_window(struct pci_bus *bus, unsigned int devfn, int offset)
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

static void ssdtv_close_config_window(void)
{
	/*
	 * Reassign base1 for use by prefetchable PCI memory
	 */

	/*
	 * And shrink base0 back to a 256M window (NOTE: MAP0 already correct)
	 */
	mb();
}

static int ssdtv_read_config(struct pci_bus *bus, unsigned int devfn, int where,
			  int size, u32 *val)
{
	unsigned long addr;
	unsigned long flags;
	u32 v;

	spin_lock_irqsave(&ssdtv_lock, flags);
	addr = ssdtv_open_config_window(bus, devfn, where);

	switch (size) {
	case 1:
		v = ioread8(addr);
		break;

	case 2:
		v = ioread16(addr);
		break;

	default:
		v = ioread32(addr);
		break;
	}

	ssdtv_close_config_window();
	spin_unlock_irqrestore(&ssdtv_lock, flags);

	*val = v;
	return PCIBIOS_SUCCESSFUL;
}

static int ssdtv_write_config(struct pci_bus *bus, unsigned int devfn, int where,
			   int size, u32 val)
{
	unsigned long addr;
	unsigned long flags;

	spin_lock_irqsave(&ssdtv_lock, flags);
	addr = ssdtv_open_config_window(bus, devfn, where);

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

	ssdtv_close_config_window();
	spin_unlock_irqrestore(&ssdtv_lock, flags);

	return PCIBIOS_SUCCESSFUL;
}


static struct pci_ops pci_ssdtv_ops = {
	.read	= ssdtv_read_config,
	.write	= ssdtv_write_config,
};


static int ssdtv_fault(unsigned long addr, unsigned int fsr, struct pt_regs *regs)
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

int __init pci_ssdtv_setup_resources(struct resource **resource)
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

int __init ssdtv_setup(int nr, struct pci_sys_data *sys)
{
	int ret = 0;

	if (nr == 0) {
		sys->mem_offset = AHB_PCI_MEM_BASE;
		ret = pci_ssdtv_setup_resources(sys->resource);
	}

	return ret;
}

struct pci_bus *ssdtv_scan_bus(int nr, struct pci_sys_data *sys)
{
	return pci_scan_bus(sys->busnr, &pci_ssdtv_ops, sys);
}

int __init ssdtv_map_irq(struct pci_dev *dev, u8 slot, u8 pin)
{
	return IRQ_PCI;
}

irqreturn_t ssdtv_pci_irq(int irq, void *devid, struct pt_regs *regs)

{
	unsigned long status;

	status = R_PCIINTST & PINTST_CLEAR ;
	/*to do : act by status*/

	if(status & PINTST_PRD)
		printk("[KERNEL] PCI reset is deasserted\n");
	if(status & PINTST_PRA)
		printk("[KERNEL] PCI reset is asserted\n");
	if(status & PINTST_MFE) {
		if(status & PINTST_RDE)
			printk("[KERNEL] PCI/CardBus Master read fatal error \n");
		if(status & PINTST_WDE)
			printk("[KERNEL] PCI/CardBus Master write fatal error \n");
		printk("Fatal Error PCI address 0x%08x\n", R_PCIINTAD);
	}

	if(status & PINTST_MPE) {
		if(status & PINTST_RDE)
			printk("[KERNEL] PCI/CardBus Master read parity error \n");
		if(status & PINTST_WDE)
			printk("[KERNEL] PCI/CardBus Master write parity error \n");
		printk("Parity Error PCI address 0x%08x\n", R_PCIINTAD);
	}

	if(status & PINTST_TPE) {
		if(status & PINTST_RDE)
			printk("[KERNEL] PCI/CardBus Target read parity error \n");
		if(status & PINTST_WDE)
			printk("[KERNEL] PCI/CardBus Target write parity error \n");
	}

	if(status & PINTST_PME)
		printk("[KERNEL] PCI PME# asserted\n");
	if(status & PINTST_BPA)
		printk("[KERNEL] PCI PCIBELPA[BEL] is set\n");
	if(status & PINTST_SER)
		printk("[KERNEL] PCI SERR# is asserted in host mode\n");
	if(status & PINTST_AER)
		printk("[KERNEL] PCI AHB Error response is detected\n");
	if(status & PINTST_DME)
		printk("[KERNEL] PCI DMA error\n");
		
	R_PCIINTST = status;

	return IRQ_HANDLED;
}

extern struct pci_ops pci_ssdtv_ops;

void __init pci_ssdtv_pre_init(void)
{
	unsigned long flags;
	unsigned int regVal;
//	int 	maskBit;

	spin_lock_irqsave(&ssdtv_lock, flags);

	hook_fault_code(4, ssdtv_fault, SIGBUS, "external abort on linefetch");
	hook_fault_code(6, ssdtv_fault, SIGBUS, "external abort on linefetch");
	hook_fault_code(8, ssdtv_fault, SIGBUS, "external abort on non-linefetch");
	hook_fault_code(10, ssdtv_fault, SIGBUS, "external abort on non-linefetch");

	R_PCIINTEN	= 0x00000000;
	R_PCICON 	= 0x00000013;

	/* PCIDIAG */
	regVal		= R_PCIDIAG0 & 0xFFFFFFF0;
#if defined(CONFIG_SSDTV_PCI_EXT_CLK)
	regVal		|= (1 << 2);
#endif
	R_PCIDIAG0	= regVal;

	/* PCIRCC */
	regVal 		= R_PCIRCC & 0xFFFFFFF0;
#if defined(CONFIG_SSDTV_PCI_INTERNAL_33MHZ)
	regVal		|= (1 << 3);	/* use 1/2 pci bus clock */
#endif

	/* assert internal reset (drive PCI_NRESET to low)
	 * This is not useful when using external reset */
	R_PCIRCC	= regVal;	/* assert bus & logic reset signal */
	R_PCIRCC	= regVal | 3;	/* release bus & logic reset signal */

	R_PCIBATAPM 	= 0x00000000;  		// PCI Memory Window Base
	R_PCIBATAPI	= 0x00000000;  		// PCI I/O Window Base
	R_PCISET		= 0x00000400;
	
	/* Memory Base Address 0 from PCI to AHB */
	R_PCIBAM0	= ~(MACH_MEM0_SIZE - 1);
	/* Base Address Translation */
	R_PCIBATPA0	= MACH_MEM0_BASE;
	/* location & size */
	R_PCIHBAR0	= MACH_MEM0_BASE;

#if  (N_MEM_CHANNEL > 1)
	/* Memory Base Address 1 from PCI to AHB */
	/* Base Address Mask */
	R_PCIBAM1	= ~(MACH_MEM1_SIZE - 1);
	/* Base Address Translation */
	R_PCIBATPA1	= MACH_MEM1_BASE;
	/* location & size disable */
	R_PCIHBAR1	= MACH_MEM1_BASE;
#endif

	/* I/O Base Address 2 from PCI to AHB */
	/* Base Address Mask */
	R_PCIBAM2	= 0xFFFFF000;
	/* location & size disable */
	R_PCIHBAR2	= 0x30030000;
	/* Base Address Translation */
	R_PCIBATPA2	= 0x30030000;


	R_PCIHLINE	= 0x00001008;

	R_PCIHLTIT	&= 0xffff0000;

	/* Int Line:26(0x1a), Int Pin:INTA# */
	R_PCIHLTIT	|= 0x0000011A;

	R_PCIHTIMER	= 0x00008080;

	/* I/O, Memory, Bus Master, ..., SERR Enable and Clear Status */
	R_PCIHSC 	= 0x00000357;

	R_PCICON 	= 0x00000393;

	R_PCIINTEN	= 0x0;      /* pci interrutp disable*/

	R_PCIINTST	= PINTST_CLEAR;		/* clear */

	request_irq(IRQ_PCI, ssdtv_pci_irq, SA_SHIRQ,"PCI irq", (void *)0xa700);

	spin_unlock_irqrestore(&ssdtv_lock, flags);
	
}

#ifdef CONFIG_SDP74
extern unsigned int sdp74_pci_init(void);
#endif

void __init pci_ssdtv_post_init(void)
{
	unsigned long flags;

#ifdef CONFIG_SDP74
	R_PCIBATAPM 	= sdp74_pci_init(); // PCI Memory Window Base
	printk(KERN_INFO "ssdtv pci memory base is 0x%08x\n", R_PCIBATAPM);
#endif

	spin_lock_irqsave(&ssdtv_lock, flags);

	R_PCIINTST	= PINTST_CLEAR;		/* clear */
	R_PCIINTEN	= PINTEN_ALL_HOST;      /* pci interrutp disable*/

	spin_unlock_irqrestore(&ssdtv_lock, flags);
}

