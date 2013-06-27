#ifndef _ASM_X86_LTT_H
#define _ASM_X86_LTT_H
/*
 * linux/include/asm-x86/ltt.h
 *
 * Copyright (C) 2005,2006 - Mathieu Desnoyers (mathieu.desnoyers@polymtl.ca)
 *
 * x86 time and TSC definitions for ltt
 */

#include <linux/timex.h>
#include <asm/processor.h>
#include <asm/atomic.h>

#define LTT_ARCH_TYPE LTT_ARCH_TYPE_X86
#define LTT_ARCH_VARIANT LTT_ARCH_VARIANT_NONE

/* Minimum duration of a probe, in cycles */
#define LTT_MIN_PROBE_DURATION 200

#ifdef CONFIG_HAVE_LTT_SYNTHETIC_TSC
/* Only for testing. Never needed on x86. */
u64 ltt_read_synthetic_tsc(void);
#endif

#ifdef CONFIG_HAVE_LTT_UNSTABLE_TSC
extern cycles_t ltt_last_tsc;
extern int ltt_tsc_is_sync;

/*
 * Support for architectures with non-sync TSCs.
 * When the local TSC is discovered to lag behind the highest TSC counter, we
 * increment the TSC count of an amount that should be, ideally, lower than the
 * execution time of this routine, in cycles : this is the granularity we look
 * for : we must be able to order the events.
 */
static inline cycles_t ltt_async_tsc_read(void)
{
	cycles_t new_tsc;
	cycles_t last_tsc;

	new_tsc = get_cycles_sync();
	do {
		last_tsc = ltt_last_tsc;
		if (new_tsc < last_tsc)
			new_tsc = last_tsc + LTT_MIN_PROBE_DURATION;
		/*
		 * If cmpxchg fails with a value higher than the new_tsc, don't
		 * retry : the value has been incremented and the events
		 * happened almost at the same time.
		 * We must retry if cmpxchg fails with a lower value :
		 * it means that we are the CPU with highest frequency and
		 * therefore MUST update the value.
		 */
	} while (cmpxchg64(&ltt_last_tsc, last_tsc, new_tsc) < new_tsc);
	return new_tsc;
}

static inline u32 ltt_get_timestamp32(void)
{
	if (ltt_tsc_is_sync)
		return (u32)get_cycles_sync(); /* only need the 32 LSB */
	else
		return (u32)ltt_async_tsc_read();
}

static inline u64 ltt_get_timestamp64(void)
{
	if (ltt_tsc_is_sync)
		return get_cycles_sync();
	else
		return ltt_async_tsc_read();
}
#else /* CONFIG_HAVE_LTT_UNSTABLE_TSC */
static inline u32 ltt_get_timestamp32(void)
{
	return (u32)get_cycles_sync(); /* only need the 32 LSB */
}

static inline u64 ltt_get_timestamp64(void)
{
	return get_cycles_sync();
}
#endif /* CONFIG_HAVE_LTT_UNSTABLE_TSC */

/*
 * Periodic IPI to have an upper bound on TSC inaccuracy.
 * TODO: should implement this in ltt-test-tsc.ko.
 */
static inline void ltt_add_timestamp(unsigned long ticks)
{ }

static inline unsigned int ltt_frequency(void)
{
	return cpu_khz;
}

static inline u32 ltt_freq_scale(void)
{
	return 1000;
}

#endif /* _ASM_X86_LTT_H */
