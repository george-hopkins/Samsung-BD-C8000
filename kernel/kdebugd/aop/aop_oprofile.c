/*
 *  linux/drivers/oprofile/aop_oprofile.c
 *
 *  Advance oprofile related functions
 *
 *  Copyright (C) 2009  Samsung	
 *
 *  2009-07-06  Created by karuna.nithy@samsung.com.
 *
 */

#include <linux/proc_fs.h>
#include <linux/kdebugd.h>
#include <linux/vmalloc.h>
#include <linux/kthread.h>
#include <linux/kernel.h>
#include <linux/ktime.h>
#include <linux/time.h>
#include <linux/dcookies.h>
#include <linux/mm.h>
#include <asm/uaccess.h>

#include "aop_oprofile.h"
#include "aop_report.h"
#include "aop_kernel.h"
#include "aop_report_symbol.h"

/* trace on/off flag */
#define AOP_TRACE_ON    0

/* default event buffer sampling rate*/
#define AOP_CONFIG_DEFAULT_SAMPLING_RATE	1000 
#define AOP_CONFIG_MIN_SAMPLING_RATE	100 
#define AOP_CONFIG_MAX_SAMPLING_RATE	3000

/* default event buffer size*/
#define AOP_CONFIG_DEFAULT_EVENT_BUFF		(2*1024*1024) 

#if AOP_TRACE_ON

#define AOP_PRINT(fmt,args...) do {		\
  printk(KERN_INFO "%s: " fmt,__FUNCTION__ ,##args); \
  } while(0)

#else

#define AOP_PRINT(fmt...) do { } while(0)

#endif /* #if AOP_TRACE_ON */

#ifndef assert
#define assert(cond)  do { \
      int val = (int)(cond); \
      if(!val) printk(KERN_ERR "ASSERT FAIL: " #cond "\n");  \
      WARN_ON(!val); } while(0)
#else
#error assert is already defined.
#endif

/* task state related defines */
#define AOP_TSK_NOT_CREATED		0
#define AOP_TSK_ALREADY_CREATED	1

/* enum for adv oprofile submenu */
enum {
	AOP_MENU_CHANGE_CONFIG =1, 	/* to set oprofile configuration */
	AOP_MENU_SHOW_CONFIG, 	/* to show oprofile configuration */
	AOP_MENU_START_PROFILE,	/* to start oprofile and aop buff task	*/
	AOP_MENU_STOP_PROFILE,	/* to stop oprofile and aop buff task  */
	AOP_MENU_SHOW_CUR_STATE,/* to show oprofile running state */
	AOP_MENU_REPORT_APPL_SAMPLE, 	/* generate report for application Samples */
	AOP_MENU_REPORT_LIB_SAMPLE,		/* generate report for library Samples */
	AOP_MENU_REPORT_KERNEL_SAMPLE,	/* generate report for kernel Samples */
	AOP_MENU_REPORT_TGID,			/* generate report --- ProcessID wise */
	AOP_MENU_REPORT_TID,			/* generate report --- ThreadID wise */
	AOP_MENU_REPORT_ALL_SAMPLE,		/* generate report for all Samples */
	AOP_MENU_REPORT_SYMBOL_SYSTEM_WIDE,	/*system wide symbol info including 
											application & library name */
	AOP_MENU_REPORT_SYMBOL_APPL_SAMPLE, /*application wise symbol info 
											including function name & library name */
	AOP_MENU_REPORT_SYMBOL_APPL_LIB_SAMPLE,	/* application wise symbol info
												including library name */
	AOP_MENU_LOAD_SYMBOL_FROM_USB
#if AOP_TRACE_ON
	,	AOP_DUMP_ALL_SAMPLES
#endif /* AOP_TRACE_ON */
};

/* enum for adv oprofile configuration submenu */
enum {
	AOP_CONFIG_BUFF_SIZE =1, 	/* to configure event cache size */
	AOP_CONFIG_SORT_OPTION,		/* to configure sort option for reports */
	AOP_CONFIG_DEMANGLE_SYM_NAME,	/* to configure symbol name demangle option */
        AOP_CONFIG_ADDITIVE_DESTRUCTIVE	/* to Configure download ELF Database as Additive or Destructive */
};

/* string for adv oprofile sort option */
static const char *aop_config_sort_string[3] = {
	"Default",		/* sort option none */
	"Sort by vma address", 	/* sort by vma address */
	"sort by samples"	/* sort report by samples */
};

/* string for adv update ELF database */
static const char *aop_config_elf_load_string[2] = {
	"Destructive",		/* Update ELF Database as Destructive*/
	"Additive", 	/* Update ELF Database as Additive*/
};

/* this buffer size is defined based on the memory requirement experiment
  * done for 30 min. For 30mins of profiling, we got 1.2MB data.
  * It may be reduced by setup filter setting
  */
static unsigned long aop_event_buffer_size = AOP_CONFIG_DEFAULT_EVENT_BUFF;

/* aop report sort option, by default it is none.
It may be changed using setup configuration option */
int aop_config_sort_option = AOP_SORT_BY_SAMPLES;

/* aop report symbol demangle option, by default it is none.
It may be changed using setup configuration option.
By default demangling function name is ON */
int aop_config_demangle = 1; 

/* aop additive/ substractive option for download the ELF
database from USB. By default it is Additive. */
int aop_config_additive = 1;

/* global structure for a buffer used to store the event buffer data*/
struct aop_cache_type aop_cache;

/* op buffer task releated varaiables are declared here */
/* this variable is used to store op buffer task */ 
static struct task_struct *aop_buff_tsk;

/* to update task create status */
static unsigned int aop_buff_tsk_created;

/* to know oprofile running status */
static unsigned int aop_profile_running;

/* copy the dcookie pointer, when register a decookie. this will later used
to unregister the dcookie */
static void *aop_dcookie_user_data;

static int aop_stop_oprofile (void);

/* To wakeup buffer waiter from adv oprofile module */
static int aop_oprofile_wake_up_buffer(void)
{
	int retval = 1;
	//mutex_lock(&start_mutex);
	if (aop_profile_running){
		/* wake up the daemon to read what remains */
		wake_up_buffer_waiter();
		retval = 0;
	}
	//mutex_unlock(&start_mutex);

	return retval;
}

/* deallocate/free memory*/
static void aop_free_event_cache ( void )
{
	/* allocat memory for given buffer size */
	if (aop_cache.buffer)
		vfree(aop_cache.buffer);

	/* initialize the wr/rd offset values */
	aop_cache.wr_offset = 0;
	aop_cache.rd_offset = 0;
}

/* allocate memory to copy event data */
static int aop_alloc_event_cache ( void )
{
	/* allocat memory for given buffer size */
	aop_cache.buffer = vmalloc(aop_event_buffer_size);

	/* assert when failed to allocate memory */
	assert(aop_cache.buffer);
	if(!aop_cache.buffer) {
		return -ENOMEM; /* no memory!! */
	}

	/* initialize the wr/rd offset values */
	aop_cache.wr_offset = 0;
	aop_cache.rd_offset = 0;

	/* return success */
	return 0;
}

/* clear event cache buffer */
static void aop_clear_event_cache(void)
{
	/* clear cached event data */
	if (aop_cache.buffer)
		memset(aop_cache.buffer, 0, aop_event_buffer_size);

	/* initialize the wr/rd offset values */
	aop_cache.rd_offset = 0;
	aop_cache.wr_offset = 0;
}

/* Wakeup event buffer and cache the event data buffer.
 before read the data, must wakeup the event waiter
 and read data from event buffer */
static int aop_put_data(void)
{
	/* to wakeup event waiter */
	if ( aop_oprofile_wake_up_buffer () == 0 ){
		ssize_t retval = -1;
		unsigned long data_size, remain_buff_size; 

		/* check whether enough buffer is there at event cache buffer ot not */
		remain_buff_size = aop_event_buffer_size - aop_cache.wr_offset;
		if ( remain_buff_size >  fs_buffer_size * ULONG_SIZE)
			data_size = fs_buffer_size * ULONG_SIZE;
		else
			data_size = remain_buff_size;

		/* event cache buffer is enough to store event buffer then read data from event buff */
		if ( data_size ) {
			retval = aop_read_event_buffer (
				aop_cache.buffer + aop_cache.wr_offset/ULONG_SIZE, 
				&data_size);
			AOP_PRINT ("Data read %lu\n", data_size );
		} else {
			AOP_PRINT ("Sec_op_cache FULL w =%d\n", aop_cache.wr_offset);
			return -1;
		}

		/* update the cache wr offeset, when data read is succeed */
		if ( retval > 0 )
			aop_cache.wr_offset += retval;
	}
	return 0; /* upon success */
}

/* op buffer task function, which can wakeup event 1000 milliseconds and 
verify oprofile is stopped or not, if not stopped read data from event buffer
and put it in op cache buffer */
static int aop_buff_task (void *arg)
{
	int buff_full = 0;
	while (1){

		/* break the task function, when kthead_stop is called */
		if (kthread_should_stop())
			break;

		/* when oprofile is stopped don't process 
		aop_buff_tsk_created variable is also used 
		to keep track of task running status */
		if (!aop_buff_tsk_created) 
			break;

		/* copy event_data to aop_cache */
		if ( aop_put_data () == -1 ) {
			/* mark buffer full state */
			buff_full = 1;
			break;
		}

		/* profile sampling rate */
		msleep(AOP_CONFIG_DEFAULT_SAMPLING_RATE);
	}

	/* when buffer is full stop the adv oprofile */
	if (buff_full)
		aop_stop_oprofile ();

	return 0;
}

/* aop buffer task create function, which is called when oprofile start is request from user.
when this function is already created and not running, then task running status is set,
the op buffer task read this task running status flag and cache the event data for
further report generation. if the task is not created, then create & run aop_tsk */
static int aop_buff_tsk_create(void)
{
    int ret = AOP_TSK_NOT_CREATED;

	/* if task is already created return error code */
	if ( aop_buff_tsk_created ) {
		/* task already created */
		return AOP_TSK_ALREADY_CREATED;
	}

	/* reset the aop buffer and task state */
	aop_clear_event_cache();

	/* set the tsk create flag */
	aop_buff_tsk_created = 1;

	/* create aop buffer task */
    aop_buff_tsk = kthread_create(aop_buff_task, NULL, "aop_tsk");
    if (IS_ERR(aop_buff_tsk)) {
        ret = PTR_ERR(aop_buff_tsk);
        aop_buff_tsk = NULL;
		aop_buff_tsk_created = 0;
        return ret;
    } 

	/* update task flag and wakeup process */
    aop_buff_tsk->flags |= PF_NOFREEZE;
    wake_up_process(aop_buff_tsk);

    return ret;
}

/* stop the buff task, stop means, just clear the tsk running state */
static int aop_buff_tsk_stop(void)
{
	/* stop the buff task */
	if ( aop_buff_tsk_created ) {
		//kthread_stop (aop_buff_tsk);

		aop_buff_tsk = NULL;

		/* set the task create flag to zero */
		aop_buff_tsk_created = 0;

		/* return success */
		return 0;
	}

	return 1; /* return error */
}

/*
  * This function is to start oprofile and aop buff task 
  * It is called from kdebug menu, when user choose start oprofile option
  */
static int aop_start_oprofile (void)
{
	int ret;

	/* create aop buff task, if it is already created and stopped by oprofile, 
	then, the task running status will be set to wake the thread to cache event data */
	ret = aop_buff_tsk_create ();
	if ( ret != AOP_TSK_NOT_CREATED ) {
		AOP_PRINT ("Task %s!!!\n", 
			(ret == AOP_TSK_ALREADY_CREATED)?"exists":"create failed");
		printk ("Adv oprofile already running!!!\n");
		return 1;
	}

	/*free up the buffer that is taken by the system 
	when collecting and processing by tid and tgid*/
	aop_free_tgid_tid_resources();

	/* start oprofile */
	oprofile_start ();

	printk ("Adv oprofile running...\n");

	/* set the oprofile running status */
	aop_profile_running = 1;

	/* as this return value is mean to show or not show the kdebugd menu options */
	return 0;
}

/* Function is to stop oprofile and stop the task which cache the event buffer
Once stop the called by kdebugd, oprofile is stopped and collected samples
are processed and ready for report */
static int aop_stop_oprofile (void)
{
	/* stop oprofile, which stop buffering the event data */
	oprofile_stop ();
	//oprofile_shutdown ();

	/* stop aop buff task, it means set the task running status 0, to stop cache event data */
	if ( aop_buff_tsk_stop ()) {
		/* oprofile not started or already stopped so return */
		printk ("Adv oprofile stopped already!!!\n" );
		return 1;
	}

	/* Clear the even buffer and set the writing position to zero.
	This is not done in oprofile_stop()*/
	aop_event_buffer_clear();

	/* once, oprofile is stopped, all the collected samples are processed and stored 
	in seperate buffer as per filter option & oprofile configuration */
	aop_process_all_samples ();
	printk ("Profiling Stopped!!!(Total Samples : %lu)\n", aop_nr_total_samples);

	/* set the oprofile running status */
	aop_profile_running = 0;
	
	return 0;	
}

#if AOP_TRACE_ON
static int aop_dump_all_samples(void)
{
	/*Read from the zeroth entry to maximum filled.
	    Cache the write offset.*/	    
	int tmp_buf_write_pos = aop_cache.wr_offset/ULONG_SIZE;
	int count = 0;
	printk ("\n%s: Dump raw data %d\n", __FUNCTION__, tmp_buf_write_pos);
	for (count=0;count < aop_cache.wr_offset/ULONG_SIZE; count++)
		printk ("0x%04x 0x%08lx\n", count, aop_cache.buffer[count]);

	return 0;
}
#endif /* AOP_TRACE_ON */

/* to show oprofile current state */
static int aop_profile_current_state (void)
{
	/* show user the current oprofile state */
	printk ("Profile State: ");
	if (aop_profile_running)
		printk ("Running\n");
	else
		printk ("Not Running\n");

	return 0;
}

#if AOP_TRACE_ON
/* Function is used to view the aop cache wr pointer and total size.
It is used for debugging purpose */
static int aop_print_current_wr_offer ( void)
{
	printk ("aop cache status [%d/%d]\n", 
		aop_cache.wr_offset, SEC_OP_EVENT_BUFF_SIZE );
	return 0;
}
#endif /* AOP_TRACE_ON */

/*
  * This function sets the size of buffer in which the raw data is collected.
  * By default, the size is 2*1024*1024 which is capable of storing data 
  * for 30 Mins.
  */
static int aop_config_set_buffer_size (void)
{
	unsigned long buffer_size = 0;
	
	printk("\nCurrent Buffer size = %lu bytes\n", aop_event_buffer_size / 1024);
	
	if(!aop_profile_running){
		printk("Default Buffer size of 2048 KB is sufficient for "
					"collecting oprofile data of approx 30 mins.\n");	
		printk("Enter Buffersize(in KB)-> ");
		buffer_size = debugd_get_event();
		printk("\n");
		buffer_size = buffer_size * 1024; /*Converting the value into Bytes*/
		
		if(buffer_size == aop_event_buffer_size){
			return 0; /*if same as previous, then return*/
		}else if(!buffer_size){
			return -1; /*if it is zero, then return*/
		}else if(buffer_size < 30 * 1024)	{
			/*If it is less then the minimum value, then return*/
			printk("\nMinimum buffer size of  30KB is required "
						"for oprofiling of 30Secs. Please enter again...\n");
			return -1;
		}else if(buffer_size > 8 * 1024 * 1024){
			/*If it is more then the maximum value, then return*/
			printk("\nBuffer size exceeds the maximum buffer size of 8000KB "
						"for oprofiling of 2Hrs. Please enter again...\n");
			return -1;
		}else{
			aop_event_buffer_size = buffer_size;
			aop_free_event_cache();
			/* allocate memory for event cache */
			if ( aop_alloc_event_cache ()== -ENOMEM){
				AOP_PRINT ("Failed memory alloc aop_alloc_event_cache\n");
				return 1;
			}
		}
		printk("\nbuffer size is %lu\n", aop_event_buffer_size);
	}else{
		printk("\nStop Oprofile before setting the buffer size.\n");
	}

	/* as this return value is mean to show or not show the kdebugd 
	menu options */
	return 0;
}

/* This function is used to show oprofile configuration */
static void aop_config_show_configuration (void)
{
	printk("\nAdv Oprofile Current Settings\n");
	printk("--------------------------------------------------\n");
	printk("Buffer size = %lu KB\n", aop_event_buffer_size / 1024);
	printk("Sort option = %s\n", 
				aop_config_sort_string[aop_config_sort_option] );
	printk("Symbol name demangle = %s\n", 
				aop_config_demangle ?"Yes" : "No" );
	printk("Update ELF Database as = %s\n", 
				aop_config_elf_load_string[aop_config_additive] );
}

/* This is oprofile configuration menu, user can setup, buffer size, sampling rate
report sort options etc., */
static void aop_config_menu (void)
{
	int operation = 0;

	printk("1)  aop config: set buffer size "
					"(2048 KBytes by default).\n");
	printk("2)  aop config: set symbol report sort option.\n");
	printk("3)  aop config: set demangle symbol option.\n");
	printk("4)  aop config: set ELF Update as Additive or Destructive"
		"(Default is Additive)\n");
	printk("\nSelect Option==>  ");

	operation = debugd_get_event();
	printk("\n");

	switch(operation)
	{
		case AOP_CONFIG_BUFF_SIZE:
			/* to setup oprofile buffer size */
			aop_config_set_buffer_size();
			break;
		case AOP_CONFIG_SORT_OPTION:
			/* to setup sort key */
			printk ("Symbol Report Sort Option:\n");
			printk ("---------------------------------\n");
			printk ("\tIndex  Sort by\n");
			printk ("---------------------------------\n");
			printk ("\t[0] - %s\n", aop_config_sort_string[0]);
			printk ("\t[1] - %s\n", aop_config_sort_string[1]);
			printk ("\t[2] - %s\n", aop_config_sort_string[2]);
			printk ("---------------------------------\n");
			printk("Sort Option ==> ");
			operation = debugd_get_event();
			printk ("\n");

			if (operation>2)
				printk("Invalid sort option\n");
			else
				aop_config_sort_option = operation;
			break;
		case AOP_CONFIG_DEMANGLE_SYM_NAME:
			/* to update symbol demangle option */
			printk ("Symbol Name demangle option: [0(No)/1(Yes)] ==> ");
			operation = debugd_get_event();
			printk ("\n");
		
			if (operation>1)
				printk("Invalid demangle option\n");
			else
				aop_config_demangle = operation;
			break;
		case AOP_CONFIG_ADDITIVE_DESTRUCTIVE:
			/* to update symbol demangle option */
			printk ("Download ELF Database as : 0[%s] / 1[%s] ==> ",
						aop_config_elf_load_string[0],
						aop_config_elf_load_string[1]);
			operation = debugd_get_event();
			printk ("\n");
		
			if (operation>1)
				printk("Invalid demangle option\n");
			else
				aop_config_additive = operation;
			break;
		default:
			break;
	}
	printk("Adv Oprofile configuration menu exit....\n");
}

/* This is main oprofile main menu, and it is catagorized into three
1. setup, 2. report & 3. USB & ELF parser 
It just call appropriate api's for the option listed in this function.  */
static int aop_oprofile_menu (void)
{
	int operation = 0;
	int ret = 1;

	do {
		if (ret) {
			printk("\n1)  Adv Oprofile: setup configuration.");
			printk("\n2)  Adv Oprofile: show configuration.");
			printk("\n3)  Adv Oprofile: start oprofile.");
			printk("\n4)  Adv Oprofile: stop oprofile.");
			printk("\n5)  Adv Oprofile: show current profile state.");
			printk("\n------------------------------------------------"
						"--------------------");
			printk("\n6)  Adv Oprofile: generate report "
						"for application Samples.");
			printk("\n7)  Adv Oprofile: generate report for library Samples.");
			printk("\n8)  Adv Oprofile: generate report for kernel Samples.");
			printk("\n9)  Adv Oprofile: generate report process ID wise.");
			printk("\n10) Adv Oprofile: generate report thread ID wise.");
			printk("\n11) Adv Oprofile: generate report for all Samples.");
			printk("\n12) Adv Oprofile: generate report "
						"for system-wide symbol name.");
			printk("\n13) Adv Oprofile: generate report "
						"for single application symbol.");
			printk("\n14) Adv Oprofile: generate report for "
							"single application symbol including libraries.");
			printk("\n------------------------------------------------"
						"--------------------");
			printk("\n15) Adv Oprofile: Symbol (ELF) Menu.");
#if AOP_TRACE_ON
			printk("\n16) Adv Oprofile: Dump all samples.");
#endif /* AOP_TRACE_ON */
			printk("\n99) Adv Oprofile: Exit Menu");
			printk("\n\nSelect Option==>  ");
		}

		operation = debugd_get_event();
		printk("\n");

		if ( operation >= AOP_MENU_REPORT_APPL_SAMPLE && 
				operation <= AOP_MENU_REPORT_SYMBOL_APPL_LIB_SAMPLE &&
				aop_profile_running )
		{
			printk("Adv Oprofile is running....\n"
					"Stop adv Oprofile before view report\n");
			continue;
		}

		switch(operation)
		{
			case AOP_MENU_CHANGE_CONFIG:
				/* to open/setup oprofile configuration */
				if(!aop_profile_running){
					printk("Setup configuration.\n");
					aop_config_menu();
					ret = 1; /* must show the oprofile menu */
				}else
					printk("Stop Adv Oprofile before change configuration\n");
				break;
			case AOP_MENU_SHOW_CONFIG:
				/* to show oprofile configuration */
				aop_config_show_configuration();
				ret = 0;
				break;
			case AOP_MENU_START_PROFILE:
				/* to start oprofile and aop buff task  */
				ret = aop_start_oprofile();
				break;
			case AOP_MENU_STOP_PROFILE:
				/* to stop oprofile and aop buff task  */
				ret = aop_stop_oprofile();
				break;
			case AOP_MENU_SHOW_CUR_STATE:
				/* to show oprofile running state */
				ret = aop_profile_current_state();
				break;
			case AOP_MENU_REPORT_APPL_SAMPLE:
				/* generate report for application Samples */
				printk("\nReport Application Samples Only:\n");
				aop_op_generate_app_samples();
				ret = 1; /* must show the oprofile menu */
				break;
			case AOP_MENU_REPORT_LIB_SAMPLE:
				/* generate report for library Samples */
				printk("\nReport Library Samples Only:\n");
				aop_op_generate_lib_samples ();
				ret = 1; /* must show the oprofile menu */
				break;
			case AOP_MENU_REPORT_KERNEL_SAMPLE:
				/* generate report for kernel Samples */
				printk("\nReport Kernel & Kernel Module Samples Only:\n");
				ret = aop_generate_kernel_samples ();
				break;
			case AOP_MENU_REPORT_TGID:
				/* generate report --- ProcessID wise */
				printk("Report Process ID Wise Samples Only:\n");
				ret = aop_op_generate_report_tgid ();
				break;
			case AOP_MENU_REPORT_TID:
				/* generate report --- ThreadID wise */
				printk("Report Thread ID Wise Samples Only:\n");
				ret = aop_op_generate_report_tid ();
				ret = 1; /* must show the oprofile menu */
				break;
			case AOP_MENU_REPORT_ALL_SAMPLE:
				/* generate report for all Samples */
				printk("Report All Samples:\n");
				ret = aop_op_generate_all_samples ();
				break;
			case AOP_MENU_REPORT_SYMBOL_SYSTEM_WIDE:
				/*Dump the system wide symbol info including application & library name */
				printk("Report System wide function name:\n");
				ret = aop_sym_report_system_wide_function_samples();
				break;
			case AOP_MENU_REPORT_SYMBOL_APPL_SAMPLE:
				/* Dump the application wise function name & library name */
				printk("Report Application Wise Samples:\n");
				ret = aop_sym_report_per_application ();
				break;
			case AOP_MENU_REPORT_SYMBOL_APPL_LIB_SAMPLE:
				/* Dump the application wise function name including library name */
				printk("Report Application Wise Samples Including Library:\n");
				ret = aop_sym_report_per_application_n_lib ();
				break;
			case AOP_MENU_LOAD_SYMBOL_FROM_USB:
				/* load elf database from USB */
				printk("Load Symbols from elf file:\n");
				aop_sym_kdmenu ();
				ret = 1; /* to show menu */
				break;
#if AOP_TRACE_ON
			case AOP_DUMP_ALL_SAMPLES:
				aop_dump_all_samples ();
				break;
#endif /* AOP_TRACE_ON */
			default:
				printk("Adv Oprofile invalid option....\n");
				ret = 1; /* to show menu */
				break;
		}
	}while (operation != 99 );
	printk("Adv Oprofile menu exit....\n");

	/* as this return value is mean to show or not show the kdebugd menu options */
	return ret;
}

/*
  * oprofile init function, which initialize advance oprofile stop and start functions
  * and allocate event cache memory and open the event buffer to allow
  * sample data to be stored in event buffer & cpu buffer.
  */
static int __init aop_kdebug_init(void)
{
	/* allocate memory for event cache */
	if ( aop_alloc_event_cache () == -ENOMEM){
		AOP_PRINT ("Failed memory alloc aop_alloc_event_cache\n");
		return 1;
	}

	/* allocate memory for event cache */
	if ( aop_sym_report_init ()!= 0){
		AOP_PRINT ("Failed to init symbol info head list\n");
		return 1;
	}

	/* open event/cpu buffer to collect samples and register dcookie link */
	aop_event_buffer_open(aop_dcookie_user_data);

	/* adv oprofile menu options */
	kdbg_register("Profile: Advanced Oprofile",
	              aop_oprofile_menu , NULL);
	
#if AOP_TRACE_ON
	kdbg_register("aop_oprofile: wr offset",
	              aop_print_current_wr_offer, NULL);
#endif
	return 0;
}
__initcall(aop_kdebug_init);


