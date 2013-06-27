/*
 *  linux/drivers/oprofile/aop_report.h
 *
 *  Report generation releated functions declaration are done here
 *
 *  Copyright (C) 2009  Samsung	
 *
 *  2009-09-07  Created by karuna.nithy@samsung.com.
 *
 */

#ifndef _LINUX_AOP_REPORT_H
#define _LINUX_AOP_REPORT_H

/*process all the samples from the buffer*/
extern int aop_process_all_samples(void);

/*Dump the application data*/
extern int aop_op_generate_app_samples(void);

/*Dump the library data*/
extern int aop_op_generate_lib_samples(void);

/*Dump whole data*/
extern int aop_op_generate_all_samples(void);

/*Dump data- TGID wise*/
extern int aop_op_generate_report_tgid(void);

/*Dump data- TID wise*/
extern int aop_op_generate_report_tid(void);

/*free all the resources that are taken by the system 
while processing the tid and tgid data*/
extern void aop_free_tgid_tid_resources(void);

#endif	/* !_LINUX_AOP_REPORT_H */

