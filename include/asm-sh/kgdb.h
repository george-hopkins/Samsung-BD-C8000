/*
 * May be copied or modified under the terms of the GNU General Public
 * License.  See linux/COPYING for more information.
 *
 * Based on a file that was modified or based on files by: Glenn Engel,
 * Jim Kingdon, David Grothe <dave@gcom.com>, Tigran Aivazian <tigran@sco.com>,
 * Amit S. Kale <akale@veritas.com>, sh-stub.c from Ben Lee and
 * Steve Chamberlain, Henry Bell <henry.bell@st.com>
 *
 * Maintainer: Tom Rini <trini@kernel.crashing.org>
 *
 */

#ifndef __KGDB_H
#define __KGDB_H

#include <asm-generic/kgdb.h>

/* Based on sh-gdb.c from gdb-6.1, Glenn
     Engel at HP  Ben Lee and Steve Chamberlain */
#define NUMREGBYTES	112	/* 92 */
#define NUMCRITREGBYTES	(9 << 2)
#define BUFMAX		400

#ifndef __ASSEMBLY__
struct kgdb_regs {
	unsigned long regs[16];
	unsigned long pc;
	unsigned long pr;
	unsigned long gbr;
	unsigned long vbr;
	unsigned long mach;
	unsigned long macl;
	unsigned long sr;
};

#define BREAKPOINT()		asm("trapa #0xff");
#define BREAK_INSTR_SIZE	2
#define CACHE_FLUSH_IS_SAFE	1

#endif				/* !__ASSEMBLY__ */
#endif
