/*
 *  linux/drivers/oprofile/aop_oprofile.h
 *
 *  Advance oprofile related declarations
 *
 *  Copyright (C) 2009  Samsung	
 *
 *  2009-07-06  Created by sk.bansal.
 *
 */

/*size of the unsigned long integer*/
#define ULONG_SIZE	(sizeof(unsigned long))

/*maximum length of the name of the directory.*/
#define DIR_NAME_PATH_LENGTH_MAX 1024

/*function pointer to the various handlers which 
are used to decode the samples in oprofiling*/
typedef void (*aop_handler_t)(void);

/* generic type for holding cookie value */
typedef unsigned long long cookie_t;

/* generic type for holding addresses */
typedef unsigned long vma_t;

/*raw oprofile data is stored*/
struct aop_cache_type {
	unsigned long* buffer; /*pointer to the buffer in which the raw data is collected*/
	int wr_offset; /*write offset at which the raw data value is written*/
	int rd_offset;
};


extern unsigned long fs_buffer_size;

extern int oprofile_start(void);

extern void oprofile_stop(void);

/*total samples = user+kernel*/
extern unsigned long aop_nr_total_samples;

/* external functions used in this file are declared here */
extern void msleep(unsigned int msecs);
extern ssize_t aop_read_event_buffer(unsigned long *buf, 
		unsigned long *size);
extern int aop_event_buffer_open(void *aop_dcookie_user_data);
extern void aop_event_buffer_clear(void);
extern int aop_sym_kdmenu(void);

/* Wake up the waiting process if any. This happens
 * on "echo 0 >/dev/oprofile/enable" so the daemon
 * processes the data remaining in the event buffer.
 */
extern void wake_up_buffer_waiter(void);

/*structure variable to store the pointer to the 
raw data buffer and write and read positions.*/
extern struct aop_cache_type aop_cache;

/*This is called from the kernel space to decode 
the cookie value into the directory PATH*/
extern asmlinkage long 
	aop_sys_lookup_dcookie(u64 cookie64, char *buf, size_t len);

/*TRACING enum*/
enum tracing_type {
	TRACING_OFF,
	TRACING_START,
	TRACING_ON
};

/*This data structure will hold the transient data while 
processing the raw data collected in profiling*/
typedef struct {
	enum tracing_type tracing;
	cookie_t cookie; /*coolie value of library*/
	cookie_t app_cookie;/*coolie value of application*/
	vma_t pc; /*program counter value*/
	unsigned long event; /*event value*/
	int in_kernel; /*in_kernel = 1 if the context is kernel, =0 if it is in user context*/
	unsigned long cpu; /*cpu number*/
	pid_t tid; /*thread ID*/
	pid_t tgid; /*thread group ID*/
} op_data;

/*store the kernel image or kernel module name 
with start and end address*/
struct kernel_image {
	char * name; /*kernel image name*/
	vma_t start; /*start address of the kernel image*/
	vma_t end; /*end address of the kernel image*/
};

/*link list to store the tgid and tid related data which is collected after processing the raw data.
This includes tgid value. 
sample count denotes the number of samples collected for that pid*/
typedef struct aop_pid_list {
	pid_t pid; /*thread ID*/
	pid_t tgid; /*thread group ID*/	
       unsigned int samples_count;
	char *thread_name;   
       struct aop_pid_list *next;
}aop_pid_list;

/*link list to store the application and library related data which is collected 
after processing the raw data.
This includes cookie value which is decoded into the PATH of the application or library. 
sample count denotes the number of samples collected for that application or library*/
typedef struct aop_image_list {
    cookie_t cookie_value; /*cookie value which is decoded 
    						into the PATH of the application.*/
    unsigned int samples_count;
    struct aop_image_list *next;
}aop_image_list;

struct aop_report_all_list {
	struct list_head report_list;
	int is_kernel; /*is_kernel = 1 if this report is for kernel, =0 otherwise */
	union {
		cookie_t cookie_value; /*cookie value which is decoded into img name */
		char *kernel_name;
	}report_type;
    unsigned int samples_count;
};

enum {
	AOP_TYPE_TGID,
	AOP_TYPE_TID,
	AOP_TYPE_APP,
	AOP_TYPE_LIB,
	AOP_TYPE_KERN
};

/* adv oprofile sort options */
enum {
	AOP_SORT_DEFAULT,		/* sort option none */
	AOP_SORT_BY_VMA, 		/* sort by vma address */
	AOP_SORT_BY_SAMPLES		/* sort report by samples */
};

