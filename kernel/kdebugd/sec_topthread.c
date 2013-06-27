/*
 *  linux/kernel/sec_topthread.c
 *
 *  CPU Performance Profiling Solution, topthread releated functions
 *
 *  Copyright (C) 2009  Samsung	
 *
 *  2009-05-11  Created by Karunanithy & Namit.
 *
 */

#include <linux/proc_fs.h>
#include <linux/kdebugd.h>
#include <linux/sec_topthread.h>
#include <asm/uaccess.h>

#include <linux/sec_topthread.h>

#define JIFFI_TO_PERCENT(x)	(((int)(x)) * 100 / (int)HZ)

/* to store all running thread info */
static struct topthread_info topthread_buff[TOPTHREAD_MAX_THREAD];
static int max_no_of_thread=0;	/* max thread added in topthread_buff */
static int topthread_disp_flag = 0;	/* top thread display using sysrq on/off flag */

/* to store top 5 thread info to store at proc file */
static struct topthread_buffer topthread_proc_buff[TOPTHREAD_MAX_PROC_ENTRY];
static int topthread_idx;		/* write index of the circular buffer array */
static int topthread_buffer_full;		/* flag which incates buffer full state */

/* This function is to show topthread info header */
static void topthread_get_header (int header_row)
{
	int loop;

	/* write cp thread info header */
	switch ( header_row )
	{
		case TOPTHREAD_TITLE_ROW_1:
			printk ("\n\n");
			printk ("time    total ");

			for ( loop=0; loop<TOPTHREAD_MAX_ITEM; loop++) 
				printk("top#%d     ", loop+1);

			printk("Etc\n");
			break;

		case TOPTHREAD_TITLE_ROW_2:
			printk("             ");

			for ( loop=0; loop<TOPTHREAD_MAX_ITEM; loop++)
				printk("thread    ");

			printk("thread\n");
			break;


		case TOPTHREAD_TITLE_ROW_3:
			printk("======= ===== ");

			for ( loop=0; loop<TOPTHREAD_MAX_ITEM; loop++)
				printk("========= ");

			printk("========\n");
			break;

		default:
			printk ("Invalid header row %d\n", header_row );
			break;
	}
}

/* This function is to show topthread info header */
static void topthread_show_header (void)
{
	topthread_get_header (TOPTHREAD_TITLE_ROW_1);
	topthread_get_header (TOPTHREAD_TITLE_ROW_2);
	topthread_get_header (TOPTHREAD_TITLE_ROW_3);
}

/*
 * This function is to show topthread info in the following format
 * 1sec   80.00%   21(60.00%)  22(15.00%)  50(5.00%)   81(0%)     85(0%)  (0%)
 * 2sec  100.00%  21(80.00%)  22(15.00%)  50(5.00%)   81(0%)     85(0%)  (0%)
 * 3sec   50.00%   22(40.00%)  50(5.00%)    21(5.00%)   81(0%)     85(0%)  (0%)
 * 4sec   40.00%   25(20.00%)  21(10.00%)  45(5.00%)   81(5.00%) 85(0%)  (0%)
 */
static void topthread_show_given_entry (int idx)
{
	unsigned int topthread_cpu_time = 0;
	int loop=0;
	unsigned int excess_usage;

	if (idx > (TOPTHREAD_MAX_PROC_ENTRY-1) )
		/* invalid entry */
		return;

	/* to show sec */
	printk ("%4ldSec ", topthread_proc_buff[idx].sec);

	/* to show total cpu usage */
	printk("%4d%% ", JIFFI_TO_PERCENT(topthread_proc_buff[idx].total));

	/* to show pid & cpu usage */
	while (loop < TOPTHREAD_MAX_ITEM ){
		if (topthread_proc_buff[idx].pid[loop] != 0) {
			printk("%3d",topthread_proc_buff[idx].pid[loop]);
			printk("(%3d%%) ", topthread_proc_buff[idx].cpu_time[loop]);
		} else {
			printk("   (  0%%) ");
		}

		topthread_cpu_time += 
			topthread_proc_buff[idx].cpu_time[loop];

		loop++;
	}

	/* to show running thread cpu usage excludes top 5 thread  */
	excess_usage = 
		( JIFFI_TO_PERCENT(topthread_proc_buff[idx].total)  - topthread_cpu_time);
	printk(" (%3d%%)\n", excess_usage);
}

/* To update the running task info in topthread structure */
static void topthread_store_info( void )
{
	int loop;
	pid_t cur_pid = current->pid;

	/* no need to store idle task time */
	if (cur_pid == 0 )
		return;

	/* calculate all task cpu usage */
	for(loop=0;loop<max_no_of_thread;loop++) {
		if (cur_pid == topthread_buff[loop].pid){ 
			topthread_buff[loop].cpu_time += jiffies_to_cputime(1);

			return;
		}
	}

	/*
	 * we expect there is no more than 250 (TOPTHREAD_MAX_THREAD)
	 * threads running between 1 second, so we don't need to care about 
	 * the else part, ie., if (max_no_of_thread > TOPTHREAD_MAX_THREAD)
	 */

	if( max_no_of_thread < TOPTHREAD_MAX_THREAD ) {
		topthread_buff[max_no_of_thread].pid  = cur_pid;
		topthread_buff[max_no_of_thread].cpu_time = jiffies_to_cputime(1);
		max_no_of_thread++;
	} else {
		printk (KERN_WARNING "Can not add thread info, max item reached\n");
	}
}

/*
 * recusively quick sort the list.
 * example
 *	Before: 3 2 1 6 1 1 33 10 1 40
 *	After : 40 33 10 6 3 2 1 1 1 1
 */
void topthread_q_sort(struct topthread_info buffer[], unsigned int left, unsigned int right)
{
        unsigned int pivot;
	unsigned int l_hold, r_hold;
	int tmp_pid;

        l_hold = left;
        r_hold = right;
        pivot = buffer[left].cpu_time;
	tmp_pid = buffer[left].pid;

        while (left < right)
        {
                // if the value is equal or bigger than pivot, don't need to move.
                while ((buffer[right].cpu_time <= pivot) && (left < right))
                        right --;

                // if the value is smaller, current value move to pivot location.
                if (left != right)
                {
                        buffer[left].cpu_time = buffer[right].cpu_time;
                        buffer[left].pid = buffer[right].pid;
                }
                // find the bigger value than pivot from left to current location.
                while ((buffer[left].cpu_time >= pivot) && (left < right))
                        left ++;
                if (left != right)
                {
                        buffer[right].cpu_time = buffer[left].cpu_time;
                        buffer[right].pid = buffer[left].pid;
                        right --;
                }
        }
        // if scan is finished, change the pivot location to current.
        // there are only samller value on the left-side of pivot.
        buffer[left].cpu_time = pivot;
        buffer[left].pid = tmp_pid;

        pivot = left;
        left = l_hold;
        right = r_hold;

        // recursive call
        if (left < pivot)
                topthread_q_sort(buffer, left, pivot - 1);
        if (right > pivot)
                topthread_q_sort(buffer, pivot+1, right);
}

/* quick sort the list.
 * example
 *	Before: 3 2 1 6 1 1 33 10 1 40
 *	After : 40 33 10 6 3 2 1 1 1 1
 */

void topthread_quickSort(struct topthread_info buffer[], int array_size)
{
        topthread_q_sort(buffer, 0, array_size -1);
}

/* Find top 5 thread  info and display */
static void topthread_find_top5thread_n_display(void)
{
	int loop;
	static int time_count=0;		/* to keep the seconds display */

	/* initialize to zero */
	memset(&topthread_proc_buff[topthread_idx], 0, 
			sizeof(struct topthread_buffer));

	/* Sort the array of thread cpu usage
	 * to reduce the rearrange time use the quick sort algorithm
	 */
	if (max_no_of_thread > 0)
		topthread_quickSort(topthread_buff, max_no_of_thread);

	/* get the data from rearranged array of thread cpu usage */
	topthread_proc_buff[topthread_idx].sec = kdbg_get_uptime();

	for(loop=0; loop<max_no_of_thread; loop++)
	{
		if (loop<TOPTHREAD_MAX_ITEM) {
			topthread_proc_buff[topthread_idx].pid[loop] = 
							topthread_buff[loop].pid;
			topthread_proc_buff[topthread_idx].cpu_time[loop] = 
				JIFFI_TO_PERCENT(topthread_buff[loop].cpu_time);
		}

		topthread_proc_buff[topthread_idx].total += 
					topthread_buff[loop].cpu_time;
	}

	/* to reset tothread buffer just reset the below variable */
	max_no_of_thread = 0;

	/* if show flag is on, then show it to user */
	if (topthread_disp_flag) {
		time_count++;

		if ((time_count%20)== 0)
			topthread_show_header ();

		topthread_show_given_entry (topthread_idx);
	}

	/* if the cpu usage is non zero then increment the circular buffer write index */
	topthread_idx++;

	if( topthread_idx >= TOPTHREAD_MAX_PROC_ENTRY) {
		topthread_buffer_full = 1;
		topthread_idx = 0;
	}
}
/* Turn off the prints of topthread
 */
void turnoff_topthread (void)
{
	if (topthread_disp_flag == 1) {
		topthread_disp_flag = 0;

		printk("\nTOPTHREAD> Off\n");
	}
}

/*
 * This function is called at every topthread info sysrq request 
 * It is used to enable/disable the topthread info display
 * Once it is enabled, it will the topthread header text and reset necessary flags
 */
void sec_topthread_show_cpu_usage (void)
{
	topthread_disp_flag = (topthread_disp_flag) ? 0 : 1;

	if ( topthread_disp_flag ) {
		printk("TOPTHREAD> On\n");

		topthread_show_header ();
	} else {
		printk("TOPTHREAD> Off\n");
	}
}

/* 
 * This will dump the bufferd data of topthread cpu usage from the buffer.
 * This Function is called from the kdebug menu. 
 */
void sec_topthread_dump_cpu_usage (void)
{
	int i = 0;
	int buffer_count = 0;
	int idx = 0;

	if(topthread_buffer_full){
		buffer_count = TOPTHREAD_MAX_PROC_ENTRY;
		idx = topthread_idx;
	} else {
		buffer_count = topthread_idx;
		idx = 0;
	}

	topthread_show_header ();

	for(i = 0; i<buffer_count; i++, idx++) {
		idx = idx % TOPTHREAD_MAX_PROC_ENTRY;

		topthread_show_given_entry(idx);
	}
}

/* This function is called at every timer interrupt tick.
 * update topthread buffer 
 */
void sec_topthread_timer_interrupt_handler(void)
{
	static unsigned long prev_jiffi = 0;
	unsigned long cur_jiffi = jiffies;
	unsigned long diff_jiffi;

	/* when the system is botting, prevjiffies should be reset */
	if (0 == prev_jiffi) {
		prev_jiffi = jiffies;
	}

	/* add and update task info into topthreadinfo structure */
	topthread_store_info ();

	/* calculate 1sec difference inorder to show the top 5 thread info periodically */
	diff_jiffi = cur_jiffi - prev_jiffi;

	if(diff_jiffi >= TOPTHREAD_UPDATE_TICKS){
		prev_jiffi = cur_jiffi;
		/* to update top5 thread info to circular buffer  */
		topthread_find_top5thread_n_display();
	}
}

static int sec_topthread_control(void)
{
        int operation = 0;
	int ret=1;

        printk("\nSelect Operation....");
        printk("\n1. Turn On/Off the CPU Usage prints for each thread");
        printk("\n2. Dump CPU Usage history for each thread(%d sec)", TOPTHREAD_MAX_PROC_ENTRY);
        printk("\n==>  ");

        operation = debugd_get_event();

	printk("\n\n");

        switch(operation)
        {
                case 1:
                        sec_topthread_show_cpu_usage();
			ret = 0;	/* don't print the menu */
                        break;
                case 2:
                        sec_topthread_dump_cpu_usage();
                        break;
                default:
                        break;
        }
        return ret;
}

/*
 * topthread proc init function
 */
static int __init topthread_proc_init(void)
{    
	kdbg_register("Counter Monitor: CPU Usage for eath thread", sec_topthread_control, turnoff_topthread);

	return 0;
}
__initcall(topthread_proc_init);


