/*
 * kdebugd.h
 *
 * Copyright (C) 2009 Samsung Electronics
 * Created by lee jung-seung
 *
 * NOTE:
 *
 */

#ifndef __KDEBUGD_H__
#define __KDEBUGD_H__

/*      
 * Events (results of Get Kdebugd Event)
 */     

typedef unsigned int debugd_event_t;
#define DEBUGD_MAX_EVENTS         64 
#define DEBUGD_MAX_CHARS          16
struct debugd_queue {
    unsigned int            event_head;
    unsigned int            event_tail;
    debugd_event_t		events[DEBUGD_MAX_EVENTS];
};

extern char sched_serial[10];
extern void dump_pid_maps(struct task_struct* task);

extern unsigned int kdebugd_running;
extern struct debugd_queue kdebugd_queue;
extern int kdebugd_start(void);
extern void queue_add_event(struct debugd_queue *q, debugd_event_t event);

extern unsigned long kdbg_get_uptime(void);

extern unsigned int debugd_get_event(void);
extern int kdbg_register(char* name,int (*func)(void), void (*turnoff)(void));

extern void task_state_help(void);

extern struct task_struct *find_user_task_with_pid(void);

#ifdef CONFIG_KDEBUGD_COUNTER_MONITOR
typedef int (*counter_monitor_func_t)(void);

extern int register_counter_monitor_func(counter_monitor_func_t func);
#endif

#endif	// __KDEBUGD_H__
