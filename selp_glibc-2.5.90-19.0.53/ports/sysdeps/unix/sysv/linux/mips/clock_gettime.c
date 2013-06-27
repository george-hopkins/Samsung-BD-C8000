#ifdef SHARED

#include "time.h"

/* Prevent the normal definition of clock_gettime.  */
#undef librt_hidden_def
#define librt_hidden_def(sym)

#include <sysdeps/unix/sysv/linux/clock_gettime.c>

#include <shlib-compat.h>
asm (".globl _clock_gettime");
asm ("_clock_gettime = __GI_clock_gettime");
asm (".globl __clock_gettime");
asm ("__clock_gettime = __GI_clock_gettime");
compat_symbol (librt, _clock_gettime, clock_gettime, GLIBC_2_0);
versioned_symbol (librt, __clock_gettime, clock_gettime, GLIBC_2_2);

#else
#include <sysdeps/unix/sysv/linux/clock_gettime.c>
#endif
