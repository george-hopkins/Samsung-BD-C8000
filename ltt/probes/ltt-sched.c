/*
 * LTTng scheduler probe
 *
 * Mathieu Desnoyers, January 2008.
 */

#include <linux/ltt-tracer.h>
#include <linux/module.h>

static __attribute__((no_instrument_function))
void sched_probe(void *probe_data, void *call_data,
		const char *fmt, va_list *args);

static struct ltt_available_probe pdata = {
	.name = "scheduler",
	.format = "prev %p next %p",
	.probe_func = sched_probe,
};

static __attribute__((no_instrument_function))
void sched_probe(void *probe_data, void *call_data,
		const char *fmt, va_list *args)
{
	struct task_struct *prev, *next;

	prev = va_arg(*args, struct task_struct *);
	next = va_arg(*args, struct task_struct *);

	__trace_mark(0, kernel_sched_schedule, call_data,
		"prev_pid %d next_pid %d prev_state %ld",
		prev->pid, next->pid, prev->state);
}

static int __init probe_init(void)
{
	return ltt_probe_register(&pdata);
}
module_init(probe_init);

static void __exit probe_exit(void)
{
	int ret;
	ret = ltt_probe_unregister(&pdata);
	WARN_ON(ret);

}
module_exit(probe_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Mathieu Desnoyers");
MODULE_DESCRIPTION("Linux Trace Toolkit Scheduler Probe");
