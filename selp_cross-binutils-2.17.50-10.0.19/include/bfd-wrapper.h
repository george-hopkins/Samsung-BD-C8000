#ifndef __MV_BFD_WRAPPER_H
#define __MV_BFD_WRAPPER_H

#ifdef __x86_64__
#include <bfd64.h>
#else
#include <bfd32.h>
#endif

#endif /* ! __MV_BFD_WRAPPER_H */
