/*
 *  linux/drivers/oprofile/aop_sym_db.h
 *
 *  Symbol (ELF) releated functions & defined here 
 *
 *  Copyright (C) 2009  Samsung	
 *
 *  2009-10-01 Created by gaurav.jindal@samsung.com.
 *
 */

#ifndef _LINUX_AOP_ELF_PARSER_H
#define _LINUX_AOP_ELF_PARSER_H

#include <linux/list.h>

/* Max no of files to parsed. This includes the current(.) and parent directory(..). */
#define AOP_MAX_FILES 				200

/* Max length of ELF path */
#define AOP_MAX_PATH_LEN 			256

/* Max elf name length */
#define AOP_MAX_ELF_FILE_NAME_LEN	128

/* Max num of USB supported */
#define AOP_MAX_USB 				3

/*
 *   The structure temporary dirent which is used for storing 
 *   the info  which retreive from sys_getdents64 it has been 
 *   already defind in kernel using it by custmized  
 *   The structure contains ELF reated information
 */
struct aop_dir_list
{
	int num_files;
	char path_dir[AOP_MAX_PATH_LEN];
	struct ___dirent64 {
		u64		d_ino;
		s64		d_off;
		unsigned short	d_reclen;
		unsigned char	d_type;
		char		d_name[AOP_MAX_ELF_FILE_NAME_LEN];
	}dirent[AOP_MAX_FILES];	
};

/*
 *   The structure contains no fo files related information found in USB
 */
struct aop_usb_path
{
	char name[AOP_MAX_USB][AOP_MAX_PATH_LEN];
	int num_usb;
};

typedef int (*aop_cmp)(const struct list_head *a, const struct list_head *b);

typedef void (*aop_symbol_load_notification)(int elf_load_flag );

/*
 *   The fucntion prototype for reading the file in the given directory 
 */
int aop_dir_read(struct aop_dir_list *dir_list );

/*
 *   The fucntion prototype for reading the file in the given directory 
 */
int aop_usb_detect(struct aop_usb_path *usb_path);

/* Get the func name  after Search the symbol by 
 recieving filename and addres(program counter)  */
extern int aop_get_symbol(char* pfilename, 
			unsigned int *symbol_addr, char * pfunc_name);

/*  Link list  sorting in ascending order */
extern void aop_list_sort(struct list_head *head, aop_cmp cmp) ;

extern void aop_sym_register_oprofile_elf_load_notification_func ( 
			aop_symbol_load_notification func );

#endif	/* !_LINUX_AOP_ELF_PARSER_H */
