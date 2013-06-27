/*
 * kdbg-trace.c (09.04.10)
 *
 * Copyright (C) 2009 Samsung Electronics
 * Created by js.lee (js07.lee@samsung.com)
 *
 * NOTE:
 *
 */

#include <asm/uaccess.h>
#include <linux/kdebugd.h>
#include <linux/mm.h>

extern struct task_struct* get_task_with_pid(void);
extern struct vm_area_struct* 
find_correspond_vma(struct mm_struct * mm, unsigned long addr);

static struct mm_struct* bt_mm;

static inline unsigned int get_user_value(unsigned long* addr)
{
    unsigned int retval;

    if( bt_mm == NULL )
        return 0;

    if(find_correspond_vma(bt_mm,(unsigned long)addr) == NULL){
        printk("\n[ALERT]This task get invalid fp value");
        return 0;
    }

     __get_user(retval, addr);
    return retval;
}

static void show_stackframe(unsigned int* frame)
{     
	unsigned int val[4];
    int i;
    
    for(i = 0; i < 4 ; i++)
    {
        val[i] = get_user_value((unsigned long*)frame++);

        if(val[i] == 0){
            frame = 0;
            return;
        }
    }

	printk("fp : 0x%08x ip : 0x%08x lr : 0x%08x pc : 0x%08x \n",
			val[0],val[1],val[2],val[3]);
    return;
}

static unsigned int 
__user_bt(unsigned int* fp,unsigned int* start, unsigned int* end)
{
	unsigned int* frame_base = fp;
    unsigned int call_depth  = 0;
	printk("\n================================================================================");
	printk("\nStack area(0x%p ~ 0x%p)",start,end);
	printk("\n================================================================================\n");
	while(frame_base > start && frame_base < end && frame_base != 0) {
		frame_base -= 3;
		show_stackframe(frame_base);    
		/* frame_base = *(frame_base); */
        frame_base = (unsigned int*)get_user_value((unsigned long*)frame_base);
        if(call_depth++ > 30) break;
	}
	printk("===============================================================================\n");
    return call_depth;
}
static unsigned int 
user_bt(unsigned fp,unsigned start, unsigned end)
{       
    unsigned int ret = 0;
	mm_segment_t fs;

	if(!fp)
         return ret;
	if(fp < start || fp > end)
        return ret;

	fs = get_fs();
	set_fs(KERNEL_DS);

	/* Backtrace with frame pointer */
	ret = __user_bt((unsigned*)fp,(unsigned*)start,(unsigned*)end);
	set_fs(fs);

	return ret;
}

/* Dump backtrace(User) w/ pid, */
static void __show_user_stack_backtrace(void)
{
    struct task_struct* tsk;
    struct pt_regs* regs;
    struct vm_area_struct *vma;

	tsk = find_user_task_with_pid();
	if (tsk == NULL) return;

    regs = task_pt_regs(tsk);

    bt_mm = tsk->mm;

    vma = find_vma(bt_mm, tsk->user_ssp);
    if (vma) { 
        user_bt(regs->ARM_fp,regs->ARM_sp,vma->vm_end);
    }       
}

static int show_user_stack_backtrace(void)
{
    __show_user_stack_backtrace();

    return 1;
}

static int __init kdbg_trace_init(void)
{
    kdbg_register("Dump backtrace(User)",show_user_stack_backtrace, NULL);
    return 0;
}
__initcall(kdbg_trace_init);
