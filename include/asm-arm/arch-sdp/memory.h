/****************************************************************8
 * linux/include/asm-arm/arch-sdp/memory.h
 * 
 * Copyright (C) 2009 Samsung Electronics co.
 * Author : tukho.kim@samsung.com
 *
 */
#ifndef __ASM_ARCH_MEMORY_H
#define __ASM_ARCH_MEMORY_H

/*
 * Virtual view <-> DMA view memory address translations
 * virt_to_bus: Used to translate the virtual address to an
 *              address suitable to be passed to set_dma_addr
 * bus_to_virt: Used to convert an address for DMA operations
 *              to an address that the kernel can use.
 */
#define __virt_to_bus(x)	 __virt_to_phys(x)
#define __bus_to_virt(x)	 __phys_to_virt(x)

#if defined(CONFIG_MACH_LEONID)
#   include <asm/arch/memory-leonid.h>
#elif defined(CONFIG_MACH_AQUILA)
#   include <asm/arch/memory-aquila.h>
#elif defined(CONFIG_MACH_CORONA)
#   include <asm/arch/memory-corona.h>
#endif 

#endif /*__ASM_ARCH_MEMORY_H*/

