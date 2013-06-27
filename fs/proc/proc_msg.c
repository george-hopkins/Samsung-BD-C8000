/*
 *  linux/fs/proc/proc_msg.c
 *
 *  Copyright (C) 2008, 2009 Samsung Electornics
 *
 *  msg handling functions
 */

/*
#include <asm/uaccess.h>
#include <linux/config.h>
#include <linux/errno.h>
#include <linux/time.h>
#include <linux/proc_fs.h>
#include <linux/stat.h>
#include <linux/init.h>
#include <linux/capability.h> 
#include <linux/file.h>
#include <linux/string.h>
#include <linux/seq_file.h>
#include <linux/namei.h>
#include <linux/namespace.h>
#include <linux/mm.h>
#include <linux/smp_lock.h>
#include <linux/rcupdate.h>
#include <linux/kallsyms.h>
#include <linux/mount.h>
#include <linux/security.h>
#include <linux/ptrace.h>
#include <linux/seccomp.h>
#include <linux/cpuset.h>
#include <linux/audit.h> 
#include <linux/poll.h>
 */

#include <linux/proc_fs.h>

extern const struct file_operations proc_dmsg_operations;
extern int opt_nodtvlogd;

/*
 * Create Proc Entry
 */
static int __init init_procfs_msg(void)
{
	struct proc_dir_entry *entry;

	if (opt_nodtvlogd) return 0;    /* enable or disable by kernel argument */

	entry = create_proc_entry("dmsg", S_IRUSR | S_IWUSR, &proc_root);
	if (entry)
		entry->proc_fops = &proc_dmsg_operations;

	return 0;
}

/*
 * Remove Proc Entry
 */

static void __exit cleanup_procfs_msg(void)
{
	if (opt_nodtvlogd) return;    /* enable or disable by kernel argument */

	remove_proc_entry("dmsg", &proc_root);
}

module_init(init_procfs_msg);
module_exit(cleanup_procfs_msg);

