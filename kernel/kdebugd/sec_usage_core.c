/*
 *  linux/kernel/sec_usage_core.c
 *
 *  Copyright (C) 2009 Samsung Electronics
 *
 *  2009-11-05  Created by Choi Young-Ho (yh2005.choi@samsung.com)
 *
 *  Counter Monitor ����� ���Ͽ� kusage_cored Ŀ�� �����带 ����.
 *  register_counter_monitor_func �Լ��� ��ϵ� �Լ��� 1�ʿ� �ѹ���
 *  ȣ���Ѵ�.
 *
 *  Disk Usage�� ��� hrtimer�� �����ϴ� ��� Mutex wait ������ 
 *  scheduling while atomic ������ �߻��� �� �־� �̷��� Ŀ�� �������
 *  ó���Ѵ�.
 */

#include <linux/poll.h>
#include <linux/kthread.h>
#include <linux/ktime.h>
#include <linux/proc_fs.h>
#include <linux/kdebugd.h>

#define MAX_COUNTER_MONITOR	10

wait_queue_head_t usage_cored_wq;

static counter_monitor_func_t counter_monitor_funcs[MAX_COUNTER_MONITOR] = {NULL};

/*
 * Name: register_counter_monitor_func
 * Desc: Counter Monitor ��� �Լ��� ����Ѵ�
 */
int register_counter_monitor_func(counter_monitor_func_t func)
{
    int i;
    for ( i = 0; i < MAX_COUNTER_MONITOR - 1; ++ i) {
        if (counter_monitor_funcs[i] == NULL) {
            counter_monitor_funcs[i] = func;
            return 0;
        }
    }

    printk("full of counter_monitor_funcs \n");
    return -1;
}

/*
 * Name: usage_cored
 * Desc: ��ϵ� Counter Monitor ��� �Լ��� 1�ʿ� �� ����
 *       ȣ���ϴ� Ŀ�� ������
 */
static int usage_cored(void *p)
{
	int rc = 0;

	daemonize("kusage_cored");

	while (1) {
		counter_monitor_func_t* funcp = &counter_monitor_funcs[0];

		while (*funcp != NULL) {
			rc = (*funcp++)();
		}

		wait_event_interruptible_timeout(usage_cored_wq, 0, 1*HZ); // sleep 1 sec
	}

	BUG();

	return 0;
}

/*
 * Name: kdbg_usage_core_init
 * Desc: kusage_cored Ŀ�� �����带 �����Ų��
 */
static int __init kdbg_usage_core_init(void)
{
	pid_t pid;

	init_waitqueue_head(&usage_cored_wq);

	pid = kernel_thread(usage_cored, NULL, CLONE_KERNEL);
	BUG_ON(pid < 0);
	
	return 0;
}

__initcall(kdbg_usage_core_init);
