/*
 *  linux/kernel/sec_diskusage.c
 *
 *  Disk Performance Profiling Solution, disk usage releated functions
 *
 *  Copyright (C) 2009  Samsung	
 *
 *  2009-05-31  Created by Choi Young-Ho, SISC
 *
 */
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/genhd.h>
#include <linux/kernel.h>
#include <linux/blkdev.h>
#include <linux/init.h>
#include <linux/spinlock.h>
#include <linux/seq_file.h>
#include <linux/slab.h>
#include <linux/kmod.h>
#include <linux/kobj_map.h>
#include <linux/buffer_head.h>
#include <linux/mutex.h>

#include <linux/poll.h>
#include <linux/kthread.h>
#include <linux/ktime.h>

#include <linux/proc_fs.h>
#include <linux/kdebugd.h>
#include <linux/sec_diskusage.h>

/*
 * The buffer that will store disk usage data.
 */
struct sec_diskstats_struct sec_diskstats_table[SEC_DISKSTATS_NR_ENTRIES];

/* Buffer Index */
int sec_diskstats_idx = 0;

/*
 * Turn ON the disk usage processing.
 */
static void sec_diskusage_init(void);


/*The flag which incates whether the buffer array is full(value is 1) 
 *or is partially full(value is 0).
 */
int sec_diskusage_is_buffer_full = 0;

/*The flag which will be turned on or off when sysrq feature will 
 *turn on or off respectively.
 * 1 = ON
 * 0 = OFF
 */
int sec_diskusage_status = 0;

/* Mutex lock variable to save data structure while printing gnuplot data*/
DEFINE_MUTEX(diskdump_lock);

/* This function is to show header */
void diskusage_show_header(void)
{
	printk("Time     Utilization  Throughput    Read     Write\n");
	printk("======== ===========  ==========  ========  ========\n");
}

/* Dump the buffered data of disk usage from the buffer.
 * It prints history of disk usage (max 120 sec).
 */
static void dump_disk_result(void)
{
	int i = 0;
	int buffer_count = 0;
	int idx = 0;

	if (sec_diskusage_is_buffer_full) {
		buffer_count = SEC_DISKSTATS_NR_ENTRIES;
		idx = sec_diskstats_idx;
	} else {
		buffer_count = sec_diskstats_idx;
		idx = 0;
	}
	
	printk("Time     Utilization  Throughput    Read     Write\n");
	printk("======== ===========  ==========  ========  ========\n");

	for(i = 0; i < buffer_count; ++i, ++idx) {
		idx = idx % SEC_DISKSTATS_NR_ENTRIES;

		printk("%4ld Sec       %3lu %%    %5lu KB  %5lu KB  %5lu KB\n", 
				sec_diskstats_table[idx].sec,
				sec_diskstats_table[idx].utilization, 
				sec_diskstats_table[idx].nkbyte_read 
					+ sec_diskstats_table[idx].nkbyte_write, 
				sec_diskstats_table[idx].nkbyte_read, 
				sec_diskstats_table[idx].nkbyte_write );
	}
}

/* Dump the buffered data of disk usage from the buffer.
 * It prints history of disk usage (max 120 sec).
 */
static void dump_gnuplot_disk_result(void)
{
	int i = 0;
	int buffer_count = 0;
	int idx = 0;

	if (sec_diskusage_is_buffer_full) {
		buffer_count = SEC_DISKSTATS_NR_ENTRIES;
		idx = sec_diskstats_idx;
	} else {
		buffer_count = sec_diskstats_idx;
		idx = 0;
	}
	
	printk("Time     Utilization  Throughput    Read     Write\n");

	for(i = 0; i < buffer_count; ++i, ++idx) {
		idx = idx % SEC_DISKSTATS_NR_ENTRIES;

		printk("%4ld           %3lu       %5lu     %5lu     %5lu   \n", 
				sec_diskstats_table[idx].sec,
				sec_diskstats_table[idx].utilization, 
				sec_diskstats_table[idx].nkbyte_read 
					+ sec_diskstats_table[idx].nkbyte_write, 
				sec_diskstats_table[idx].nkbyte_read, 
				sec_diskstats_table[idx].nkbyte_write );
	}
}

/* Dump the bufferd data of disk usage from the buffer.
 * This Function is called from the kdebug menu. 
 * It prints the gnuplot data.
 */
void sec_diskusage_gnuplot_dump(void)
{
	printk("\n\n");

	printk("{{{#!gnuplot\n");
	printk("reset\n");
	printk("set title \"Disk Usage\"\n");
	printk("set ylabel \"Throughput(KBytes)\"\n");
	printk("set key autotitle columnheader\n");
	printk("set xtics rotate by 90 offset character 0, -9, 0 \n");
	printk("set grid %s\n",
		 (sec_diskusage_is_buffer_full || (sec_diskstats_idx > 20)) ? "ytics":"");
	printk("set lmargin 10\n");
	printk("set rmargin 1\n");
	printk("set multiplot\n");
	printk("set size 1, 0.6\n");
	printk("set origin 0, 0.4\n");
	printk("set bmargin 0\n");
	printk("#\n");
	printk("plot \"-\" using 3:xtic(1) with lines lt 1, '' using 4 with lines, '' using 5 with lines\n");

	/* lock to save the buffer while printing gnuplot data */
	mutex_lock(&diskdump_lock);

	dump_gnuplot_disk_result();
	printk("e\n");

	/* Since gnuplot grammar, print data several tims */
	dump_gnuplot_disk_result();
	printk("e\n");

	/* Since gnuplot grammar, print data several tims */
	dump_gnuplot_disk_result();
	printk("e\n");

	printk("reset\n");
	printk("set ylabel \"Utilization(%%)\"\n");
	printk("set xlabel \"time(sec)\"\n");
	printk("set key autotitle columnheader\n");
	printk("set auto x\n");
	printk("set xtics rotate by 90\n");
	printk("set yrange[0:100]\n");
	printk("set grid %s\n",
		 (sec_diskusage_is_buffer_full || (sec_diskstats_idx > 20)) ? "ytics":"");
	printk("set lmargin 10\n");
	printk("set rmargin 1\n");
	printk("set bmargin\n");
	printk("set format x\n");
	printk("set size 1.0, 0.4\n");
	printk("set origin 0.0, 0.0\n");
	printk("set tmargin 0\n");
	printk("plot \"-\" using 2:xtic(1) with filledcurve x1 lt 3\n");

	/* Since gnuplot grammar, print data several tims */
	dump_gnuplot_disk_result();
	
	/* unlock */
	mutex_unlock(&diskdump_lock);

	printk("e\n");
	printk("unset multiplot\n");
	printk("}}}\n\n");

}

/* initialize disk usage buffer and variable */

void sec_diskusage_init()
{
	/* Turn ON the processing of dumping the disk usage data.*/

	memset(sec_diskstats_table, 0, SEC_DISKSTATS_NR_ENTRIES* 
			sizeof(struct sec_diskstats_struct));
	sec_diskusage_is_buffer_full = 0;

	register_counter_monitor_func(sec_diskstats_dump);
}

/*
 *Turn off the prints of diskusage
 */
void turnoff_diskusage(void)
{
	if (sec_diskusage_status) {
		sec_diskusage_status = 0;
		printk("\nDisk Usage Dump OFF\n");	
	}
}

/*
 *Turn the prints of diskusage on 
 *or off depending on the previous status.
 */
void sec_diskusage_prints_OnOff(void)
{
	sec_diskusage_status = (sec_diskusage_status) ? 0: 1;

	if (sec_diskusage_status) {
		printk("Disk Usage Dump ON\n");	

		diskusage_show_header();
	} else {
		printk("Disk Usage Dump OFF\n");	
	}
}

/* kdebugd submenu function */
static int sec_diskusage_control(void)
{
        int operation = 0;
	int ret = 1;

        printk("\nSelect Operation....");
        printk("\n1. Turn On/Off the Disk Usage prints");
        printk("\n2. Dump Disk Usage history(%d sec)", SEC_DISKSTATS_NR_ENTRIES);
        printk("\n3. Dump Disk Usage gnuplot history(%d sec)", SEC_DISKSTATS_NR_ENTRIES);
        printk("\n==>  ");

        operation = debugd_get_event();

	printk("\n\n");

        switch(operation)
        {
                case 1:
                        sec_diskusage_prints_OnOff();
			ret = 0;	/* don't print the menu */
                        break;
                case 2:
                        dump_disk_result();
                        break;
                case 3:
                        sec_diskusage_gnuplot_dump();
                        break;
                default:
                        break;
        }
        return ret;
}

/* diskuage module init */
static int __init kdbg_diskusage_init(void)
{
	sec_diskusage_init();

	kdbg_register("Counter Monitor: Disk Usage", sec_diskusage_control, turnoff_diskusage);

	return 0;
}  	
__initcall(kdbg_diskusage_init);

