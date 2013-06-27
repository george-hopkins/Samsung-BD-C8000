/*
 *  linux/kernel/sec_cpuusage.h
 *
 *  CPU Performance Profiling Solution
 *
 *  Copyright (C) 2009  Samsung	
 *
 *  2009-05-11  Created by sk.bansal.
 *
 */
 
#ifndef __LINUX_CPUUSAGE_H__
#define __LINUX_CPUUSAGE_H__

#include <linux/kernel_stat.h>

#include <asm/uaccess.h>
#include <asm/unistd.h>
#include <asm/div64.h>
#include <asm/timex.h>
#include <asm/io.h>

/*
 * Buffer size (default=120)
 */
#define SEC_CPUUSAGE_BUFFER_ENTRIES CONFIG_KDEBUGD_CPUUSAGE_BUFFER_SIZE

/*
The time delay after which the data is updated in the buffer.
It is configurable. This is in seconds.
*/
#define SEC_CPUUSAGE_DELAY_SECS 1

/* Number of ticks after which the data 
  * will be updated in the buffer
  */
#define SEC_CPUUSAGE_UPDATE_TICKS    (SEC_CPUUSAGE_DELAY_SECS * HZ)

/* Number of ticks after which 
  *  the prints will start coming for sysrq
  */
#define SEC_CPUUSAGE_SYSRQ_FACTOR    (CONFIG_HZ / SEC_CPUUSAGE_UPDATE_TICKS)

/*
 * It is called at every timer interrupt tick. 
 */
void sec_cpuusage_interrupt(void);

/*Dump the bufferd data of cpu usage from the buffer.
This Function is called from the kdebug menu. It prints the data 
same as printed by cat /proc/cpuusage-gnuplot*/
void sec_cpuusage_gnuplot_dump(void);

/*Dump the bufferd data of cpu usage from the buffer.
This Function is called from the kdebug menu. It prints the data 
same as printed by cat /proc/cpuusage*/
void sec_cpuusage_dump(void);

/*
 *Turn the prints of cpuusage on 
 *or off depending on the previous status.
 */
void sec_cpuusage_prints_OnOff(void);

/*
 * Structure that holds data.
 */
struct sec_cpuusage_struct
{
    unsigned long sec_count;   /*maintains the seconds 
				count at which the data is updated.*/
    int sys_time;   /*time taken by cpu 
			       while processing in kernel mode*/
    int user_time;   /*time taken by cpu 
                                while processing in user mode*/
};
#endif //__LINUX_CPUUSAGE_H__
