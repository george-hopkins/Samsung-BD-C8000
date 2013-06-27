/*
 *  linux/kernel/sec_topthread.h
 *
 *  CPU Performance Profiling Solution, topthread releated functions definitions
 *
 *  Copyright (C) 2009  Samsung	
 *
 *  2009-05-11  Created by Karunanithy.
 *
 */

#ifndef _LINUX_SEC_TOPTHREAD_H
#define _LINUX_SEC_TOPTHREAD_H

/* consider less then 200 thread will run in a second */
#define TOPTHREAD_MAX_THREAD            200     

/* to show cpu usage in 00.00% format */
#define TOPTHREAD_CPUUSAGE_DECIMAL_FORMAT       100

/* threadinfo display duration in sec */
#define TOPTHREAD_DELAY_SECS            1       
#define TOPTHREAD_UPDATE_TICKS          (TOPTHREAD_DELAY_SECS * HZ)
#define TOPTHREAD_TITLE_ROW_1           0       /* title row one */
#define TOPTHREAD_TITLE_ROW_2           1       /* title row two */
#define TOPTHREAD_TITLE_ROW_3           2       /* title row thredd */

/*
  * Last 120 topthread info will be buffered in proc
  */
#define TOPTHREAD_MAX_PROC_ENTRY	CONFIG_KDEBUGD_TOPTHREAD_BUFFER_SIZE

#define TOPTHREAD_MAX_ITEM              CONFIG_KDEBUGD_MAX_TOPTHREAD

/* top thread usage storage structure */
struct topthread_info
{
        pid_t pid;                                              /* to store process ID */
        unsigned int cpu_time;         /* to store cpu usage */
};

/* top thread buffer structure */
struct topthread_buffer
{
        unsigned long sec;
        pid_t pid[TOPTHREAD_MAX_ITEM];                          /* to store process ID */
        unsigned int cpu_time[TOPTHREAD_MAX_ITEM];      /* to store topthread cpu time */
        unsigned int total;                                             /* to store total cpu time */
};

/* 
  * To enable/disable topthread cpu usage display 
  */
extern void sec_topthread_show_cpu_usage (void);

/* 
  * Function used to show the last n items of the topthread info from topthread queue
  */
extern void sec_topthread_dump_cpu_usage ( void );

/* 
  * This function is used to handle topthread info storage and validation
  * It is called from timer interrupt handler
  */
extern void sec_topthread_timer_interrupt_handler(void);

#endif	/* !_LINUX_SEC_TOPTHREAD_H */

