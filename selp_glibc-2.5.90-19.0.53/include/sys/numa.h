/* Copyright (C) 2003,2004 Andi Kleen, SuSE Labs.

   libnuma is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; version
   2.1.

   libnuma is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should find a copy of v2.1 of the GNU Lesser General Public License
   somewhere on your Linux system; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA */

#ifndef _NUMA_H
#define _NUMA_H 1

/* Simple NUMA poliy library */

#include <string.h>

#if defined(__x86_64__) || defined(__i386__)
#define NUMA_NUM_NODES 	64
#else
#define NUMA_NUM_NODES	1024
#endif

typedef struct {
	unsigned long n[NUMA_NUM_NODES/(sizeof(unsigned long)*8)];
} nodemask_t;

static inline void nodemask_zero(nodemask_t *mask)
{
	memset(mask->n, 0, sizeof(mask->n));
}

static inline void nodemask_set(nodemask_t *mask, int node)
{
	mask->n[node / (8*sizeof(unsigned long))] |=
		(1UL<<(node%(8*sizeof(unsigned long))));
}

static inline void nodemask_clr(nodemask_t *mask, int node)
{
	mask->n[node / (8*sizeof(unsigned long))] &=
		~(1UL<<(node%(8*sizeof(unsigned long))));
}
static inline int nodemask_isset(nodemask_t *mask, int node)
{
	if ((unsigned)node >= NUMA_NUM_NODES)
		return 0;
	if (mask->n[node / (8*sizeof(unsigned long))] &
		(1UL<<(node%(8*sizeof(unsigned long)))))
		return 1;
	return 0;
}
static inline int nodemask_equal(nodemask_t *a, nodemask_t *b)
{
	int i;
	for (i = 0; i < NUMA_NUM_NODES/(sizeof(unsigned long)*8); i++)
		if (a->n[i] != b->n[i])
			return 0;
	return 1;
}
static inline int nodemask_empty(nodemask_t *mask)
{
	int i;
	for (i = 0; i < NUMA_NUM_NODES/(sizeof(unsigned long)*8); i++)
		if (mask->n[i])
			return 0;
	return 1;
}

#endif
