/*
 *  linux/kernel/sec_memusage.c
 *
 *  Memory Performance Profiling Solution, memory usage releated functions
 *
 *  Copyright (C) 2009  Samsung	
 *
 *  2009-06-05  Created by Choi Young-Ho
 *
 */
#include <linux/poll.h>
#include <linux/kthread.h>
#include <linux/ktime.h>
#include <linux/time.h>
#include <linux/proc_fs.h>
#include <linux/swap.h>
#include <linux/mm.h>

#include <linux/kdebugd.h>
#include <linux/sec_memusage.h>

/*
 * Turn ON the mem usage processing.
 */
static void sec_memusage_init(void);

/*
 * The buffer that will store mem usage data.
 */
static struct sec_memusage_struct sec_memusage_buffer[SEC_MEMUSAGE_BUFFER_ENTRIES];

/*The index of the buffer array at which the data will be written.*/
static int sec_memusage_index;

/*The flag which incates whether the buffer array is full(value is 1) 
 *or is partially full(value is 0).
 */
static int sec_memusage_is_buffer_full;

/*The flag which will be turned on or off when sysrq feature will 
 *turn on or off respectively.
 */
/* Flag is used to turn the sysrq for mem usage on or off */
static int sec_memusage_status = 0;

/* Timer for polling */
struct hrtimer memory_usage_timer;

/* Interval for timer */
struct timespec show_memory_interval = { .tv_sec = 1, .tv_nsec = 0 };

/* This function is to show header */
static void memusage_show_header(void)
{
	printk("time       total      used       free       buffers    cached\n");
	printk("======== ========== ========== ========== ========== ==========\n");
}

/*Dump the bufferd data of mem usage from the buffer.
  This Function is called from the kdebug menu. It prints the data 
  same as printed by cat /proc/memusage-gnuplot*/
void sec_memusage_gnuplot_dump(void)
{
	int last_row = 0, saved_row = 0;
	int idx, i;
	int limit_count;

	if(sec_memusage_is_buffer_full){
		limit_count = SEC_MEMUSAGE_BUFFER_ENTRIES;
		last_row = sec_memusage_index;
	}else{
		limit_count = sec_memusage_index;
		last_row = 0;
	}
	saved_row = last_row;

	printk("\n\n\n");
        printk("{{{#!gnuplot\n");
        printk("reset\n");
        printk("set title \"Memory Usage\"\n");
        printk("set xlabel \"time(sec)\"\n");
        printk("set ylabel \"Usage(kB)\"\n");
        printk("set yrange [0:%d]\n", sec_memusage_buffer[0].totalram);
        printk("set key invert reverse Left outside\n");
        printk("set key autotitle columnheader\n");
        printk("set auto x\n");
        printk("set xtics nomirror rotate by 90\n");
        printk("set style data histogram\n"); 
        printk("set style histogram rowstacked\n");
        printk("set style fill solid border -1\n");
        printk("set grid ytics\n");
        printk("set boxwidth 0.7\n");
        printk("set lmargin 11\n");
        printk("set rmargin 1\n");
        printk("#\n");
        printk("plot \"-\" using 2:xtic(1), '' using 3, '' using 4\n");

        /* Becuase of gnuplot grammar, we should print data twice
         * to draw the two graphic lines on the chart.
	 */
	for (i=0; i<3; i++)
	{
		printk("time    used    buffers  cached\n");
		for(idx=0;idx < limit_count; idx++)
		{
			int index = last_row % SEC_MEMUSAGE_BUFFER_ENTRIES;
			last_row++;


			printk("%04ld  %7lu %7u %7ld\n", 
					sec_memusage_buffer[index].sec,
					sec_memusage_buffer[index].totalram
						- sec_memusage_buffer[index].freeram
						- sec_memusage_buffer[index].bufferram
						- sec_memusage_buffer[index].cached,
					sec_memusage_buffer[index].bufferram,
					sec_memusage_buffer[index].cached );
		}
		printk("e\n");
		last_row = saved_row;
	}

	printk("}}}\n");
}

/*Dump the bufferd data of mem usage from the buffer.
 *This Function is called from the kdebug menu. 
 */
void sec_memusage_dump(void)
{
	int i = 0;
	int buffer_count = 0;
	int idx = 0;

	if(sec_memusage_is_buffer_full){
		buffer_count = SEC_MEMUSAGE_BUFFER_ENTRIES;
		idx = sec_memusage_index;
	}else{
		buffer_count = sec_memusage_index;
		idx = 0;
	}

	printk("\n\n");
	memusage_show_header();


	for(i = 0; i<buffer_count; ++i, ++idx) {
		idx = idx % SEC_MEMUSAGE_BUFFER_ENTRIES;

		printk("%04ld sec %7u kB %7u kB %7u kB %7u kB %7ld kB\n", 
				sec_memusage_buffer[idx].sec,
				sec_memusage_buffer[idx].totalram,
				sec_memusage_buffer[idx].totalram - sec_memusage_buffer[idx].freeram,
				sec_memusage_buffer[idx].freeram,
				sec_memusage_buffer[idx].bufferram,
				sec_memusage_buffer[idx].cached );
	}
}

#define P2K(x) ((x) << (PAGE_SHIFT - 10))

static void kdbg_show_mem_stat(void)
{
	struct sysinfo i;
	long cached;

	si_meminfo(&i);
	si_swapinfo(&i);
	cached = global_page_state(NR_FILE_PAGES) - 
			total_swapcache_pages - i.bufferram;

	if (cached < 0)
		cached = 0;

	sec_memusage_buffer[sec_memusage_index].sec = kdbg_get_uptime();
	sec_memusage_buffer[sec_memusage_index].totalram = P2K(i.totalram);
	sec_memusage_buffer[sec_memusage_index].freeram = P2K(i.freeram);
	sec_memusage_buffer[sec_memusage_index].bufferram = P2K(i.bufferram);
	sec_memusage_buffer[sec_memusage_index].cached = P2K(cached);

	if (sec_memusage_status)
	{ 
		printk("%04ld sec %7lu kB %7lu kB %7lu kB %7lu kB %7lu kB\n", 
				kdbg_get_uptime(),
				P2K(i.totalram) ,
				P2K(i.totalram) - P2K(i.freeram) ,
				P2K(i.freeram) ,
				P2K(i.bufferram) ,
				P2K(cached) );

		if((sec_memusage_index % 20) == 0)
			memusage_show_header();
	}
	


	sec_memusage_index++;

	if(sec_memusage_index >= SEC_MEMUSAGE_BUFFER_ENTRIES){
		sec_memusage_is_buffer_full = 1;
		sec_memusage_index = 0;
	}              
}


enum hrtimer_restart mem_show_func(struct hrtimer *ht)
{
	ktime_t now;

	kdbg_show_mem_stat();

	now = ktime_get();

	hrtimer_forward(&memory_usage_timer, now, timespec_to_ktime(show_memory_interval));
	return HRTIMER_RESTART;
}               

static void start_memory_usage(void)
{
	hrtimer_init(&memory_usage_timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
	memory_usage_timer.expires = timespec_to_ktime(show_memory_interval);
	memory_usage_timer.function = mem_show_func;
	hrtimer_start(&memory_usage_timer, memory_usage_timer.expires, HRTIMER_MODE_REL);
}


void sec_memusage_init()
{
	/* Turn ON the processing of dumping the mem usage data.*/

	memset(sec_memusage_buffer, 0,  SEC_MEMUSAGE_BUFFER_ENTRIES * 
			sizeof(struct sec_memusage_struct));
	sec_memusage_index = 0;
	sec_memusage_is_buffer_full = 0;

	start_memory_usage();
}

/*
 *Turn off the prints of memusage
 */
void turnoff_memusage(void)
{
	if (sec_memusage_status) {
		sec_memusage_status = 0;
		printk("\nMemory USAGE Dump OFF\n");	
	}
}

/*
 *Turn the prints of memusage on 
 *or off depending on the previous status.
 */
void sec_memusage_prints_OnOff(void)
{
	sec_memusage_status = (sec_memusage_status) ? 0: 1;

	if (sec_memusage_status) {
		printk("Memory USAGE Dump ON\n");	

		memusage_show_header();
	} else {
		printk("Memory USAGE Dump OFF\n");	
	}
}

static int sec_memusage_control(void)
{
	int operation = 0;
	int ret = 1;

	printk("\nSelect Operation....");
	printk("\n1. Turn On/Off the Memory Usage prints");
	printk("\n2. Dump Memory Usage history(%d sec)", SEC_MEMUSAGE_BUFFER_ENTRIES);
	printk("\n3. Dump Memory Usage gnuplot history(%d sec)", SEC_MEMUSAGE_BUFFER_ENTRIES);
	printk("\n==>  ");

	operation = debugd_get_event();

	printk("\n\n");

	switch(operation)
	{
		case 1:
			sec_memusage_prints_OnOff();
			ret = 0;	/* don't print the menu */
			break;
		case 2:
			sec_memusage_dump();
			break;
		case 3:
			sec_memusage_gnuplot_dump();
			break;
		default:
			break;
	}
	return ret;
}

static int __init kdbg_memusage_init(void)
{
	sec_memusage_init();

	kdbg_register("Counter Monitor: Memory Usage", sec_memusage_control, turnoff_memusage);

	return 0;
}  	
__initcall(kdbg_memusage_init);

