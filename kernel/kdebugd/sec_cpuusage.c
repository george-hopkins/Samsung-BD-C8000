/*
 *  linux/kernel/sec_cpuusage.c
 *
 *  CPU Performance Profiling Solution, cpu usage releated functions
 *
 *  Copyright (C) 2009  Samsung	
 *
 *  2009-05-11  Created by sk.bansal.
 *
 */
#include <linux/proc_fs.h>
#include <linux/kdebugd.h>
#include "linux/sec_cpuusage.h"

/*
 * update the buffer with the cpu usage data.
 * It is called every SEC_CPUUSAGE_DELAY_SECS seconds.
 */
static void sec_cpuusage_update_buffer(void);

/*
 * Create the string of #(for user percentage) 
 * and @(for system percentage).
 */
static char* sec_cpuusage_create_string(char* pictStr, 
		int cpuusage_user_perc, int cpuusage_sys_perc);

/*
 * Turn ON the cpu usage processing.
 */
static void sec_cpuusage_init(void);

/*
 * The buffer that will store cpu usage data.
 */
static struct sec_cpuusage_struct sec_cpuusage_buffer[SEC_CPUUSAGE_BUFFER_ENTRIES];

/*The index of the buffer array at which the data will be written.*/
static int sec_cpuusage_index;

/*The flag which incates whether the buffer array is full(value is 1) 
 *or is partially full(value is 0).
 */
static int sec_cpuusage_is_buffer_full;

/*The flag which will be turned on or off when kdebugd feature will 
 *turn on or off respectively. (0=off, 1=on)
 */
static int sec_cpuusage_status = 0;

/* This function is to show header */
static void cpuusage_show_header(void)
{
	printk("\ntime(1 Sec)\ttotal\tsys\tuser\n");
	printk("============\t====\t====\t===\t===================\n");
}

/*Create the cpu usage string like this " #####@@@@@@@ "
 *This string make easier to see the cpu usage on serial console.
 */
char* sec_cpuusage_create_string(char* pictStr, int user_perc,
		int sys_perc)
{
	/*This will print # per 5 percent of user percentage*/
	int user_count = user_perc / 5;

	/*This will print @ per 5 percent of system percentage*/
	int sys_count = sys_perc / 5;

	if(user_perc + sys_perc > 100){
		printk("Real %%age can not be greater than 100\n");
		strncpy(pictStr, "!!!", 3);
		return pictStr;
	}	
	while(user_count--)
		strncat(pictStr, "#", 1);
	while(sys_count--)
		strncat(pictStr, "@", 1);	
	return pictStr;
}

/* (x) is the difference between previous ticks and current ticks.
 * So it can be changed from u64 to int.
 */
#define TIME_TO_PERCENT(x)	(((int)(x)) * 100 / (int)SEC_CPUUSAGE_UPDATE_TICKS)

/*
 * update the buffer with the cpu usage data.
 * It is called every SEC_CPUUSAGE_DELAY_SECS seconds.
 */
void sec_cpuusage_update_buffer(void)
{
	int cpu;

	static cputime64_t prev_cpu_data_user;
	static cputime64_t prev_cpu_data_system;

	cputime64_t cur_cpu_data_user = 0;
	cputime64_t cur_cpu_data_system = 0;

	int user_perc;
	int sys_perc;


	/* cpu usage is devided into several values in kernel,
	 * so it should be added to user or system time.
	 */
	for_each_online_cpu(cpu)
	{
		cur_cpu_data_user += kstat_cpu(cpu).cpustat.user + 
			kstat_cpu(cpu).cpustat.nice + 
			kstat_cpu(cpu).cpustat.user_rt;
		cur_cpu_data_system += kstat_cpu(cpu).cpustat.system + 
			kstat_cpu(cpu).cpustat.softirq + 
			kstat_cpu(cpu).cpustat.irq + 
			kstat_cpu(cpu).cpustat.iowait +                                    
			kstat_cpu(cpu).cpustat.system_rt;
	}

	/*Calculate the cpu usage in last SEC_CPUUSAGE_DELAY_SECS second(s).*/
	user_perc = (int)TIME_TO_PERCENT(cur_cpu_data_user - prev_cpu_data_user);
	sys_perc = (int)TIME_TO_PERCENT(cur_cpu_data_system - prev_cpu_data_system);

	sec_cpuusage_buffer[sec_cpuusage_index].sec_count = kdbg_get_uptime();        
	sec_cpuusage_buffer[sec_cpuusage_index].user_time= user_perc;
	sec_cpuusage_buffer[sec_cpuusage_index].sys_time= sys_perc;

	/*  
	 * Turn On/Off the CPU Usgae prints (1=off, 0=on)
	 */
	if(sec_cpuusage_status){
		char pict_str[21] = "";
		printk("%04ld Sec\t%02d%%\t%02d%%\t%02d%%\t%s\n"
				,sec_cpuusage_buffer[sec_cpuusage_index].sec_count
				,(user_perc + sys_perc)
				,user_perc
				,sys_perc
				,sec_cpuusage_create_string(pict_str, user_perc, sys_perc));

		if((sec_cpuusage_index % 20) == 0)
			cpuusage_show_header();
	}

	++sec_cpuusage_index;

	if(sec_cpuusage_index == SEC_CPUUSAGE_BUFFER_ENTRIES){
		sec_cpuusage_is_buffer_full = 1;
		sec_cpuusage_index = 0;
	}              

	prev_cpu_data_user = cur_cpu_data_user;
	prev_cpu_data_system = cur_cpu_data_system;  
}

/*Dump the bufferd data of cpu usage from the buffer.
 *This Function is called from the kdebug menu.
 */
void sec_cpuusage_gnuplot_dump(void)
{
	int last_row = 0, saved_row = 0;
	int idx, i;
	int limit_count;

	if(sec_cpuusage_is_buffer_full){
		limit_count = SEC_CPUUSAGE_BUFFER_ENTRIES;
		last_row = sec_cpuusage_index;
	}else{
		limit_count = sec_cpuusage_index;
		last_row = 0;
	}
	saved_row = last_row;

	printk("\n\n");
	printk("{{{#!gnuplot\n");
	printk("reset\n");
	printk("set title \"CPU Usage\"\n");
	printk("set xlabel \"time(sec)\"\n");
	printk("set ylabel \"Usage(%%)\"\n");
	printk("set yrange [0:100]\n");
	printk("set key invert reverse Left outside\n");
	printk("set key autotitle columnheader\n");
	printk("set auto x\n");
	printk("set xtics nomirror rotate by 90\n");
	printk("set style data histogram\n");
	printk("set style histogram rowstacked\n");
	printk("set style fill solid border -1\n");
	printk("set grid ytics\n");
	printk("set boxwidth 0.7\n");
	printk("set lmargin 8\n");
	printk("set rmargin 1\n");
	printk("#\n");
	printk("plot \"-\" using 3:xtic(1), '' using 4\n");

	/* Becuase of gnuplot grammar, we should print data twice
	 * to draw the two graphic lines on the chart.
	 */
	for (i=0; i<2; i++)
	{
		printk("Sec\t\tTotal\tSys\tUsr\n");
		for(idx=0;idx < limit_count; idx++)
		{
			int index = last_row % SEC_CPUUSAGE_BUFFER_ENTRIES;
			last_row++;
			printk(	"%04ld\t\t%02u%%\t%02u%%\t%02u%%\n",
				sec_cpuusage_buffer[index].sec_count,        
				sec_cpuusage_buffer[index].sys_time 
				+ sec_cpuusage_buffer[index].user_time,    
				sec_cpuusage_buffer[index].sys_time,    
				sec_cpuusage_buffer[index].user_time);
		}
		printk("e\n");
		last_row = saved_row;
	}

	printk("}}}\n");
}

/*
 * This function is called at every timer interrupt tick. 
 */
void sec_cpuusage_interrupt(void)
{
	static int boot_first=1;
	static unsigned long prev_jiffies = 0;
	unsigned long jiffies_difference;	
	unsigned long cur_jiffies = jiffies;

	if (unlikely(boot_first == 1)) {
		prev_jiffies = jiffies;
		boot_first=0;
	}

	jiffies_difference = cur_jiffies - prev_jiffies;

	if(jiffies_difference >= SEC_CPUUSAGE_UPDATE_TICKS){
		prev_jiffies = cur_jiffies;
		sec_cpuusage_update_buffer();
	}	
}


/*Dump the bufferd data of cpu usage from the buffer.
 *This Function is called from the kdebug menu.
 */
void sec_cpuusage_dump(void)
{
	int i = 0;
	int buffer_count = 0;
	int idx = 0;
	char pict_str[21] = "";

	if(sec_cpuusage_is_buffer_full){
		buffer_count = SEC_CPUUSAGE_BUFFER_ENTRIES;
		idx = sec_cpuusage_index;
	} else {
		buffer_count = sec_cpuusage_index;
		idx = 0;
	}

	cpuusage_show_header();

	for(i = 0; i<buffer_count; ++i, ++idx){
		idx = idx % SEC_CPUUSAGE_BUFFER_ENTRIES;

		memset(pict_str, 0, 21);
		printk("%04ld Sec\t%02u%%\t%02u%%\t%02u%%\t%s\n",
				sec_cpuusage_buffer[idx].sec_count, 
				sec_cpuusage_buffer[idx].sys_time
				+ sec_cpuusage_buffer[idx].user_time,
				sec_cpuusage_buffer[idx].sys_time,
				sec_cpuusage_buffer[idx].user_time,
				sec_cpuusage_create_string(pict_str, 
					sec_cpuusage_buffer[idx].sys_time, 
					sec_cpuusage_buffer[idx].user_time));
	}	
}

/*
 * Initialize the cpu usage buffer and global variables.
 */
void sec_cpuusage_init()
{
	memset(sec_cpuusage_buffer, 0,  SEC_CPUUSAGE_BUFFER_ENTRIES * 
			sizeof(struct sec_cpuusage_struct));
	sec_cpuusage_index = 0;
	sec_cpuusage_is_buffer_full = 0;
}

/*
 *Turn off the prints of cpuusage on 
 */
void turnoff_sec_cpuusage(void)
{
	if (sec_cpuusage_status == 1){
		sec_cpuusage_status = 0;
		printk("\nCPU USAGE Dump OFF\n");	
	}
}

/*
 *Turn the prints of cpuusage on 
 *or off depending on the previous status.
 */
void sec_cpuusage_prints_OnOff(void)
{
	sec_cpuusage_status = (sec_cpuusage_status) ? 0: 1;

	if (sec_cpuusage_status) {
		printk("CPU USAGE Dump ON\n");	

		cpuusage_show_header();
	} else {
		printk("CPU USAGE Dump OFF\n");	
	}
}

static int sec_cpuusage_control(void)
{
	int operation = 0;
	int ret = 1;

	printk("\nSelect Operation....");
	printk("\n1. Turn On/Off the CPU Usage prints"); 
	printk("\n2. Dump CPU Usage history(%d sec)", SEC_CPUUSAGE_BUFFER_ENTRIES);
	printk("\n3. Dump CPU Usage gnuplot history(%d sec)", SEC_CPUUSAGE_BUFFER_ENTRIES);
	printk("\n==>  ");  

	operation = debugd_get_event();

	printk("\n\n");

	switch(operation)
	{
		case 1:
			sec_cpuusage_prints_OnOff();
			ret = 0;	/* don't print the menu */
			break;
		case 2:
			sec_cpuusage_dump();
			break;
		case 3:
			sec_cpuusage_gnuplot_dump();
			break;
		default:
			break;
	}
	return ret;
}

static int __init proc_cpuusage_init(void)
{
	sec_cpuusage_init();

	kdbg_register("Counter Monitor: CPU Usage", sec_cpuusage_control, turnoff_sec_cpuusage);

	return 0;
}  	
__initcall(proc_cpuusage_init);

