/*
 * kdbg-core.c
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

#include "kdbg-version.h"

static DECLARE_WAIT_QUEUE_HEAD(kdebugd_wait);
static DEFINE_SPINLOCK(kdebugd_queue_lock);
struct debugd_queue kdebugd_queue;

struct task_struct *kdebugd_tsk;
unsigned int kdebugd_running = 0;

struct proc_dir_entry *kdebugd_dir;
/*
 * Debugd event queue management.
 */
static inline int queue_empty(struct debugd_queue *q)
{
	return q->event_head == q->event_tail;
}

static inline debugd_event_t queue_get_event(struct debugd_queue *q)
{
	q->event_tail = (q->event_tail + 1) % DEBUGD_MAX_EVENTS;
	return q->events[q->event_tail];
}

void queue_add_event(struct debugd_queue *q, debugd_event_t event)
{
	unsigned long flags;

	spin_lock_irqsave(&kdebugd_queue_lock, flags);
	q->event_head = (q->event_head + 1) % DEBUGD_MAX_EVENTS;
	if (q->event_head == q->event_tail) {
		static int notified = 0;

		if (notified == 0){
			printk("\nkdebugd: an event queue overflowed\n");
			notified = 1;
		}
		q->event_tail = (q->event_tail + 1) % DEBUGD_MAX_EVENTS;
	}
	q->events[q->event_head] = event;
	spin_unlock_irqrestore(&kdebugd_queue_lock, flags);

	wake_up_interruptible(&kdebugd_wait);
}

debugd_event_t debugd_get_event(void)
{
	debugd_event_t event = 0;
	wait_event_interruptible(kdebugd_wait,
			!queue_empty(&kdebugd_queue) || kthread_should_stop());
	spin_lock_irq(&kdebugd_queue_lock);
	if (!queue_empty(&kdebugd_queue))
		event = queue_get_event(&kdebugd_queue);
	spin_unlock_irq(&kdebugd_queue_lock);
	return event;
}

/* command input common code */
struct task_struct* get_task_with_pid(void)
{
	struct task_struct* tsk;

	debugd_event_t event;

	printk("\nEnter pid of task...");	
	printk("\n===>  ");	
	event = debugd_get_event();

	tsk =  find_task_by_pid(event);

	if(tsk == NULL)	return NULL;

	printk("\n\nPid: %d, comm: %20s\n", tsk->pid, tsk->comm);
	return tsk;
}

struct task_struct *find_user_task_with_pid(void)
{
	struct task_struct* tsk;

	tsk = get_task_with_pid();

	if(tsk == NULL || !tsk->mm){
		printk("\n[ALERT] %s Thread", (tsk==NULL) ? "No":"Kernel");
		return NULL;
	}

	return tsk;
}

void task_state_help(void)
{
	printk("\nRMSDTtZX:\n");
	printk("R : TASK_RUNNING         M:TASK_RUNNING_MUTEX S:TASK_INTERRUPTIBLE\n");
	printk("D : TASK_UNITERRUPTIBLE  T:TASK_STOPPED       t:TASK_TRACED\n");
	printk("Z : EXIT_ZOMBIE          X:EXIT_DEAD          \n");
	printk("\nSched Policy\n");
	printk("SCHED_NORMAL : %d\n",SCHED_NORMAL);
	printk("SCHED_FIFO   : %d\n",SCHED_FIFO);
	printk("SCHED_RR     : %d\n",SCHED_RR);
	printk("SCHED_BATCH  : %d\n",SCHED_BATCH);
}

/*
 *    uptime
 */

unsigned long kdbg_get_uptime(void)
{
	struct timespec uptime;

	do_posix_clock_monotonic_gettime(&uptime);

	return (unsigned long) uptime.tv_sec;
}

/*
 *    kdebugd()
 */
#define KDBG_FUNC_NO 30

struct kdbg_entry {
	const char* name;
	int (*execute)(void);
	void (*turnoff)(void);
};

struct kdbg_base {
	unsigned int index;
	struct kdbg_entry entry[KDBG_FUNC_NO];
};

static struct kdbg_base k_base;

int kdbg_register(char* name,int (*func)(void), void (*turnoff)(void))
{
	int idx = k_base.index;
	struct kdbg_entry *cur;

	if(idx >=  KDBG_FUNC_NO ){
		printk(KERN_ERR "[ERROR] Can not add kdebugd function !!!\n");
		return -ENOMEM;
	}

	cur = &(k_base.entry[idx]);

	cur->name = name;
	cur->execute = func;
	cur->turnoff = turnoff;
	k_base.index ++;

	return 0;
}

/* There is no unregister function :) */
#if 0
int kdbg_unregister(void)
{
}
#endif 

static void debugd_menu(void)
{
	int i;
	printk("\n --- Menu ----------------------------------------------------------");
	printk("\n Select Kernel Debugging Category. - %s", KDBG_VERSION);
	for(i = 0 ; i < k_base.index ; i++)
	{
		if( i % 4 == 0 ) 
			printk("\n -------------------------------------------------------------------");
		printk("\n %-2d) %s.",i+1, k_base.entry[i].name); 
	}
	printk("\n -------------------------------------------------------------------");
	printk("\n 99) exit");
	printk("\n -------------------------------------------------------------------");
	printk("\n -------------------------------------------------------------------");
	printk("\n Samsung Electronic - DMC - VD Division - Software Lab1 - SP Part ");
	printk("\n -------------------------------------------------------------------");

}

static int kdebugd(void *arg)
{
	debugd_event_t event;
	unsigned int idx;
	int menu_flag = 1;

	do {
		if (kthread_should_stop())
			break;

		if (menu_flag) {
			debugd_menu();
			printk("\n%s #> ",sched_serial);
		} else {
			menu_flag = 1;
		}

		event = debugd_get_event(); 
		idx = event - 1;

		/* exit */    
		if(event == 99)
			goto out;

		/* execute operations */
		if(event <= k_base.index && event > 0)
		{
			printk("\n[SP_DEBUG] %d. %s\n",event,k_base.entry[idx].name);
			menu_flag = (*k_base.entry[idx].execute)();
		} else {
			for(idx=0; idx<k_base.index; idx++) {
				if (k_base.entry[idx].turnoff != NULL)
					(*k_base.entry[idx].turnoff)();
			}
		}
	}while(1);

out:
	printk("\n[SP_DEBUG] Kdebugd Exit....");
	kdebugd_running = 0;

	return 0;
}

int kdebugd_start(void)
{
	int ret = 0;

	kdebugd_tsk = kthread_create(kdebugd, NULL, "kdebugd");
	if (IS_ERR(kdebugd_tsk)) {
		ret = PTR_ERR(kdebugd_tsk);
		kdebugd_tsk = NULL;
		return ret;
	} 

	kdebugd_tsk->flags |= PF_NOFREEZE;
	wake_up_process(kdebugd_tsk);

	return ret;
}

static int __init kdebugd_init(void)
{
	int rv = 0;
	kdebugd_dir = proc_mkdir("kdebugd", NULL);
	if(kdebugd_dir == NULL) {
		rv = -ENOMEM;
	}
	return rv;
}
static void __exit kdebugd_cleanup(void)
{
	remove_proc_entry("kdebugd", NULL);
}
module_init(kdebugd_init);
module_exit(kdebugd_cleanup);

