/*
 *  linux/fs/proc/dmsg.c
 *
 *  Copyright (C) 2008, 2009 by Samsug Electronics
 *
 */

#include <linux/types.h>
#include <linux/errno.h>
#include <linux/time.h>
#include <linux/kernel.h>
#include <linux/poll.h>
#include <linux/fs.h>

#include <asm/uaccess.h>
#include <asm/io.h>

/*
 * TODO: Add extern variable to wait until log is available.
 *
 * extern wait_queue_head_t log_wait;
 */


/*
 * TODO: Add extern function to access uninitialized buffer
 *
 * extern int do_syslog(int type, char __user *bug, int count);
 */

static int dmsg_open(struct inode * inode, struct file * file)
{
	return 0;
}

static int dmsg_release(struct inode * inode, struct file * file)
{
	return 0;
}

static ssize_t dmsg_read(struct file *file, char __user *buf,
				size_t count, loff_t *ppos)
{
	/*
	 * TODO: Add function to read uninitialized buffer
	 *             *  do_syslog(9, NULL, 0) return number of chars in buffer.
	 *             *  do_syslog(2, buf, count) return chars in buffer.
	 *
	 * if ((file->f_flags & O_NONBLOCK) && !do_syslog(9, NULL, 0))
	 *     return -EAGAIN;
	 * return do_syslog(2, buf, count);
	 */

	return 0;
}

static ssize_t dmsg_write(struct file *file, const char __user *buf,
				size_t count, loff_t *ppos)
{
	char buffer[8], *end;
	int idx;

	if (!count) return count;

	memset(buffer, 0, 8);

	if (count > 7)
		count = 7;

	if (copy_from_user(buffer, buf, count))
		return -EFAULT;

	idx = simple_strtol(buffer, &end, 0);

	printk(KERN_CRIT"[%s] idx = %d\n", __FUNCTION__, idx);  // for debug

	switch(idx) {
		case 1:
			do_dtvlog(10, NULL, 0);
			break;
		case 2:
			do_dtvlog(4, NULL, 0);
			break;
		default:
			printk(KERN_CRIT"\nUsage(DTVLOGD):\n");
			printk(KERN_CRIT"    echo 1 > /proc/dmsg   # show info\n");
			printk(KERN_CRIT"    echo 2 > /proc/dmsg   # flush log to usb\n");
	}


	/* TODO: Add function to control printf buffer */

	if (*end == '\n')
		end++;

	if (end - buffer == 0)
		return -EIO;

	return count;
}

static unsigned int dmsg_poll(struct file *file, poll_table *wait)
{
	/*
	 * TODO: Add functioin to poll uninitialized buffer
	 *
	 * poll_wait(file, &log_wait, wait);
	 * if (do_syslog(9, NULL, 0))
	 *     return POLLIN | POLLRDNORM;
	 */
	return 0;
}


const struct file_operations proc_dmsg_operations = {
	.read       = dmsg_read,
	.write      = dmsg_write,
	.poll       = dmsg_poll,
	.open       = dmsg_open,
	.release    = dmsg_release,
};
