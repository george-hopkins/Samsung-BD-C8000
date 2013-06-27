/*
 * Copyright (C)	2002, Karim Yaghmour
 *		 	2005, Mathieu Desnoyers
 *
 * PowerPC definitions for tracing system
 */

#ifndef _ASM_PPC_LTT_H
#define _ASM_PPC_LTT_H

#include <asm-generic/ltt.h>
#include <asm/reg.h>
#include <asm-ppc/time.h>

/* Current arch type */
#define LTT_ARCH_TYPE LTT_ARCH_TYPE_PPC

/* PowerPC variants */
#define LTT_ARCH_VARIANT_PPC_4xx 1	/* 4xx systems (IBM embedded series) */
#define LTT_ARCH_VARIANT_PPC_6xx 2	/* 6xx/7xx/74xx/8260/POWER3 systems
					   (desktop flavor) */
#define LTT_ARCH_VARIANT_PPC_8xx 3	/* 8xx system (Motoral embedded series)
					 */
#define LTT_ARCH_VARIANT_PPC_ISERIES 4	/* 8xx system (iSeries) */

/* Current variant type */
#if defined(CONFIG_4xx)
#define LTT_ARCH_VARIANT LTT_ARCH_VARIANT_PPC_4xx
#elif defined(CONFIG_6xx)
#define LTT_ARCH_VARIANT LTT_ARCH_VARIANT_PPC_6xx
#elif defined(CONFIG_8xx)
#define LTT_ARCH_VARIANT LTT_ARCH_VARIANT_PPC_8xx
#elif defined(CONFIG_PPC_ISERIES)
#define LTT_ARCH_VARIANT LTT_ARCH_VARIANT_PPC_ISERIES
#else
#define LTT_ARCH_VARIANT LTT_ARCH_VARIANT_NONE
#endif

static inline void _ltt_get_tb32(u32 *p)
{
	unsigned lo;
	asm volatile("mftb %0"
		 : "=r" (lo));
	p[0] = lo;
}

static inline u32 ltt_get_timestamp32(void)
{
	u32 ret;
	if (__is_processor(1))
		return ltt_get_timestamp32_generic();

	_ltt_get_tb32((u32 *)&ret);
	return ret;
}

/* from arch/ppc/xmon/xmon.c */
static inline void _ltt_get_tb64(unsigned *p)
{
	unsigned hi, lo, hiagain;

	do {
		asm volatile("mftbu %0; mftb %1; mftbu %2"
			 : "=r" (hi), "=r" (lo), "=r" (hiagain));
	} while (hi != hiagain);
	p[0] = hi;
	p[1] = lo;
}

static inline u64 ltt_get_timestamp64(void)
{
	u64 ret;
	if (__is_processor(1))
		return ltt_get_timestamp64_generic();

	_ltt_get_tb64((unsigned *)&ret);
	return ret;
}

static inline void ltt_add_timestamp(unsigned long ticks)
{
	if (__is_processor(1))
		ltt_add_timestamp_generic(ticks);
}

static inline unsigned int ltt_frequency(void)
{
	if (__is_processor(1))
		return ltt_frequency_generic();
	else
		return (tb_ticks_per_jiffy * HZ);
}

static inline u32 ltt_freq_scale(void)
{
	if (__is_processor(1))
		return ltt_freq_scale_generic();
	else
		return 1;
}

#endif /* _ASM_PPC_LTT_H */
