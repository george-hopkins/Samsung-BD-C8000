/*
 *  linux/drivers/oprofile/aop_kernel.c
 *
 *  Advance oprofile kernel module sampling related functions are declared here
 *
 *  Copyright (C) 2009  Samsung	
 *
 *  2009-08-07  Created by karuna.nithy@samsung.com.
 *
 */

#include <linux/proc_fs.h>
#include <linux/kdebugd.h>
#include <linux/module.h>
#include <linux/vmalloc.h>

#include "aop_oprofile.h"
#include "aop_kernel.h"
#include "aop_sym_db.h"

/* trace on/off flag */
//#define AOP_KERNEL_TRACE_ON

#ifdef AOP_KERNEL_TRACE_ON
#define AOP_KERNEL_PRINT(fmt,args...) do {		\
  printk(KERN_INFO "%s: " fmt,__FUNCTION__ ,##args); \
  } while(0)

#else
#define AOP_KERNEL_PRINT(fmt...) do { } while(0)
#endif /* #ifdef AOP_KERNEL_TRACE_ON */

/*link list to store the kernel and kernel module data which 
is collected after processing the raw data.
sample count denotes the number of samples collected for that kernel image*/
struct aop_kernel_list {
	struct list_head kern_list;
    struct kernel_image *ki;
    unsigned int samples_count;
	pid_t pid; /*thread ID*/
};

/*The samples are processed and the data specific to kernel which
are refereed are extracted and collected in this structure. aop_kernel_list_head is the 
head of node of the linked list*/
static struct list_head *aop_kernel_list_head = NULL;

/*count of the total kernel samples collected on 
processing the buffer sec_op_cache*/
unsigned long aop_nr_kernel_samples;

/*if vmlinux is not input for kernel samples, then this name will taken*/
static char *vmlinux_name = "no-vmlinux";

/* this will be changed later when filter option is implemented */
static char *kernel_name = "KERNEL"; 

/* kernel filter option */
static int no_vmlinux = 0;

/*total samples = user+kernel*/
extern unsigned long aop_nr_total_samples;

/* aop report sort option, by default it is none.
It may be changed using setup configuration option */
extern int aop_config_sort_option;

/* Function used to allocate kernel data with kernel name & their range */
static struct aop_kernel_list * aop_malloc_kernel_data ( 
	char *k_name, unsigned long start, unsigned long end )
{
	struct aop_kernel_list * kernel_item;

	/* allocate memory to store a kernel data */
	kernel_item = 
		(struct aop_kernel_list*)vmalloc(sizeof(struct aop_kernel_list)); 
	if (kernel_item) {
		struct kernel_image *ki;

		/* allocate memory to store kernel image */
		ki = (struct kernel_image*)vmalloc(sizeof(struct kernel_image));
		if (ki) {
			int k_name_length = strlen(k_name)+3; /* 2 to append [] + 1 null char */

			/* allocate memory to store kernel name */
			ki->name = (char*)vmalloc(k_name_length);
			if (!ki->name) {
				AOP_KERNEL_PRINT ("Unable to create ki name\n");
				vfree(kernel_item);
				vfree(ki);
				return NULL;
			}

			/* copy kernel name and update start/end position */
			snprintf(ki->name, k_name_length, "[%s]", k_name);
			ki->start = start;
			ki->end = end;

			/* asign this ki to kernel item */
			kernel_item->ki = ki;
		} else {
			AOP_KERNEL_PRINT ("Unable to create ki\n");
			vfree(kernel_item);
			return NULL;
		}

		/* initialize the other part of kernel item */
		kernel_item->pid = -1;
		kernel_item->samples_count = 0;
	} else {
		AOP_KERNEL_PRINT ("Unable to create aop_kernel_list_head\n");
	}

	/* return new kernel item to caller */
	return kernel_item;
}

/* free allocated kernel data */
void aop_free_kernel_data ( void )
{
	struct aop_kernel_list *kernel_item;
	struct list_head *pos, *q;

	if (aop_kernel_list_head) {
		list_for_each_safe(pos, q, aop_kernel_list_head)
		{
			/* loop thru all the nodes and free the allocated memory*/
			if((kernel_item = list_entry(pos, 
					struct aop_kernel_list, kern_list))){
				list_del(pos);
				if (kernel_item->ki) {
					if (kernel_item->ki->name) {
						/* free kernel image name memory */
						vfree(kernel_item->ki->name);
					}
					/* free kernel image memory */
					vfree(kernel_item->ki);
				} 
				/* free kernel item memory */
				vfree(kernel_item);
			}	
		}

		vfree (aop_kernel_list_head);
		aop_kernel_list_head = NULL;
	}

	aop_nr_kernel_samples = 0;
}

/* update kernel data with/without vmlinux, 
TODO-need to send vmlinux name & their range.
In case if we want to create kernel image for vmlinux, 
we need to do create it before arive the first sample */
void aop_create_vmlinux( void ) 
{
	struct aop_kernel_list *new_kern_data = NULL;

	if(!(aop_kernel_list_head = 
		(struct list_head*)vmalloc(sizeof(struct list_head)))){
		/* failed to allocate memory to store kernel samples */
		printk ("%s> malloc error\n", __FUNCTION__ );
		return;
	}

	INIT_LIST_HEAD(aop_kernel_list_head);

	/* create first list entry as vmlinux or no-vmlinux files */
	/* symbol data is not exists, so create new node and update it in aop_kernel_list_head */
	new_kern_data = aop_malloc_kernel_data (
		(( no_vmlinux == 1) ? vmlinux_name : kernel_name), 0, 0 );

	if(!new_kern_data) {
		AOP_KERNEL_PRINT("Add kernel data failed: no memory\n");
		return; /* no memory!! */
	}

 	list_add_tail(&new_kern_data->kern_list, aop_kernel_list_head);
 }

/* update kernel samples */
void aop_update_kernel_sample(op_data *aop_data )
{
	struct aop_kernel_list *new_kern_data = NULL;
	struct aop_kernel_list *plist, *pstVMLinux = NULL;
	struct list_head *pos, *q;
	int flag = 1;

	/* validate the aop_data before dereference it */
	if (!aop_data && !aop_kernel_list_head) {
		AOP_KERNEL_PRINT("Invalid arguments\n");
		return;
	}

	/* check whether the new sample is match with previous kernel img */
	list_for_each_safe(pos, q, aop_kernel_list_head)
	{
		/* loop thru all the nodes */
		plist = list_entry(pos, struct aop_kernel_list, kern_list);
		if(plist){
			if (plist->pid == aop_data->tid ){
				plist->samples_count++;
				goto _OUT;
			}

			if (plist->ki) {
				/* verify pc is within any of stored kernel range */
				if (plist->ki->start <= aop_data->pc && 
						plist->ki->end > aop_data->pc) {
					plist->samples_count++;
					goto _OUT;
				}
			}

			if (flag){
				/* backup the first list item ie vm linux or no-vmlinux kernel item */
				flag = 0;
				pstVMLinux = plist;
			}
		}
	}

	/* to update sample to kernel idle task */
	if ( aop_data->tid == 0 ) {
		/* get new kernel data */
		new_kern_data = aop_malloc_kernel_data ("kernel_idle", 0,0);

		/* update sample count */
		if (new_kern_data) {
			/* no need to validate ki here, as it is done while malloc kernel data */
			new_kern_data->pid = 0;
			new_kern_data->samples_count = 1;
			list_add_tail(&new_kern_data->kern_list, aop_kernel_list_head);
		} else {
			AOP_KERNEL_PRINT ("aop_malloc_kernel_data failed!!!\n");
			return;
		}
	} else {
		struct module * mod;
		/* img not found then get the module for given pc
		 and check whether it is valid module or not */
		mod = aop_get_module_struct(aop_data->pc);
		if (mod) {
			/* get new kernel data */
			new_kern_data = aop_malloc_kernel_data (
					mod->name, 
					(unsigned long)mod->module_core,
					(unsigned long)mod->module_core + mod->core_size );

			/* update sample count */
			if (new_kern_data) {
				/* no need to validate ki here, as it is done while malloc kernel data */
				new_kern_data->samples_count = 1;
				list_add_tail(&new_kern_data->kern_list, aop_kernel_list_head);
			} else {
				AOP_KERNEL_PRINT ("aop_malloc_kernel_data failed!!!\n");
				return;
			}
		}else {
			/* increment the sample count of first  kernel img (vmlinux or no-vmlinux) */
			if (pstVMLinux)
				pstVMLinux->samples_count += 1;
		}
	}

	_OUT:
	aop_nr_kernel_samples++;
}

/* kernel report info sort compare function */
int aop_kernel_report_cmp ( 
				const struct list_head *a, const struct list_head *b )
{
	struct aop_report_all_list *a_item;
	struct aop_report_all_list *b_item;
    unsigned long first_cmd_data;
    unsigned long second_cmd_data;

	a_item = list_entry(a, struct aop_report_all_list, report_list);
	if(!a_item){
		AOP_KERNEL_PRINT("[%s] : a_item is NULL\n", __FUNCTION__);	
		return 1;			
	}	

	b_item = list_entry(b, struct aop_report_all_list, report_list);
	if(!b_item){
		AOP_KERNEL_PRINT("[%s] : b_item is NULL\n", __FUNCTION__);	
		return 1;			
	}	

	/* as we have only two option we choose if stmt to compare this */
	if (aop_config_sort_option == AOP_SORT_BY_SAMPLES) {
		first_cmd_data = a_item->samples_count;
		second_cmd_data = b_item->samples_count;

		/* to do descending order */
		if ( first_cmd_data > second_cmd_data )
			return -1;
		return ( first_cmd_data == second_cmd_data )?0:1;
	}

	/* rest of comparision */
	return 0;
}

/* to prepare kernel list to show at report all symbol report & report kernel symbol report */
int aop_kernel_prepare_report ( struct list_head *kern_list_head )
{
	struct aop_kernel_list *plist;
	struct list_head *pos, *q;

	list_for_each_safe(pos, q, aop_kernel_list_head)
	{
		/* loop thru all the nodes */
		plist = list_entry(pos, struct aop_kernel_list, kern_list);
		if(plist){
			if (plist->samples_count) {
				struct aop_report_all_list *img_data;

				img_data = (struct aop_report_all_list*)
									vmalloc(sizeof(struct aop_report_all_list));
				if(!img_data) {
					AOP_KERNEL_PRINT("Add image data failed: no memory\n");
					return 0; /* no memory!!, at the same time we report other data  */
				}

				/* assign the sample information to  new img data */
				img_data->is_kernel = 1;
				img_data->report_type.kernel_name = plist->ki->name;
				img_data->samples_count = plist->samples_count;
				list_add_tail(&img_data->report_list, kern_list_head);
			}
		}
	}

	return 0;
}

/*Dump the kernel data*/
int aop_generate_kernel_samples(void)
{
	if(aop_nr_kernel_samples) {
		struct list_head *kern_head;
		struct aop_report_all_list *plist;
		struct list_head *pos, *q;

		if(!(kern_head=(struct list_head*)vmalloc(sizeof(struct list_head)))){
			return 1;
		}

		INIT_LIST_HEAD(kern_head);

		aop_kernel_prepare_report ( kern_head );

		aop_list_sort(kern_head, aop_kernel_report_cmp);

		printk ("Samples\t  %%\tkernel[module]name\n");
		printk ("------------------------------------------\n");
		/* print kernel module samples */
		list_for_each_safe(pos, q, kern_head)
		{
			/* loop thru all the nodes */
			plist = list_entry(pos, struct aop_report_all_list, report_list);
			if(plist){
				int perc = ((plist->samples_count*100)/aop_nr_total_samples);
				printk("%8u %3d%%\t%s\n", plist->samples_count, perc,
					plist->report_type.kernel_name );

				list_del(pos);

				/* free all report item memory */
				vfree(plist);
			}
		}
		printk ("------------------------------------------\n");
		vfree (kern_head);
	}
	return 1;
}

