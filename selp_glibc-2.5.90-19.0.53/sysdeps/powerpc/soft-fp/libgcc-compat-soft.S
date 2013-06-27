/* pre-.hidden libgcc compatibility
   Copyright (C) 2002 Free Software Foundation, Inc.
   This file is part of the GNU C Library.

   The GNU C Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2.1 of the License, or (at your option) any later version.

   The GNU C Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with the GNU C Library; if not, write to the Free
   Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
   02111-1307 USA.  */

	.file	"libgcc-compat-soft.S"

#include <shlib-compat.h>

#if SHLIB_COMPAT (libc, GLIBC_2_0, GLIBC_2_2_6)

# ifndef INTUSE
#  define INTUSE(name) name##_internal
# endif

#define __adddf3_v_glibc20	INTUSE (__adddf3)
#define __addsf3_v_glibc20	INTUSE (__addsf3)
#define __divdf3_v_glibc20	INTUSE (__divdf3)
#define __divsf3_v_glibc20	INTUSE (__divsf3)
#define __eqdf2_v_glibc20	INTUSE (__eqdf2)
#define __eqsf2_v_glibc20	INTUSE (__eqsf2)
#define __extendsfdf2_v_glibc20	INTUSE (__extendsfdf2)
#define __fixdfsi_v_glibc20	INTUSE (__fixdfsi)
#define __fixsfsi_v_glibc20	INTUSE (__fixsfsi)
#define __fixunsdfsi_v_glibc20	INTUSE (__fixunsdfsi)
#define __floatsidf_v_glibc20	INTUSE (__floatsidf)
#define __floatsisf_v_glibc20	INTUSE (__floatsisf)
#define __gedf2_v_glibc20	INTUSE (__gedf2)
#define __gesf2_v_glibc20	INTUSE (__gesf2)
#define __gtdf2_v_glibc20	INTUSE (__gtdf2)
#define __gtsf2_v_glibc20	INTUSE (__gtsf2)
#define __ledf2_v_glibc20	INTUSE (__ledf2)
#define __lesf2_v_glibc20	INTUSE (__lesf2)
#define __ltdf2_v_glibc20	INTUSE (__ltdf2)
#define __ltsf2_v_glibc20	INTUSE (__ltsf2)
#define __muldf3_v_glibc20	INTUSE (__muldf3)
#define __mulsf3_v_glibc20	INTUSE (__mulsf3)
#define __nedf2_v_glibc20	INTUSE (__nedf2)
#define __negdf2_v_glibc20	INTUSE (__negdf2)
#define __negsf2_v_glibc20	INTUSE (__negsf2)
#define __nesf2_v_glibc20	INTUSE (__nesf2)
#define __subdf3_v_glibc20	INTUSE (__subdf3)
#define __subsf3_v_glibc20	INTUSE (__subsf3)
#define __truncdfsf2_v_glibc20	INTUSE (__truncdfsf2)

	.symver __adddf3_v_glibc20,__adddf3@GLIBC_2.0
	.symver __addsf3_v_glibc20,__addsf3@GLIBC_2.0
	.symver __divdf3_v_glibc20,__divdf3@GLIBC_2.0
	.symver __divsf3_v_glibc20,__divsf3@GLIBC_2.0
	.symver __eqdf2_v_glibc20,__eqdf2@GLIBC_2.0
	.symver __eqsf2_v_glibc20,__eqsf2@GLIBC_2.0
	.symver __extendsfdf2_v_glibc20,__extendsfdf2@GLIBC_2.0
	.symver __fixdfsi_v_glibc20,__fixdfsi@GLIBC_2.0
	.symver __fixsfsi_v_glibc20,__fixsfsi@GLIBC_2.0
	.symver __fixunsdfsi_v_glibc20,__fixunsdfsi@GLIBC_2.0
	.symver __floatsidf_v_glibc20,__floatsidf@GLIBC_2.0
	.symver __floatsisf_v_glibc20,__floatsisf@GLIBC_2.0
	.symver __gedf2_v_glibc20,__gedf2@GLIBC_2.0
	.symver __gesf2_v_glibc20,__gesf2@GLIBC_2.0
	.symver __gtdf2_v_glibc20,__gtdf2@GLIBC_2.0
	.symver __gtsf2_v_glibc20,__gtsf2@GLIBC_2.0
	.symver __ledf2_v_glibc20,__ledf2@GLIBC_2.0
	.symver __lesf2_v_glibc20,__lesf2@GLIBC_2.0
	.symver __ltdf2_v_glibc20,__ltdf2@GLIBC_2.0
	.symver __ltsf2_v_glibc20,__ltsf2@GLIBC_2.0
	.symver __muldf3_v_glibc20,__muldf3@GLIBC_2.0
	.symver __mulsf3_v_glibc20,__mulsf3@GLIBC_2.0
	.symver __nedf2_v_glibc20,__nedf2@GLIBC_2.0
	.symver __negdf2_v_glibc20,__negdf2@GLIBC_2.0
	.symver __negsf2_v_glibc20,__negsf2@GLIBC_2.0
	.symver __nesf2_v_glibc20,__nesf2@GLIBC_2.0
	.symver __subdf3_v_glibc20,__subdf3@GLIBC_2.0
	.symver __subsf3_v_glibc20,__subsf3@GLIBC_2.0
	.symver __truncdfsf2_v_glibc20,__truncdfsf2@GLIBC_2.0

#ifdef HAVE_DOT_HIDDEN
	.hidden __adddf3
	.hidden __addsf3
	.hidden __divdf3
	.hidden __divsf3
	.hidden __eqdf2
	.hidden __eqsf2
	.hidden __extendsfdf2
	.hidden __fixdfsi
	.hidden __fixsfsi
	.hidden __fixunsdfsi
	.hidden __floatsidf
	.hidden __floatsisf
	.hidden __gedf2
	.hidden __gesf2
	.hidden __gtdf2
	.hidden __gtsf2
	.hidden __ledf2
	.hidden __lesf2
	.hidden __ltdf2
	.hidden __ltsf2
	.hidden __muldf3
	.hidden __mulsf3
	.hidden __nedf2
	.hidden __negdf2
	.hidden __negsf2
	.hidden __nesf2
	.hidden __subdf3
	.hidden __subsf3
	.hidden __truncdfsf2
#endif

	.section	".text"

	.align 2
	.globl __adddf3_v_glibc20
	.type __adddf3_v_glibc20,@function
__adddf3_v_glibc20:
	b __adddf3@local
.Lfe__adddf3:
	.size __adddf3_v_glibc20,.Lfe__adddf3-__adddf3_v_glibc20

	.align 2
	.globl __addsf3_v_glibc20
	.type __addsf3_v_glibc20,@function
__addsf3_v_glibc20:
	b __addsf3@local
.Lfe__addsf3:
	.size __addsf3_v_glibc20,.Lfe__addsf3-__addsf3_v_glibc20

	.align 2
	.globl __divdf3_v_glibc20
	.type __divdf3_v_glibc20,@function
__divdf3_v_glibc20:
	b __divdf3@local
.Lfe__divdf3:
	.size __divdf3_v_glibc20,.Lfe__divdf3-__divdf3_v_glibc20

	.align 2
	.globl __divsf3_v_glibc20
	.type __divsf3_v_glibc20,@function
__divsf3_v_glibc20:
	b __divsf3@local
.Lfe__divsf3:
	.size __divsf3_v_glibc20,.Lfe__divsf3-__divsf3_v_glibc20

	.align 2
	.globl __eqdf2_v_glibc20
	.type __eqdf2_v_glibc20,@function
__eqdf2_v_glibc20:
	b __eqdf2@local
.Lfe__eqdf2:
	.size __eqdf2_v_glibc20,.Lfe__eqdf2-__eqdf2_v_glibc20

	.align 2
	.globl __eqsf2_v_glibc20
	.type __eqsf2_v_glibc20,@function
__eqsf2_v_glibc20:
	b __eqsf2@local
.Lfe__eqsf2:
	.size __eqsf2_v_glibc20,.Lfe__eqsf2-__eqsf2_v_glibc20

	.align 2
	.globl __extendsfdf2_v_glibc20
	.type __extendsfdf2_v_glibc20,@function
__extendsfdf2_v_glibc20:
	b __extendsfdf2@local
.Lfe__extendsfdf2:
	.size __extendsfdf2_v_glibc20,.Lfe__extendsfdf2-__extendsfdf2_v_glibc20

	.align 2
	.globl __fixdfsi_v_glibc20
	.type __fixdfsi_v_glibc20,@function
__fixdfsi_v_glibc20:
	b __fixdfsi@local
.Lfe__fixdfsi:
	.size __fixdfsi_v_glibc20,.Lfe__fixdfsi-__fixdfsi_v_glibc20

	.align 2
	.globl __fixsfsi_v_glibc20
	.type __fixsfsi_v_glibc20,@function
__fixsfsi_v_glibc20:
	b __fixsfsi@local
.Lfe__fixsfsi:
	.size __fixsfsi_v_glibc20,.Lfe__fixsfsi-__fixsfsi_v_glibc20

	.align 2
	.globl __fixunsdfsi_v_glibc20
	.type __fixunsdfsi_v_glibc20,@function
__fixunsdfsi_v_glibc20:
	b __fixunsdfsi@local
.Lfe__fixunsdfsi:
	.size __fixunsdfsi_v_glibc20,.Lfe__fixunsdfsi-__fixunsdfsi_v_glibc20

	.align 2
	.globl __floatsidf_v_glibc20
	.type __floatsidf_v_glibc20,@function
__floatsidf_v_glibc20:
	b __floatsidf@local
.Lfe__floatsidf:
	.size __floatsidf_v_glibc20,.Lfe__floatsidf-__floatsidf_v_glibc20

	.align 2
	.globl __floatsisf_v_glibc20
	.type __floatsisf_v_glibc20,@function
__floatsisf_v_glibc20:
	b __floatsisf@local
.Lfe__floatsisf:
	.size __floatsisf_v_glibc20,.Lfe__floatsisf-__floatsisf_v_glibc20

	.align 2
	.globl __gedf2_v_glibc20
	.type __gedf2_v_glibc20,@function
__gedf2_v_glibc20:
	b __gedf2@local
.Lfe__gedf2:
	.size __gedf2_v_glibc20,.Lfe__gedf2-__gedf2_v_glibc20

	.align 2
	.globl __gesf2_v_glibc20
	.type __gesf2_v_glibc20,@function
__gesf2_v_glibc20:
	b __gesf2@local
.Lfe__gesf2:
	.size __gesf2_v_glibc20,.Lfe__gesf2-__gesf2_v_glibc20

	.align 2
	.globl __gtdf2_v_glibc20
	.type __gtdf2_v_glibc20,@function
__gtdf2_v_glibc20:
	b __gtdf2@local
.Lfe__gtdf2:
	.size __gtdf2_v_glibc20,.Lfe__gtdf2-__gtdf2_v_glibc20

	.align 2
	.globl __gtsf2_v_glibc20
	.type __gtsf2_v_glibc20,@function
__gtsf2_v_glibc20:
	b __gtsf2@local
.Lfe__gtsf2:
	.size __gtsf2_v_glibc20,.Lfe__gtsf2-__gtsf2_v_glibc20

	.align 2
	.globl __ledf2_v_glibc20
	.type __ledf2_v_glibc20,@function
__ledf2_v_glibc20:
	b __ledf2@local
.Lfe__ledf2:
	.size __ledf2_v_glibc20,.Lfe__ledf2-__ledf2_v_glibc20

	.align 2
	.globl __lesf2_v_glibc20
	.type __lesf2_v_glibc20,@function
__lesf2_v_glibc20:
	b __lesf2@local
.Lfe__lesf2:
	.size __lesf2_v_glibc20,.Lfe__lesf2-__lesf2_v_glibc20

	.align 2
	.globl __ltdf2_v_glibc20
	.type __ltdf2_v_glibc20,@function
__ltdf2_v_glibc20:
	b __ltdf2@local
.Lfe__ltdf2:
	.size __ltdf2_v_glibc20,.Lfe__ltdf2-__ltdf2_v_glibc20

	.align 2
	.globl __ltsf2_v_glibc20
	.type __ltsf2_v_glibc20,@function
__ltsf2_v_glibc20:
	b __ltsf2@local
.Lfe__ltsf2:
	.size __ltsf2_v_glibc20,.Lfe__ltsf2-__ltsf2_v_glibc20

	.align 2
	.globl __muldf3_v_glibc20
	.type __muldf3_v_glibc20,@function
__muldf3_v_glibc20:
	b __muldf3@local
.Lfe__muldf3:
	.size __muldf3_v_glibc20,.Lfe__muldf3-__muldf3_v_glibc20

	.align 2
	.globl __mulsf3_v_glibc20
	.type __mulsf3_v_glibc20,@function
__mulsf3_v_glibc20:
	b __mulsf3@local
.Lfe__mulsf3:
	.size __mulsf3_v_glibc20,.Lfe__mulsf3-__mulsf3_v_glibc20

	.align 2
	.globl __nedf2_v_glibc20
	.type __nedf2_v_glibc20,@function
__nedf2_v_glibc20:
	b __nedf2@local
.Lfe__nedf2:
	.size __nedf2_v_glibc20,.Lfe__nedf2-__nedf2_v_glibc20

	.align 2
	.globl __negdf2_v_glibc20
	.type __negdf2_v_glibc20,@function
__negdf2_v_glibc20:
	b __negdf2@local
.Lfe__negdf2:
	.size __negdf2_v_glibc20,.Lfe__negdf2-__negdf2_v_glibc20

	.align 2
	.globl __negsf2_v_glibc20
	.type __negsf2_v_glibc20,@function
__negsf2_v_glibc20:
	b __negsf2@local
.Lfe__negsf2:
	.size __negsf2_v_glibc20,.Lfe__negsf2-__negsf2_v_glibc20

	.align 2
	.globl __nesf2_v_glibc20
	.type __nesf2_v_glibc20,@function
__nesf2_v_glibc20:
	b __nesf2@local
.Lfe__nesf2:
	.size __nesf2_v_glibc20,.Lfe__nesf2-__nesf2_v_glibc20

	.align 2
	.globl __subdf3_v_glibc20
	.type __subdf3_v_glibc20,@function
__subdf3_v_glibc20:
	b __subdf3@local
.Lfe__subdf3:
	.size __subdf3_v_glibc20,.Lfe__subdf3-__subdf3_v_glibc20

	.align 2
	.globl __subsf3_v_glibc20
	.type __subsf3_v_glibc20,@function
__subsf3_v_glibc20:
	b __subsf3@local
.Lfe__subsf3:
	.size __subsf3_v_glibc20,.Lfe__subsf3-__subsf3_v_glibc20

	.align 2
	.globl __truncdfsf2_v_glibc20
	.type __truncdfsf2_v_glibc20,@function
__truncdfsf2_v_glibc20:
	b __truncdfsf2@local
.Lfe__truncdfsf2:
	.size __truncdfsf2_v_glibc20,.Lfe__truncdfsf2-__truncdfsf2_v_glibc20

#endif
