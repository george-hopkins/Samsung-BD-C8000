/*
 *  linux/arch/arm/mach-ssdtv/pci.c
 *
 *  Copyright (C) 1999 ARM Limited
 *  Copyright (C) 2007 Samsung electronics.co
 *
 */
#include <linux/kernel.h>
#include <linux/pci.h>
#include <linux/ptrace.h>
#include <linux/interrupt.h>
#include <linux/init.h>

#include <asm/irq.h>
#include <asm/system.h>

#include <asm/mach/pci.h>
#include <asm/mach-types.h>

static inline int bridge_swizzle(int pin, unsigned int slot) 
{
	return (pin + slot) & 3;
}

/*
 * This routine handles multiple bridges.
 */
u8 __init ssdtv_swizzle(struct pci_dev *dev, u8 *pinp)
{
	int pin = *pinp;

	if (pin == 0)
		pin = 1;

	pin -= 1;
	while (dev->bus->self) {
		pin = bridge_swizzle(pin, PCI_SLOT(dev->devfn));
		/*
		 * move up the chain of bridges, swizzling as we go.
		 */
		dev = dev->bus->self;
	}
	*pinp = pin + 1;

	return PCI_SLOT(dev->devfn);
}

extern void pci_ssdtv_pre_init(void);
extern void pci_ssdtv_post_init(void);

extern int ssdtv_map_irq(struct pci_dev *dev, u8 slot, u8 pin);
extern int ssdtv_setup(int nr, struct pci_sys_data *sys);
extern struct pci_bus *ssdtv_scan_bus(int nr, struct pci_sys_data *sys);

static struct hw_pci ssdtv_pci __initdata = {
	.swizzle		= ssdtv_swizzle,
	.map_irq		= ssdtv_map_irq,
	.setup			= ssdtv_setup,
	.nr_controllers		= 1,
	.scan			= ssdtv_scan_bus,
	.preinit		= pci_ssdtv_pre_init,
	.postinit               = pci_ssdtv_post_init,
};

extern void pci_common_init(struct hw_pci *hw);

static int __init ssdtv_pci_init(void)
{
	pci_common_init(&ssdtv_pci);
	return 0;
}

subsys_initcall(ssdtv_pci_init);
