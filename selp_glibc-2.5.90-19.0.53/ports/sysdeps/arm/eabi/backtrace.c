/* Return backtrace of current program state.  ARM EABI version.
   Copyright (C) 1998, 2000, 2002, 2004, 2005 Free Software Foundation, Inc.
   This file is part of the GNU C Library.
   Contributed by Ulrich Drepper <drepper@cygnus.com>, 1998.

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

#include <execinfo.h>
#include <signal.h>
#include <frame.h>
#include <sigcontextinfo.h>
#include <bp-checks.h>
#include <ldsodefs.h>
#include <unwind.h>

struct trace_arg
{
  void **array;
  int cnt, size;
  void *last_frame;
};

#ifdef SHARED
static _Unwind_Reason_Code (*unwind_backtrace) (_Unwind_Trace_Fn, void *);
static _Unwind_VRS_Result (*unwind_vrs_get) (_Unwind_Context *,
					     _Unwind_VRS_RegClass, _uw,
					     _Unwind_VRS_DataRepresentation,
					     void *);
static _Unwind_Ptr (*unwind_getcfa) (struct _Unwind_Context *);
static void *libgcc_handle;

static void
init (void)
{
  libgcc_handle = __libc_dlopen ("libgcc_s.so.1");

  if (libgcc_handle == NULL)
    return;

  unwind_backtrace = __libc_dlsym (libgcc_handle, "_Unwind_Backtrace");
  unwind_vrs_get = __libc_dlsym (libgcc_handle, "_Unwind_VRS_Get");
  unwind_getcfa = __libc_dlsym (libgcc_handle, "_Unwind_GetCFA");
  if (unwind_backtrace == NULL
      || unwind_vrs_get == NULL || unwind_getcfa == NULL)
    {
      unwind_backtrace = NULL;
      __libc_dlclose (libgcc_handle);
      libgcc_handle = NULL;
    }
}
#else
# define unwind_backtrace _Unwind_Backtrace
# define unwind_vrs_get _Unwind_VRS_Get
# define unwind_getcfa _Unwind_GetCFA
#endif

/* On ARM EABI _Unwind_GetIP() is a macro which refers to _Unwind_GetGR(),
   which is a static inline function which calls _Unwind_VRS_Get().
   Define our own version to use our dynamic symbols.  */
static inline _Unwind_Ptr
unwind_getip (struct _Unwind_Context *context)
{
  _Unwind_Word val;

  unwind_vrs_get (context, _UVRSC_CORE, 15, _UVRSD_UINT32, &val);
  return val & ~(_Unwind_Word) 1;
}

static _Unwind_Reason_Code
backtrace_helper (struct _Unwind_Context *ctx, void *a)
{
  struct trace_arg *arg = a;

  /* We are first called with address in the __backtrace function.
     Skip it.  */
  if (arg->cnt != -1)
    arg->array[arg->cnt] = (void *) unwind_getip (ctx);
  if (++arg->cnt == arg->size)
    return _URC_END_OF_STACK;

  arg->last_frame = (void *) unwind_getcfa (ctx);
  return _URC_NO_REASON;
}


/* This implementation assumes a stack layout that matches the defaults
   used by gcc's `__builtin_frame_address' and `__builtin_return_address'
   (FP is the frame pointer register):

	  +-----------------+     +-----------------+
    FP -> | previous FP --------> | previous FP ------>...
	  |                 |     |                 |
	  | return address  |     | return address  |
	  +-----------------+     +-----------------+

   First try as far to get as far as possible using
   _Unwind_Backtrace which handles -fomit-frame-pointer
   as well, but requires .eh_frame info.  Then fall back to
   walking the stack manually.  */

/* Get some notion of the current stack.  Need not be exactly the top
   of the stack, just something somewhere in the current frame.  */
#ifndef CURRENT_STACK_FRAME
# define CURRENT_STACK_FRAME  ({ char __csf; &__csf; })
#endif

/* By default we assume that the stack grows downward.  */
#ifndef INNER_THAN
# define INNER_THAN <
#endif

/* By default assume the `next' pointer in struct layout points to the
   next struct layout.  */
#ifndef ADVANCE_STACK_FRAME
# define ADVANCE_STACK_FRAME(next) BOUNDED_1 ((struct layout *) (next))
#endif

int
__backtrace (array, size)
     void **array;
     int size;
{
  struct trace_arg arg = { .array = array, .size = size, .cnt = -1 };
#ifdef SHARED
  __libc_once_define (static, once);

  __libc_once (once, init);
#endif

  if (unwind_backtrace != NULL && size >= 1)
    unwind_backtrace (backtrace_helper, &arg);
  else
    arg.cnt++;

  if (arg.cnt > 1 && arg.array[arg.cnt - 1] == NULL)
    --arg.cnt;
  else if (arg.cnt < size)
    {
      struct layout *current = arg.last_frame;
      void *__unbounded top_stack = CURRENT_STACK_FRAME;

      while (arg.cnt < size)
	{
	  if ((void *) current INNER_THAN top_stack
	      || !((void *) current INNER_THAN __libc_stack_end))
	    /* This means the address is out of range.  Note that for the
	       toplevel we see a frame pointer with value NULL which clearly
	       is out of range.  */
	    break;

	  array[arg.cnt++] = current->return_address;

	  current = ADVANCE_STACK_FRAME (current->next);
	}
    }
  return arg.cnt != -1 ? arg.cnt : 0;
}
weak_alias (__backtrace, backtrace)
libc_hidden_def (__backtrace)


#ifdef SHARED
/* Free all resources if necessary.  */
libc_freeres_fn (free_mem)
{
  unwind_backtrace = NULL;
  if (libgcc_handle != NULL)
    {
      __libc_dlclose (libgcc_handle);
      libgcc_handle = NULL;
    }
}
#endif
