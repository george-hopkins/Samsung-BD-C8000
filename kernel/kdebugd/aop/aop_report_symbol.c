/*
 *  linux/drivers/oprofile/aop_report_symbol.c
 *
 *  Advance oprofile system wide symbol report  releated functions are declared here
 *
 *  Copyright (C) 2009  Samsung	
 *
 *  2009-09-01  Created by karuna.nithy@samsung.com.
 *
 */

#include <linux/proc_fs.h>
#include <linux/kdebugd.h>
#include <linux/kernel.h>
#include <linux/vmalloc.h>
#include <linux/dcache.h>
#include <linux/kallsyms.h>
#include <linux/list.h>

#include "aop_oprofile.h"
#include "aop_report_symbol.h"
#include "aop_sym_db.h"

/* trace on/off flag */
//#define AOP_SYM_TRACE_ON

#ifdef AOP_SYM_TRACE_ON
#define AOP_SYM_PRINT(fmt,args...) do {		\
  printk(KERN_INFO "%s: " fmt,__FUNCTION__ ,##args); \
  } while(0)

#else
#define AOP_SYM_PRINT(fmt...) do { } while(0)
#endif /* #ifdef AOP_SYM_TRACE_ON */

#define NO_COOKIE 0LLU

/* to show report with function name only */
#define AOP_SHOW_FUNCTION_ONLY	0

/* to show report with function name only */
#define AOP_SHOW_FUNCTION_N_LIB	1

/* to define max kernel symbol length based on ksym length and module name length */
#define AOP_MAX_SYM_LEN ( sizeof("%s [%s]")          \
				     + 22 /* KSYM_NAME_LEN */    \
				     + 10 /* OFFSET */ \
				     + 10 /* MODULE_NAME_LEN */ + 1 )

/*To get the kernel indication string
If the given sample is for kernel then show (k) after the vma address otherwise leave blank */
#define AOP_KERNEL_IDENTIFICATION(X) ((X)?"(k)":"   ")

/*link list to store the symbol related data which 
is collected after processing the raw data.
This includes cookie value which is decoded into the PATH of the library & appl. 
sample count denotes the number of samples collected for that library,
vma and pc to store symbol progran counter. */
struct aop_sym_list {
	struct list_head sym_list;
    cookie_t img_cookie; /*coolie value of library*/
    cookie_t app_cookie; /*coolie value of application*/
    unsigned int addr; /*program counter value*/
    unsigned int samples;
	int in_kernel; /*in_kernel = 1 if the context is kernel, =0 if it is in user context*/
	pid_t tid;
};

/*The samples are processed and the data specific to kernel which
are refereed are extracted and collected in this structure. aop_kernel_list_head is the 
head of node of the linked list*/
//static struct aop_sym_list *aop_sym_list_head = NULL;
static struct list_head *aop_sym_list_head;
static struct list_head *aop_sym_elf_list_head;

static int aop_sym_elf_load_done = 0;

/*The samples are processed and the data specific to application which
are run are extracted and collected in this structure. aop_app_list_head is
the head of node of the linked list*/
extern aop_image_list *aop_app_list_head;

/*total samples = user+kernel*/
extern unsigned long aop_nr_total_samples;

/* aop report sort option, by default it is none.
It may be changed using setup configuration option */
extern int aop_config_sort_option;

/* aop report symbol demangle option, by default it is none.
It may be changed using setup configuration option */
extern int aop_config_demangle;

/* symbol name demangle function */
extern int aop_sym_demangle(char* buff, char* new_buff, int buf_len);

/*decode the cookie into the name of application and libraries.*/
static char* aop_decode_cookie_without_path (
					cookie_t cookie, char *buf, size_t buf_size)
{
	long ret = 0;

	if(buf) {
		if ( cookie > 0 )
			/*call the function to decode the cookie value without directory PATH*/
			ret = aop_lookup_dcookie(cookie, buf, buf_size);
		else
			/* if cookie is not available then return KERNEL */
			ret = snprintf(buf, DNAME_INLINE_LEN_MIN, "[KERNEL]");

		/* append null character at the end  */
		if (ret != -EINVAL )
			buf[ret]='\0';
	}

	/* return buf, it is usefull, incase if this function called in printk function */
	return buf;
}

/*decode the cookie into name of kernel application 
It is based on logic of __print_symbol() function in kallsyms.c file. 
Store the symbol name corresponding to address in buffer */
static void
aop_decode_cookie_kernel_symbol(
			char *buffer, int buf_len, unsigned long address, 
			unsigned long *offset )
{
	char *modname;
	const char *name;
	unsigned long size;
	char namebuf[KSYM_NAME_LEN+1];

	/* validate the buffer length */
	if(buf_len < (AOP_MAX_SYM_LEN) ) {
	    AOP_SYM_PRINT("insufficient length buf_len= %d\n", buf_len);
		snprintf(buffer, AOP_MAX_SYM_LEN, "0x%lx", address);
	    return;
    }

	*offset = 0;

	/* get the symbol name, module name etc for given kernel address */
	name = kallsyms_lookup(address, &size, offset, &modname, namebuf);

	if (!name)
        /* we require only 11 bytes for storing this, safely assume
           that buf_len is sufficiently more than 11, considering
           KSYM_NAME_LEN is itself 127 (normally) and we already
           checked above */
		snprintf(buffer, AOP_MAX_SYM_LEN, "0x%lx", address);
	else {
        /* Note: the precision values are hard-coded here, any
           change in the related macro should be corrected here
           also. */
		if (modname)
 			snprintf(buffer, AOP_MAX_SYM_LEN,
 				"[%.10s] %.22s", modname, name );
		else
			snprintf(buffer, AOP_MAX_SYM_LEN, "%.32s", name);
	}

	/* On overflow of buffer, it would have already corrupted
	   memory, but anyway this check may be helpful. BTW, it
	   should never overflow because we already handled that in
	   the beginning. */
	WARN_ON(strlen(buffer) >= buf_len);
}

/* symbol report info sort compare function */
static int __aop_sym_report_cmp ( 
				const struct list_head *a, const struct list_head *b )
{
	struct aop_sym_list *a_item;
	struct aop_sym_list *b_item;
    unsigned long first_cmd_data;
    unsigned long second_cmd_data;

	a_item = list_entry(a, struct aop_sym_list, sym_list);
	if(!a_item){
		AOP_SYM_PRINT("[%s] : a_item is NULL\n", __FUNCTION__);	
		return 1;			
	}	

	b_item = list_entry(b, struct aop_sym_list, sym_list);
	if(!b_item){
		AOP_SYM_PRINT("[%s] : b_item is NULL\n", __FUNCTION__);	
		return 1;			
	}	

	/* as we have only two option we choose if stmt to compare this */
	switch (aop_config_sort_option)
	{
		case AOP_SORT_BY_VMA:
	    	first_cmd_data = a_item->addr;
    		second_cmd_data = b_item->addr;

			/* to do ascending order */
			if ( first_cmd_data < second_cmd_data )
				return -1;
			break;

		case AOP_SORT_BY_SAMPLES:
    		first_cmd_data = a_item->samples;
    		second_cmd_data = b_item->samples;

			/* to do descending order */
			if ( first_cmd_data > second_cmd_data )
				return -1;
			break;

		default:
			return 0; /* to keep the last sort order */
	}

	/* rest of comparision */
	return ( first_cmd_data == second_cmd_data )?0:1;
}

/* create new list head */
struct list_head *__aop_sym_report_alloc_list_head (void)
{
	struct list_head *sym_list_head;
	if(!(sym_list_head = (struct list_head*)vmalloc(sizeof(struct list_head)))){
		return NULL;
	}else
		return sym_list_head;
}

/* delete all allocated list entry */
static void __aop_sym_report_free_list_entry ( struct list_head *plist_head )
{
	struct aop_sym_list *plist;
	struct list_head *pos, *q;

	if (plist_head) {
		list_for_each_safe(pos, q, plist_head)
		{
			/* loop thru all the nodes and free the allocated memory*/
			plist = list_entry(pos, struct aop_sym_list, sym_list);
			if(plist){
				list_del(pos);

				/* free sym item memory */
				vfree(plist);
			}	
		}
	}
}

/* delete all allocated list entry */
static int __aop_sym_report_no_of_list_entry ( struct list_head *plist_head )
{
	struct aop_sym_list *plist;
	struct list_head *pos, *q;
	int no_of_list_entry = 0;

	if (plist_head) {
		list_for_each_safe(pos, q, plist_head)
		{
			/* loop thru all the nodes and free the allocated memory*/
			plist = list_entry(pos, struct aop_sym_list, sym_list);
			if(plist){
				no_of_list_entry++;
			}	
		}
	}

	return no_of_list_entry;
}

/* Function to sort the symbol list  
If the sort option is same as prev, then skip the sorting again */
static void __aop_sym_report_sort ( struct list_head *sym_head )
{
	/* aop_sym_list_head is always initialize with null pointer 
	so not needed to verify here */
	aop_list_sort(sym_head, __aop_sym_report_cmp);
}

/* update samples to generate symbol wise report */
static void aop_sym_report_update_sample( 
				struct list_head *func_list_head, 
				op_data *aop_data, unsigned int sample  )
{
	struct aop_sym_list *new_sym_data = NULL;
	struct aop_sym_list *plist;
	struct list_head *pos, *q;
	cookie_t cookie; /*coolie value of library*/

	/* validate the aop_data before dereference it */
	if (!aop_data && !func_list_head) {
		AOP_SYM_PRINT("Invalid arguments\n");
		return;
	}

	/* to compare the cookie value with existing list */
	if (aop_data->in_kernel)
		/* The logic here: if we're in the kernel, the cached cookie is
		 * meaningless (though not the app_cookie if separate_kernel )
		 */
		cookie = NO_COOKIE;
	else
		cookie = aop_data->cookie;

	/* check whether the given pc match with previous kernel img */
	list_for_each_safe(pos, q, func_list_head)
	{
		/* loop thru all the nodes */
		plist = list_entry(pos, struct aop_sym_list, sym_list);
		if(plist){
			/* verify pc is within any of stored kernel range */
			if ((cookie == plist->img_cookie) && 
			   (aop_data->app_cookie == plist->app_cookie) && 
			   (aop_data->pc == plist->addr) &&
			   (aop_data->tid == plist->tid)){
				plist->samples += sample;
				return;
			}
		}	
	}

	/* symbol data is not exists, so create new node and update it in aop_kernel_list_head */
	new_sym_data = (struct aop_sym_list*)vmalloc(sizeof(struct aop_sym_list));
	if(!new_sym_data) {
		AOP_SYM_PRINT("Add sym data failed: no memory\n");
		return; /* no memory!! */
	}

	/* assign the sample information to  new sym data */
	new_sym_data->addr = aop_data->pc;
	new_sym_data->in_kernel = aop_data->in_kernel;
	new_sym_data->tid = aop_data->tid;	
	new_sym_data->img_cookie = cookie;
	new_sym_data->app_cookie = aop_data->app_cookie;
	new_sym_data->samples = sample;
	list_add_tail(&new_sym_data->sym_list, func_list_head);
}

/*It list the application name and copy the app cookie value for further process */
static int aop_show_application_img ( cookie_t **app_cookie )
{
	aop_image_list *tmp_app_data = aop_app_list_head;
	int count = 0, i =0;
	cookie_t *tmp_app_cookie;

	/* loop thru app data and show the application name */
	while(tmp_app_data){
		char app_name[DNAME_INLINE_LEN_MIN+1];

		if (!count){
			printk ("\n\n\tIndex Application Images\n");
			printk ("------------------------------------\n");
		}

		/* get the application name and show to user */
		aop_decode_cookie_without_path(tmp_app_data->cookie_value, 
					app_name, DNAME_INLINE_LEN_MIN);
		printk ("\t[%d] - %s\n", count+1, app_name );
		tmp_app_data = tmp_app_data->next;
		count++;
	}

	/* to show kernel names for the all application */
	tmp_app_data = aop_app_list_head;
	while(tmp_app_data){
		char app_name[DNAME_INLINE_LEN_MIN+1];

		/* get the application name and show to user */
		aop_decode_cookie_without_path(tmp_app_data->cookie_value, 
					app_name, DNAME_INLINE_LEN_MIN);
		printk ("\t[%d] - %s (KERNEL)\n", count+i+1, app_name );
		tmp_app_data = tmp_app_data->next;
		i++;
	}

	/* allocate memory for app_cookie to store appl img cookie value */
	if (count) {
		/* Not need to verify before copy data to *app_cookie 
		(as app_cookie point to address of the caller functions local variable
		not needed to verify it here) */
		*app_cookie = (cookie_t *)vmalloc ( count * sizeof(cookie_t));
		if ( !*app_cookie ) {
			AOP_SYM_PRINT("%d> No memory\n", __LINE__ );
			return 0;
		}
		printk ("------------------------------------\n\n");
	}else{
		printk ("No Application Image Found!!!\n");
	}

	/* copy the pointer  to tmp, to fill the app cookie value */
	tmp_app_cookie = *app_cookie;

	/* get list head to copy app cookie value */
	tmp_app_data = aop_app_list_head;
	i = 0;
	while(tmp_app_data){
		/* as the max no of items to copy is calculated earlier, 
		memory verification is not done here */
		*(tmp_app_cookie+i) = tmp_app_data->cookie_value;
		tmp_app_data = tmp_app_data->next;
		i++;
	}

	/* return the no of application image found with kernel */
	return count+count;
}

/* build system wide function name & samples */
static struct list_head *aop_sym_report_build_function_samples ( void )
{
	if (aop_sym_elf_list_head)
		return aop_sym_elf_list_head;
	else {
		struct aop_sym_list *plist;
		struct list_head *pos, *q;
		char img_name[DNAME_INLINE_LEN_MIN+1];
		char *func_name;
		int list_index = 1, total_no_of_entries = 
				__aop_sym_report_no_of_list_entry (aop_sym_list_head); 
		int prev_per = 0;
		
		/* rebuild new list with function name wise samples */
		if(!(aop_sym_elf_list_head = __aop_sym_report_alloc_list_head ())){
			AOP_SYM_PRINT("out of memory - [aop_sym_elf_list_head]\n");
			return aop_sym_list_head;
		}

		func_name = (char*)vmalloc(DIR_NAME_PATH_LENGTH_MAX);
		if(!func_name) {
			AOP_SYM_PRINT("func_name: no memory\n");	
			vfree (aop_sym_elf_list_head);
			return aop_sym_list_head;
		}		

		INIT_LIST_HEAD(aop_sym_elf_list_head);

		/* update function wise samples */
		list_for_each_safe(pos, q, aop_sym_list_head){
			/* loop thru all the nodes */
			plist = list_entry(pos, struct aop_sym_list, sym_list);
			if(plist){
				unsigned long offset = 0;
				unsigned int addr = plist->addr;
				op_data aop_data;

				if (aop_sym_elf_load_done) {
					int per = ((list_index*100)/total_no_of_entries);
					if (!(per%10) && (prev_per!=per)){
						prev_per = per;
						printk("Creating Database...%d%%\r", per);
					}
					list_index++;
				}

				/* get the function name of the given PC & elf file */
				func_name[0] = '\0';
				if ( plist->in_kernel) {
					aop_decode_cookie_kernel_symbol(
						func_name, DIR_NAME_PATH_LENGTH_MAX, addr, &offset);
					addr = addr-offset;
				} else {
					/* get library image name */
					aop_decode_cookie_without_path(plist->img_cookie, 
								img_name, DNAME_INLINE_LEN_MIN);
					aop_get_symbol(img_name, &addr, func_name);
				}

				/* assign the new addr & other member value to aop data */
				aop_data.pc = addr;
				aop_data.in_kernel = plist->in_kernel;
				aop_data.tid = plist->tid;	
				aop_data.cookie = plist->img_cookie;
				aop_data.app_cookie = plist->app_cookie;

				aop_sym_report_update_sample( aop_sym_elf_list_head, 
						&aop_data, plist->samples);
			}
		}

		/* to sort the symbol report before print */
		__aop_sym_report_sort (aop_sym_elf_list_head);

		if (aop_sym_elf_load_done)
			printk("\n");

		vfree (func_name);

		return aop_sym_elf_list_head;
	}
}

/* aop symbol report function which shows per application & library names 
if show_lib is AOP_SHOW_FUNCTION_N_LIB, then show application wise sample 
including library name otherwise just show function names of the selected application 
*/
static int aop_sym_report_internel(int show_lib)
{
	struct aop_sym_list *plist;
	struct list_head *pos, *q;
	int perc, total_samples = 0;
	char img_name[DNAME_INLINE_LEN_MIN+1];
	char app_name[DNAME_INLINE_LEN_MIN+1];
	int no_of_appl_img, no_of_org_appl_img, choice;
	cookie_t *app_cookie = NULL;
	int appl_index, kernel;
	char *func_name, *d_fname;
	struct list_head *func_list_head;

	func_name = (char*)vmalloc(DIR_NAME_PATH_LENGTH_MAX);
	if(!func_name) {
		AOP_SYM_PRINT("func_name: no memory\n");	
		return 1;
	}		

	d_fname = (char*)vmalloc(DIR_NAME_PATH_LENGTH_MAX);
	if(!d_fname) {
		AOP_SYM_PRINT("d_fname: no memory\n");	
		vfree (func_name);
		return 1;
	}		

	/* list appl img and find the total no of appl img profiled */
	no_of_appl_img = aop_show_application_img (&app_cookie);
	if (!no_of_appl_img) {
		AOP_SYM_PRINT("%d> No sample or memory\n", __LINE__ );
		goto __END;
	}

	no_of_org_appl_img = no_of_appl_img / 2;

	/* list all application img and take copy of that dcookie */
	printk("Select the Appl Image Index ==> ");
	choice = debugd_get_event();
	printk("\n");

	/* validate the choice */
	if ( (choice < 1) || (choice > no_of_appl_img) ) {
		AOP_SYM_PRINT ("Invalid application image\n");
		goto __END;
	}

	choice--;
	appl_index = choice % no_of_org_appl_img;
	kernel = choice / no_of_org_appl_img;

	/* get the user selection application name */
	aop_decode_cookie_without_path( *(app_cookie+appl_index), app_name, 
														DNAME_INLINE_LEN_MIN);

	func_list_head = aop_sym_report_build_function_samples();

	/* to draw header text */
	printk ("\n\nSymbol profiling for application \"%s%s\" %s:\n", 
		app_name, ((kernel) ? "(KERNEL)" : " "),
		((show_lib == AOP_SHOW_FUNCTION_N_LIB)?"including library ":" "));
	if (show_lib == AOP_SHOW_FUNCTION_N_LIB)
		printk ("vma\t\tSamples  %%  image name\t\tsymbol name\n");
	else
		printk ("vma\t\tSamples  %%  symbol name\n");
	printk ("---------------------------------------------------"
				"-----------------------\n");

	/* print symbol wise samples */
	list_for_each_safe(pos, q, func_list_head)
	{
		/* loop thru all the nodes */
		plist = list_entry(pos, struct aop_sym_list, sym_list);
		if(plist){
			int show_sample = 1;
			unsigned int pc_value = plist->addr;
			unsigned long offset = 0;
			perc = ((plist->samples*100)/aop_nr_total_samples);

			/* verify application is same as user selected application */
			if (plist->app_cookie == *(app_cookie+appl_index) ) {
				/* get the app name */
				aop_decode_cookie_without_path(plist->app_cookie, 
							app_name, DNAME_INLINE_LEN_MIN);

				/* get the img name */
				aop_decode_cookie_without_path(plist->img_cookie, 
							img_name, DNAME_INLINE_LEN_MIN);

				/* get the function name of the given PC & elf file */
				d_fname[0] = '\0';
				func_name[0] = '\0';

				if ( plist->in_kernel) {
					if (!kernel)
						show_sample = 0; /* don't show samples */
					else {
						/* get symbol name from kallsymbol */
						aop_decode_cookie_kernel_symbol(
								d_fname, DIR_NAME_PATH_LENGTH_MAX, pc_value,
								&offset );
						snprintf(func_name, DIR_NAME_PATH_LENGTH_MAX, d_fname);
					}
				} else {
					if (kernel)
						show_sample = 0; /* don't show samples */
					else if	( aop_get_symbol(img_name, 
									&pc_value, func_name)!= 0 ) {
						/* get symbol name from elf file */
						snprintf(d_fname, DIR_NAME_PATH_LENGTH_MAX, "???");
						snprintf(func_name, DIR_NAME_PATH_LENGTH_MAX, d_fname);
						pc_value = 0;
					} else if (aop_config_demangle) {
						aop_sym_demangle(func_name, 
							d_fname, DIR_NAME_PATH_LENGTH_MAX);
					}
				}

				if (show_sample) {

					/* if report option with library then show library */
					if (show_lib == AOP_SHOW_FUNCTION_N_LIB)
						printk("%08x%s %8u   %3u%% %-20s %.32s\n", 
								plist->addr, 
								AOP_KERNEL_IDENTIFICATION(plist->in_kernel), 
								plist->samples, 
								perc,img_name, 
								(aop_config_demangle)?d_fname :func_name );
					else
						/* show sample without library name */
						printk("%08x%s %8u   %3u%% %.32s\n", 
							plist->addr, 
							AOP_KERNEL_IDENTIFICATION(plist->in_kernel), 
							plist->samples, perc,
							(aop_config_demangle)?d_fname :func_name );

					/* sum the samples of particular application */
					total_samples += plist->samples;
				}
			}

		}	
	}

	printk ("Total Samples found %d\n", total_samples);

__END:

	/* free all allocated resouce */
	vfree (func_name);
	vfree (d_fname);
	if (app_cookie)
		vfree (app_cookie);

    return 0;
}

/* elf symbol load/unload notification callback function */
static void aop_sym_elf_load_notification_callback ( int elf_load_flag )
{
	AOP_SYM_PRINT("Elf database%sloaded\n", (elf_load_flag)? " " : " un" );

	/* this variable is not used now, but in future it may be used, so i keep it */
	aop_sym_elf_load_done = elf_load_flag;

	/* if elf database is modified we need to create new list, so free the elf list data */
	if (aop_sym_elf_list_head) {
		__aop_sym_report_free_list_entry(aop_sym_elf_list_head);
		vfree (aop_sym_elf_list_head);
		aop_sym_elf_list_head = NULL;
	}
}

/* free allocated symbo data */
void aop_sym_report_free_sample_data ( void )
{
	__aop_sym_report_free_list_entry(aop_sym_list_head);

	if (aop_sym_elf_list_head) {
		__aop_sym_report_free_list_entry(aop_sym_elf_list_head);
		vfree (aop_sym_elf_list_head);
		aop_sym_elf_list_head = NULL;
	}
}

/* initialize apo symbol report head list */
int aop_sym_report_init ( void )
{
	aop_sym_register_oprofile_elf_load_notification_func (
			aop_sym_elf_load_notification_callback );
	
	if(!(aop_sym_list_head = __aop_sym_report_alloc_list_head ())){
		AOP_SYM_PRINT("out of memory - [aop_sym_list_head]\n");
		return -ENOMEM;
	}

	INIT_LIST_HEAD(aop_sym_list_head);
	return 0;
}

/* update all samples to generate symbol wise report */
void aop_sym_report_update_sample_data( op_data *aop_data )
{
	return aop_sym_report_update_sample (aop_sym_list_head, aop_data, 1 );
}

/* Dump the application wise function name & library name */
int aop_sym_report_per_application ( void )
{
	/* to report per application wise function name */
	aop_sym_report_internel(AOP_SHOW_FUNCTION_ONLY);

	return 1; /* to show menu */
}

/* Dump the application wise function name including library name */
int aop_sym_report_per_application_n_lib ( void )
{
	/* to report per application wise function name */
	aop_sym_report_internel(AOP_SHOW_FUNCTION_N_LIB);

	return 1; /* to show menu */
}

/*Report the symbol information of the image(application or library) specified
by image_type. This will print only the user context samples and not the kernel one.*/
int aop_sym_report_per_image_user_samples ( 
							int image_type, cookie_t img_cookie )
{
	int perc;
	cookie_t cookie = NO_COOKIE;
	unsigned int pc_value = 0;	
	char img_name[DNAME_INLINE_LEN_MIN+1];
	struct aop_sym_list *tmp_sym_data;
	struct list_head *pos, *q;
	char *func_name, *d_fname;
	struct list_head *func_list_head;

	func_name = (char*)vmalloc(DIR_NAME_PATH_LENGTH_MAX);
	if(!func_name) {
		AOP_SYM_PRINT("func_name: no memory\n");	
		return 1;
	}		

	d_fname = (char*)vmalloc(DIR_NAME_PATH_LENGTH_MAX);
	if(!d_fname) {
		AOP_SYM_PRINT("d_fname: no memory\n");	
		vfree (func_name);
		return 1;
	}		

	func_list_head = aop_sym_report_build_function_samples ();
	
	printk ("vma\t\tSamples  %%  symbol name\n");
	printk("------------------------------------------------\n");	

	list_for_each_safe(pos, q, func_list_head)
	{
		/* loop thru all the nodes */
		tmp_sym_data = list_entry(pos, struct aop_sym_list, sym_list);
		if(tmp_sym_data){
			if(image_type == 0){
				cookie = tmp_sym_data->app_cookie;
			}else if(image_type == 1)	{
				cookie = tmp_sym_data->img_cookie;
			}else{
				printk("\nInvalid Image Type Requested...\n");
				vfree (func_name);
				vfree (d_fname);
				return -1;
			}		
			if(cookie == img_cookie && tmp_sym_data->in_kernel == 0){
				pc_value = tmp_sym_data->addr;

				d_fname[0] = '\0';
				func_name[0] = '\0';

				/* get appl name */
				aop_decode_cookie_without_path(tmp_sym_data->img_cookie, 
							img_name, DNAME_INLINE_LEN_MIN);	
				func_name[0] = '\0';
				if ( aop_get_symbol(img_name, &pc_value, func_name)!= 0 ) {
					snprintf(d_fname, DIR_NAME_PATH_LENGTH_MAX, "???");
					snprintf(func_name, DIR_NAME_PATH_LENGTH_MAX, d_fname);
					pc_value = 0;
				} else if (aop_config_demangle){
					aop_sym_demangle(func_name, 
						d_fname, DIR_NAME_PATH_LENGTH_MAX);
				}

				perc = ((tmp_sym_data->samples*100)/aop_nr_total_samples);
				printk("%08x %8u   %3u%% %.32s\n", tmp_sym_data->addr,
					tmp_sym_data->samples, perc, 
					(aop_config_demangle)? d_fname : func_name );
			}
		}
	}

	vfree (func_name);
	vfree (d_fname);
	return 0;
}

/*Report the symbol information of the thread ID(TID) specified by user.*/
int aop_sym_report_per_tid ( pid_t pid_user )
{
	int perc;
	unsigned int pc_value = 0;	
	char img_name[DNAME_INLINE_LEN_MIN+1];
	struct aop_sym_list *tmp_sym_data;
	struct list_head *pos, *q;
	char *func_name, *d_fname;
	struct list_head *func_list_head;
	int report_found = 0;
	int list_index = 1, total_no_of_entries = 
			__aop_sym_report_no_of_list_entry (aop_sym_list_head); 
	int prev_per = 0;

	func_name = (char*)vmalloc(DIR_NAME_PATH_LENGTH_MAX);
	if(!func_name) {
		AOP_SYM_PRINT("func_name: no memory\n");	
		return 1;
	}		

	d_fname = (char*)vmalloc(DIR_NAME_PATH_LENGTH_MAX);
	if(!d_fname) {
		AOP_SYM_PRINT("d_fname: no memory\n");	
		vfree (func_name);
		return 1;
	}		

	/* rebuild new list with function name wise samples */
	if(!(func_list_head = __aop_sym_report_alloc_list_head ())){
		AOP_SYM_PRINT("out of memory - [func_list_head]\n");
		vfree (func_name);
		vfree (d_fname);
		return 1;
	}

	INIT_LIST_HEAD(func_list_head);

	/* update function wise samples */
	list_for_each_safe(pos, q, aop_sym_list_head)
	{
		if (aop_sym_elf_load_done) {
			int per = ((list_index*100)/total_no_of_entries);
			if (!(per%10) && (prev_per!=per)){
				prev_per = per;
				printk("Creating Database...%d%%\r", per);
			}
			list_index++;
		}

		/* loop thru all the nodes */
		tmp_sym_data = list_entry(pos, struct aop_sym_list, sym_list);
		if(tmp_sym_data && (tmp_sym_data->tid == pid_user)){
			unsigned long offset = 0;
			unsigned int addr = tmp_sym_data->addr;
			op_data aop_data;

			/* get the function name of the given PC & elf file */
			func_name[0] = '\0';
			if ( tmp_sym_data->in_kernel) {
				aop_decode_cookie_kernel_symbol(
					func_name, DIR_NAME_PATH_LENGTH_MAX, addr, &offset);
				addr = addr-offset;
			} else {
				/* get library image name */
				aop_decode_cookie_without_path(tmp_sym_data->img_cookie, 
							img_name, DNAME_INLINE_LEN_MIN);
				aop_get_symbol(img_name, &addr, func_name);
			}
	
			/* assign the new addr & other member value to aop data */
			aop_data.pc = addr;
			aop_data.in_kernel = tmp_sym_data->in_kernel;
			aop_data.tid = tmp_sym_data->tid;	
			aop_data.cookie = tmp_sym_data->img_cookie;
			aop_data.app_cookie = tmp_sym_data->app_cookie;
	
			aop_sym_report_update_sample( func_list_head, 
					&aop_data, tmp_sym_data->samples);

			report_found++;
		}
	}

	if (!report_found){
		printk("Given PID is not valid...\n");
		goto __END;
	}

	/* to sort the symbol report before print */
	__aop_sym_report_sort (func_list_head);

	if (aop_sym_elf_load_done)
		printk("\n");

	printk ("\nvma\t\tSamples  %% image name\t\tsymbol name\n");
	printk ("-------------------------------------------------------------"
			"------------\n");

	list_for_each_safe(pos, q, func_list_head)
	{
		/* loop thru all the nodes */
		tmp_sym_data = list_entry(pos, struct aop_sym_list, sym_list);
		if(tmp_sym_data){
			unsigned long offset = 0;
			pc_value = tmp_sym_data->addr;
			d_fname[0] = '\0';
			func_name[0] = '\0';

			/* get appl name */
			aop_decode_cookie_without_path(tmp_sym_data->img_cookie, 
						img_name, DNAME_INLINE_LEN_MIN);	
			func_name[0] = '\0';

			if ( tmp_sym_data->in_kernel) {
				aop_decode_cookie_kernel_symbol(
							d_fname, DIR_NAME_PATH_LENGTH_MAX, pc_value,
							&offset);
				snprintf(func_name, DIR_NAME_PATH_LENGTH_MAX, d_fname);
			} else {
				if ( aop_get_symbol(img_name, &pc_value, func_name)!= 0 ) {
					snprintf(d_fname, DIR_NAME_PATH_LENGTH_MAX, "???");
					snprintf(func_name, DIR_NAME_PATH_LENGTH_MAX, d_fname);
					pc_value = 0;
				} else if (aop_config_demangle) {
					aop_sym_demangle(func_name, 
						d_fname, DIR_NAME_PATH_LENGTH_MAX);
				}
			}

			perc = ((tmp_sym_data->samples*100)/aop_nr_total_samples);
			printk("%08x%s %8u   %3u%% %-20s %.32s\n",
				tmp_sym_data->addr, 
				AOP_KERNEL_IDENTIFICATION (tmp_sym_data->in_kernel),  
				tmp_sym_data->samples, perc, img_name, 
				(aop_config_demangle)?d_fname :func_name );
		}
	}

__END:
	vfree (func_name);
	vfree (d_fname);

	__aop_sym_report_free_list_entry(func_list_head);
	vfree (func_list_head);

	return 0;
}

/* dump system wide function name & samples */
int aop_sym_report_system_wide_function_samples ( void )
{
	struct aop_sym_list *plist;
	struct list_head *pos, *q;
	int total_samples = 0;
	char img_name[DNAME_INLINE_LEN_MIN+1];
	char *func_name, *d_fname;
	struct list_head *func_list_head;

	if (!aop_nr_total_samples) {
		printk("No Samples found\n");
		return 1;
	}

	func_name = (char*)vmalloc(DIR_NAME_PATH_LENGTH_MAX);
	if(!func_name) {
		AOP_SYM_PRINT("func_name: no memory\n");	
		return 1;
	}		

	d_fname = (char*)vmalloc(DIR_NAME_PATH_LENGTH_MAX);
	if(!d_fname) {
		AOP_SYM_PRINT("d_fname: no memory\n");	
		vfree (func_name);
		return 1;
	}		

	func_list_head = aop_sym_report_build_function_samples ();

	printk ("vma\t\tSamples  %%  image name\t	 symbol name\n");
	printk ("--------------------------------------------------------------\n");

	total_samples = 0;
	/* print symbol wise samples */
	list_for_each_safe(pos, q, func_list_head)
	{
		/* loop thru all the nodes */
		plist = list_entry(pos, struct aop_sym_list, sym_list);
		if(plist){
			unsigned int pc_value = plist->addr;
			int perc = ((plist->samples*100)/aop_nr_total_samples);

			/* get library image name */
			aop_decode_cookie_without_path(plist->img_cookie, 
						img_name, DNAME_INLINE_LEN_MIN);

			/* get the function name of the given PC & elf file */
			d_fname[0] = '\0';
			func_name[0] = '\0';
			if ( plist->in_kernel) {
				unsigned long offset = 0;
				aop_decode_cookie_kernel_symbol(
							d_fname, DIR_NAME_PATH_LENGTH_MAX, pc_value,
							&offset);
				snprintf(func_name, DIR_NAME_PATH_LENGTH_MAX, d_fname);
			} else {
				if ( aop_get_symbol(img_name, &pc_value, func_name)!= 0 ) {
					snprintf(d_fname, DIR_NAME_PATH_LENGTH_MAX, "???");
					snprintf(func_name, DIR_NAME_PATH_LENGTH_MAX, d_fname);
					pc_value = 0;
				} else if (aop_config_demangle) {
					aop_sym_demangle(func_name, 
						d_fname, DIR_NAME_PATH_LENGTH_MAX);
				}
			}

			printk("%08x%s %8u   %3u%% %-20s %.32s\n",
					plist->addr, 
					AOP_KERNEL_IDENTIFICATION (plist->in_kernel), 
					plist->samples, perc, img_name, 
					(aop_config_demangle)?d_fname :func_name );

			total_samples += plist->samples;
		}
	}
	printk ("Total Samples %d\n", total_samples);

	vfree (func_name);
	vfree (d_fname);

	return 0;
}

