/*
 * memory-corona.h : Define memory address space.
 * 
 * Copyright (C) 2009 Samsung Electronics co.
 * <tukho.kim@samsung.com>
 *
 */
#ifndef _MEMORY_CORONA_H_
#define _MEMORY_CORONA_H_

/* Address space for BD memory map */

#if defined(CONFIG_CORONA_MEMMAP_40_20)
#define SYS_MEM0_SIZE		(40 << 20)
#define SYS_MEM1_SIZE		(20 << 20)
#elif defined(CONFIG_CORONA_MEMMAP_40_30)
#define SYS_MEM0_SIZE		(40 << 20)
#define SYS_MEM1_SIZE		(30 << 20)
#elif defined(CONFIG_CORONA_MEMMAP_40_130)
#define SYS_MEM0_SIZE		(40 << 20)
#define SYS_MEM1_SIZE		(130 << 20)
#elif defined(CONFIG_CORONA_MEMMAP_90_80)
#define SYS_MEM0_SIZE		(89 << 20)
#define SYS_MEM1_SIZE		(80 << 20)
#elif defined(CONFIG_CORONA_MEMMAP_50_50)
#define SYS_MEM0_SIZE		(50 << 20)
#define SYS_MEM1_SIZE		(50 << 20)
#elif defined(CONFIG_CORONA_MEMMAP_56_66)
#define SYS_MEM0_SIZE		(56 << 20)
#define SYS_MEM1_SIZE		(66 << 20)
#elif defined(CONFIG_CORONA_MEMMAP_50_300)
#define SYS_MEM0_SIZE		(50 << 20)
#define SYS_MEM1_SIZE		(300 << 20)
#else
#define SYS_MEM0_SIZE		(128 << 20)
#define SYS_MEM1_SIZE		(128 << 20)
#endif

#define MACH_MEM0_BASE 		0x60000000
#define MACH_MEM0_SIZE 		SZ_256M
#define MACH_MEM1_BASE 		0x80000000
#if defined(CONFIG_CORONA_MEMMAP_50_300)
#define MACH_MEM1_SIZE 		SZ_512M
#else
#define MACH_MEM1_SIZE 		SZ_256M
#endif

#define MACH_BD_SUB0_BASE	0xd8000000
#define MACH_BD_SUB0_SIZE	(MACH_MEM0_SIZE - SYS_MEM0_SIZE)
#define MACH_BD_SUB1_BASE	0xe4000000
#define MACH_BD_SUB1_SIZE	(MACH_MEM1_SIZE - SYS_MEM1_SIZE)

#define PHYS_OFFSET		MACH_MEM0_BASE

/* ===========================================================
 * Discontinous Memory configuration 
 * ==========================================================*/
#ifdef CONFIG_DISCONTIGMEM
/* Valencia DDR Bank
 * node 0:  0x60000000-0x7FFFFFFF - 512MB
 * node 1:  0x80000000-0x9FFFFFFF - 512MB
 */

#define RANGE(n, start, size)   \
	        ((unsigned long)n >= start && (unsigned long)n < (start+size))

/* Kernel size limit 512Mbyte */
#define KVADDR_MASK	0x1FFFFFFF
/* Bank size limit 512MByte */
#define MACH_MEM_MASK	0x1FFFFFFF
#define MACH_MEM_SHIFT	31

#ifndef __ASSEMBLY__
extern const unsigned int sdp_sys_mem0_size;
#endif

#define __phys_to_virt(x) \
	(((x >> MACH_MEM_SHIFT) & 1) ? \
		(PAGE_OFFSET + sdp_sys_mem0_size + (x & MACH_MEM_MASK) ) : \
		(PAGE_OFFSET + (x & MACH_MEM_MASK)) )

#define __virt_to_phys(x) \
	(RANGE(x, PAGE_OFFSET, sdp_sys_mem0_size) ? \
		(MACH_MEM0_BASE + (x & KVADDR_MASK)) : \
		(MACH_MEM1_BASE + ((x & KVADDR_MASK) - sdp_sys_mem0_size)) )

#define KVADDR_TO_NID(addr) \
	(RANGE(addr, PAGE_OFFSET, sdp_sys_mem0_size) ? 0 : 1)

#define PFN_TO_NID(pfn) \
	(((pfn) >> (MACH_MEM_SHIFT - PAGE_SHIFT)) & 1)

#define LOCAL_MAP_PFN_NR(addr)  ( (((unsigned long)addr & MACH_MEM_MASK) >> PAGE_SHIFT)  )

#define LOCAL_MAP_KVADDR_NR(kaddr) \
	((((unsigned long)kaddr & KVADDR_MASK) < sdp_sys_mem0_size) ? \
	((unsigned long)kaddr & KVADDR_MASK) >> PAGE_SHIFT : \
	(((unsigned long)kaddr & KVADDR_MASK) - sdp_sys_mem0_size) >> PAGE_SHIFT )

#endif	/* CONFIG_DISCONTIGMEM */

#endif

