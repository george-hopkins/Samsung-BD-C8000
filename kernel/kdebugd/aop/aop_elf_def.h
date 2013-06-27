/*
 *  linux/drivers/oprofile/aop_elf_def.h
 *
 *  ELF file releated defines are defined here
 *
 *  Copyright (C) 2009  Samsung	
 *
 *  2009-09-02 Created by gaurav.jindal@samsung.com.
 *
 */

/* Legal values for e_type (object file type).  */

#ifndef _LINUX_AOP_ELF_DEF_H
#define _LINUX_AOP_ELF_DEF_H

#define AOP_NONE		0		/* No file type */
#define AOP_REL		1		/* Relocatable file */
#define AOP_EXEC		2		/* Executable file */
#define AOP_DYN		3		/* Shared object file */
#define AOP_CORE		4		/* Core file */
#define	AOP_NUM		5		/* Number of defined types */
#define AOP_LOOS		0xfe00		/* OS-specific range start */
#define AOP_HIOS		0xfeff		/* OS-specific range end */
#define AOP_LOPROC	0xff00		/* Processor-specific range start */
#define AOP_HIPROC	0xffff		/* Processor-specific range end */

/* Legal values for e_machine (architecture).  */

#define AOP_NONE		 0		/* No machine */
#define AOP_M32		 1		/* AT&T WE 32100 */
#define AOP_SPARC	 2		/* SUN SPARC */
#define AOP_386		 3		/* Intel 80386 */
#define AOP_68K		 4		/* Motorola m68k family */
#define AOP_88K		 5		/* Motorola m88k family */
#define AOP_860		 7		/* Intel 80860 */
#define AOP_MIPS		 8		/* MIPS R3000 big-endian */
#define AOP_S370		 9		/* IBM System/370 */
#define AOP_MIPS_RS3_LE	10		/* MIPS R3000 little-endian */

#define AOP_PARISC	15		/* HPPA */
#define AOP_VPP500	17		/* Fujitsu VPP500 */
#define AOP_SPARC32PLUS	18		/* Sun's "v8plus" */
#define AOP_960		19		/* Intel 80960 */
#define AOP_PPC		20		/* PowerPC */
#define AOP_PPC64	21		/* PowerPC 64-bit */
#define AOP_S390		22		/* IBM S390 */

#define AOP_V800		36		/* NEC V800 series */
#define AOP_FR20		37		/* Fujitsu FR20 */
#define AOP_RH32		38		/* TRW RH-32 */
#define AOP_RCE		39		/* Motorola RCE */
#define AOP_ARM		40		/* ARM */
#define AOP_FAKE_ALPHA	41		/* Digital Alpha */
#define AOP_SH		42		/* Hitachi SH */
#define AOP_SPARCV9	43		/* SPARC v9 64-bit */
#define AOP_TRICORE	44		/* Siemens Tricore */
#define AOP_ARC		45		/* Argonaut RISC Core */
#define AOP_H8_300	46		/* Hitachi H8/300 */
#define AOP_H8_300H	47		/* Hitachi H8/300H */
#define AOP_H8S		48		/* Hitachi H8S */
#define AOP_H8_500	49		/* Hitachi H8/500 */
#define AOP_IA_64	50		/* Intel Merced */
#define AOP_MIPS_X	51		/* Stanford MIPS-X */
#define AOP_COLDFIRE	52		/* Motorola Coldfire */
#define AOP_68HC12	53		/* Motorola M68HC12 */
#define AOP_MMA		54		/* Fujitsu MMA Multimedia Accelerator*/
#define AOP_PCP		55		/* Siemens PCP */
#define AOP_NCPU		56		/* Sony nCPU embeeded RISC */
#define AOP_NDR1		57		/* Denso NDR1 microprocessor */
#define AOP_STARCORE	58		/* Motorola Start*Core processor */
#define AOP_ME16		59		/* Toyota ME16 processor */
#define AOP_ST100	60		/* STMicroelectronic ST100 processor */
#define AOP_TINYJ	61		/* Advanced Logic Corp. Tinyj emb.fam*/
#define AOP_X86_64	62		/* AMD x86-64 architecture */
#define AOP_PDSP		63		/* Sony DSP Processor */

#define AOP_FX66		66		/* Siemens FX66 microcontroller */
#define AOP_ST9PLUS	67		/* STMicroelectronics ST9+ 8/16 mc */
#define AOP_ST7		68		/* STmicroelectronics ST7 8 bit mc */
#define AOP_68HC16	69		/* Motorola MC68HC16 microcontroller */
#define AOP_68HC11	70		/* Motorola MC68HC11 microcontroller */
#define AOP_68HC08	71		/* Motorola MC68HC08 microcontroller */
#define AOP_68HC05	72		/* Motorola MC68HC05 microcontroller */
#define AOP_SVX		73		/* Silicon Graphics SVx */
#define AOP_ST19		74		/* STMicroelectronics ST19 8 bit mc */
#define AOP_VAX		75		/* Digital VAX */
#define AOP_CRIS		76		/* Axis Communications 32-bit embedded processor */
#define AOP_JAVELIN	77		/* Infineon Technologies 32-bit embedded processor */
#define AOP_FIREPATH	78		/* Element 14 64-bit DSP Processor */
#define AOP_ZSP		79		/* LSI Logic 16-bit DSP Processor */
#define AOP_MMIX		80		/* Donald Knuth's educational 64-bit processor */
#define AOP_HUANY	81		/* Harvard University machine-independent object files */
#define AOP_PRISM	82		/* SiTera Prism */
#define AOP_AVR		83		/* Atmel AVR 8-bit microcontroller */
#define AOP_FR30		84		/* Fujitsu FR30 */
#define AOP_D10V		85		/* Mitsubishi D10V */
#define AOP_D30V		86		/* Mitsubishi D30V */
#define AOP_V850		87		/* NEC v850 */
#define AOP_M32R		88		/* Mitsubishi M32R */
#define AOP_MN10300	89		/* Matsushita MN10300 */
#define AOP_MN10200	90		/* Matsushita MN10200 */
#define AOP_PJ		91		/* picoJava */
#define AOP_OPENRISC	92		/* OpenRISC 32-bit embedded processor */
#define AOP_ARC_A5	93		/* ARC Cores Tangent-A5 */
#define AOP_XTENSA	94		/* Tensilica Xtensa Architecture */
#define AOP_EM_NUM		95
/* Legal values for e_type (object file type).  */

#define ET_NONE		0		/* No file type */
#define ET_REL		1		/* Relocatable file */
#define ET_EXEC		2		/* Executable file */
#define ET_DYN		3		/* Shared object file */
#define ET_CORE		4		/* Core file */
#define	ET_NUM		5		/* Number of defined types */
#define ET_LOOS		0xfe00		/* OS-specific range start */
#define ET_HIOS		0xfeff		/* OS-specific range end */
#define ET_LOPROC	0xff00		/* Processor-specific range start */
#define ET_HIPROC	0xffff		/* Processor-specific range end */


/* If it is necessary to assign new unofficial AOP_* values, please
   pick large random numbers (0x8523, 0xa7f2, etc.) to minimize the
   chances of collision with official or non-GNU unofficial values.  */

#define AOP_ALPHA	0x9026

/* Legal values for e_version (version).  */

#define AOP_EV_NONE		0		/* Invalid ELF version */
#define AOP_EV_CURRENT	1		/* Current version */
#define AOP_EV_NUM		2


#define AOP_OSABI	7		/* OS ABI identification */
#define AOP_ELFOSABI_NONE		0	/* UNIX System V ABI */
#define AOP_ELFOSABI_SYSV		0	/* Alias.  */
#define AOP_ELFOSABI_HPUX		1	/* HP-UX */
#define AOP_ELFOSABI_NETBSD		2	/* NetBSD.  */
#define AOP_ELFOSABI_LINUX		3	/* Linux.  */
#define AOP_ELFOSABI_SOLARIS	6	/* Sun Solaris.  */
#define AOP_ELFOSABI_AIX		7	/* IBM AIX.  */
#define AOP_ELFOSABI_IRIX		8	/* SGI Irix.  */
#define AOP_ELFOSABI_FREEBSD	9	/* FreeBSD.  */
#define AOP_ELFOSABI_TRU64		10	/* Compaq TRU64 UNIX.  */
#define AOP_ELFOSABI_MODESTO	11	/* Novell Modesto.  */
#define AOP_ELFOSABI_OPENBSD	12	/* OpenBSD.  */
#define AOP_ELFOSABI_ARM		97	/* ARM */
#define AOP_ELFOSABI_STANDALONE	255	/* Standalone (embedded) application */

#define EI_ABIVERSION	8		/* ABI version */

//#define EI_PAD		9		/* Byte index of padding bytes */

/* Legal values for ST_BIND subfield of st_info (symbol binding).  */

#define AOP_STB_LOCAL	0		/* Local symbol */
#define AOP_STB_GLOBAL	1		/* Global symbol */
#define AOP_STB_WEAK	2		/* Weak symbol */
#define	AOP_STB_NUM		3		/* Number of defined types.  */
#define AOP_STB_LOOS	10		/* Start of OS-specific */
#define AOP_STB_HIOS	12		/* End of OS-specific */
#define AOP_STB_LOPROC	13		/* Start of processor-specific */
#define AOP_STB_HIPROC	15		/* End of processor-specific */

/* Legal values for ST_TYPE subfield of st_info (symbol type).  */

#define AOP_STT_NOTYPE	0		/* Symbol type is unspecified */
#define AOP_STT_OBJECT	1		/* Symbol is a data object */
#define AOP_STT_FUNC	2		/* Symbol is a code object */
#define AOP_STT_SECTION	3		/* Symbol associated with a section */
#define AOP_STT_FILE	4		/* Symbol's name is file name */
#define AOP_STT_COMMON	5		/* Symbol is a common data object */
#define AOP_STT_TLS		6		/* Symbol is thread-local data object*/
#define	AOP_STT_NUM		7		/* Number of defined types.  */
#define AOP_STT_LOOS	10		/* Start of OS-specific */
#define AOP_STT_HIOS	12		/* End of OS-specific */
#define AOP_STT_LOPROC	13		/* Start of processor-specific */
#define AOP_STT_HIPROC	15		/* End of processor-specific */


/* Symbol table indices are found in the hash buckets and chain table
   of a symbol hash table section.  This special index value indicates
   the end of a chain, meaning no further symbols are found in that bucket.  */

#define AOP_TN_UNDEF	0		/* End of a chain.  */


/* Symbol visibility specification encoded in the st_other field.  */
#define AOP_STV_DEFAULT	0		/* Default symbol visibility rules */
#define AOP_STV_INTERNAL	1		/* Processor specific hidden class */
#define AOP_STV_HIDDEN	2		/* Sym unavailable in other modules */
#define AOP_STV_PROTECTED	3		/* Not preemptible, not exported */


/* Legal values for p_type (segment type).  */

#define	AOP_PT_NULL		0		/* Program header table entry unused */
#define AOP_PT_LOAD		1		/* Loadable program segment */
#define AOP_PT_DYNAMIC	2		/* Dynamic linking information */
#define AOP_PT_INTERP	3		/* Program interpreter */
#define AOP_PT_NOTE		4		/* Auxiliary information */
#define AOP_PT_SHLIB	5		/* Reserved */
#define AOP_PT_PHDR		6		/* Entry for header table itself */
#define AOP_PT_TLS		7		/* Thread-local storage segment */
#define	AOP_PT_NUM		8		/* Number of defined types */
#define AOP_PT_LOOS		0x60000000	/* Start of OS-specific */
#define AOP_PT_GNU_EH_FRAME	0x6474e550	/* GCC .eh_frame_hdr segment */
//#define AOP_PT_GNU_STACK	0x6474e551	/* Indicates stack executability */
#define AOP_PT_GNU_RELRO	0x6474e552	/* Read-only after relocation */
#define AOP_PT_LOSUNW	0x6ffffffa
#define AOP_PT_SUNWBSS	0x6ffffffa	/* Sun Specific segment */
#define AOP_PT_SUNWSTACK	0x6ffffffb	/* Stack segment */
#define AOP_PT_HISUNW	0x6fffffff
#define AOP_PT_HIOS		0x6fffffff	/* End of OS-specific */
#define AOP_PT_LOPROC	0x70000000	/* Start of processor-specific */
#define AOP_PT_HIPROC	0x7fffffff	/* End of processor-specific */


/* Legal values for sh_type (section type).  */

#define AOP_SHT_NULL	  0		/* Section header table entry unused */
#define AOP_SHT_PROGBITS	  1		/* Program data */
#define AOP_SHT_SYMTAB	  2		/* Symbol table */
#define AOP_SHT_STRTAB	  3		/* String table */
#define AOP_SHT_RELA	  4		/* Relocation entries with addends */
#define AOP_SHT_HASH	  5		/* Symbol hash table */
#define AOP_SHT_DYNAMIC	  6		/* Dynamic linking information */
#define AOP_SHT_NOTE	  7		/* Notes */
#define AOP_SHT_NOBITS	  8		/* Program space with no data (bss) */
#define AOP_SHT_REL		  9		/* Relocation entries, no addends */
#define AOP_SHT_SHLIB	  10		/* Reserved */
#define AOP_SHT_DYNSYM	  11		/* Dynamic linker symbol table */
#define AOP_SHT_INIT_ARRAY	  14		/* Array of constructors */
#define AOP_SHT_FINI_ARRAY	  15		/* Array of destructors */
#define AOP_SHT_PREINIT_ARRAY 16		/* Array of pre-constructors */
#define AOP_SHT_GROUP	  17		/* Section group */
#define AOP_SHT_SYMTAB_SHNDX  18		/* Extended section indeces */
//#define	AOP_SHT_NUM		  19		/* Number of defined types.  */
#define AOP_SHT_LOOS	  0x60000000	/* Start OS-specific */
#define AOP_SHT_GNU_LIBLIST	  0x6ffffff7	/* Prelink library list */
#define AOP_SHT_CHECKSUM	  0x6ffffff8	/* Checksum for DSO content.  */
#define AOP_SHT_LOSUNW	  0x6ffffffa	/* Sun-specific low bound.  */
#define AOP_SHT_SUNW_move	  0x6ffffffa
#define AOP_SHT_SUNW_COMDAT   0x6ffffffb
#define AOP_SHT_SUNW_syminfo  0x6ffffffc
#define AOP_SHT_GNU_verdef	  0x6ffffffd	/* Version definition section.  */
#define AOP_SHT_GNU_verneed	  0x6ffffffe	/* Version needs section.  */
#define AOP_SHT_GNU_versym	  0x6fffffff	/* Version symbol table.  */
#define AOP_SHT_HISUNW	  0x6fffffff	/* Sun-specific high bound.  */
#define AOP_SHT_HIOS	  0x6fffffff	/* End OS-specific type */
#define AOP_SHT_LOPROC	  0x70000000	/* Start of processor-specific */
#define AOP_SHT_HIPROC	  0x7fffffff	/* End of processor-specific */
#define AOP_SHT_LOUSER	  0x80000000	/* Start of application-specific */
//#define AOP_SHT_HIUSER	  0x8fffffff	/* End of application-specific */


/* Legal values for sh_flags (section flags).  */

//#define AOP_SHF_WRITE	     (1 << 0)	/* Writable */
//#define AOP_SHF_ALLOC	     (1 << 1)	/* Occupies memory during execution */
//#define AOP_SHF_EXECINSTR	     (1 << 2)	/* Executable */
#define AOP_SHF_MERGE	     (1 << 4)	/* Might be merged */
#define AOP_SHF_STRINGS	     (1 << 5)	/* Contains nul-terminated strings */
#define AOP_SHF_INFO_LINK	     (1 << 6)	/* `sh_info' contains SHT index */
#define AOP_SHF_LINK_ORDER	     (1 << 7)	/* Preserve order after combining */
#define AOP_SHF_OS_NONCONFORMING (1 << 8)	/* Non-standard OS specific handling
					   required */
#define AOP_SHF_GROUP	     (1 << 9)	/* Section is member of a group.  */
#define AOP_SHF_TLS		     (1 << 10)	/* Section hold thread-local data.  */
#define AOP_SHF_MASKOS	     0x0ff00000	/* OS-specific.  */
#define AOP_SHF_MASKPROC	     0xf0000000	/* Processor-specific */
#define AOP_SHF_ORDERED	     (1 << 30)	/* Special ordering requirement
					   (Solaris).  */
#define AOP_SHF_EXCLUDE	     (1 << 31)	/* Section is excluded unless
					   referenced or allocated (Solaris).*/

/* Legal values for e_type (object file type).  */

#define AOP_NONE		0		/* No file type */
#define AOP_REL		1		/* Relocatable file */
#define AOP_EXEC		2		/* Executable file */
#define AOP_DYN		3		/* Shared object file */
#define AOP_CORE		4		/* Core file */
#define	AOP_ET_NUM		5		/* Number of defined types */
#define AOP_LOOS		0xfe00		/* OS-specific range start */
#define AOP_HIOS		0xfeff		/* OS-specific range end */
#define AOP_LOPROC	0xff00		/* Processor-specific range start */
#define AOP_HIPROC	0xffff		/* Processor-specific range end */

/* Like SHN_COMMON but the symbol will be allocated in the .lbss
   section.  */
#define AOP_SHN_X86_64_LCOMMON 	0xff02

#define AOP_SHF_X86_64_LARGE	0x10000000

//<------- For Section Header


/* Names for the values of the `has_arg' field of `struct option'.  */

#define no_argument		0
#define required_argument	1
#define optional_argument	2

#endif /* !_LINUX_AOP_ELF_DEF_H */

