/*
 *  linux/include/asm-arm/arch-s5h2111/hardware.h
 *
 *  This file contains the hardware definitions of Lake SOC
 *
 *  Copyright (C) 2003 ARM Limited.
 *  Copyright (C) 2006 Samsung elctronics co.
 *  Author : seongkoo.cheong@samsung.com
 *
 */
#ifndef __ASM_ARCH_HARDWARE_H
#define __ASM_ARCH_HARDWARE_H

#include <asm/sizes.h>
#include <asm/arch/platform.h>

#define pcibios_assign_all_busses() 1
#define PCIBIOS_MIN_IO		0x6000
#define PCIBIOS_MIN_MEM 	0x00100000

#endif

