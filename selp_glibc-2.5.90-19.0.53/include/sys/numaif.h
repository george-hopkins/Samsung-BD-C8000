#ifndef NUMAIF_H
#define NUMAIF_H 1

/* Kernel interface for NUMA API */

/* System calls */
extern long __get_mempolicy(int *__policy,
			    unsigned long *__nmask, unsigned long __maxnode,
			    void *__addr, int __flags);
extern long __mbind(void *__start, unsigned long __len, int __mode,
		    unsigned long *__nmask, unsigned long __maxnode,
		    unsigned __flags);
extern long __set_mempolicy(int __mode, unsigned long *__nmask,
			    unsigned long __maxnode);

/* Policies */
#define MPOL_DEFAULT     0
#define MPOL_PREFERRED   1
#define MPOL_BIND        2
#define MPOL_INTERLEAVE  3

#define MPOL_MAX MPOL_INTERLEAVE

/* Flags for get_mem_policy */
#define MPOL_F_NODE    (1<<0)   /* return next il node or node of address */
#define MPOL_F_ADDR    (1<<1)  /* look up vma using address */

/* Flags for mbind */
#define MPOL_MF_STRICT  (1<<0)  /* Verify existing pages in the mapping */
#define MPOL_MF_MOVE    (1<<1)  /* Attempt to move invalid pages in the
				   mapping */
#define MPOL_MF_NOREPLACE (1<<2) /* do not replace existing shared policies
				    that overlap - applies only to file
				    mappings */

#endif
