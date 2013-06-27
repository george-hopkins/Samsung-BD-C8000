#Add for KDEBUGD
obj-$(CONFIG_KDEBUGD)   += kdbg-core.o
obj-$(CONFIG_KDEBUGD_MISC)   += kdbg-func.o
obj-$(CONFIG_KDEBUGD_TRACE)   += kdbg-trace.o
obj-$(CONFIG_KDEBUGD_COUNTER_MONITOR) += sec_usage_core.o
obj-$(CONFIG_KDEBUGD_COUNTER_MONITOR) += sec_cpuusage.o
obj-$(CONFIG_KDEBUGD_COUNTER_MONITOR) += sec_topthread.o
obj-$(CONFIG_KDEBUGD_COUNTER_MONITOR) += sec_memusage.o
obj-$(CONFIG_KDEBUGD_COUNTER_MONITOR) += sec_diskusage.o
obj-$(CONFIG_KDEBUGD_COUNTER_MONITOR) += sec_netusage.o

ifeq ($(CONFIG_ADVANCE_OPROFILE),y)
#Add for AOP
obj-y   += aop/
endif

