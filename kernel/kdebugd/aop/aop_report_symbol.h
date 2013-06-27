/*
 *  linux/drivers/oprofile/aop_report_symbol.h
 *
 *  Report symbol info releated functions declaration are done here
 *
 *  Copyright (C) 2009  Samsung	
 *
 *  2009-09-02  Created by karuna.nithy@samsung.com.
 *
 */

#ifndef _LINUX_AOP_REPORT_SYMBOL_H
#define _LINUX_AOP_REPORT_SYMBOL_H

/* extern the aop_lookup_dcookie function which is defined in dcookies.c file */
extern asmlinkage long aop_lookup_dcookie(
			u64 cookie64, char * buf, size_t len);

/* update symbol sample function which is called from oprofile sample parse function */
extern void aop_sym_report_update_sample_data( op_data *aop_data );

/* free allocated kernel data releated to symbol report */
extern void aop_sym_report_free_sample_data ( void );

/* Dump the application wise function name & library name */
extern int aop_sym_report_per_application ( void );

/* Dump the application wise function name including library name */
extern int aop_sym_report_per_application_n_lib ( void );

/* initialize apo symbol report head list */
extern int aop_sym_report_init ( void );

/*Report the symbol information of the image(application or library) specified
by image_type. */
extern int aop_sym_report_per_image_user_samples ( 
							int image_type, cookie_t img_cookie );

/*Report the symbol information of the thread ID(TID) specified by user.*/
extern int aop_sym_report_per_tid ( pid_t pid_user );

/* Dump system wide function name & samples */
extern int aop_sym_report_system_wide_function_samples ( void );

#endif	/* !_LINUX_AOP_REPORT_SYMBOL_H */

