/*
 *  linux/arch/arm/mach-sdp/pci.c
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
u8 __init sdp_swizzle(struct pci_dev *dev, u8 *pinp)
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

extern void pci_sdp_pre_init(void);
extern void pci_sdp_post_init(void);

extern int sdp_map_irq(struct pci_dev *dev, u8 slot, u8 pin);
extern int sdp_setup(int nr, struct pci_sys_data *sys);
extern struct pci_bus *sdp_scan_bus(int nr, struct pci_sys_data *sys);

static struct hw_pci sdp_pci __initdata = {
	.swizzle		= sdp_swizzle,
	.map_irq		= sdp_map_irq,
	.setup			= sdp_setup,
	.nr_controllers		= 1,
	.scan			= sdp_scan_bus,
	.preinit		= pci_sdp_pre_init,
	.postinit               = pci_sdp_post_init,
};

extern void pci_common_init(struct hw_pci *hw);

static int __init sdp_pci_init(void)
{
	pci_common_init(&sdp_pci);
	return 0;
}

subsys_initcall(sdp_pci_init);
