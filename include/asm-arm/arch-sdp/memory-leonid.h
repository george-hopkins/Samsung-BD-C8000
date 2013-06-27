/****************************************************************8
 * linux/include/asm-arm/arch-ssdtv/memory-leonid.h
 * 
 * Copyright (C) 2008 Samsung Electronics co.
 * Author : tukho.kim@samsung.com 06/19/2008
 *
 */
#ifndef __ASM_MACH_LEONID_MEMORY_H
#define __ASM_MACH_LEONID_MEMORY_H

/* Memory Define */
#define N_MEM_CHANNEL			2

#define MACH_MEM0_BASE 			0x60000000	/* West		*/
#define MACH_MEM0_SIZE 			SZ_128M
#define MACH_MEM1_BASE 			0x70000000	/* East		*/
#define MACH_MEM1_SIZE 			SZ_256M

#define SYS_MEM0_SIZE		(68 << 20)
#define SYS_MEM1_SIZE		(101 << 20)

#   define PHYS_OFFSET				(MACH_MEM0_BASE)

/*Define for Dicontnous memory */
#ifdef CONFIG_DISCONTIGMEM
/*
 * The nodes are matched with the physical SDRAM banks as follows
 * Only support West Base 
 * 	node 0:  0x60000000-0x6FFFFFFF - 256MB
 * 	node 1:  0x70000000-0x7FFFFFFF - 256MB
 */
#define RANGE(n, start, size)   \
	        ((unsigned long)n >= start && (unsigned long)n < (start+size))

/* Kernel size limit 256Mbyte */
#define KVADDR_MASK		(0x0FFFFFFF)	
/* Bank  size limit 256MByte */
#define MACH_MEM_MASK	(0x0FFFFFFF)
#define MACH_MEM_SHIFT	(28)	/*0x10000000*/

/* BANK Membase Mapping*/
#define __phys_to_virt(x)  (((x >> MACH_MEM_SHIFT) & 1) ? \
				(PAGE_OFFSET + SYS_MEM0_SIZE + (x & MACH_MEM_MASK) ) : \
				(PAGE_OFFSET + (x & MACH_MEM_MASK)) )

#define __virt_to_phys(x)  (RANGE(x, PAGE_OFFSET, SYS_MEM0_SIZE) ? \
				(MACH_MEM0_BASE + (x & KVADDR_MASK)) : \
				(MACH_MEM1_BASE + ((x & KVADDR_MASK) - SYS_MEM0_SIZE)) )

/*
 * Given a kernel address(virtual address), find the home node of the underlying memory.
 */
#define KVADDR_TO_NID(addr) 	(RANGE(addr, PAGE_OFFSET, SYS_MEM0_SIZE) ? 0 : 1)

/*
 * Given a page frame number, convert it to a node id.
 */
#define PFN_TO_NID(pfn)		(((pfn) >> (MACH_MEM_SHIFT - PAGE_SHIFT)) & 1)

/*
 * Given a kaddr, ADDR_TO_MAPBASE finds the owning node of the memory
 * and returns the mem_map of that node.
 */
#define ADDR_TO_MAPBASE(kaddr)	NODE_MEM_MAP(KVADDR_TO_NID(kaddr))

/*
 * Given a page frame number, find the owning node of the memory
 * and returns the mem_map of that node.
 */
#define PFN_TO_MAPBASE(pfn)	NODE_MEM_MAP(PFN_TO_NID(pfn))

/*
 * Given a kaddr, LOCAL_MEM_MAP finds the owning node of the memory
 * and returns the index corresponding to the appropriate page in the
 * node's mem_map.
 */

/* BANK Membase Mapping*/
#define LOCAL_MAP_NR(addr) 	( (((unsigned long)addr & MACH_MEM_MASK) >> PAGE_SHIFT)  ) 

#endif /* CONFIG_DISCONTIGMEM */

#endif /*__ASM_MACH_LEONID_MEMORY_H*/
