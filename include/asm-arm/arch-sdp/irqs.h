/*
 * linux/include/asm-arm/arch-ssdtv/irqs.h
 *
 * Copyright (C) 2006 SAMSUNG ELECTRONICS  
 * Author : tukho.kim@samsung.com
 *
 */

#ifndef __SDP_IRQS_H
#define __SDP_IRQS_H

#if defined(CONFIG_ARCH_SDP83)
#include "irqs-sdp83.h"
#elif defined(CONFIG_ARCH_SDP92)
#include "irqs-sdp92.h"
#elif defined(CONFIG_ARCH_SDP93)
#include "irqs-sdp93.h"
#endif

#endif /* __SDP_IRQS_H */

