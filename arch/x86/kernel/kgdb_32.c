/*
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2, or (at your option) any
 * later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 */

/*
 * Copyright (C) 2000-2001 VERITAS Software Corporation.
 * Copyright (C) 2007 Wind River Systems, Inc.
 * Copyright (C) 2007 MontaVista Software, Inc.
 */
/*
 *  Contributor:     Lake Stevens Instrument Division$
 *  Written by:      Glenn Engel $
 *  Updated by:	     Amit Kale<akale@veritas.com>
 *  Updated by:	     Tom Rini <trini@kernel.crashing.org>
 *  Updated by:	     Jason Wessel <jason.wessel@windriver.com>
 *  Modified for 386 by Jim Kingdon, Cygnus Support.
 *  Origianl kgdb, compatibility with 2.1.xx kernel by
 *  David Grothe <dave@gcom.com>
 *  Additional support from Tigran Aivazian <tigran@sco.com>
 */

#include <linux/string.h>
#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/smp.h>
#include <linux/spinlock.h>
#include <linux/delay.h>
#include <asm/vm86.h>
#include <asm/system.h>
#include <linux/ptrace.h>		/* for linux pt_regs struct */
#include <linux/kgdb.h>
#include <linux/init.h>
#include <linux/kdebug.h>
#include <asm/apicdef.h>
#include <asm/desc.h>

#include "mach_ipi.h"

/* Put the error code here just in case the user cares.  */
int gdb_i386errcode;
/* Likewise, the vector number here (since GDB only gets the signal
   number through the usual means, and that's not very specific).  */
int gdb_i386vector = -1;

void regs_to_gdb_regs(unsigned long *gdb_regs, struct pt_regs *regs)
{
	gdb_regs[_EAX] = regs->eax;
	gdb_regs[_EBX] = regs->ebx;
	gdb_regs[_ECX] = regs->ecx;
	gdb_regs[_EDX] = regs->edx;
	gdb_regs[_ESI] = regs->esi;
	gdb_regs[_EDI] = regs->edi;
	gdb_regs[_EBP] = regs->ebp;
	gdb_regs[_DS] = regs->xds;
	gdb_regs[_ES] = regs->xes;
	gdb_regs[_PS] = regs->eflags;
	gdb_regs[_CS] = regs->xcs;
	gdb_regs[_PC] = regs->eip;
	gdb_regs[_ESP] = (int)(&regs->esp);
	gdb_regs[_SS] = __KERNEL_DS;
	gdb_regs[_FS] = 0xFFFF;
	gdb_regs[_GS] = 0xFFFF;
}

/*
 * Extracts ebp, esp and eip values understandable by gdb from the values
 * saved by switch_to.
 * thread.esp points to ebp. flags and ebp are pushed in switch_to hence esp
 * prior to entering switch_to is 8 greater then the value that is saved.
 * If switch_to changes, change following code appropriately.
 */
void sleeping_thread_to_gdb_regs(unsigned long *gdb_regs, struct task_struct *p)
{
	gdb_regs[_EAX] = 0;
	gdb_regs[_EBX] = 0;
	gdb_regs[_ECX] = 0;
	gdb_regs[_EDX] = 0;
	gdb_regs[_ESI] = 0;
	gdb_regs[_EDI] = 0;
	gdb_regs[_EBP] = *(unsigned long *)p->thread.esp;
	gdb_regs[_DS] = __KERNEL_DS;
	gdb_regs[_ES] = __KERNEL_DS;
	gdb_regs[_PS] = 0;
	gdb_regs[_CS] = __KERNEL_CS;
	gdb_regs[_PC] = p->thread.eip;
	gdb_regs[_ESP] = p->thread.esp;
	gdb_regs[_SS] = __KERNEL_DS;
	gdb_regs[_FS] = 0xFFFF;
	gdb_regs[_GS] = 0xFFFF;
}

void gdb_regs_to_regs(unsigned long *gdb_regs, struct pt_regs *regs)
{
	regs->eax = gdb_regs[_EAX];
	regs->ebx = gdb_regs[_EBX];
	regs->ecx = gdb_regs[_ECX];
	regs->edx = gdb_regs[_EDX];
	regs->esi = gdb_regs[_ESI];
	regs->edi = gdb_regs[_EDI];
	regs->ebp = gdb_regs[_EBP];
	regs->xds = gdb_regs[_DS];
	regs->xes = gdb_regs[_ES];
	regs->eflags = gdb_regs[_PS];
	regs->xcs = gdb_regs[_CS];
	regs->eip = gdb_regs[_PC];
}

static struct hw_breakpoint {
	unsigned enabled;
	unsigned type;
	unsigned len;
	unsigned addr;
} breakinfo[4] = {
	{ .enabled = 0 },
	{ .enabled = 0 },
	{ .enabled = 0 },
	{ .enabled = 0 },
};

static void kgdb_correct_hw_break(void)
{
	int breakno;
	int breakbit;
	int correctit = 0;
	unsigned long dr7;

	get_debugreg(dr7, 7);
	for (breakno = 0; breakno < 4; breakno++) {
		breakbit = 2 << (breakno << 1);
		if (!(dr7 & breakbit) && breakinfo[breakno].enabled) {
			correctit = 1;
			dr7 |= breakbit;
			dr7 &= ~(0xf0000 << (breakno << 2));
			dr7 |= ((breakinfo[breakno].len << 2) |
				 breakinfo[breakno].type) <<
			       ((breakno << 2) + 16);
			switch (breakno) {
			case 0:
				set_debugreg(breakinfo[0].addr, 0);
				break;

			case 1:
				set_debugreg(breakinfo[1].addr, 1);
				break;

			case 2:
				set_debugreg(breakinfo[2].addr, 2);
				break;

			case 3:
				set_debugreg(breakinfo[3].addr, 3);
				break;
			}
		} else if ((dr7 & breakbit) && !breakinfo[breakno].enabled) {
			correctit = 1;
			dr7 &= ~breakbit;
			dr7 &= ~(0xf0000 << (breakno << 2));
		}
	}
	if (correctit)
		set_debugreg(dr7, 7);
}

static int kgdb_remove_hw_break(unsigned long addr, int len,
				enum kgdb_bptype bptype)
{
	int i;

	for (i = 0; i < 4; i++)
		if (breakinfo[i].addr == addr && breakinfo[i].enabled)
			break;
	if (i == 4)
		return -1;

	breakinfo[i].enabled = 0;
	return 0;
}

static void kgdb_remove_all_hw_break(void)
{
	int i;

	for (i = 0; i < 4; i++)
		memset(&breakinfo[i], 0, sizeof(struct hw_breakpoint));
}

static int kgdb_set_hw_break(unsigned long addr, int len,
			     enum kgdb_bptype bptype)
{
	int i;
	unsigned type;

	for (i = 0; i < 4; i++)
		if (!breakinfo[i].enabled)
			break;
	if (i == 4)
		return -1;

	switch (bptype) {
	case bp_hardware_breakpoint:
		type = 0;
		len  = 1;
		break;
	case bp_write_watchpoint:
		type = 1;
		break;
	case bp_access_watchpoint:
		type = 3;
		break;
	default:
		return -1;
	}

	if (len == 1 || len == 2 || len == 4)
		breakinfo[i].len  = len - 1;
	else
		return -1;

	breakinfo[i].enabled = 1;
	breakinfo[i].addr = addr;
	breakinfo[i].type = type;
	return 0;
}

void kgdb_disable_hw_debug(struct pt_regs *regs)
{
	/* Disable hardware debugging while we are in kgdb */
	set_debugreg(0, 7);
}

void kgdb_post_master_code(struct pt_regs *regs, int e_vector, int err_code)
{
	/* Master processor is completely in the debugger */
	gdb_i386vector = e_vector;
	gdb_i386errcode = err_code;
}

#ifdef CONFIG_SMP
void kgdb_roundup_cpus(unsigned long flags)
{
	send_IPI_allbutself(APIC_DM_NMI);
}
#endif

int kgdb_arch_handle_exception(int e_vector, int signo,
			       int err_code, char *remcom_in_buffer,
			       char *remcom_out_buffer,
			       struct pt_regs *linux_regs)
{
	long addr;
	char *ptr;
	int newPC;
	int dr6;

	switch (remcom_in_buffer[0]) {
	case 'c':
	case 's':
		/* try to read optional parameter, pc unchanged if no parm */
		ptr = &remcom_in_buffer[1];
		if (kgdb_hex2long(&ptr, &addr))
			linux_regs->eip = addr;
		newPC = linux_regs->eip;

		/* clear the trace bit */
		linux_regs->eflags &= ~TF_MASK;
		atomic_set(&cpu_doing_single_step, -1);

		/* set the trace bit if we're stepping */
		if (remcom_in_buffer[0] == 's') {
			linux_regs->eflags |= TF_MASK;
			debugger_step = 1;
			atomic_set(&cpu_doing_single_step,
				   raw_smp_processor_id());
		}

		get_debugreg(dr6, 6);
		if (!(dr6 & 0x4000)) {
			int breakno;
			for (breakno = 0; breakno < 4; ++breakno) {
				if (dr6 & (1 << breakno) &&
				    breakinfo[breakno].type == 0) {
					/* Set restore flag */
					linux_regs->eflags |= X86_EFLAGS_RF;
					break;
				}
			}
		}
		set_debugreg(0, 6);
		kgdb_correct_hw_break();

		return 0;
	}
	/* this means that we do not want to exit from the handler */
	return -1;
}

static inline int single_step_cont(struct pt_regs *regs,
			struct die_args *args)
{
	/* single step exception from kernel space to user space so
	 * eat the exception and continue the process
	 */
	printk(KERN_ERR "KGDB: trap/step from kernel to user space,"
			" resuming...\n");
	kgdb_arch_handle_exception(args->trapnr, args->signr,
			args->err, "c", "", regs);

	return NOTIFY_STOP;
}

static int kgdb_notify(struct notifier_block *self, unsigned long cmd,
		       void *ptr)
{
	struct die_args *args = ptr;
	struct pt_regs *regs = args->regs;

	switch (cmd) {
	case DIE_NMI:
		if (atomic_read(&debugger_active)) {
			/* KGDB CPU roundup */
			kgdb_nmihook(raw_smp_processor_id(), regs);
			return NOTIFY_STOP;
		}
		return NOTIFY_DONE;
	case DIE_NMI_IPI:
		if (atomic_read(&debugger_active)) {
			/* KGDB CPU roundup */
			if (kgdb_nmihook(raw_smp_processor_id(), regs))
				return NOTIFY_DONE;
			return NOTIFY_STOP;
		}
		return NOTIFY_DONE;
	case DIE_NMIWATCHDOG:
		if (atomic_read(&debugger_active)) {
			/* KGDB CPU roundup */
			kgdb_nmihook(raw_smp_processor_id(), regs);
			return NOTIFY_STOP;
		}
		/* Enter debugger */
		break;
	case DIE_DEBUG:
		if (atomic_read(&cpu_doing_single_step) ==
			raw_smp_processor_id() &&
			user_mode(regs))
			return single_step_cont(regs, args);
		/* fall through */
	default:
		if (user_mode(regs))
			return NOTIFY_DONE;
	}

	if (kgdb_may_fault) {
		kgdb_fault_longjmp(kgdb_fault_jmp_regs);
		return NOTIFY_STOP;
	}

	if (kgdb_handle_exception(args->trapnr, args->signr,
	   args->err, regs))
		return NOTIFY_DONE;

	return NOTIFY_STOP;
}

static struct notifier_block kgdb_notifier = {
	.notifier_call = kgdb_notify,
};

int kgdb_arch_init(void)
{
	register_die_notifier(&kgdb_notifier);
	return 0;
}

/*
 * Skip an int3 exception when it occurs after a breakpoint has been
 * removed. Backtrack eip by 1 since the int3 would have caused it to
 * increment by 1.
 */

int kgdb_skipexception(int exception, struct pt_regs *regs)
{
	if (exception == 3 && kgdb_isremovedbreak(regs->eip - 1)) {
		regs->eip -= 1;
		return 1;
	}
	return 0;
}

unsigned long kgdb_arch_pc(int exception, struct pt_regs *regs)
{
	if (exception == 3)
		return instruction_pointer(regs) - 1;

	return instruction_pointer(regs);
}

struct kgdb_arch arch_kgdb_ops = {
	.gdb_bpt_instr = {0xcc},
	.flags = KGDB_HW_BREAKPOINT,
	.set_hw_breakpoint = kgdb_set_hw_break,
	.remove_hw_breakpoint = kgdb_remove_hw_break,
	.remove_all_hw_break = kgdb_remove_all_hw_break,
	.correct_hw_break = kgdb_correct_hw_break,
};
