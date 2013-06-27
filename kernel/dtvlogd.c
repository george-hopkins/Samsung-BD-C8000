/*
 * dtvlogd.c
 *
 * Copyright (C) 2008, 2009 Samsung Electronics
 * Created by choi young-ho, kim gun-ho
 *
 * This used to save console message to RAM area.
 *
 * NOTE:
 *
 *
 */

#include <linux/kernel.h>
#include <linux/string.h>
#include <linux/autoconf.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/smp_lock.h>
#include <linux/ctype.h>
#include <linux/delay.h>
#include <linux/wait.h>
#include <linux/file.h>
#include <linux/namei.h>
#include <linux/vfs.h>
#include <linux/fcntl.h>
#include <asm/system.h>
#include <asm/io.h>
#include <asm/uaccess.h>

/* Control Preemption */

#define DTVLOGD_PREEMPT_COUNT_SAVE(cnt) \
	do { 				\
		cnt = preempt_count();	\
		while(preempt_count())	\
			preempt_enable_no_resched(); \
	} while (0)

#define DTVLOGD_PREEMPT_COUNT_RESTORE(cnt) \
	do { 				\
		int i;			\
		for(i=0; i<cnt; i++)	\
			preempt_disable(); \
	} while (0)

/* Control Option */

#define DTVLOGD_VERSION	"2.2.5 (Barcelona)"

int opt_version[3] = { 2, 2, 5 };
int opt_dtvlogd = 0;	/* enable or disable by dtvlogd.conf */
int opt_interval = 5;
int opt_nodtvlogd = 0;	/* enable or disable by kernel argument */

/* Buffer Size Config */
#define CONFIG_DLOG_BUF_SHIFT     17  /* 128KB */

/* Dtvlogd main Log Buffer */
#define __DLOG_BUF_LEN	(1 << CONFIG_DLOG_BUF_SHIFT)
#define DLOG_BUF_MASK	(dlog_buf_len - 1)
#define DLOG_BUF(idx)	dlog_buf[(idx) & DLOG_BUF_MASK]
#define DLOG_INDEX(idx)	((idx) & DLOG_BUF_MASK)

static char __dlog_buf[__DLOG_BUF_LEN];
static char *dlog_buf = __dlog_buf;
static int dlog_buf_len = __DLOG_BUF_LEN;

static int dlogbuf_start, dlogbuf_end;
static int dlogged_chars = 0;
static int dlogbuf_writeok = 0;
static int dlogbuf_writefail = 0;

static DEFINE_SPINLOCK(dlogbuf_lock);
static DECLARE_MUTEX(dlogbuf_sem);
/* removed DECLARE_MUTEX_LOCKED() kernel 2.6.24 
static DECLARE_MUTEX_LOCKED(dlogbuf_complete_sem); */
static DECLARE_MUTEX(dlogbuf_complete_sem);

wait_queue_head_t dlogbuf_wq;
int dlogbuf_complete=0;


/* Uninitialized memory buffer */
/* 
 * Barcelona doesn't support Emergency Log Memory Map
 *
 * +------------------+ 0x0000 0000 (128KB)
 * |  Reserved Area   |
 * |   (63.9KB)       |
 * +------------------+ 0x0000 0010 (64KB + 16Byte)
 * | Mgmt Data (16B)  | 
 * +------------------+ 0x0000 0000 (64KB)
 * | Emeg Log Buffer  |
 * |    (64KB)        |
 * +------------------+ 0x0000 0000 (0KB)
 */

#ifdef CONFIG_DTVLOGD_EMERGENCY_LOG
#define __DRAM_BUF_LEN    64 * 1024       /* Current 64Kb */
#define DRAM_BUF_MASK (__DRAM_BUF_LEN - 1)
#define DRAM_INDEX(idx) 	((idx) & DRAM_BUF_MASK)

#define RAM_BUF_ADDR  0x6FF00000
#define RAM_DATA_ADDR (RAM_BUF_ADDR + __DRAM_BUF_LEN)

#define RAM_INFO_ADDR(base)	(base + 0x0)
#define RAM_START_ADDR(base)	(base + 0x4)
#define RAM_END_ADDR(base)	(base + 0x8)
#define RAM_CHARS_ADDR(base)	(base + 0xC)

static int dram_buf_len = __DRAM_BUF_LEN;

volatile void __iomem *ram_char_buf;
volatile void __iomem *ram_info_buf;
static int ramwrite_start = 0;
static int ram_clear_flush = 0;
static unsigned int ram_log_magic = 0;

static int drambuf_start, drambuf_end, drambuf_chars;
#endif

/* 
 * Uninitialized memory operation
 */
#ifdef CONFIG_DTVLOGD_EMERGENCY_LOG
static void write_ram_info(void)
{
    writel(drambuf_start, (void __iomem *)RAM_START_ADDR(ram_info_buf));
    writel(drambuf_end, (void __iomem *)RAM_END_ADDR(ram_info_buf));
    writel(drambuf_chars, (void __iomem *)RAM_CHARS_ADDR(ram_info_buf));
}
#endif


/*
 * rambuf_init()
 */
#ifdef CONFIG_DTVLOGD_EMERGENCY_LOG
static void rambuf_init(void)
{
	unsigned int magic = 0xbabe;
	unsigned int temp;

	ram_char_buf = ioremap(RAM_BUF_ADDR, dram_buf_len);
	ram_info_buf = ioremap(RAM_DATA_ADDR, sizeof(u32) * 4);

	temp = (unsigned int)readl(ram_info_buf);

	ram_log_magic = temp;

	if (magic == temp)	
	{
		ram_clear_flush = 1;
	}
	else {
		ramwrite_start = 1;
		drambuf_start = drambuf_end = drambuf_chars = 0;
		writel(magic, ram_info_buf);
	}
}
#endif

/*
 *	EXTERN FUNCTIONS
 */

extern int dtvlogd_write(const char __user *str, int count, int type);
static int usblog_find_device_dir(char *dev);
extern int vfs_statfs_native(struct super_block *sb, struct statfs *buf);

/*
 *	USB I/O FUNCTIONS
 */

#define LOGNAME_SIZE	32

char logname[LOGNAME_SIZE];

/*
 *	print_option()
 */

static void print_option(void)
{
	int mylen = dlog_buf_len;
	int mystart = dlogbuf_start;
	int myend = dlogbuf_end;
	int mychars = dlogged_chars;

	printk(KERN_CRIT"\n\n\n");
	printk(KERN_CRIT"==================================================\n");
	printk(KERN_CRIT"= DTVLOGD v%s\n", DTVLOGD_VERSION);
	printk(KERN_CRIT"==================================================\n");

#ifdef CONFIG_DTVLOGD_USB_LOG
	printk(KERN_CRIT"USB Log File   = %s\n", (logname[0]=='/')?logname:"(not found)" );
	printk(KERN_CRIT"Config Version = %d.%d.%d\n", opt_version[0], 
							opt_version[1], opt_version[2]);
	printk(KERN_CRIT"Use DTVLOGD?   = %s\n", (opt_dtvlogd)?"Yes":"No" );
	printk(KERN_CRIT"Save Interval  = %d sec\n", opt_interval);
#else
	printk(KERN_CRIT"DTVLOGD - USB Log : disabled by kernel config\n");
#endif

#ifdef CONFIG_DTVLOGD_EMERGENCY_LOG
	printk(KERN_CRIT"EMERG MAGIC    = 0x%x (0xbabe)\n", ram_log_magic);
#endif
	printk(KERN_CRIT"\n");

	printk(KERN_CRIT"Buffer Size    = %d\n", mylen);
	printk(KERN_CRIT"Start Index    = %d\n", mystart);
	printk(KERN_CRIT"End Index      = %d\n", myend);
	printk(KERN_CRIT"Saved Chars    = %d\n", mychars);
	printk(KERN_CRIT"Write Ok       = %d\n", dlogbuf_writeok);
	printk(KERN_CRIT"Write Fail     = %d\n", dlogbuf_writefail);
	printk(KERN_CRIT"\n");

}

/*
 *	print_usb_status()
 */

static void print_usb_status(void)
{
	char dev[LOGNAME_SIZE];
     struct file *fp;
	struct inode *inode;

	if ( usblog_find_device_dir(dev) ) {
		struct nameidata nd;
		int error;

		error = path_lookup(dev, LOOKUP_FOLLOW, &nd);

		if (!error) {
			struct kstatfs tmp;
			error = vfs_statfs(nd.dentry, &tmp);

			printk(KERN_CRIT"USB Available  = %lld MB\n", (tmp.f_bavail/1024) * (tmp.f_bsize / 1024) );

			path_release(&nd);
		} else
			return;

		strncat(dev, "/log.txt", LOGNAME_SIZE-1);
		fp = filp_open(dev, O_RDONLY | O_LARGEFILE, 0600);
		if (IS_ERR(fp)) {
			printk(KERN_CRIT"Log File Size  = 0 MB\n");
			return;
		}

		inode = fp->f_mapping->host;
		printk(KERN_CRIT"Log File Size  = %lld MB\n", inode->i_size / (1024*1024));

		filp_close(fp, NULL);
	}
}

/*
 *	usblog_parse_option()
 */

static void usblog_parse_option(char *lval, char *rval)
{
        if ((lval == NULL) || (rval == NULL)) return;

        if (!strncmp(lval, "version", 7))
        {
                sscanf(rval, "%d.%d.%d", &opt_version[0], &opt_version[1], &opt_version[2]);
        } 
        else if (!strncmp(lval, "dtvlogd", 7))
        {
                sscanf(rval, "%d", &opt_dtvlogd);
        } 
	else if (!strncmp(lval, "interval", 8))
	{
		sscanf(rval, "%d", &opt_interval);
	}
}

/*
 *	usblog_option()
 */

static int usblog_option(char *dev_dir)
{
	char buf[128], option_file[32];
	char lval[32], rval[32];
	char *p;
        struct file *fp;
	ssize_t ret = -EBADF;

	snprintf(option_file, 32, "%s/dtvlogd.conf", dev_dir);

	fp = filp_open(option_file, O_RDONLY | O_LARGEFILE, 0600);

	if (IS_ERR(fp)) return 0;

	ret = fp->f_op->read(fp, buf, 128, &fp->f_pos);

	if (ret < 0) return 0;

	for (p=buf; p != NULL; )
	{
		memset(lval, 0, 32);
		memset(rval, 0, 32);

		sscanf(p, "%s = %s", lval, rval);

		if (strlen(lval) > 1)
			usblog_parse_option(lval, rval);

		p = strstr(p, "\n");

		if (p == NULL) break;
		else p += 1;
	}

	filp_close(fp, NULL);

	print_option();
	print_usb_status();

	return 1;
}

/*
 *	usblog_find_device_dir()
 */

static int usblog_find_device_dir(char *dev)
{
        struct file *fp;
        char buf[192];
        char *p;
        int ret = -ENOENT;

        fp = filp_open("/dtv/usb/usblog", O_RDONLY | O_LARGEFILE, 0600);

        if (IS_ERR(fp)) return 0;

        ret = fp->f_op->read(fp, buf, 192, &fp->f_pos);

        if (ret < 0) return 0;

        filp_close(fp, NULL);

        for (p=buf; p != NULL; )
        {
                p = strstr(p, "MountDir");

                if (p == NULL) break;
                else {
                        sscanf(p, "MountDir : %s", dev);
                        return 1;
                }
        }

        return 0;                              /* no usb device */
}

/*
 *	usblog_find_device()
 */

static int usblog_find_device(char *dev)
{
        int ret = 0;


        if (dev[0] == '/')
                return 1;

	if ( (ret = usblog_find_device_dir(dev)) )
	{
		usblog_option(dev);
		strncat(dev, "/log.txt", LOGNAME_SIZE-1);
	}
	return ret;
}

/*
 *	usblog_open()
 */

static struct file* usblog_open(void)
{
        struct file *fp;

	if (!usblog_find_device(logname))
		return NULL;

        fp = filp_open(logname, O_CREAT|O_WRONLY|O_APPEND|O_LARGEFILE, 0600);

        return fp;
}

/*
 *	usblog_close()
 */

int usblog_close(struct file *filp, fl_owner_t id)
{
	return filp_close(filp, id);
}

/*
 *	__usblog_write()
 */


static int __usblog_write(struct file *file, char *str, int len)
{
     int ret;

     ret = file->f_op->write(file, str, len, &file->f_pos);

        return 0;
}

/*
 *	usblog_write()
 */

void usblog_write(char *s, int len)
{
        struct file *file;

	file = usblog_open();

	if (!IS_ERR(file)) {
		__usblog_write(file, s, len);
		usblog_close(file, NULL);
		dlogbuf_writeok++;
	} else {
		memset(logname, 0, LOGNAME_SIZE);
		dlogbuf_writefail++;
	}
}

/*
 *	usblog_flush()
 *
 *
 *	Flush all buffer message to usb.
 */

void usblog_flush(void)
{
#ifdef CONFIG_DTVLOGD_USB_LOG
	static char flush_buf[__DLOG_BUF_LEN];
	int len = 0;

#if 0
	/* DEBUG */
	do {
		int mylen = dlog_buf_len;
		int mystart = dlogbuf_start;
		int myend = dlogbuf_end;
		int mychars = dlogged_chars;

		printk(KERN_CRIT"[%s] dlog_buf_len  = %d\n", __FUNCTION__, mylen);
		printk(KERN_CRIT"[%s] dlogbuf_start = %d\n", __FUNCTION__, mystart);
		printk(KERN_CRIT"[%s] dlogbuf_end   = %d\n", __FUNCTION__, myend);
		printk(KERN_CRIT"[%s] dlogged_chars = %d\n", __FUNCTION__, mychars);
	} while(0);
#endif

	if (dlogged_chars == 0)
		return;

	if (!usblog_find_device(logname))
		return;

	if (!opt_dtvlogd)	/* enable or disable by dtvlogd.conf */
		return;

	spin_lock_irq(&dlogbuf_lock);
	
	if (DLOG_INDEX(dlogbuf_start) >= DLOG_INDEX(dlogbuf_end))
		len = dlog_buf_len - DLOG_INDEX(dlogbuf_start);
	else
		len = DLOG_INDEX(dlogbuf_end) - DLOG_INDEX(dlogbuf_start);

	memcpy(&flush_buf[0], &DLOG_BUF(dlogbuf_start), len);

	if (DLOG_INDEX(dlogbuf_start) >= DLOG_INDEX(dlogbuf_end)) {
		memcpy(&flush_buf[len], &DLOG_BUF(0), DLOG_INDEX(dlogbuf_end));
		len += DLOG_INDEX(dlogbuf_end);
	}

	dlogbuf_start = dlogbuf_end;
	dlogged_chars = 0;

	spin_unlock_irq(&dlogbuf_lock);

	usblog_write(flush_buf, len);
#endif
}

#ifdef CONFIG_DTVLOGD_EMERGENCY_LOG
static void ram_flush(void)
{
	char ram_logname[LOGNAME_SIZE];
	static char flush_buf[__DRAM_BUF_LEN];
	int len, i;
	int count = 0;
	int start, end, chars;
	struct file *fp;
	
	printk("\n\n\n*******************************************\n");
	printk("*******************************************\n");
	printk("******** Emergency Log Backup *************\n");
	printk("*******************************************\n");
	printk("*******************************************\n\n\n\n");
	
	snprintf(ram_logname, LOGNAME_SIZE, "/mtd_rwarea/emeg_log.txt");

	fp = filp_open(ram_logname, O_CREAT|O_WRONLY|O_TRUNC|O_LARGEFILE, 0644);
	if (IS_ERR(fp)) return;

	spin_lock_irq(&dlogbuf_lock);

	start = (int)readl((void __iomem *)RAM_START_ADDR(ram_info_buf));
	end  = (int)readl((void __iomem *)RAM_END_ADDR(ram_info_buf));
	chars = (int)readl((void __iomem *)RAM_CHARS_ADDR(ram_info_buf));

	if (DRAM_INDEX(start) >= DRAM_INDEX(end))
		len = dram_buf_len - DRAM_INDEX(start);
	else
		len = DRAM_INDEX(end) - DRAM_INDEX(start);

	for (i=start ; i<(start + len) ; i++)
	{
		flush_buf[count] = (char)readb(ram_char_buf + DRAM_INDEX(i));
		count++;
	}

	if (DRAM_INDEX(start) >= DRAM_INDEX(end)) {
		for (i=0 ; i<DRAM_INDEX(end) ; i++) {
			flush_buf[count] = (char)readb(ram_char_buf + DRAM_INDEX(i));
			count++;
		}
		len += DRAM_INDEX(end);
	}

	spin_unlock_irq(&dlogbuf_lock);

	fp->f_op->write(fp, flush_buf, len, &fp->f_pos);
	
	filp_close(fp, NULL);

	ramwrite_start = 1;
	drambuf_start = drambuf_end = drambuf_chars = 0;
}
#endif

static void dlogbuf_write(char c)
{
	DLOG_BUF(dlogbuf_end) = c;

#ifdef CONFIG_DTVLOGD_EMERGENCY_LOG
	if (ramwrite_start == 1)
	{
		writeb(c, ram_char_buf + DRAM_INDEX(drambuf_end));
		drambuf_end++;
	
		if (drambuf_end - drambuf_start > dram_buf_len)
			drambuf_start = drambuf_end - dram_buf_len;
		if (drambuf_chars < dram_buf_len)
			drambuf_chars++;
	
		write_ram_info();
	}
#endif

	dlogbuf_end++;

	if (dlogbuf_end - dlogbuf_start > dlog_buf_len)
		dlogbuf_start = dlogbuf_end - dlog_buf_len;
	if (dlogged_chars < dlog_buf_len)
		dlogged_chars++;
}

/*
 *	LOG FUNCTIONS
 */

/* 
 *  Referenced do_syslog()
 *
 *  Commands to do_dtvlog:
 *
 *      0 -- Close the log. Currently a NOP.
 *      1 -- Open the log. Currently a NOP.
 *      2 -- Read from dlog buffer. 
 *      3 -- Write printf/printk messages to dlog buffer.
 *      4 -- Read and clear all messages remaining in the dlog buffer.
 *      5 -- Synchronously read and clear all messages remaining in the dlog buffer.
 *      6 -- Read from ram buffer when booting time.
 *     10 -- Print number of unread characters in the dtvlog buffer.
 */
int do_dtvlog(int type, const char __user *buf, int len)
{
	int error = 0;
	int preempt;
	unsigned long i;
	char c;

	if (opt_nodtvlogd) return 0;	/* enable or disable by kernel argument */

	switch (type) {
	
	case 0:
		break;
	case 1:
		break;
	/* Read from kernel buffer */
	case 2:
		i = 0;
		spin_lock_irq(&dlogbuf_lock);
		while (!error && (dlogbuf_start != dlogbuf_end) && i < len) {
			c = DLOG_BUF(dlogbuf_start);
			dlogbuf_start++;
			spin_unlock_irq(&dlogbuf_lock);
			error = __put_user(c, buf);
			buf++;
			i++;
			cond_resched();	
			spin_lock_irq(&dlogbuf_lock);
		}
		spin_unlock_irq(&dlogbuf_lock);
		if (!error)
			error = i;

		break;

	/* Write some data to ring buffer. */
	case 3: 
		dtvlogd_write(buf, len, 2);		// from printk
		break;
	case 4:
		down_interruptible(&dlogbuf_sem);
		usblog_flush();
		up(&dlogbuf_sem);

		if (dlogbuf_complete) {
			up(&dlogbuf_complete_sem);
			dlogbuf_complete = 0;
		}
		break;
	case 5:
		DTVLOGD_PREEMPT_COUNT_SAVE(preempt);
		dlogbuf_complete = 1;
		wake_up_interruptible(&dlogbuf_wq);
		down_interruptible(&dlogbuf_complete_sem);
		DTVLOGD_PREEMPT_COUNT_RESTORE(preempt);

#ifdef CONFIG_DTVLOGD_EMERGENCY_LOG
		writel(0xdead, ram_info_buf);
#endif
		break;
#ifdef CONFIG_DTVLOGD_EMERGENCY_LOG
	case 6:
		down_interruptible(&dlogbuf_sem);
		ram_flush();
		up(&dlogbuf_sem);
		break;
#endif
	case 9: /* Number of chars in log buffer */
		error = dlogbuf_end - dlogbuf_start;
		break;
	case 10: /* Print number of unread characters in the dtvlog buffer. */
		print_option();
		print_usb_status();
		break;

	/* Write some data to ring buffer. */
	case 11: 
		dtvlogd_write(buf, len, 1);		// from printf
		break;
	
	default:
		error = -EINVAL;
		break;
	}
	
	return error;

}

int dtvlogd_print(char *str, int len)
{
	int printed_len=0;
	char *p;
	static int start_newline = 1;
	static char print_buf[1024];
	/* Prevent overflow */
	if (len > 1023) len = 1023;

	printed_len = snprintf(print_buf, len + 1, "%s\n", str);

	/* Copy the output into log buf */
	for (p = print_buf; *p ; p++) 
	{
		if (start_newline)
		{
			start_newline = 0;
			if (!*p)
				break;
		}

		dlogbuf_write(*p);	

		if (*p == '\n')
			start_newline = 1;
	}
	return printed_len;
}

/*
 *	is_hw_reset()
 */

int is_hw_reset(const char *str, int len)
{
	int i;
	static char hw_reset[9] = { 0xff, 0xff, 0x1d, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1d };
	static char hw_reset2[9] = { 0xff, 0xff, 0x12, 0x00, 0x00, 0x00, 0x00, 0x00, 0x12 };

	if (len != 9) return 0;

	for(i=0; i<9; i++)
	{
		if ( (str[i] != hw_reset[i]) 
			&& (str[i] != hw_reset2[i]) ) return 0;
	}
	
	return 1;
}


/*
 * @str   : Serial string (printf, printk msg)
 * @count : Serial string count
 * @type  : 1->printf, 2->printk 
 */

int dtvlogd_write(const char __user *str, int count, int type)
{
	int len;
	char usr_buf[256];
	char *buf;

	if (type == 1)
	{	/* printf */

		/* Prevent overflow */
		if (count > 256) count = 256;

		if (copy_from_user(usr_buf, str, count))
			return -EFAULT;

		buf = usr_buf;
	} else if (type == 2) 
	{	/* printk */
		buf = str;
	} else {
		BUG();
	}

	/*
	 * Don't save micom command. It's not a ascii character.
	 * The exeDSP write a micom command to serial.
	 */

	if (!isascii(buf[0]) || count == 0) {
#if 0
		/* DEBUG */
		int i;
		printk(KERN_CRIT"[%s] not ascii(%d) = ", __FUNCTION__, count);
		for (i=0; i<count; i++)
			printk("%x ", str[i]);
		printk("\n");
#endif
		if (is_hw_reset(buf, count)) {
			do_dtvlog(5, NULL, 0);
		}
		return 1;
	}

	len = dtvlogd_print(buf, count);

	return 1;
}

/*
 *	DTVLOGD KERNEL THREAD FUNCTIONS
 */

static int __init nodtvlogd_kernel(char *str)
{
        if (*str)
                return 0;

	opt_nodtvlogd = 1;	/* enable or disable by kernel argument */

	printk(KERN_CRIT"==================================================\n");
	printk(KERN_CRIT"= NO DTVLOGD v%s\n", DTVLOGD_VERSION);
	printk(KERN_CRIT"==================================================\n");

        return 1;
}

__setup("nodtvlogd", nodtvlogd_kernel);

/*
 *	dtvlogd()
 */

static int dtvlogd(void *p)
{
	daemonize("kdtvlogd");

#ifdef CONFIG_DTVLOGD_EMERGENCY_LOG
	rambuf_init();
#endif

	wait_event_interruptible_timeout(dlogbuf_wq, dlogbuf_complete, 15*HZ); /* run after 15sec */
	
	for ( ; ; ) {
#ifdef CONFIG_DTVLOGD_EMERGENCY_LOG
		if (ram_clear_flush == 1)
		{
			do_dtvlog(6, NULL, 0);
			ram_clear_flush = 0;
		}
#endif
		do_dtvlog(4, NULL, 0);

		wait_event_interruptible_timeout(dlogbuf_wq, dlogbuf_complete, opt_interval*HZ);
	}

	return 0;
}

/*
 *	dtvlogd_init()
 */

static int __init dtvlogd_init(void)
{
	pid_t pid;

	if (opt_nodtvlogd) return 0;	/* enable or disable by kernel argument */

	/* removed DECLARE_MUTEX_LOCKED() kernel 2.6.24 */
	init_MUTEX_LOCKED(&dlogbuf_complete_sem);

	init_waitqueue_head(&dlogbuf_wq);

	pid = kernel_thread(dtvlogd, NULL, CLONE_KERNEL);
	BUG_ON(pid < 0);

	return 0;
}

module_init(dtvlogd_init)

