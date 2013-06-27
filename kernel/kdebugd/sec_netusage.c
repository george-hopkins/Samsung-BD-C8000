/*
 *  linux/kernel/sec_netusage.c
 *
 *  Network Performance Profiling Solution, network usage releated functions
 *
 *  Copyright (C) 2009  Samsung	
 *
 *  2009-05-29  Created by Choi Young-Ho, Kim Geon-Ho
 *
 */
#include <linux/poll.h>
#include <linux/kthread.h>
#include <linux/ktime.h>
#include <linux/time.h>
#include <linux/netdevice.h>

#include <linux/proc_fs.h>
#include <linux/kdebugd.h>
#include <linux/sec_netusage.h>

/*
 * Turn ON the net usage processing.
 */
static void sec_netusage_init(void);

/*
 * The buffer that will store net usage data.
 */
static struct sec_netusage_struct sec_netusage_buffer[SEC_NETUSAGE_BUFFER_ENTRIES];

/*The index of the buffer array at which the data will be written.*/
static int sec_netusage_write_index;

/*The flag which incates whether the buffer array is full(value is 1) 
 *or is partially full(value is 0).
 */
static int sec_netusage_is_buffer_full;

/*The flag which will be turned on or off when sysrq feature will 
 *turn on or off respectively.
 */
/* Flag is used to turn the sysrq for net usage on or off */
static int sec_netusage_status = 0;

/* Timer for polling */
struct hrtimer network_usage_timer;

/* Interval for timer */
struct timespec show_network_interval = { .tv_sec = 1, .tv_nsec = 0 };

/* This function is to show header */
static void netusage_show_header(void)
{
	printk("\ntime(1 Sec)\tRx(Bytes)\tTx(Bytes)\n");
	printk("============\t===========\t===========\n");
}

/*Dump the bufferd data of net usage from the buffer.
  This Function is called from the kdebug menu. It prints the data 
  same as printed by cat /proc/netusage-gnuplot*/
void sec_netusage_gnuplot_dump(void)
{
	int last_row = 0, saved_row = 0;
	int idx, i;
	int limit_count;

	if(sec_netusage_is_buffer_full){
		limit_count = SEC_NETUSAGE_BUFFER_ENTRIES;
		last_row = sec_netusage_write_index;
	}else{
		limit_count = sec_netusage_write_index;
		last_row = 0;
	}
	saved_row = last_row;

	printk("\n\n\n");
	printk("{{{#!gnuplot\n");
	printk("reset\n");
	printk("set title \"Network Usage\"\n");
	printk("set xlabel \"time(sec)\"\n");
	printk("set ylabel \"Usage(Bytes)\"\n");
	printk("set key autotitle columnheader\n");
	printk("set auto x\n");
	printk("set xtics nomirror rotate by 90\n");
	printk("set grid ytics\n");
	printk("set lmargin 10\n");
	printk("set rmargin 1\n");
	printk("#\n");
	printk("plot \"-\" using 2:xtic(1) with lines, '' using 3 with lines\n");

        /* Becuase of gnuplot grammar, we should print data twice
         * to draw the two graphic lines on the chart.
	 */
	for (i=0; i<2; i++)
	{
		printk("Sec\t\tRx\tTx\n");
		for(idx=0;idx < limit_count; idx++)
		{
			int index = last_row % SEC_NETUSAGE_BUFFER_ENTRIES;
			last_row++;
			printk(	"%04ld\t\t%u\t%u\n",
					sec_netusage_buffer[index].sec,        
					sec_netusage_buffer[index].rx,    
					sec_netusage_buffer[index].tx);
		}
		printk("e\n");
		last_row = saved_row;
	}

	printk("}}}\n");
}

/*Dump the bufferd data of net usage from the buffer.
 *This Function is called from the kdebug menu. 
 */
void sec_netusage_dump(void)
{
	int i = 0;
	int buffer_count = 0;
	int idx = 0;

	if(sec_netusage_is_buffer_full){
		buffer_count = SEC_NETUSAGE_BUFFER_ENTRIES;
		idx = sec_netusage_write_index;
	}else{
		buffer_count = sec_netusage_write_index;
		idx = 0;
	}

	netusage_show_header();

	for(i = 0; i<buffer_count; ++i, ++idx) {
		idx = idx % SEC_NETUSAGE_BUFFER_ENTRIES;

		printk("%04ld Sec\t%u\t\t%u\n",
				sec_netusage_buffer[idx].sec, 
				sec_netusage_buffer[idx].rx,    
				sec_netusage_buffer[idx].tx);
	}
}

static void show_net_stat(struct net_device *dev)
{
	static unsigned long old_rx_bytes=0, old_tx_bytes=0;
	unsigned long rx_persec, tx_persec;

	if (dev->get_stats)
	{
		struct net_device_stats *stats = dev->get_stats(dev);

		rx_persec = (stats->rx_bytes - old_rx_bytes);
		tx_persec = (stats->tx_bytes - old_tx_bytes);

		old_rx_bytes = stats->rx_bytes;
		old_tx_bytes = stats->tx_bytes;

		if (sec_netusage_status)
		{ 
			printk("+--------------------------------------------+\n");
			printk("|   Receive (Bytes)   |   Transmit (Bytes)   |\n");
			printk("|   %8lu          |   %8lu           |\n", rx_persec,  tx_persec);
			printk("+--------------------------------------------+\n");
		}

		sec_netusage_buffer[sec_netusage_write_index].sec = 
							kdbg_get_uptime();
		sec_netusage_buffer[sec_netusage_write_index].rx = rx_persec;
		sec_netusage_buffer[sec_netusage_write_index].tx = tx_persec;

		++sec_netusage_write_index;

		if(sec_netusage_write_index >= SEC_NETUSAGE_BUFFER_ENTRIES){
			sec_netusage_is_buffer_full = 1;
			sec_netusage_write_index = 0;
		}              
	}
	else if (sec_netusage_status)
	{
		printk("No statistics available.\n");
	}
}


enum hrtimer_restart net_show_func(struct hrtimer *ht)
{
	struct net_device *dev;
	char dev_name[10] = "eth0";
	ktime_t now;

	dev = dev_get_by_name(&init_net, dev_name);

	if (!dev)
	{       
		//printk(KERN_ERR "%s doesn't exits, aborting.\n", dev_name);
	} else {       
		show_net_stat(dev);
	}

	now = ktime_get();

	hrtimer_forward(&network_usage_timer, now, timespec_to_ktime(show_network_interval));
	return HRTIMER_RESTART;
}               

static void start_network_usage(void)
{
	hrtimer_init(&network_usage_timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
	network_usage_timer.expires = timespec_to_ktime(show_network_interval);
	network_usage_timer.function = net_show_func;
	hrtimer_start(&network_usage_timer, network_usage_timer.expires, HRTIMER_MODE_REL);
}


void sec_netusage_init()
{
	/* Turn ON the processing of dumping the net usage data.*/

	memset(sec_netusage_buffer, 0,  SEC_NETUSAGE_BUFFER_ENTRIES * 
			sizeof(struct sec_netusage_struct));
	sec_netusage_write_index = 0;
	sec_netusage_is_buffer_full = 0;

	start_network_usage();
}

/*
 *Turn off the prints of netusage 
 */
void turnoff_netusage(void)
{
	if(sec_netusage_status){
		sec_netusage_status = 0;
		printk("\nNetwork USAGE Dump OFF\n");	
	}

}

/*
 *Turn the prints of netusage on 
 *or off depending on the previous status.
 */
void sec_netusage_prints_OnOff(void)
{
	sec_netusage_status = (sec_netusage_status) ? 0: 1;

	if(sec_netusage_status){
		printk("Network USAGE Dump ON\n");	
	} else {
		printk("Network USAGE Dump OFF\n");	
	}

}

static int sec_netusage_control(void)
{
	int operation = 0;
	int ret = 1;

	printk("\nSelect Operation....");
	printk("\n1. Turn On/Off the Network Usage prints");
	printk("\n2. Dump Network Usage history(%d sec)", SEC_NETUSAGE_BUFFER_ENTRIES);
	printk("\n3. Dump Network Usage gnuplot history(%d sec)", SEC_NETUSAGE_BUFFER_ENTRIES);
	printk("\n==>  ");

	operation = debugd_get_event();

	printk("\n\n");

	switch(operation)
	{
		case 1:
			sec_netusage_prints_OnOff();
			ret = 0;	/* don't print the menu */
			break;
		case 2:
			sec_netusage_dump();
			break;
		case 3:
			sec_netusage_gnuplot_dump();
			break;
		default:
			break;
	}
	return ret;
}

static int __init kdbg_netusage_init(void)
{
	sec_netusage_init();

	kdbg_register("Counter Monitor: Network Usage", sec_netusage_control, turnoff_netusage);

	return 0;
}  	
__initcall(kdbg_netusage_init);

