/*
 *  linux/drivers/oprofile/aop_report.c
 *
 *  Advance oprofile related functions
 *
 *  Copyright (C) 2009  Samsung	
 *
 *  2009-07-06  Created by sk.bansal.
 *
 */

#include <linux/syscalls.h>
#include <linux/proc_fs.h>
#include <linux/kdebugd.h>
#include <linux/module.h>
#include <linux/vmalloc.h>

#include "aop_report.h"
#include "aop_oprofile.h"
#include "aop_kernel.h"
#include "../drivers/oprofile/event_buffer.h"
#include "aop_report_symbol.h"
#include "aop_sym_db.h"

/*debug logs will be on if DEBUG_LOGS is 1 otherwise OFF*/
#define IS_DEBUG_ON 0
#if IS_DEBUG_ON
#define aop_printk(fmt,args...) printk(fmt,##args)
#else
#define aop_printk(fmt,...)
#endif

/* Each escaped entry is prefixed by ESCAPE_CODE
 * then one of the following codes, then the
 * relevant data.
 * These #defines live in this file so that arch-specific
 * buffer sync'ing code can access them.
 */
#define ESCAPE_CODE                     ~0UL
#define CTX_SWITCH_CODE                 1
#define CPU_SWITCH_CODE                 2
#define COOKIE_SWITCH_CODE              3
#define KERNEL_ENTER_SWITCH_CODE        4
#define KERNEL_EXIT_SWITCH_CODE         5
#define MODULE_LOADED_CODE              6
#define CTX_TGID_CODE                   7
#define TRACE_BEGIN_CODE                8
#define TRACE_END_CODE                  9
#define XEN_ENTER_SWITCH_CODE           10
#define SPU_PROFILING_CODE              11
#define SPU_CTX_SWITCH_CODE             12

/*for chelsea architecture and linux kernel 2.6.18*/
size_t kernel_pointer_size = 4;

/*position from which the value is to be read from the raw data buffer*/
static unsigned long aop_read_buf_pos;

/*samples which are remaining to get processed. Its value is cached with 
write position of the buffer at the time of initialization of raw data buffer*/
static int aop_samples_remaining;

/*
 * Transient values used for parsing the raw data buffer aop_cache.
The relevant values are updated when each buffer entry is processed.
 */
static op_data aop_trans_data;

/*The samples are processed and the data specific to application which
are run are extracted and collected in this structure. aop_app_list_head is
the head of node of the linked list*/
aop_image_list *aop_app_list_head = NULL;

/*The samples are processed and the data specific to libraries which
are referred are extracted and collected in this structure.  aop_lib_list_head is the 
head of node of the linked list*/
static aop_image_list *aop_lib_list_head = NULL;

/*The samples are processed and the data specific to tgid which
are run are extracted and collected in this structure. aop_tgid_list_head is
the head of node of the linked list*/
static aop_pid_list *aop_tgid_list_head = NULL;

/*The samples are processed and the data specific to pid which
are referred are extracted and collected in this structure.  aop_tid_list_head is the 
head of node of the linked list*/
static aop_pid_list *aop_tid_list_head = NULL;

/*count of the total user samples collected on 
processing the buffer aop_cache*/
static unsigned long aop_nr_user_samples;

/*total samples = user+kernel*/
unsigned long aop_nr_total_samples;

/*Thread name for the pid which is not registered in the kernel in task_struct*/
static char* thread_name = "Idle Task";

/*count of the total kernel samples collected on 
processing the buffer aop_cache*/
extern unsigned long aop_nr_kernel_samples;

/* aop report sort option, by default it is none.
It may be changed using setup configuration option */
extern int aop_config_sort_option;

/*enum for denoting the type of image to be 
decoded nd get the symbol information.*/
enum
{
	IMAGE_TYPE_APPLICATION,
	IMAGE_TYPE_LIBRARY
};

/*sort the linked list by sample count.
Algorithm used is selection sort.
This sorts the application and library based linked list*/
static void aop_sort_image_list(int type)
{
	struct aop_image_list *a = NULL;
	struct aop_image_list *b = NULL;
	struct aop_image_list *c = NULL;
	struct aop_image_list *d = NULL;
	struct aop_image_list *tmp = NULL;
	struct aop_image_list *head = NULL;
	
	switch(type)
	{
		case AOP_TYPE_APP:
			head = aop_app_list_head;
		break;	
		case AOP_TYPE_LIB:
			head = aop_lib_list_head;
		break;
		default:
			printk("\nwrong type(%d)\n", type);
		return;
	}

	a = c = head;
	
	while(a->next != NULL) {
		d = b = a->next;
		while(b != NULL) {
			if(a->samples_count < b->samples_count) {
				/* neighboring linked list node */
				if(a->next == b) { 
					if(a == head) {
						a->next = b->next;
						b->next = a;
						tmp = a;
						a = b;
						b = tmp;
						head = a;
						c = a;
						d = b;
						b = b->next;
					} else {
						a->next = b->next;
						b->next = a;
						c->next = b;
						tmp = a;
						a = b;
						b = tmp;
						d = b;
						b = b->next;
					}
				} else {
					if(a == head) {
						tmp = b->next;
						b->next = a->next;
						a->next = tmp;
						d->next = a;
						tmp = a;
						a = b;
						b = tmp;
						d = b;
						b = b->next;
						head = a;
					} else {
						tmp = b->next;
						b->next = a->next;
						a->next = tmp;
						c->next = b;
						d->next = a;
						tmp = a;
						a = b;
						b = tmp;
						d = b;
						b = b->next;
					}
				}
			} else {
				d = b;
				b = b->next;
			}
		}
		c = a;
		a = a->next;
	}
	switch(type)
	{
		case AOP_TYPE_APP:
			aop_app_list_head = head;
		break;	
		case AOP_TYPE_LIB:
			aop_lib_list_head = head;
		break;
		default:
			printk("\nwrong type(%d)\n", type);
		return;
	}	
}

/*sort the linked list by sample count.
Algorithm used is selection sort.
This sorts the TID and IGID based linked list*/
static void aop_sort_pid_list(int type)
{
	struct aop_pid_list *a = NULL;
	struct aop_pid_list *b = NULL;
	struct aop_pid_list *c = NULL;
	struct aop_pid_list *d = NULL;
	struct aop_pid_list *tmp = NULL;
	struct aop_pid_list *head = NULL;
	
	switch(type)
	{
		case AOP_TYPE_TGID:
			head = aop_tgid_list_head;
		break;	
		case AOP_TYPE_TID:
			head = aop_tid_list_head;
		break;
		default:
			printk("\nwrong type(%d)\n", type);
		return;
	}

	a = c = head;
	
	while(a->next != NULL) {
		d = b = a->next;
		while(b != NULL) {
			if(a->samples_count < b->samples_count) {
				/* neighboring linked list node */
				if(a->next == b) { 
					if(a == head) {
						a->next = b->next;
						b->next = a;
						tmp = a;
						a = b;
						b = tmp;
						head = a;
						c = a;
						d = b;
						b = b->next;
					} else {
						a->next = b->next;
						b->next = a;
						c->next = b;
						tmp = a;
						a = b;
						b = tmp;
						d = b;
						b = b->next;
					}
				} else {
					if(a == head) {
						tmp = b->next;
						b->next = a->next;
						a->next = tmp;
						d->next = a;
						tmp = a;
						a = b;
						b = tmp;
						d = b;
						b = b->next;
						head = a;
					} else {
						tmp = b->next;
						b->next = a->next;
						a->next = tmp;
						c->next = b;
						d->next = a;
						tmp = a;
						a = b;
						b = tmp;
						d = b;
						b = b->next;
					}
				}
			} else {
				d = b;
				b = b->next;
			}
		}
		c = a;
		a = a->next;
	}
	switch(type)
	{
		case AOP_TYPE_TGID:
			aop_tgid_list_head = head;
		break;	
		case AOP_TYPE_TID:
			aop_tid_list_head = head;
		break;
		default:
			printk("\nwrong type(%d)\n", type);
		return;
	}
}


/*free all the resources that are taken by the system 
while processing the tid and tgid data*/
void aop_free_tgid_tid_resources(void)
{
	aop_pid_list *tmp_tgid_data = aop_tgid_list_head;
	aop_pid_list *tmp_pid_data = aop_tid_list_head;
	
	/*free the TGID data*/	
	while(tmp_tgid_data){
		aop_tgid_list_head = tmp_tgid_data->next;
		vfree(tmp_tgid_data->thread_name);
		vfree(tmp_tgid_data);
		tmp_tgid_data = aop_tgid_list_head;
	}
	/*free the PID data*/	
	while(tmp_pid_data){
		aop_tid_list_head = tmp_pid_data->next;
		vfree(tmp_pid_data->thread_name);
		vfree(tmp_pid_data);
		tmp_pid_data = aop_tid_list_head;
	}
	aop_tgid_list_head = NULL;
	aop_tid_list_head = NULL;		
}

/*free all the resources taken by the system while processing the data*/
static void aop_free_resources(void)
{
	aop_image_list *tmp_app_data = aop_app_list_head;
	aop_image_list *tmp_lib_data = aop_lib_list_head;
	aop_read_buf_pos = 0;
	aop_samples_remaining = 0;
	memset(&aop_trans_data, 0, sizeof(op_data));

	aop_free_kernel_data (); /*free the kernel data*/

	aop_sym_report_free_sample_data (); /* free the sym data */

	/*free the application data*/
	while(tmp_app_data){
		aop_app_list_head = tmp_app_data->next;
		vfree(tmp_app_data);
		tmp_app_data = aop_app_list_head;
	}
	/*free the library data*/	
	while(tmp_lib_data){
		aop_lib_list_head = tmp_lib_data->next;
		vfree(tmp_lib_data);
		tmp_lib_data = aop_lib_list_head;
	}
	aop_app_list_head = NULL;
	aop_lib_list_head = NULL;
	aop_nr_user_samples = 0;
	aop_nr_total_samples = 0;
}

/*initializes the data and structures before processing*/
static void aop_init_processing(void)
{
	/*caching the write offset before processing.
	Otherwise the race conditions may occur.*/
	aop_samples_remaining = aop_cache.wr_offset / ULONG_SIZE;

	aop_printk("%s:Data entries to process aop_samples_remaining=%d\n",
	__FUNCTION__, aop_samples_remaining);	
}

/*decode the cookie into the path name of application and libraries.*/
static char* aop_decode_cookie(cookie_t cookie, char *buf, size_t buf_size)
{
	/*call the function to decode the cookie value into directory PATH*/
	if(buf)
		aop_sys_lookup_dcookie(cookie, buf, buf_size);
	return buf;
}

/*Delete the nodes in the linked list whose sample count is zero.*/
static void aop_clean_tgid_list(void)
{
	aop_pid_list *tmp_tgid_data = NULL;
	aop_pid_list *tmp = NULL;	

	while(!aop_tgid_list_head->samples_count){
		tmp = aop_tgid_list_head->next;
		vfree(aop_tgid_list_head->thread_name);
		vfree(aop_tgid_list_head);
		aop_tgid_list_head = tmp;
		if(!aop_tgid_list_head){
			printk("\nList Empty!!!\n");
			return;
		}
	}

	tmp = aop_tgid_list_head;
	tmp_tgid_data = tmp->next;
	while(tmp_tgid_data){
		if(!tmp_tgid_data->samples_count){
			tmp->next = tmp_tgid_data->next;
			vfree(tmp_tgid_data->thread_name);
			vfree(tmp_tgid_data);
			tmp_tgid_data = tmp->next;
		}else{
			tmp = tmp->next;
			tmp_tgid_data = tmp->next;
		}
	}
}

/*Delete the nodes in the linked list whose sample count is zero.*/
static void aop_clean_tid_list(void)
{
	aop_pid_list *tmp_tid_data = NULL;
	aop_pid_list *tmp = NULL;	

	while(!aop_tid_list_head->samples_count){
		tmp = aop_tid_list_head->next;
		vfree(aop_tid_list_head->thread_name);
		vfree(aop_tid_list_head);
		aop_tid_list_head = tmp;
		if(!aop_tid_list_head){
			printk("\nList Empty!!!\n");
			return;
		}
	}

	tmp = aop_tid_list_head;
	tmp_tid_data = tmp->next;
	while(tmp_tid_data){
		if(!tmp_tid_data->samples_count){
			tmp->next = tmp_tid_data->next;
			vfree(tmp_tid_data->thread_name);
			vfree(tmp_tid_data);
			tmp_tid_data = tmp->next;
		}else{
			tmp = tmp->next;
			tmp_tid_data = tmp->next;
		}
	}
}

/* Get the process name for given pid/tgid */
static void aop_get_comm_name ( int flag, pid_t pid, char *t_name )
{
	struct task_struct* tsk = NULL;

	if (!t_name)
		return;

	read_lock(&tasklist_lock); 
	tsk =  find_task_by_pid(pid);
	if(tsk){
		aop_printk ("Given %s %d, TID %d TGID %d comm name %s\n", 
				(flag)?"tgid":"pid", pid, tsk->pid, tsk->tgid, tsk->comm );
		memcpy(t_name, tsk->comm, TASK_COMM_LEN);			
	}else{
		if(pid){
			/*This is for the thread which are created and died 
			between the time the sampling stops and processing is done*/
			strncpy(t_name, "---", TASK_COMM_LEN);
		}else{
			/*This is for idle task having pid = 0*/
			strncpy(t_name, thread_name, TASK_COMM_LEN);
	}
	}
	read_unlock(&tasklist_lock); 
}

void* aop_create_node(int type)
{
	void *ret_buf = NULL;
	aop_pid_list *tmp_tgid_data = NULL;
	aop_pid_list *tmp_tid_data = NULL;	
	switch(type)
	{
	case AOP_TYPE_TGID:
		tmp_tgid_data = (aop_pid_list*)vmalloc(sizeof(aop_pid_list));
		if(!tmp_tgid_data) {
			printk("%s: tmp_tgid_data: no memory\n", __FUNCTION__);
			return NULL; /* no memory!! */
		}
		tmp_tgid_data->thread_name = (char*)vmalloc(TASK_COMM_LEN);
		if(!tmp_tgid_data->thread_name) {
			printk("%s: tmp_tgid_data->thread_name: no memory\n", __FUNCTION__);
			vfree(tmp_tgid_data);
			return NULL; /* no memory!! */
		}	
		ret_buf = (void*)tmp_tgid_data;		
	break;	
	case AOP_TYPE_TID:
		tmp_tid_data = (aop_pid_list*)vmalloc(sizeof(aop_pid_list));
		if(!tmp_tid_data) {
			printk("%s: tmp_tid_data: no memory\n", __FUNCTION__);
			return NULL; /* no memory!! */
		}
		tmp_tid_data->thread_name = (char*)vmalloc(TASK_COMM_LEN);
		if(!tmp_tid_data->thread_name) {
			printk("%s: tmp_tid_data->thread_name: no memory\n", __FUNCTION__);
			vfree(tmp_tid_data);
			return NULL; /* no memory!! */
		}	
		ret_buf = (void*)tmp_tid_data;		
	break;	
	}
	return ret_buf;
}


/*Create the linked list while the samples are added to the buffer.
This creation is done at the sampling time and not at the processing
time of raw data. This is because at the processing time, some threads/processes
might be cleaned up and doesn't have any task_struct for them.*/
void aop_create_tgid_list(struct task_struct const * tsk)
{
	aop_pid_list *tmp_tgid_data = NULL;
	aop_pid_list *tgiditem = NULL;	

	if(!tsk){
		return;
	}

	/*Check if it is the first sample.If it is, create the head node of
	the tgid link list.*/
	if(!aop_tgid_list_head){
		aop_tgid_list_head = (aop_pid_list*)aop_create_node(AOP_TYPE_TGID);
		if(!aop_tgid_list_head) {
			printk("\nreturning...\n");
			return;
		}

		aop_get_comm_name(1, tsk->tgid, aop_tgid_list_head->thread_name);
		aop_printk("Created tgid %d, comm %s & TID COMM %s \n", tsk->tgid, 
						aop_tgid_list_head->thread_name,
						tsk->comm);
		aop_tgid_list_head->tgid= tsk->tgid;
		aop_tgid_list_head->pid= tsk->pid;		
		aop_tgid_list_head->samples_count = 0;
		aop_tgid_list_head->next = NULL;
		return;
	}
	
	tmp_tgid_data = aop_tgid_list_head;
	
	/*add the sample in tgid link list and increase the count*/
	while(1){
		if(tmp_tgid_data){
			if(tmp_tgid_data->tgid == tsk->tgid){
				/*if match found, increment the count*/
				break;
			}
			if(tmp_tgid_data->next){
				tmp_tgid_data = tmp_tgid_data->next;
			}else{
				/*create the node*/
				tgiditem = (aop_pid_list*)aop_create_node(AOP_TYPE_TGID);
				if(!tgiditem) {
					printk("\nreturning...\n");
					return;
				}
				memcpy(tgiditem->thread_name, tsk->comm, TASK_COMM_LEN);					
				tgiditem->tgid= tsk->tgid;
				tgiditem->pid= tsk->pid;				
				tgiditem->samples_count = 0;
				tgiditem->next = NULL;
				tmp_tgid_data->next = tgiditem;
				break;
			}
		}
	}
}

/*add the sample to generate the report by TGID(process ID) wise.*/
void aop_add_sample_tgid(void)
{
	aop_pid_list *tmp_tgid_data = NULL;
	aop_pid_list *tgiditem = NULL;		

	/*Check if it is the first sample.If it is, create the head node of
	the tgid link list.*/
	if(!aop_tgid_list_head){
		aop_tgid_list_head = (aop_pid_list*)aop_create_node(AOP_TYPE_TGID);
		if(!aop_tgid_list_head){
			printk("\nreturning...\n");
			return;
		}		

		aop_get_comm_name(1, aop_trans_data.tgid, 
					aop_tgid_list_head->thread_name);
		aop_tgid_list_head->tgid= aop_trans_data.tgid;
		aop_tgid_list_head->pid= aop_trans_data.tid;		
		aop_tgid_list_head->samples_count = 1;
		aop_tgid_list_head->next = NULL;
		return;
	}
	
	tmp_tgid_data = aop_tgid_list_head;
	
	/*add the sample in tgid link list and increase the count*/
	while(1){
		if(tmp_tgid_data){
			if(tmp_tgid_data->tgid == aop_trans_data.tgid){
				/*if match found, increment the count*/
				tmp_tgid_data->samples_count++;
				break;
			}
			if(tmp_tgid_data->next){
				tmp_tgid_data = tmp_tgid_data->next;
			}else{
				/*create the node for some pids which are not 
				yet registered in link list*/
				tgiditem = (aop_pid_list*)aop_create_node(AOP_TYPE_TGID);
				if(!tgiditem) {
					printk("\nreturning...\n");
					return;
				}
				
				aop_get_comm_name(1, aop_trans_data.tgid, tgiditem->thread_name);
				tgiditem->tgid= aop_trans_data.tgid;
				tgiditem->pid= aop_trans_data.tid;				
				tgiditem->samples_count = 1;
				tgiditem->next = NULL;
				tmp_tgid_data->next = tgiditem;				
				break;
			}
		}else{
			printk("\naop_add_sample_tgid:check head of link list\n");
			break;
		}
	}
}

/*Create the linked list while the samples are added to the buffer.
This creation is done at the sampling time and not at the processing
time of raw data. This is because at the processing time, some threads/processes
might be cleaned up(dead) and doesn't have any task_struct for them.*/
void aop_create_tid_list(struct task_struct const * tsk)
{
	aop_pid_list *tmp_tid_data = NULL;
	aop_pid_list *tiditem = NULL;	

	if(!tsk){
		return;
	}

	/*Check if it is the first sample.If it is, create the head node of
	the tid link list.*/
	if(!aop_tid_list_head){
		aop_tid_list_head = (aop_pid_list*)aop_create_node(AOP_TYPE_TID);
		if(!aop_tid_list_head) {
			printk("\nreturning...\n");
			return;
		}
		memcpy(aop_tid_list_head->thread_name, tsk->comm, TASK_COMM_LEN);			
		aop_tid_list_head->pid= tsk->pid;
		aop_tid_list_head->tgid= tsk->tgid;		
		aop_tid_list_head->samples_count = 0;
		aop_tid_list_head->next = NULL;
		return;
	}
	
	tmp_tid_data = aop_tid_list_head;
	
	/*add the sample in tid link list and increase the count*/
	while(1){
		if(tmp_tid_data){
			if(tmp_tid_data->pid == tsk->pid){
				/*if match found, increment the count*/
				break;
			}
			if(tmp_tid_data->next){
				tmp_tid_data = tmp_tid_data->next;
			}else{
				/*create the node*/
				tiditem = (aop_pid_list*)aop_create_node(AOP_TYPE_TID);
				if(!tiditem) {
					printk("\nreturning...\n");
					return;
				}
				memcpy(tiditem->thread_name, tsk->comm, TASK_COMM_LEN);					
				tiditem->pid= tsk->pid;
				tiditem->tgid= tsk->tgid;				
				tiditem->samples_count = 0;
				tiditem->next = NULL;
				tmp_tid_data->next = tiditem;
				break;
			}
		}
	}
}

/*add the sample to generate the report by tid(thread ID) wise.*/
static void aop_add_sample_tid(void)
{
	aop_pid_list *tmp_tid_data = NULL;
	aop_pid_list *tiditem = NULL;		
	
	/*Check if it is the first sample.If it is, create the head node of
	the tid link list.*/
	if(!aop_tid_list_head){
		aop_tid_list_head = (aop_pid_list*)aop_create_node(AOP_TYPE_TID);
		if(!aop_tid_list_head){
			printk("\nreturning...\n");
			return;
		}		
		aop_get_comm_name(0, aop_trans_data.tid, aop_tid_list_head->thread_name);
		aop_tid_list_head->pid= aop_trans_data.tid;
		aop_tid_list_head->tgid= aop_trans_data.tgid;		
		aop_tid_list_head->samples_count = 1;
		aop_tid_list_head->next = NULL;
		return;
	}
	
	tmp_tid_data = aop_tid_list_head;
	
	/*add the sample in tid link list and increase the count*/
	while(1){
		if(tmp_tid_data){
			if(tmp_tid_data->pid == aop_trans_data.tid){
				/*if match found, increment the count*/
				tmp_tid_data->samples_count++;
				break;
			}
			if(tmp_tid_data->next){
				tmp_tid_data = tmp_tid_data->next;
			}else{
				/*create the node for some pids which are not 
				yet registered in link list*/
				tiditem = (aop_pid_list*)aop_create_node(AOP_TYPE_TID);
				if(!tiditem) {
					printk("\nreturning...\n");
					return;
				}

				aop_get_comm_name(0, aop_trans_data.tid, tiditem->thread_name);
				tiditem->pid= aop_trans_data.tid;
				tiditem->tgid= aop_trans_data.tgid;				
				tiditem->samples_count = 1;
				tiditem->next = NULL;
				tmp_tid_data->next = tiditem;				
				break;
			}
		}else{
			printk("\naop_add_sample_tid:check head of link list\n");
			break;
		}
	}
}

/*context changes are prefixed by an escape code.
This will return if the code is escape code or not.*/
static inline int aop_is_escape_code(uint64_t code)
{
	return kernel_pointer_size == 4 ? code == ~0LU : code == ~0LLU;
}

/*pop the raw data buffer value*/
static int aop_pop_buffer_value(unsigned long *val)
{
	if (!aop_samples_remaining) {
		printk("%s: BUG: popping empty buffer !\n", __FUNCTION__);
		return -ENOBUFS;
	}
	*val = aop_cache.buffer[aop_read_buf_pos++];
	aop_samples_remaining--;

	return 0;
}

/*returns if size number of elements are in data buffer or not.*/
static int aop_enough_remaining(size_t size)
{
	if (aop_samples_remaining >= size)
		return 1;

	printk("%s: Dangling ESCAPE_CODE.\n", __FUNCTION__);
	return 0;
}

/*process the pc and event sample*/
static void aop_put_sample(unsigned long pc)
{
	unsigned long event;
	aop_image_list *tmp_app_data = NULL;
	aop_image_list *tmp_lib_data = NULL;
	aop_image_list *appitem = NULL;
	aop_image_list *libitem = NULL;
				
	/*before popping the value, check if it avaiable in data buffer*/
	if (!aop_enough_remaining(1)) {
		aop_samples_remaining = 0;
		return;
	}

	if(aop_pop_buffer_value(&event) == -ENOBUFS){
		aop_samples_remaining = 0;		
		printk("%s, Buffer empty...returning\n",__FUNCTION__);
		return;
	}

	aop_nr_total_samples++;

	/*add the sample for tgid*/
	aop_add_sample_tgid();

	/*add the sample for pid*/	
	aop_add_sample_tid();	

	if (aop_trans_data.tracing != TRACING_ON)
		aop_trans_data.event = event;

	aop_trans_data.pc = pc;

	/* to log symbol wise samples */
	aop_sym_report_update_sample_data (&aop_trans_data);

	/*find the context, whether the sample is for kernel context or user*/
	if(aop_trans_data.in_kernel){
		aop_update_kernel_sample (&aop_trans_data);
	}else{
		aop_nr_user_samples++;

		/*Check if it is the first sample.If it is, create the head nodes of
		the library and application link list.*/
		if(!aop_app_list_head){
			aop_app_list_head = (aop_image_list*)vmalloc(sizeof(aop_image_list));
			if(!aop_app_list_head) {
				printk("%s: aop_app_list_head: no memory\n", __FUNCTION__);
				aop_nr_user_samples = 0;
				return; /* no memory!! */
			}		
			aop_app_list_head->cookie_value= aop_trans_data.app_cookie;
			aop_app_list_head->samples_count = 1;
			aop_app_list_head->next = NULL;
		}
		
		if(!aop_lib_list_head){		
			aop_lib_list_head = (aop_image_list*)vmalloc(sizeof(aop_image_list));
			if(!aop_lib_list_head) {
				printk("%s: aop_lib_list_head: no memory\n", __FUNCTION__);
				aop_nr_user_samples = 0;
				vfree(aop_app_list_head);
				return; /* no memory!! */
			}			
			aop_lib_list_head->cookie_value= aop_trans_data.cookie;
			aop_lib_list_head->samples_count = 1;
			aop_lib_list_head->next = NULL;
			return;
		}
		
		tmp_app_data = aop_app_list_head;
		tmp_lib_data = aop_lib_list_head;

		/*add the sample in application link list and increase the count*/
		while(1){
			if(tmp_app_data){
				if(tmp_app_data->cookie_value == aop_trans_data.app_cookie)	{
					/*if match found, increment the count*/
					tmp_app_data->samples_count++;
					break;
				}
				if(tmp_app_data->next){
					tmp_app_data = tmp_app_data->next;
				}else{
					/*create the node*/
					appitem = (aop_image_list*)vmalloc(sizeof(aop_image_list));
					if(!appitem) {
						printk("%s: appitem: no memory\n", __FUNCTION__);
						break; /* no memory!! */
					}					
					appitem->cookie_value= aop_trans_data.app_cookie;
					appitem->samples_count = 1;
					appitem->next = NULL;
					tmp_app_data->next = appitem;
					break;
				}
			}
		}		

		/*add the sample in library link list and increase the count*/		
		while(1){
			if(tmp_lib_data){
				if(tmp_lib_data->cookie_value == aop_trans_data.cookie){
					/*if match found, increment the count*/
					tmp_lib_data->samples_count++;
					break;
				}
				if(tmp_lib_data->next){
					tmp_lib_data = tmp_lib_data->next;
				}else{
					/*create the node*/
					libitem = (aop_image_list*)vmalloc(sizeof(aop_image_list));
					if(!libitem) {
						printk("%s: libitem: no memory\n", __FUNCTION__);
						break; /* no memory!! */
					}					
					libitem->cookie_value= aop_trans_data.cookie;
					libitem->samples_count = 1;
					libitem->next = NULL;
					tmp_lib_data->next = libitem;
					break;
				}
			}
		}
	}
}


static void aop_code_unknown(void)
{
	aop_printk("%s: Unknown code !\n", __FUNCTION__);
}

static void aop_code_ctx_switch(void)
{
	unsigned long val;

	aop_printk("%s ", __FUNCTION__);

	/*This handler would require 5 samples in the data buffer.
	check if 5 elements exists in buffer.*/
	if (!aop_enough_remaining(5)) {
		aop_samples_remaining = 0;
		return;
	}

	if(aop_pop_buffer_value(&val) == -ENOBUFS){
		aop_samples_remaining = 0;		
		printk("%s, TID Buffer empty... returning ---\n",__FUNCTION__);
		return;
	}
	aop_trans_data.tid = val;
	aop_printk("tid %d ", aop_trans_data.tid );

	if(aop_pop_buffer_value(&val) == -ENOBUFS){
		aop_samples_remaining = 0;		
		printk("%s, APP_COOKIE Buffer empty...returning ---\n",__FUNCTION__);
		return;
	}	
	aop_trans_data.app_cookie = val;
	
	aop_printk("app_cookie %lu ", (unsigned long)aop_trans_data.app_cookie );
	
	/*
	must be ESCAPE_CODE, CTX_TGID_CODE, tgid. Like this
	because tgid was added later in a compatible manner.
	*/
	if(aop_pop_buffer_value(&val) == -ENOBUFS){
		aop_samples_remaining = 0;		
		printk("%s, ESCAPE_CODE Buffer empty...returning ---\n",__FUNCTION__);
		return;
	}
	if(aop_pop_buffer_value(&val) == -ENOBUFS){
		aop_samples_remaining = 0;		
		printk("%s, CTX_TGID_CODE Buffer empty...returning ---\n",__FUNCTION__);
		return;
	}	
	
	if(aop_pop_buffer_value (&val) == -ENOBUFS){
		aop_samples_remaining = 0;		
		printk("%s, TGID Buffer empty...returning ---\n",__FUNCTION__);
		return;
	}
	aop_trans_data.tgid = val;
	aop_printk("tgid %d\n", aop_trans_data.tgid );
}


static void aop_code_cpu_switch(void)
{
	unsigned long val;
	if (!aop_enough_remaining(1)) {
		aop_samples_remaining = 0;
		return;
	}

	if(aop_pop_buffer_value(&val) == -ENOBUFS){
		aop_samples_remaining = 0;		
		printk("%s, Buffer empty...returning\n",__FUNCTION__);
		return;
	}
	aop_trans_data.cpu = val;
	
}

static void aop_code_cookie_switch(void)
{
	unsigned long val;
	aop_printk("%s ", __FUNCTION__);

	if (!aop_enough_remaining(1)) {
		aop_samples_remaining = 0;
		return;
	}

	if(aop_pop_buffer_value(&val) == -ENOBUFS){
		aop_samples_remaining = 0;		
		printk("%s, Buffer empty...returning\n",__FUNCTION__);
		return;
	}	
	aop_trans_data.cookie = val;

	aop_printk("cookie %lu \n", (unsigned long)aop_trans_data.cookie );
}


static void aop_code_kernel_enter(void)
{
	aop_printk("%s: inside kernel\n", __FUNCTION__);
	aop_trans_data.in_kernel = 1;
}


static void aop_code_user_enter(void)
{
	aop_printk("%s: outside kernel\n", __FUNCTION__);	
	aop_trans_data.in_kernel = 0;
}


static void aop_code_module_loaded(void)
{
	aop_printk("%s: module loaded\n", __FUNCTION__);	
}

static void aop_code_trace_begin(void)
{
	aop_printk("%s: TRACE_BEGIN\n", __FUNCTION__);
	aop_trans_data.tracing = TRACING_START;
}

/*handlers are registered which are responsible for every type of code*/
aop_handler_t handlers[TRACE_END_CODE + 1] = {
	&aop_code_unknown,
	&aop_code_ctx_switch,
	&aop_code_cpu_switch,
	&aop_code_cookie_switch,
	&aop_code_kernel_enter,
 	&aop_code_user_enter,
	&aop_code_module_loaded,
	&aop_code_unknown,
	&aop_code_trace_begin,
};

/*process all the samples from the buffer*/
int aop_process_all_samples(void)
{
	unsigned long code;
	int count_sample, total_no_of_samples, prev_per = 0;
	int total_escape_code = 0;

	/*reset all the resiurces taken for processing*/
	aop_free_resources();

	/*initialize the data that is required for processing*/
	aop_init_processing();

	/* init kernel data with/without vmlinux */
	aop_create_vmlinux ();

	total_no_of_samples = aop_samples_remaining;

	/*process all the samples one by one.*/
	while(aop_samples_remaining) {		
		count_sample = total_no_of_samples - aop_samples_remaining;
		if (count_sample) {
			int per = ((count_sample*100)/total_no_of_samples);
			if (!(per%10) && (prev_per!=per)){
				prev_per = per;
				printk("Processing Samples ...%d%%\r", per);
			}
		}

		if(aop_pop_buffer_value(&code) == -ENOBUFS){
			aop_samples_remaining = 0;			
			printk("\n%s, Buffer empty...returning\n",__FUNCTION__);
			return -ENOBUFS;
		}

		if (!aop_is_escape_code(code)) {
			aop_put_sample(code);
			continue;
		} else {
			total_escape_code++;
		}

		if (!aop_samples_remaining) {
			printk("\n%s: Dangling ESCAPE_CODE.\n", __FUNCTION__);
			break;
		}

		/*started with ESCAPE_CODE, next is type*/
		if(aop_pop_buffer_value (&code) == -ENOBUFS){
			aop_samples_remaining = 0;			
			printk("\n%s, Buffer empty...returning\n",__FUNCTION__);
			return -ENOBUFS;
		}		
	
		if (code >= TRACE_END_CODE) {
			printk("\n%s: Unknown code %lu\n", __FUNCTION__, code);
			continue;
		}

		handlers[code]();
	}

	/* loop will quit before calculate the processing level, so updated here */
	printk("Processing Samples ...100%%\n");
	aop_printk("Total Samples processed=%lu\n", aop_nr_total_samples);
	aop_printk("Total user Samples processed=%lu\n", aop_nr_user_samples);
	aop_printk("Total kernel Samples processed=%lu\n", aop_nr_kernel_samples);	
	aop_printk("Total escape code =%d\n", total_escape_code);	
	aop_printk("Processing Done...\n");
	return 0;
}

/*Dump the application data*/
int aop_op_generate_app_samples(void)
{
	char *buf;
	size_t buf_size = DIR_NAME_PATH_LENGTH_MAX;
	aop_image_list *tmp_app_data = NULL;
	int perc = 0;
	int index = 1;
	unsigned int choice = 0;
	cookie_t app_cookie = 0;
	
	buf = (char*)vmalloc(buf_size);	
	if(!buf) {
		printk("%s: buf: no memory\n", __FUNCTION__);	
		return 0;
	}		
	aop_printk("%s: Total user Samples collected (%lu)\n",
	__FUNCTION__, aop_nr_user_samples);
	printk("Total Samples (%lu)\n", aop_nr_total_samples);
	
	if(aop_nr_user_samples){	

		if (aop_config_sort_option == AOP_SORT_BY_SAMPLES)
			aop_sort_image_list(AOP_TYPE_APP);

		tmp_app_data = aop_app_list_head;
		printk("Index\tSamples\t  %%\tApplication Image\n");
		printk("----------------------------------------\n");
		while(tmp_app_data){
			perc = tmp_app_data->samples_count * 100 / aop_nr_total_samples;
			printk("%d\t%8u %3d%%\t%s\n", index, tmp_app_data->samples_count, perc,
				aop_decode_cookie(tmp_app_data->cookie_value, buf, buf_size));
			tmp_app_data = tmp_app_data->next;
			++index;
		}
		printk("[99] Exit\n");

		while(1){
			printk("\nSelect Option (1 to %d & Exit - 99)==>", index - 1);
			choice = debugd_get_event();
			if(choice == 99){
				printk("\n");
				break;
			}			
			if(choice >= index || choice < 1){
				printk("\nInvalid choice\n");
				continue;
			}			
			tmp_app_data = aop_app_list_head;
			while(choice-1){
				tmp_app_data = tmp_app_data->next;
				choice--;
			}
			app_cookie = tmp_app_data->cookie_value;
			printk("\nSymbol profiling for Application %s\n",
				aop_decode_cookie(tmp_app_data->cookie_value, buf, buf_size));
			aop_sym_report_per_image_user_samples(IMAGE_TYPE_APPLICATION, app_cookie);	
		}
	}
	vfree(buf);	
	return 0;
}

/*Dump the library data*/
int aop_op_generate_lib_samples(void)
{
	char *buf;
	size_t buf_size = DIR_NAME_PATH_LENGTH_MAX;
	aop_image_list *tmp_lib_data = NULL;
	int perc = 0;
	int index = 1;
	unsigned int choice = 0;
	cookie_t lib_cookie = 0;
	
	buf = (char*)vmalloc(buf_size);	
	if(!buf) {
		printk("%s: buf: no memory\n", __FUNCTION__);	
		return 0;
	}		
	aop_printk("%s: Total user Samples collected (%lu)\n",
	__FUNCTION__, aop_nr_user_samples);
	
	printk("Total Samples (%lu)\n", aop_nr_total_samples);
	
	if(aop_nr_user_samples){
		if (aop_config_sort_option == AOP_SORT_BY_SAMPLES)
			aop_sort_image_list(AOP_TYPE_LIB);
		tmp_lib_data = aop_lib_list_head;		
		printk("\nIndex\tSamples\t  %%\tLibrary Image\n");
		printk("--------------------------------------------\n");
		while(tmp_lib_data){
			perc = tmp_lib_data->samples_count * 100 / aop_nr_total_samples;
			printk("%d\t%8u %3d%%\t%s\n", index, tmp_lib_data->samples_count, perc,
				aop_decode_cookie(tmp_lib_data->cookie_value, buf, buf_size));
			tmp_lib_data = tmp_lib_data->next;
			++index;
		}
		printk("[99] Exit\n");

		while(1){
			printk("\nSelect Option (1 to %d & Exit - 99)==>", index - 1);
			choice = debugd_get_event();
			if(choice == 99){
				printk("\n");
				break;
			}			
			if(choice >= index || choice < 1){
				printk("\nInvalid choice\n");
				continue;
			}			
			tmp_lib_data = aop_lib_list_head;
			while(choice-1){
				tmp_lib_data = tmp_lib_data->next;
				choice--;
			}
			lib_cookie = tmp_lib_data->cookie_value;
			printk("\nSymbol profiling for Library %s\n",
				aop_decode_cookie(tmp_lib_data->cookie_value, buf, buf_size));
			aop_sym_report_per_image_user_samples(IMAGE_TYPE_LIBRARY, lib_cookie);	
		}
	}
	vfree(buf);	
	return 0;
}

/* to prepare img list to show at report all symbol report */
static int aop_report_prepare_img_list ( struct list_head *img_list_head )
{
	aop_image_list *tmp_lib_data = aop_lib_list_head;

	while(tmp_lib_data) {
		if (tmp_lib_data->samples_count) {
			struct aop_report_all_list *img_data;

			img_data = (struct aop_report_all_list*)
								vmalloc(sizeof(struct aop_report_all_list));
			if(!img_data) {
				aop_printk("Add image data failed: no memory\n");
				return 0; /* no memory!!, at the same time we report other data  */
			}

			/* assign the sample information to  new img data */
			img_data->is_kernel = 0;
			img_data->report_type.cookie_value = tmp_lib_data->cookie_value;
			img_data->samples_count = tmp_lib_data->samples_count;
			list_add_tail(&img_data->report_list, img_list_head);
		}
		tmp_lib_data = tmp_lib_data->next;
	}

	return 0;
}

/*Dump whole data*/
int aop_op_generate_all_samples(void)
{
	char *buf;
	size_t buf_size = DIR_NAME_PATH_LENGTH_MAX;
	struct list_head *all_list_head;
	struct aop_report_all_list *plist;
	struct list_head *pos, *q;

	if(!aop_nr_kernel_samples || !aop_nr_user_samples){
		printk("No Samples found\n");
		return 1;
	}

	if(!(all_list_head=(struct list_head*)vmalloc(sizeof(struct list_head)))){
		return 1;
	}

	INIT_LIST_HEAD(all_list_head);

	buf = (char*)vmalloc(buf_size);	
	if(!buf) {
		printk("%s: buf: no memory\n", __FUNCTION__);	
		vfree (all_list_head);
		return 1;
	}		

	aop_printk("\n%s: Total kernel Samples collected (%lu)\n",
						__FUNCTION__, aop_nr_kernel_samples);
	aop_printk("\n%s: Total user Samples collected (%lu)\n",
						__FUNCTION__, aop_nr_user_samples);
	
	printk("Total Samples (%lu)\n", aop_nr_total_samples);
	printk("Samples\t  %%\tImage [module] name \n");
	printk("-----------------------------------------\n");

	if(aop_nr_kernel_samples)
		aop_kernel_prepare_report ( all_list_head );

	if(aop_nr_kernel_samples)
		aop_report_prepare_img_list ( all_list_head );

	aop_list_sort(all_list_head, aop_kernel_report_cmp);

	/* print kernel module samples */
	list_for_each_safe(pos, q, all_list_head)
	{
		/* loop thru all the nodes */
		plist = list_entry(pos, struct aop_report_all_list, report_list);
		if(plist){
			int perc = ((plist->samples_count*100)/aop_nr_total_samples);
			printk("%8u %3d%%\t%s\n", plist->samples_count, perc,
				(plist->is_kernel)? plist->report_type.kernel_name :
				aop_decode_cookie(plist->report_type.cookie_value, 
				buf, buf_size));

			list_del(pos);

			/* free all report item memory */
			vfree(plist);
		}
	}
	printk("-----------------------------------------\n");

	vfree (all_list_head);
	vfree(buf);	
	return 1;
}

/*Dump data- TGID wise*/
int aop_op_generate_report_tgid(void)
{
	aop_pid_list *tmp_tgid_data = NULL;
	int perc = 0;

	printk("Total Samples (%lu)\n", aop_nr_total_samples);
	if(aop_nr_total_samples){
			
		/*Delete the nodes in the linked list
		whose sample count is zero.*/
		aop_clean_tgid_list();
		if (aop_config_sort_option == AOP_SORT_BY_SAMPLES)
			aop_sort_pid_list(AOP_TYPE_TGID);
		tmp_tgid_data = aop_tgid_list_head;

		printk("\nSamples\t  %%\tPid\tProcess Name\n");
		printk("--------------------------------------------\n");
		while(tmp_tgid_data){
			perc = tmp_tgid_data->samples_count * 100 / aop_nr_total_samples;
			printk("%8u %3d%%\t%d\t%.20s\n", tmp_tgid_data->samples_count,
				perc, tmp_tgid_data->tgid, tmp_tgid_data->thread_name);
			tmp_tgid_data = tmp_tgid_data->next;
		}
	}
	return 0;
}

/*Dump data- TID wise*/
int aop_op_generate_report_tid(void)
{
	aop_pid_list *tmp_tid_data = NULL;
	int perc = 0;
	pid_t pid;

	printk("Total Samples (%lu)\n", aop_nr_total_samples);
	if(aop_nr_total_samples){
		/*Delete the nodes in the linked list whose sample count is zero.*/
		aop_clean_tid_list();

		if (aop_config_sort_option == AOP_SORT_BY_SAMPLES)
			aop_sort_pid_list(AOP_TYPE_TID);

		while(1){
			tmp_tid_data = aop_tid_list_head;
		
			printk("\n Samples    %%\tTID\tTGID\tProcess Name\n");
			printk("--------------------------------------------\n");
			while(tmp_tid_data){
				perc = tmp_tid_data->samples_count * 100 / aop_nr_total_samples;
				printk("%8u %3d%%\t%d\t%d\t%.20s\n", tmp_tid_data->samples_count,
					perc, tmp_tid_data->pid, tmp_tid_data->tgid, 
					tmp_tid_data->thread_name);
				tmp_tid_data = tmp_tid_data->next;
			}
			printk("\nEnter the pid for symbol wise report(9999 for Exit) ==>");
			pid = debugd_get_event();
			printk("\n");
			if(pid == 9999){
				break;
			}
			aop_sym_report_per_tid(pid);
		}		
	}
	return 0;
}

#if IS_DEBUG_ON
/*Dump the raw data from buffer aop_cache*/
static int aop_op_dump_all_samples(void)
{
	/*Read from the zeroth entry to maximum filled.
	    Cache the write offset.*/	    
	int tmp_buf_write_pos = aop_cache.wr_offset;
	int count = 0;
	printk ("\n%s: Data available %d\n", __FUNCTION__,
			tmp_buf_write_pos/ULONG_SIZE);
	for (count=0;count < tmp_buf_write_pos/ULONG_SIZE; count++)
		printk ("%lu\n", aop_cache.buffer[count]);

	return 0;
}
#endif

/*
  * oprofile report init function
  */
#if IS_DEBUG_ON
static int __init aop_opreport_kdebug_init(void)
{
	kdbg_register("sec_oprofile: process All Samples",
					aop_process_all_samples, NULL);

	kdbg_register("sec_oprofile: dump All raw data Samples",
					aop_op_dump_all_samples, NULL);
	return 0;
}
__initcall(aop_opreport_kdebug_init);
#endif

