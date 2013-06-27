#ifndef _SCSI_DISK_H
#define _SCSI_DISK_H

/*
 * More than enough for everybody ;)  The huge number of majors
 * is a leftover from 16bit dev_t days, we don't really need that
 * much numberspace.
 */
#define SD_MAJORS	16

/*
 * This is limited by the naming scheme enforced in sd_probe,
 * add another character to it if you really need more disks.
 */
#define SD_MAX_DISKS	(((26 * 26) + 26 + 1) * 26)

/*
 * Time out in seconds for disks and Magneto-opticals (which are slower).
 */
//VD_COMP_USB_PATCH
 //hongyabi patch JAN-24-2007
//#define SD_TIMEOUT            (30 * HZ)
#define SD_TIMEOUT              (5 * HZ)
#define SD_MOD_TIMEOUT		(75 * HZ)

/*
 * Number of allowed retries
 */
//VD_COMP_USB_PATCH
 //hongyabi patch JAN-24-2007
//#define SD_MAX_RETRIES                5
#define SD_MAX_RETRIES          3
#define SD_PASSTHROUGH_RETRIES	1

/*
 * Size of the initial data buffer for mode and read capacity data
 */
#define SD_BUF_SIZE		512

//VD_COMP_USB_PATCH
/* VDLinux, based SELP.4.3.1.x default patch No.2, 
   SD USB card reader, SP Team 2009-05-13 */
struct poller_type {
        int prev_state;
        int pid;
        struct completion done_notify;
};

struct scsi_disk {
	struct scsi_driver *driver;	/* always &sd_template */
	struct scsi_device *device;
	struct class_device cdev;
	struct gendisk	*disk;
	unsigned int	openers;	/* protected by BKL for now, yuck */
	sector_t	capacity;	/* size in 512-byte sectors */
	u32		index;
	u8		media_present;
	u8		write_prot;
	unsigned	WCE : 1;	/* state of disk WCE bit */
	unsigned	RCD : 1;	/* state of disk RCD bit, unused */
	unsigned	DPOFUA : 1;	/* state of disk DPOFUA bit */
	struct poller_type poller;
};
#define to_scsi_disk(obj) container_of(obj,struct scsi_disk,cdev)

#define sd_printk(prefix, sdsk, fmt, a...)				\
        (sdsk)->disk ?							\
	sdev_printk(prefix, (sdsk)->device, "[%s] " fmt,		\
		    (sdsk)->disk->disk_name, ##a) :			\
	sdev_printk(prefix, (sdsk)->device, fmt, ##a)

#endif /* _SCSI_DISK_H */
