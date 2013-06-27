/*
 * kdbg-func.c
 *
 * Copyright (C) 2009 Samsung Electronics
 * Created by lee jung-seung
 *
 * NOTE:
 *
 */

#include <linux/poll.h>
#include <linux/kthread.h>
#include <linux/ktime.h>
#include <linux/kdebugd.h>
#include <linux/proc_fs.h>
#include <asm/pgtable.h>

extern struct task_struct* get_task_with_pid(void);
extern unsigned int debugd_get_event(void);
extern int show_state_prio(void);
extern void turnoff_cpu_usage(void);
extern int show_cpu_usage(void);
extern int show_sched_history(void);

extern struct proc_dir_entry *kdebugd_dir;
struct proc_dir_entry *kdebugd_cpu_dir;
unsigned int kdebugd_nobacktrace = 0;

static int pctracing = 0;
/* kdebugd Functions */

static int show_task_state(void)
{
	kdebugd_nobacktrace = 1;
	show_state();
    task_state_help();

	return 1;
}

static int show_task_state_backtrace(void)
{
	kdebugd_nobacktrace = 0;
	show_state();

	return 1;
}

static int kill_task_for_coredump(void)
{
	struct task_struct* tsk;

	tsk = get_task_with_pid();
	if(tsk == NULL ){
		printk("\n[ALERT] NO Thread");
		return 1;	/* turn on the menu */
	}

	/* Send Signal */
	force_sig(SIGABRT, tsk);

	return 1;	/* turn on the menu */
}

/* 6. Dump task register with pid */
static int show_user_thread_regs(void)
{
    struct task_struct* tsk;
    struct pt_regs* regs;

	tsk = get_task_with_pid();
	if(tsk == NULL ){
		printk("\n[ALERT] NO Thread");
		return 1;	/* turn on the menu */
	}
	regs = task_pt_regs(tsk);
	__show_regs(regs);

	return 1;	/* turn on the menu */
}

/* 7. Dump task pid maps with pid */
static void __show_user_maps_with_pid(void)
{
    struct task_struct* tsk;

	tsk = find_user_task_with_pid();
	if (tsk == NULL) return;
	show_pid_maps(tsk);
}

static int show_user_maps_with_pid(void)
{
	__show_user_maps_with_pid();

	return 1;
}

/* 8. Dump user stack with pid */
static void __show_user_stack_with_pid(void)
{
    struct task_struct* tsk=NULL;
    struct pt_regs* regs=NULL;

	tsk = find_user_task_with_pid();

	if (tsk == NULL) return;

	regs = task_pt_regs(tsk);

	show_user_stack(tsk, regs);
}

static int show_user_stack_with_pid(void)
{
	__show_user_stack_with_pid();

	return 1;
}


/* 10. Memory Value Watcher - Data Breakpoint */
/* See that Page : 
http://stackoverflow.com/questions/56340/can-i-set-a-data-breakpoint-in-runtime-in-system-c-or-in-plain-vanilla-c 
 */

struct mem_watch {
	unsigned int pid;
	unsigned int tgid;
	unsigned int u_addr;
	unsigned int buff;
	unsigned int watching;
};

struct mem_watch  mem_watcher ={0,};
/* Look up the first VMA which satisfies  addr < vm_end,  NULL if none. */
	struct vm_area_struct* 
find_correspond_vma(struct mm_struct * mm, unsigned long addr)
{
	struct vm_area_struct *vma = NULL;

	if (mm) {
		struct rb_node * rb_node;

		rb_node = mm->mm_rb.rb_node;
		vma = NULL;

		while (rb_node) {
			struct vm_area_struct * vma_tmp;

			vma_tmp = rb_entry(rb_node,
					struct vm_area_struct, vm_rb);

			if (vma_tmp->vm_end > addr) {
				if (vma_tmp->vm_start <= addr)
				{
					vma = vma_tmp;
					break;
				}
				rb_node = rb_node->rb_left;
			} else
				rb_node = rb_node->rb_right;
		}
	}
	return vma;
}
/* FIXME: It is EXPERIMENTAL function. */
unsigned long get_kaddr(struct mm_struct *mm,unsigned long u_addr)
{
	pgd_t *pgd;
	pmd_t *pmd;
	pte_t *pte;

	unsigned long pfn, p_addr, k_addr;

	/* Start page table walking..! */
	pgd = mm->pgd;
	if (pgd_none(*pgd))
		return 0;
	if (pgd_bad(*pgd))
		return 0;

	pmd = pmd_offset(pgd, u_addr);
	if (pmd_none(*pmd))
		return 0;
	if (pmd_bad(*pmd))
		return 0;

	pte = pte_offset_map(pmd, u_addr);
	if(!pte_present(*pte))
		return 0;

	pfn = pte_pfn(*pte);

	p_addr = __pfn_to_phys(pfn) + (0x00000FFF & u_addr);
	k_addr = __phys_to_virt(p_addr);

	printk("\n=====================================================");
	printk("\n Page Table walking...!");
	printk("\n PFN :0x%08lx\n PHYS_ADDR: 0x%08lx ",pfn,__pfn_to_phys(pfn));
	printk("\n VIRT_ADDR: 0x%08lx + 0x%08lx"
			,__phys_to_virt(__pfn_to_phys(pfn))
			,0x00000FFF & u_addr);
	printk("\n VALUE    : 0x%08lx (addr:0x%08lx)",*(unsigned long*)k_addr,k_addr);

	return k_addr;
}
static void __memory_validator(void)
{
	struct task_struct* tsk;
	struct pt_regs* regs;
	struct vm_area_struct *vma = NULL;
	struct mm_struct *next_mm;
	mm_segment_t fs; 
	unsigned long k_addr;

	tsk = get_task_with_pid();
	if(tsk == NULL || !tsk->mm){
		printk("\n[ALERT] %s Thread", (tsk==NULL) ? "No":"Kernel");
		return;
	}
	regs = task_pt_regs(tsk);

	mem_watcher.watching = 0;
	next_mm = tsk->active_mm;

	/* get pid */    
	mem_watcher.pid = tsk->pid;
	mem_watcher.tgid = tsk->tgid;

	/* get address */
	printk("\nEnter memory address...."); 
	printk("\n===>  ");	
	mem_watcher.u_addr = debugd_get_event();

	/* memory address check */
	vma = find_correspond_vma(tsk->mm, mem_watcher.u_addr);
	if (vma == NULL) { 
		printk("\n[ERROR] Can not access address (0x%08x).. !!!\n",
				mem_watcher.u_addr); 
		return;
	}

	/* check current active mm */    
	if(current->active_mm != next_mm)
	{
		printk("\n[ALERT] This is Experimental Function..");
		/* 
		 * FIXME : Page table walking alogrithm is NOT perfect..
		 * So don't trust memory value, you should check memory value by yourself. 
		 */
		k_addr = get_kaddr(next_mm,mem_watcher.u_addr) ;
		if( k_addr == 0) {
			printk("\n[ALERT] Page(addr:%08x) is not loaded in memory",mem_watcher.u_addr);
			return;
		}
		mem_watcher.buff = *(unsigned long*)k_addr;
	}

	/* Get User memory value */
	fs = get_fs();
	set_fs(KERNEL_DS);
	__get_user(mem_watcher.buff,(unsigned long*)mem_watcher.u_addr);
	set_fs(fs);

	/* Print Information */
	printk("\n=====================================================");
	printk("\n Memory Value Watcher....! ");
	printk("\n [Trace]  PID:%d  value:0x%08x (addr:0x%08x)",
			mem_watcher.pid, mem_watcher.buff ,mem_watcher.u_addr);
	printk("\n=====================================================");
	mem_watcher.watching = 1;

	return;

	// Check whenever invoke schedule().!!
}

static int memory_validator(void)
{
	__memory_validator();

	return 1;
}

/* 11. Trace thread execution(look at PC) */

struct timespec timespec_interval = { .tv_sec = 0 , .tv_nsec = 100000 } ;

pid_t trace_tsk_pid;
struct hrtimer trace_thread_timer;
static unsigned int prev_pc  = 0;

static enum hrtimer_restart show_pc(struct hrtimer* timer)
{
	unsigned int cur_pc;
	struct task_struct* trace_tsk = 0;
	ktime_t now;
	// Check whether timer is still tracing already dead thread or not
	trace_tsk = find_task_by_pid(trace_tsk_pid);

	if(!trace_tsk) {
		printk("\n [SP_DEBUG] traced task is killed...\n");
		prev_pc = 0;
		return HRTIMER_NORESTART;
	}

	now = ktime_get();
	cur_pc = KSTK_EIP(trace_tsk);

	if(prev_pc != cur_pc)
		printk("\n[SP_DEBUG] Pid:%d  PC:0x%08x\t\t(TIME:%d.%d)",
				trace_tsk_pid,cur_pc,now.tv.sec,now.tv.nsec);

	prev_pc = cur_pc;

	hrtimer_forward(&trace_thread_timer,now,timespec_to_ktime(timespec_interval));
	return HRTIMER_RESTART;
}

static void start_trace_timer(void)
{
	hrtimer_init(&trace_thread_timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
	trace_thread_timer.expires = timespec_to_ktime(timespec_interval);
	trace_thread_timer.function = show_pc;
	hrtimer_start(&trace_thread_timer, trace_thread_timer.expires, HRTIMER_MODE_REL);
}

static inline void end_trace_timer(void)
{
	hrtimer_cancel( &trace_thread_timer );
}

static void turnoff_trace_thread_pc(void)
{
	if( pctracing != 0) {
		printk("\ntrace thread pc OFF!!\n");
		end_trace_timer();
		prev_pc = 0;
		pctracing = 0;
	}
}

static void __trace_thread_pc(void)
{
	struct task_struct* trace_tsk;

	if(pctracing == 0)
	{
		printk("trace thread pc ON!!\n");
		trace_tsk = get_task_with_pid();

		if(trace_tsk == NULL || !trace_tsk->mm){ 
			printk("[ALERT] %s Thread\n", trace_tsk==NULL? "No":"Kernel");
			return;
		}
		trace_tsk_pid = trace_tsk->pid;
		start_trace_timer();
		pctracing = 1;
	}else
	{
		printk("trace thread pc OFF!!\n");
		end_trace_timer();
		prev_pc = 0;
		pctracing = 0;
	}
}

static int trace_thread_pc(void)
{
	__trace_thread_pc();

	return 1;
}

extern char cpu_usage_buffer[1024];
static int proc_cpu_usage(char *page, char **start,
		off_t off, int count,
		int *eof, void *data)
{
	int len;

	len = sprintf(page, "%s",cpu_usage_buffer);

	return len;
}


static int __init kdbg_misc_init(void)
{
	int retval = 0;
	kdebugd_cpu_dir = create_proc_read_entry("cpu_usage",
			0444, kdebugd_dir,
			proc_cpu_usage,
			NULL);
	if( kdebugd_cpu_dir == NULL) {
		remove_proc_entry("cpu_usage",kdebugd_dir);
		retval = -ENOMEM;
		return retval;
	}

	kdbg_register("A list of tasks and their relation information",show_task_state, NULL);
	kdbg_register("A list of tasks and their priority information",show_state_prio, NULL);
	kdbg_register("A list of tasks and their inforamtion + backtrace(kernel)",show_task_state_backtrace, NULL);
	kdbg_register("Kill the task to create coredump", kill_task_for_coredump, NULL);
	kdbg_register("Turn On/Off CFS Scheduler prints",show_cpu_usage, turnoff_cpu_usage);
	kdbg_register("Dump task register with pid",show_user_thread_regs, NULL);
	kdbg_register("Dump task maps with pid",show_user_maps_with_pid, NULL);
	kdbg_register("Dump user stack with pid",show_user_stack_with_pid, NULL);
	kdbg_register("Memory Value Watcher",memory_validator, NULL);
	kdbg_register("Trace thread execution(look at PC)",trace_thread_pc, turnoff_trace_thread_pc);
	kdbg_register("Schedule history logger",show_sched_history, NULL);

	return retval;
}

module_init(kdbg_misc_init)
