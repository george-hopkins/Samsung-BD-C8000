/*
 * linux/include/asm-arm/arch-sdp/platform.h
 *
 *  Copyright (C) 2003-2006 Samsung Electronics
 *  Author: tukho.kim@samsung.com
 *
 */

#ifndef __SDP_PLATFORM_H
#define __SDP_PLATFORM_H

#if defined(CONFIG_MACH_LEONID)
#include <asm/arch/leonid.h>
#elif defined(CONFIG_MACH_AQUILA)
#include <asm/arch/aquila.h>
#elif defined(CONFIG_MACH_CORONA)
#include <asm/arch/corona.h>
#else
#error "Please Choose platform\n"
#endif

#endif// __SDP_PLATFORM_H
