/* Copyright (C) 2005 MontaVista Sofware, Inc.
 *
 *  Define an ELF note to hold NUMA memory policy information for this
 *  executable.
 *
 *  The linker (GNU ld 2.8 and later) recognizes an allocated section whose
 *  name begins with `.note' and creates a PT_NOTE program header entry
 *  pointing at it.
 */
	.section ".note.numapolicy", "a"
	.p2align 2
	.long 1f - 0f		/* name length */
	.long 3f - 2f		/* data length */
	.long  21		/* note type */
0:	.asciz "MontaVista Software" /* vendor name */
1:	.p2align 2		/* align to 4-byte boundary */
2:	.long 0,0,0		/* text: mode,flags,num_nodes */
	.long 0,0,0		/* data: mode,flags,num_nodes */
	.long 0,0,0		/* proc: mode,flags,num_nodes */
	.rept 8			/* text: 8 nodes max */
	.short 0
	.endr
	.rept 8			/* data: 8 nodes max */
	.short 0
	.endr
	.rept 8			/* proc: 8 nodes max */
	.short 0
	.endr
3:	.p2align 2		/* pad out section */
