#ifdef SHARED

#include "time.h"

/* Prevent the normal definition of clock_settime.  */
extern __typeof (clock_settime) clock_settime __asm__("__clock_settime");

#include <sysdeps/unix/sysv/linux/clock_settime.c>

#include <shlib-compat.h>
asm (".globl _clock_settime");
asm ("_clock_settime = __clock_settime");
compat_symbol (librt, _clock_settime, clock_settime, GLIBC_2_0);
versioned_symbol (librt, __clock_settime, clock_settime, GLIBC_2_2);

#else
#include <sysdeps/unix/sysv/linux/clock_settime.c>
#endif
