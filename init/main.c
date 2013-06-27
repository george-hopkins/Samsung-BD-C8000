/*
 *  linux/init/main.c
 *
 *  Copyright (C) 1991, 1992  Linus Torvalds
 *
 *  GK 2/5/95  -  Changed to support mounting root fs via NFS
 *  Added initrd & change_root: Werner Almesberger & Hans Lermen, Feb '96
 *  Moan early if gcc is old, avoiding bogus kernels - Paul Gortmaker, May '96
 *  Simplified starting of init:  Michael A. Griffith <grif@acm.org> 
 */

#include <linux/types.h>
#include <linux/module.h>
#include <linux/proc_fs.h>
#include <linux/kernel.h>
#include <linux/syscalls.h>
#include <linux/string.h>
#include <linux/ctype.h>
#include <linux/delay.h>
#include <linux/utsname.h>
#include <linux/ioport.h>
#include <linux/init.h>
#include <linux/smp_lock.h>
#include <linux/initrd.h>
#include <linux/hdreg.h>
#include <linux/bootmem.h>
#include <linux/tty.h>
#include <linux/gfp.h>
#include <linux/percpu.h>
#include <linux/kmod.h>
#include <linux/kernel_stat.h>
#include <linux/start_kernel.h>
#include <linux/security.h>
#include <linux/workqueue.h>
#include <linux/profile.h>
#include <linux/rcupdate.h>
#include <linux/posix-timers.h>
#include <linux/moduleparam.h>
#include <linux/kallsyms.h>
#include <linux/writeback.h>
#include <linux/cpu.h>
#include <linux/cpuset.h>
#include <linux/cgroup.h>
#include <linux/efi.h>
#include <linux/tick.h>
#include <linux/interrupt.h>
#include <linux/taskstats_kern.h>
#include <linux/delayacct.h>
#include <linux/unistd.h>
#include <linux/rmap.h>
#include <linux/irq.h>
#include <linux/mempolicy.h>
#include <linux/key.h>
#include <linux/kft.h>
#include <linux/unwind.h>
#include <linux/buffer_head.h>
#include <linux/debug_locks.h>
#include <linux/lockdep.h>
#include <linux/pid_namespace.h>
#include <linux/device.h>
#include <linux/kthread.h>
#include <linux/sched.h>
#include <linux/immediate.h>

#include <asm/io.h>
#include <asm/bugs.h>
#include <asm/setup.h>
#include <asm/sections.h>
#include <asm/cacheflush.h>

/* VDLinux, based SELP.4.3.1.x default patch No.9, 
   SELP version print, SP Team 2009-05-08 */
#include <linux/lspinfo.h> /* to display SELP LSP info */

#ifdef CONFIG_EXECUTE_AUTHULD
#include <linux/Se.h>
#include <linux/Secureboot.h>
#include <linux/aes-cmac.h>

#define AUTHULD_KERNEL_OFFSET	(0x5F000)

#define CIP_CRIT_PRINT(fmt, ...)		printk(KERN_CRIT "[CIP_KERNEL] " fmt,##__VA_ARGS__)
#define CIP_WARN_PRINT(fmt, ...)		printk(KERN_CRIT "[CIP_KERNEL] " fmt,##__VA_ARGS__)
#define CIP_DEBUG_PRINT(fmt, ...)		printk(KERN_CRIT "[CIP_KERNEL] " fmt,##__VA_ARGS__)


#endif


#ifdef CONFIG_X86_LOCAL_APIC
#include <asm/smp.h>
#endif

/*
 * This is one of the first .c files built. Error out early if we have compiler
 * trouble.
 */

#if __GNUC__ == 4 && __GNUC_MINOR__ == 1 && __GNUC_PATCHLEVEL__ == 0
#warning gcc-4.1.0 is known to miscompile the kernel.  A different compiler version is recommended.
#endif

static int kernel_init(void *);
#ifdef CONFIG_EXECUTE_AUTHULD
extern int MicomCtrl(unsigned char ctrl);
extern int secure_dec(UInt32 u32SrcAddr, UInt32 u32InDataLength, UInt32 u32DstAddr, UInt32 *pu32OutDataLength);
extern int secure_enc(UInt32 u32SrcAddr, UInt32 u32InDataLength, UInt32 u32DstAddr, UInt32 *pu32OutDataLength);
#endif
extern void init_IRQ(void);
extern void fork_init(unsigned long);
extern void mca_init(void);
extern void sbus_init(void);
extern void signals_init(void);
extern void pidhash_init(void);
extern void pidmap_init(void);
extern void prio_tree_init(void);
extern void radix_tree_init(void);
extern void free_initmem(void);
#ifdef	CONFIG_ACPI
extern void acpi_early_init(void);
#else
static inline void acpi_early_init(void) { }
#endif
#ifndef CONFIG_DEBUG_RODATA
static inline void mark_rodata_ro(void) { }
#endif
#ifdef CONFIG_ALLOC_RTSJ_MEM
extern void alloc_rtsj_mem_early_setup(void);
#else
static inline void alloc_rtsj_mem_early_setup(void) { }
#endif


void to_userspace(void);

#ifdef CONFIG_TC
extern void tc_init(void);
#endif
#ifdef CONFIG_IMMEDIATE
extern void imv_init_complete(void);
#else
static inline void imv_init_complete(void) { }
#endif

enum system_states system_state;
EXPORT_SYMBOL(system_state);

const char samsung_version[] =
    " * Samsung SELP.4.3.2.x BD Barcelona version : 0.1.0.5-17 (Baseline : DTP-SP-BARLIN-0007) \n ";
/*
 * Boot command-line arguments
 */
#define MAX_INIT_ARGS CONFIG_INIT_ENV_ARG_LIMIT
#define MAX_INIT_ENVS CONFIG_INIT_ENV_ARG_LIMIT

#ifdef CONFIG_EXECUTE_AUTHULD
static char * auth_argv_init[MAX_INIT_ARGS+2] = { "/bin/authuld", NULL, };
#endif

extern void time_init(void);
/* Default late time init is NULL. archs can override this later. */
void (*late_time_init)(void);
extern void softirq_init(void);

/* Untouched command line saved by arch-specific code. */
char __initdata boot_command_line[COMMAND_LINE_SIZE];
/* Untouched saved command line (eg. for /proc) */
char *saved_command_line;
/* Command line for parameter parsing */
static char *static_command_line;

static char *execute_command;
static char *ramdisk_execute_command;

#ifdef CONFIG_SMP
/* Setup configured maximum number of CPUs to activate */
static unsigned int __initdata max_cpus = NR_CPUS;

/*
 * Setup routine for controlling SMP activation
 *
 * Command-line option of "nosmp" or "maxcpus=0" will disable SMP
 * activation entirely (the MPS table probe still happens, though).
 *
 * Command-line option of "maxcpus=<NUM>", where <NUM> is an integer
 * greater than 0, limits the maximum number of CPUs activated in
 * SMP mode to <NUM>.
 */
#ifndef CONFIG_X86_IO_APIC
static inline void disable_ioapic_setup(void) {};
#endif

static int __init nosmp(char *str)
{
	max_cpus = 0;
	disable_ioapic_setup();
	return 0;
}

early_param("nosmp", nosmp);

static int __init maxcpus(char *str)
{
	get_option(&str, &max_cpus);
	if (max_cpus == 0)
		disable_ioapic_setup();

	return 0;
}

early_param("maxcpus", maxcpus);
#else
#define max_cpus NR_CPUS
#endif

/*
 * If set, this is an indication to the drivers that reset the underlying
 * device before going ahead with the initialization otherwise driver might
 * rely on the BIOS and skip the reset operation.
 *
 * This is useful if kernel is booting in an unreliable environment.
 * For ex. kdump situaiton where previous kernel has crashed, BIOS has been
 * skipped and devices will be in unknown state.
 */
unsigned int reset_devices;
EXPORT_SYMBOL(reset_devices);

static int __init set_reset_devices(char *str)
{
	reset_devices = 1;
	return 1;
}

__setup("reset_devices", set_reset_devices);

static char * argv_init[MAX_INIT_ARGS+2] = { "init", NULL, };
char * envp_init[MAX_INIT_ENVS+2] = { "HOME=/", "TERM=linux", NULL, };
static const char *panic_later, *panic_param;

extern struct obs_kernel_param __setup_start[], __setup_end[];

static int __init obsolete_checksetup(char *line)
{
	struct obs_kernel_param *p;
	int had_early_param = 0;

	p = __setup_start;
	do {
		int n = strlen(p->str);
		if (!strncmp(line, p->str, n)) {
			if (p->early) {
				/* Already done in parse_early_param?
				 * (Needs exact match on param part).
				 * Keep iterating, as we can have early
				 * params and __setups of same names 8( */
				if (line[n] == '\0' || line[n] == '=')
					had_early_param = 1;
			} else if (!p->setup_func) {
				printk(KERN_WARNING "Parameter %s is obsolete,"
				       " ignored\n", p->str);
				return 1;
			} else if (p->setup_func(line + n))
				return 1;
		}
		p++;
	} while (p < __setup_end);

	return had_early_param;
}

/*
 * This should be approx 2 Bo*oMips to start (note initial shift), and will
 * still work even if initially too large, it will just take slightly longer
 */
unsigned long loops_per_jiffy = (1<<12);

EXPORT_SYMBOL(loops_per_jiffy);

static int __init debug_kernel(char *str)
{
	if (*str)
		return 0;
	console_loglevel = 10;
	return 1;
}

static int __init quiet_kernel(char *str)
{
	if (*str)
		return 0;
	console_loglevel = 4;
	return 1;
}

__setup("debug", debug_kernel);
__setup("quiet", quiet_kernel);

static int __init loglevel(char *str)
{
	get_option(&str, &console_loglevel);
	return 1;
}

__setup("loglevel=", loglevel);

/*
 * Unknown boot options get handed to init, unless they look like
 * failed parameters
 */
static int __init unknown_bootoption(char *param, char *val)
{
	/* Change NUL term back to "=", to make "param" the whole string. */
	if (val) {
		/* param=val or param="val"? */
		if (val == param+strlen(param)+1)
			val[-1] = '=';
		else if (val == param+strlen(param)+2) {
			val[-2] = '=';
			memmove(val-1, val, strlen(val)+1);
			val--;
		} else
			BUG();
	}

	/* Handle obsolete-style parameters */
	if (obsolete_checksetup(param))
		return 0;

	/*
	 * Preemptive maintenance for "why didn't my misspelled command
	 * line work?"
	 */
	if (strchr(param, '.') && (!val || strchr(param, '.') < val)) {
		printk(KERN_ERR "Unknown boot option `%s': ignoring\n", param);
		return 0;
	}

	if (panic_later)
		return 0;

	if (val) {
		/* Environment option */
		unsigned int i;
		for (i = 0; envp_init[i]; i++) {
			if (i == MAX_INIT_ENVS) {
				panic_later = "Too many boot env vars at `%s'";
				panic_param = param;
			}
			if (!strncmp(param, envp_init[i], val - param))
				break;
		}
		envp_init[i] = param;
	} else {
		/* Command line option */
		unsigned int i;
		for (i = 0; argv_init[i]; i++) {
			if (i == MAX_INIT_ARGS) {
				panic_later = "Too many boot init vars at `%s'";
				panic_param = param;
			}
		}
		argv_init[i] = param;
	}
	return 0;
}

#ifdef CONFIG_KFT_STATIC_RUN
void to_userspace(void)
{
}
#endif /* CONFIG_KFT_STATIC_RUN */

static int __init init_setup(char *str)
{
	unsigned int i;

	execute_command = str;
	/*
	 * In case LILO is going to boot us with default command line,
	 * it prepends "auto" before the whole cmdline which makes
	 * the shell think it should execute a script with such name.
	 * So we ignore all arguments entered _before_ init=... [MJ]
	 */
	for (i = 1; i < MAX_INIT_ARGS; i++)
		argv_init[i] = NULL;
	return 1;
}
__setup("init=", init_setup);

static int __init rdinit_setup(char *str)
{
	unsigned int i;

	ramdisk_execute_command = str;
	/* See "auto" comment in init_setup */
	for (i = 1; i < MAX_INIT_ARGS; i++)
		argv_init[i] = NULL;
	return 1;
}
__setup("rdinit=", rdinit_setup);

#ifndef CONFIG_SMP

#ifdef CONFIG_X86_LOCAL_APIC
static void __init smp_init(void)
{
	APIC_init_uniprocessor();
}
#else
#define smp_init()	do { } while (0)
#endif

static inline void setup_per_cpu_areas(void) { }
static inline void smp_prepare_cpus(unsigned int maxcpus) { }

#else

#ifdef __GENERIC_PER_CPU
unsigned long __per_cpu_offset[NR_CPUS] __read_mostly;

EXPORT_SYMBOL(__per_cpu_offset);

static void __init setup_per_cpu_areas(void)
{
	unsigned long size, i;
	char *ptr;
	unsigned long nr_possible_cpus = num_possible_cpus();

	/* Copy section for each CPU (we discard the original) */
	size = ALIGN(PERCPU_ENOUGH_ROOM, PAGE_SIZE);
	ptr = alloc_bootmem_pages(size * nr_possible_cpus);

	for_each_possible_cpu(i) {
		__per_cpu_offset[i] = ptr - __per_cpu_start;
		memcpy(ptr, __per_cpu_start, __per_cpu_end - __per_cpu_start);
		ptr += size;
	}
}
#endif /* !__GENERIC_PER_CPU */

/* Called by boot processor to activate the rest. */
static void __init smp_init(void)
{
	unsigned int cpu;

	/* FIXME: This should be done in userspace --RR */
	for_each_present_cpu(cpu) {
		if (num_online_cpus() >= max_cpus)
			break;
		if (!cpu_online(cpu))
			cpu_up(cpu);
	}

	/* Any cleanup work */
	printk(KERN_INFO "Brought up %ld CPUs\n", (long)num_online_cpus());
	smp_cpus_done(max_cpus);
}

#endif

/*
 * We need to store the untouched command line for future reference.
 * We also need to store the touched command line since the parameter
 * parsing is performed in place, and we should allow a component to
 * store reference of name/value for future reference.
 */
static void __init setup_command_line(char *command_line)
{
	saved_command_line = alloc_bootmem(strlen (boot_command_line)+1);
	static_command_line = alloc_bootmem(strlen (command_line)+1);
	strcpy (saved_command_line, boot_command_line);
	strcpy (static_command_line, command_line);
}

/*
 * We need to finalize in a non-__init function or else race conditions
 * between the root thread and the init thread may cause start_kernel to
 * be reaped by free_initmem before the root thread has proceeded to
 * cpu_idle.
 *
 * gcc-3.4 accidentally inlines this function, so use noinline.
 */

static void noinline __init_refok rest_init(void)
	__releases(kernel_lock)
{
	int pid;

	system_state = SYSTEM_BOOTING_SCHEDULER_OK;

	kernel_thread(kernel_init, NULL, CLONE_FS | CLONE_SIGHAND);
	numa_default_policy();
	pid = kernel_thread(kthreadd, NULL, CLONE_FS | CLONE_FILES);
	kthreadd_task = find_task_by_pid(pid);
	unlock_kernel();

	/*
	 * The boot idle thread must execute schedule()
	 * at least once to get things moving:
	 */
	init_idle_bootup_task(current);
	__preempt_enable_no_resched();
	schedule();
	preempt_disable();

	/* Call into cpu_idle with preempt disabled */
	cpu_idle();
}

/* Check for early params. */
static int __init do_early_param(char *param, char *val)
{
	struct obs_kernel_param *p;

	for (p = __setup_start; p < __setup_end; p++) {
		if ((p->early && strcmp(param, p->str) == 0) ||
		    (strcmp(param, "console") == 0 &&
		     strcmp(p->str, "earlycon") == 0)
		) {
			if (p->setup_func(val) != 0)
				printk(KERN_WARNING
				       "Malformed early option '%s'\n", param);
		}
	}
	/* We accept everything at this stage. */
	return 0;
}

/* Arch code calls this early on, or if not, just before other parsing. */
void __init parse_early_param(void)
{
	static __initdata int done = 0;
	static __initdata char tmp_cmdline[COMMAND_LINE_SIZE];

	if (done)
		return;

	/* All fall through to do_early_param. */
	strlcpy(tmp_cmdline, boot_command_line, COMMAND_LINE_SIZE);
	parse_args("early options", tmp_cmdline, NULL, 0, do_early_param);
	done = 1;
}

/*
 *	Activate the first processor.
 */

static void __init boot_cpu_init(void)
{
	int cpu = smp_processor_id();
	/* Mark the boot cpu "present", "online" etc for SMP and UP case */
	cpu_set(cpu, cpu_online_map);
	cpu_set(cpu, cpu_present_map);
	cpu_set(cpu, cpu_possible_map);
}

void __init __attribute__((weak)) smp_setup_processor_id(void)
{
}

asmlinkage void __init start_kernel(void)
{
	char * command_line;
	extern struct kernel_param __start___param[], __stop___param[];

	smp_setup_processor_id();

	/*
	 * Need to run as early as possible, to initialize the
	 * lockdep hash:
	 */
	unwind_init();
	lockdep_init();
	cgroup_init_early();
	core_imv_update();

	local_irq_disable();
	early_boot_irqs_off();
	early_init_irq_lock_class();

/*
 * Interrupts are still disabled. Do necessary setups, then
 * enable them
 */
	setup_early_kft_clock();
	lock_kernel();
	tick_init();
	boot_cpu_init();
	page_address_init();
	printk(KERN_ERR);
	printk(linux_banner);
/** SELP.4.3.2.x Barcelona kernel version */
	printk(samsung_version);
	setup_arch(&command_line);
	setup_command_line(command_line);
	unwind_setup();
	setup_per_cpu_areas();
	smp_prepare_boot_cpu();	/* arch-specific boot-cpu hooks */

	/*
	 * Set up the scheduler prior starting any interrupts (such as the
	 * timer interrupt). Full topology setup happens at smp_init()
	 * time - but meanwhile we still have a functioning scheduler.
	 */
	sched_init();
	/*
	 * Disable preemption - early bootup scheduling is extremely
	 * fragile until we cpu_idle() for the first time.
	 */
	preempt_disable();

	build_all_zonelists();
	page_alloc_init();
	early_init_hardirqs();
	printk(KERN_NOTICE "Kernel command line: %s\n", boot_command_line);
	parse_early_param();
	parse_args("Booting kernel", static_command_line, __start___param,
		   __stop___param - __start___param,
		   &unknown_bootoption);
	if (!irqs_disabled()) {
		printk(KERN_WARNING "start_kernel(): bug: interrupts were "
				"enabled *very* early, fixing it\n");
		local_irq_disable();
	}
	sort_main_extable();
	trap_init();
	rcu_init();
	init_IRQ();
	pidhash_init();
	init_timers();
	hrtimers_init();
	softirq_init();
	timekeeping_init();
	time_init();
	profile_init();
	if (!irqs_disabled())
		printk("start_kernel(): bug: interrupts were enabled early\n");
	early_boot_irqs_on();
	local_irq_enable();

	/*
	 * HACK ALERT! This is early. We're enabling the console before
	 * we've done PCI setups etc, and console_init() must be aware of
	 * this. But we do want output early, in case something goes wrong.
	 */
	console_init();
	if (panic_later)
		panic(panic_later, panic_param);

	init_tracer();

	lockdep_info();

	/*
	 * Need to run this when irqs are enabled, because it wants
	 * to self-test [hard/soft]-irqs on/off lock inversion bugs
	 * too:
	 */
	locking_selftest();

#ifdef CONFIG_BLK_DEV_INITRD
	if (initrd_start && !initrd_below_start_ok &&
			initrd_start < min_low_pfn << PAGE_SHIFT) {
		printk(KERN_CRIT "initrd overwritten (0x%08lx < 0x%08lx) - "
		    "disabling it.\n",initrd_start,min_low_pfn << PAGE_SHIFT);
		initrd_start = 0;
	}
#endif
	vfs_caches_init_early();
	cpuset_init_early();
	alloc_rtsj_mem_early_setup();
	mem_init();
	kmem_cache_init();
	setup_per_cpu_pageset();
	numa_policy_init();
	if (late_time_init)
		late_time_init();
	calibrate_delay();
	pidmap_init();
	pgtable_cache_init();
	prio_tree_init();
	anon_vma_init();
#ifdef CONFIG_X86
	if (efi_enabled)
		efi_enter_virtual_mode();
#endif
	fork_init(num_physpages);
	proc_caches_init();
	buffer_init();
	unnamed_dev_init();
	key_init();
	security_init();
	vfs_caches_init(num_physpages);
	radix_tree_init();
	signals_init();
	/* rootfs populating might need page-writeback */
	page_writeback_init();
#ifdef CONFIG_PROC_FS
	proc_root_init();
#endif
	cgroup_init();
	cpuset_init();
	taskstats_init_early();
	delayacct_init();
	imv_init_complete();

	check_bugs();

	acpi_early_init(); /* before LAPIC and SMP init */

#ifdef CONFIG_PREEMPT_RT
	WARN_ON(irqs_disabled());
#endif
	/* Do the rest non-__init'ed, we're now alive */
	rest_init();
}

static int __initdata initcall_debug;

static int __init initcall_debug_setup(char *str)
{
	initcall_debug = 1;
	return 1;
}
__setup("initcall_debug", initcall_debug_setup);

extern initcall_t __initcall_start[], __initcall_end[];

static void __init do_initcalls(void)
{
	initcall_t *call;
	int count = preempt_count();

	for (call = __initcall_start; call < __initcall_end; call++) {
		ktime_t t0, t1, delta;
		char *msg = NULL;
		char msgbuf[40];
		int result;

		if (initcall_debug) {
			printk("Calling initcall 0x%p", *call);
			print_fn_descriptor_symbol(": %s()",
					(unsigned long) *call);
			printk("\n");
			t0 = ktime_get();
		}

		result = (*call)();

		if (initcall_debug) {
			t1 = ktime_get();
			delta = ktime_sub(t1, t0);

			printk("initcall 0x%p", *call);
			print_fn_descriptor_symbol(": %s()",
					(unsigned long) *call);
			printk(" returned %d.\n", result);

			printk("initcall 0x%p ran for %Ld msecs: ",
				*call, (unsigned long long)delta.tv64 >> 20);
			print_fn_descriptor_symbol("%s()\n",
				(unsigned long) *call);
		}

		if (result && result != -ENODEV && initcall_debug) {
			sprintf(msgbuf, "error code %d", result);
			msg = msgbuf;
		}
		if (preempt_count() != count) {
			msg = "preemption imbalance";
			preempt_count() = count;
		}
		if (irqs_disabled()) {
			msg = "disabled interrupts";
			local_irq_enable();
		}
		if (msg) {
			printk(KERN_WARNING "initcall at 0x%p", *call);
			print_fn_descriptor_symbol(": %s()",
					(unsigned long) *call);
			printk(": returned with %s\n", msg);
		}
	}

	/* Make sure there is no pending stuff from the initcall sequence */
	flush_scheduled_work();
}

/*
 * Ok, the machine is now initialized. None of the devices
 * have been touched yet, but the CPU subsystem is up and
 * running, and memory and process management works.
 *
 * Now we can finally start doing some real work..
 */
static void __init do_basic_setup(void)
{
	/* drivers will send hotplug events */
	init_workqueues();
	usermodehelper_init();
	driver_init();
	init_irq_proc();
	do_initcalls();
}

static int __initdata nosoftlockup;

static int __init nosoftlockup_setup(char *str)
{
	nosoftlockup = 1;
	return 1;
}
__setup("nosoftlockup", nosoftlockup_setup);

static void __init do_pre_smp_initcalls(void)
{
	extern int spawn_ksoftirqd(void);
	extern int spawn_desched_task(void);

	migration_init();
	posix_cpu_thread_init();
	spawn_ksoftirqd();
	if (!nosoftlockup)
		spawn_softlockup_task();
	spawn_desched_task();
}

static void run_init_process(char *init_filename)
{
	argv_init[0] = init_filename;
	kernel_execve(init_filename, argv_init, envp_init);
}

#ifdef CONFIG_EXECUTE_AUTHULD
static int wait_authuld(void) 
{
	int i;
	unsigned int ui32AuthCode = 0xABCDEF00;
	unsigned int ui32GetCode = 0;
	void __iomem *p;

	p = ioremap(MACH_MEM0_BASE+ SYS_MEM0_SIZE, 0x100000);
	/* total 2 min waiting.*/
	for(i=0; i<(CONFIG_TIMEOUT_ACK_AUTHULD*3); i++) 
	{
		memcpy(&ui32GetCode, (void*)(p+AUTHULD_KERNEL_OFFSET), sizeof(ui32GetCode));
		CIP_CRIT_PRINT("AUTHULD ADDRESS<0x%x>\n", (p+AUTHULD_KERNEL_OFFSET));
		if(ui32GetCode > ui32AuthCode) // auth fail
		{
			CIP_CRIT_PRINT("authentication fail!!<0x%x>\n", ui32GetCode);
			iounmap(p);
			return 0;
		}
		else if(ui32GetCode == ui32AuthCode) // auth success
		{
			CIP_DEBUG_PRINT("authentication success!!\n");
			iounmap(p);
			return 1;
		}
		else
		{
			CIP_DEBUG_PRINT("(%d)th waiting. <0x%x>\n", i, ui32GetCode);
			ssleep(20);
		}
	}

	memcpy(&ui32GetCode, (void*)(p+AUTHULD_KERNEL_OFFSET), sizeof(ui32GetCode));
	if(ui32GetCode == ui32AuthCode) // auth success
	{
		CIP_DEBUG_PRINT("authentication success!!\n");
		iounmap(p);
		return 1;
	}

	CIP_CRIT_PRINT("authentication fail!!<0x%x>\n", ui32GetCode);
	iounmap(p);
	return 0;
}

void Exception_from_authuld(const unsigned char *msg)
{
	int i;
	CIP_CRIT_PRINT("%s", msg);
	for(i=0;i<3;i++)
	{
		CIP_CRIT_PRINT("[%s::%s::%d]auth failed in kernel. System Down.\n", __FILE__, __FUNCTION__, __LINE__);
	}
#ifdef CONFIG_SHUTDOWN
	MicomCtrl(115);
	ssleep(3);
	MicomCtrl(18);
	panic("[kernel] System down\n");
#endif
}

static int check_ci_app_integrity_with_size(unsigned char *key, char *filename, int input_size, unsigned char *mac) 
{
	int fd =-1;
	mm_segment_t old_fs;
	unsigned char result[16];
	struct stat64 statbuf;
	
	sys_stat64(filename, &statbuf);
	if(input_size != statbuf.st_size)
	{
		CIP_CRIT_PRINT("%s size is wrong (pre:%d, real:%d)\n", filename, input_size, (int)statbuf.st_size);
		return 1;
	}
	
	/* key copy routine should be required here */
	old_fs = get_fs();
	set_fs(KERNEL_DS);
	fd = sys_open(filename, O_RDONLY, 0);
	if (fd >= 0) 
	{
		AES_CMAC_with_fd_and_size(key, fd, input_size, result);
	  	sys_close(fd);
	} 
	else 
	{
		CIP_WARN_PRINT("Warning: unable to open %s.\n", filename);
	}
	
	set_fs(old_fs);	
#ifdef DEBUG_SE
	int i;
	for(i=0;i<16;i++)
	{
		CIP_DEBUG_PRINT("%d :(mac) %x ,(result) %x\n",i,mac[i],result[i]);		
	}
#endif
	/* If no error occurs, return 0  */
	if(memcmp(result, mac, 16) == 0) 
		return 0;

	return 1;
}

static void execute_authuld(void) 
{
	current->flags |= PF_NOFREEZE;
	
	auth_argv_init[0] = CONFIG_AUTHULD_PATH;
#ifdef CONFIG_SHUTDOWN
	auth_argv_init[1] = "0";
#else
	auth_argv_init[1] = "1";
#endif
	kernel_execve(CONFIG_AUTHULD_PATH, auth_argv_init, envp_init);

	do_exit(0);
}

void print128(unsigned char *bytes)
{
    int j;

    for (j=0; j<16;j++) {
        CIP_DEBUG_PRINT("%02x",bytes[j]);
        if ( (j%4) == 3 ) CIP_DEBUG_PRINT(" ");
    }
    CIP_DEBUG_PRINT("\n\n");
}

#ifdef CONFIG_SECURE_SERIAL_INPUT
unsigned int is_authuld_passed = 0;
#else
unsigned int is_authuld_passed = 1;
#endif

static int check_ci_app(void) 
{
	UInt32 outLength;
	unsigned char mkey[16];
	char strMackeyPartition[20];

	macAuthULd_t macAuthUld;
	cmacKey_t mackey;

	int fd;
	int i = 0;
	int fileOpenFlag = 0;
	unsigned int ui32AuthInitCode = 0x0;
	void __iomem *p;

	current->flags |= PF_NOFREEZE;
	
	switch (getPartSize())
	{
		case 128:
			CIP_DEBUG_PRINT("kernel flash type : 128 MB\n");
			strncpy(strMackeyPartition, CMACKEY_PARTITION_128,19);
			break;
		case 1000:
			CIP_DEBUG_PRINT("kernel flash type : 1000 MB\n");
			strncpy(strMackeyPartition, CMACKEY_PARTITION_1000,19);
			break;
		default:
			CIP_DEBUG_PRINT("kernel flash type : unknown. default = 1000 MB\n");
			strncpy(strMackeyPartition, CMACKEY_PARTITION_1000,19);
			break;
	}

	/* open할 수 있을 때 까지 polling한다 */
	for(i=0;i<1000;i++) 
	{
		if( (fd=sys_open(strMackeyPartition, O_RDONLY, 0))>= 0 ) 
		{
			CIP_DEBUG_PRINT("%s can read  (after=%d)\n",strMackeyPartition,i);
			sys_close(fd);
			fileOpenFlag = 1;
			break;
		}
		msleep(10);
	}
	if( fileOpenFlag == 0 ) 
	{
		Exception_from_authuld("Unable to open mackey partition\n");
	}
    getcmackey(strMackeyPartition, &mackey);
	getAuthUld(&macAuthUld);
	secure_dec((UInt32)mackey.key, 16, (UInt32)mkey, &outLength);
#ifdef DEBUG_SE	
	for(i=0;i<16;i++)
	{
		CIP_DEBUG_PRINT("mackey.key= %x mkey=%x\n",mackey.key[i],mkey[i]);
	}
#endif
	/* open할 수 있을 때 까지 polling한다 */
	/******************** START ************************/
	fileOpenFlag = 0;
	for(i=0;i<10000;i++) 
	{
		if((fd=sys_open(CONFIG_AUTHULD_PATH, O_RDONLY, 0 ) )>= 0) 
		{
			CIP_DEBUG_PRINT("%s can read  (after=%d)\n",CONFIG_AUTHULD_PATH, i);
			sys_close(fd);
			fileOpenFlag = 1;
			break;
		}
		msleep(10);
	}
	if( fileOpenFlag == 0 ) 
	{
		Exception_from_authuld("Unable to open authuld\n");
	}
	/******************** END ************************/
	

	p = ioremap(MACH_MEM0_BASE+SYS_MEM0_SIZE, 0x100000);
	// get key
	if(check_ci_app_integrity_with_size(mkey, CONFIG_AUTHULD_PATH, macAuthUld.macAuthULD.msgLen, macAuthUld.macAuthULD.au8PreCmacResult) == 0) 
	{
		/* 정상적인 경우임 */
		CIP_CRIT_PRINT(">>> (%s) file is successfully authenticated <<< \n", CONFIG_AUTHULD_PATH);

		ui32AuthInitCode = 0x0;
		memcpy((void*)(p+AUTHULD_KERNEL_OFFSET), &ui32AuthInitCode, sizeof(ui32AuthInitCode));
		iounmap(p);
		kernel_thread((void*)execute_authuld,NULL,0);
		if(wait_authuld() < 1) 
		{
			CIP_DEBUG_PRINT("There is an error in authuld or authuld did not respond in %d minutes!!\n", CONFIG_TIMEOUT_ACK_AUTHULD);
			/* error 처리 */
			Exception_from_authuld("timeout\n");
		} 
		else 
		{
			CIP_CRIT_PRINT("Success!! Authuld is successfully completed.\n");
			is_authuld_passed = 1;
		}
	}
	else 
	{
		// authuld 인증 실패시에는 항상 fastboot 되도록 성공 메세지를 write 함.
		ui32AuthInitCode = 0xABCDEF00;
		memcpy((void*)(p+AUTHULD_KERNEL_OFFSET), &ui32AuthInitCode, sizeof(ui32AuthInitCode));
		iounmap(p);
		/* 문제가 되는 경우임 */
		CIP_DEBUG_PRINT(">>> (%s) file is illegally modified!! <<< \n", CONFIG_AUTHULD_PATH);
		Exception_from_authuld("authuld authentication failed\n");
	}

	do_exit(0);
}
#endif


/* This is a non __init function. Force it to be noinline otherwise gcc
 * makes it inline to init() and it becomes part of init.text section
 */
static int noinline init_post(void)
{
	free_initmem();
	unlock_kernel();
	mark_rodata_ro();
	system_state = SYSTEM_RUNNING;
	numa_default_policy();

	if (sys_open((const char __user *) "/dev/console", O_RDWR, 0) < 0)
		printk(KERN_WARNING "Warning: unable to open an initial console.\n");

	(void) sys_dup(0);
	(void) sys_dup(0);

#ifdef CONFIG_KFT_STATIC_RUN
      /* This is a stub function, for use as a stop trigger */
      to_userspace();
#endif /* CONFIG_KFT_STATIC_RUN */

	if (ramdisk_execute_command) {
		run_init_process(ramdisk_execute_command);
		printk(KERN_WARNING "Failed to execute %s\n",
				ramdisk_execute_command);
	}
#ifdef CONFIG_PREEMPT_RT
	WARN_ON(irqs_disabled());
#endif

	/*
	 * We try each of these until one succeeds.
	 *
	 * The Bourne shell can be used instead of init if we are
	 * trying to recover a really broken machine.
	 */
	if (execute_command) {
		run_init_process(execute_command);
		printk(KERN_WARNING "Failed to execute %s.  Attempting "
					"defaults...\n", execute_command);
	}

#if 1
        printk(KERN_ALERT"================================================================================\n");
        printk(KERN_ALERT" SAMSUNG: %s(%s)\n", LSP_SELP_VERSION, LSP_REVISION);
        printk(KERN_ALERT"         (Detailed Information: /sys/selp/vd/lspinfo/summary)                   \n");
        printk(KERN_ALERT"================================================================================\n");
#endif

#ifdef CONFIG_EXECUTE_AUTHULD
	kernel_thread((void*)check_ci_app, NULL, 0);
#endif

	run_init_process("/sbin/init");
	run_init_process("/etc/init");
	run_init_process("/bin/init");
	run_init_process("/bin/sh");

	panic("No init found.  Try passing init= option to kernel.");
}

static int __init kernel_init(void * unused)
{
	lock_kernel();
	/*
	 * init can run on any cpu.
	 */
	set_cpus_allowed(current, CPU_MASK_ALL);
	/*
	 * Tell the world that we're going to be the grim
	 * reaper of innocent orphaned children.
	 *
	 * We don't want people to have to make incorrect
	 * assumptions about where in the task array this
	 * can be found.
	 */
	init_pid_ns.child_reaper = current;

	__set_special_pids(1, 1);
	cad_pid = task_pid(current);

	smp_prepare_cpus(max_cpus);

	init_hardirqs();

	do_pre_smp_initcalls();

	smp_init();
	sched_init_smp();

	cpuset_init_smp();

	do_basic_setup();

	/*
	 * check if there is an early userspace init.  If yes, let it do all
	 * the work
	 */

	if (!ramdisk_execute_command)
		ramdisk_execute_command = "/init";

	if (sys_access((const char __user *) ramdisk_execute_command, 0) != 0) {
		ramdisk_execute_command = NULL;
		prepare_namespace();
	}
#ifdef CONFIG_PREEMPT_RT
	WARN_ON(irqs_disabled());
#endif

#define DEBUG_COUNT (defined(CONFIG_DEBUG_RT_MUTEXES) + defined(CONFIG_CRITICAL_PREEMPT_TIMING) + defined(CONFIG_CRITICAL_IRQSOFF_TIMING) + defined(CONFIG_FUNCTION_TRACE) + defined(CONFIG_DEBUG_SLAB) + defined(CONFIG_DEBUG_PAGEALLOC) + defined(CONFIG_LOCKDEP))

#if DEBUG_COUNT > 0
	printk(KERN_ERR "*****************************************************************************\n");
	printk(KERN_ERR "*                                                                           *\n");
#if DEBUG_COUNT == 1
	printk(KERN_ERR "*  REMINDER, the following debugging option is turned on in your .config:   *\n");
#else
	printk(KERN_ERR "*  REMINDER, the following debugging options are turned on in your .config: *\n");
#endif
	printk(KERN_ERR "*                                                                           *\n");
#ifdef CONFIG_DEBUG_RT_MUTEXES
	printk(KERN_ERR "*        CONFIG_DEBUG_RT_MUTEXES                                            *\n");
#endif
#ifdef CONFIG_CRITICAL_PREEMPT_TIMING
	printk(KERN_ERR "*        CONFIG_CRITICAL_PREEMPT_TIMING                                     *\n");
#endif
#ifdef CONFIG_CRITICAL_IRQSOFF_TIMING
	printk(KERN_ERR "*        CONFIG_CRITICAL_IRQSOFF_TIMING                                     *\n");
#endif
#ifdef CONFIG_FUNCTION_TRACE
	printk(KERN_ERR "*        CONFIG_FUNCTION_TRACE                                              *\n");
#endif
#ifdef CONFIG_DEBUG_SLAB
	printk(KERN_ERR "*        CONFIG_DEBUG_SLAB                                                  *\n");
#endif
#ifdef CONFIG_DEBUG_PAGEALLOC
	printk(KERN_ERR "*        CONFIG_DEBUG_PAGEALLOC                                             *\n");
#endif
#ifdef CONFIG_LOCKDEP
	printk(KERN_ERR "*        CONFIG_LOCKDEP                                                     *\n");
#endif
	printk(KERN_ERR "*                                                                           *\n");
#if DEBUG_COUNT == 1
	printk(KERN_ERR "*  it may increase runtime overhead and latencies.                          *\n");
#else
	printk(KERN_ERR "*  they may increase runtime overhead and latencies.                        *\n");
#endif
	printk(KERN_ERR "*                                                                           *\n");
	printk(KERN_ERR "*****************************************************************************\n");
#endif
	/*
	 * Ok, we have completed the initial bootup, and
	 * we're essentially up and running. Get rid of the
	 * initmem segments and start the user-mode stuff..
	 */
	init_post();
	WARN_ON(debug_direct_keyboard);

	return 0;
}
