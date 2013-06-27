/*
 *  gendisk handling
 */

#include <linux/module.h>
#include <linux/fs.h>
#include <linux/genhd.h>
#include <linux/kdev_t.h>
#include <linux/kernel.h>
#include <linux/blkdev.h>
#include <linux/init.h>
#include <linux/spinlock.h>
#include <linux/seq_file.h>
#include <linux/slab.h>
#include <linux/kmod.h>
#include <linux/kobj_map.h>
#include <linux/buffer_head.h>
#include <linux/mutex.h>

#ifdef CONFIG_KDEBUGD_COUNTER_MONITOR
#include <linux/time.h>
#include <linux/kdebugd.h>
#include <linux/sec_diskusage.h>
#endif

struct kset block_subsys;
static DEFINE_MUTEX(block_subsys_lock);

/*
 * Can be deleted altogether. Later.
 *
 */
static struct blk_major_name {
	struct blk_major_name *next;
	int major;
	char name[16];
} *major_names[BLKDEV_MAJOR_HASH_SIZE];

inline static void sec_bp_start(void);

/* index in the above - for now: assume no multimajor ranges */
static inline int major_to_index(int major)
{
	return major % BLKDEV_MAJOR_HASH_SIZE;
}

#ifdef CONFIG_PROC_FS

void blkdev_show(struct seq_file *f, off_t offset)
{
	struct blk_major_name *dp;

	if (offset < BLKDEV_MAJOR_HASH_SIZE) {
		mutex_lock(&block_subsys_lock);
		for (dp = major_names[offset]; dp; dp = dp->next)
			seq_printf(f, "%3d %s\n", dp->major, dp->name);
		mutex_unlock(&block_subsys_lock);
	}
}

#endif /* CONFIG_PROC_FS */

int register_blkdev(unsigned int major, const char *name)
{
	struct blk_major_name **n, *p;
	int index, ret = 0;

	mutex_lock(&block_subsys_lock);

	/* temporary */
	if (major == 0) {
		for (index = ARRAY_SIZE(major_names)-1; index > 0; index--) {
			if (major_names[index] == NULL)
				break;
		}

		if (index == 0) {
			printk("register_blkdev: failed to get major for %s\n",
			       name);
			ret = -EBUSY;
			goto out;
		}
		major = index;
		ret = major;
	}

	p = kmalloc(sizeof(struct blk_major_name), GFP_KERNEL);
	if (p == NULL) {
		ret = -ENOMEM;
		goto out;
	}

	p->major = major;
	strlcpy(p->name, name, sizeof(p->name));
	p->next = NULL;
	index = major_to_index(major);

	for (n = &major_names[index]; *n; n = &(*n)->next) {
		if ((*n)->major == major)
			break;
	}
	if (!*n)
		*n = p;
	else
		ret = -EBUSY;

	if (ret < 0) {
		printk("register_blkdev: cannot get major %d for %s\n",
		       major, name);
		kfree(p);
	}
out:
	mutex_unlock(&block_subsys_lock);
	return ret;
}

EXPORT_SYMBOL(register_blkdev);

void unregister_blkdev(unsigned int major, const char *name)
{
	struct blk_major_name **n;
	struct blk_major_name *p = NULL;
	int index = major_to_index(major);

	mutex_lock(&block_subsys_lock);
	for (n = &major_names[index]; *n; n = &(*n)->next)
		if ((*n)->major == major)
			break;
	if (!*n || strcmp((*n)->name, name)) {
		WARN_ON(1);
	} else {
		p = *n;
		*n = p->next;
	}
	mutex_unlock(&block_subsys_lock);
	kfree(p);
}

EXPORT_SYMBOL(unregister_blkdev);

static struct kobj_map *bdev_map;

/*
 * Register device numbers dev..(dev+range-1)
 * range must be nonzero
 * The hash chain is sorted on range, so that subranges can override.
 */
void blk_register_region(dev_t dev, unsigned long range, struct module *module,
			 struct kobject *(*probe)(dev_t, int *, void *),
			 int (*lock)(dev_t, void *), void *data)
{
	kobj_map(bdev_map, dev, range, module, probe, lock, data);
}

EXPORT_SYMBOL(blk_register_region);

void blk_unregister_region(dev_t dev, unsigned long range)
{
	kobj_unmap(bdev_map, dev, range);
}

EXPORT_SYMBOL(blk_unregister_region);

static struct kobject *exact_match(dev_t dev, int *part, void *data)
{
	struct gendisk *p = data;
	return &p->kobj;
}

static int exact_lock(dev_t dev, void *data)
{
	struct gendisk *p = data;

	if (!get_disk(p))
		return -1;
	return 0;
}

/**
 * add_disk - add partitioning information to kernel list
 * @disk: per-device partitioning information
 *
 * This function registers the partitioning information in @disk
 * with the kernel.
 */
void add_disk(struct gendisk *disk)
{
	disk->flags |= GENHD_FL_UP;
	blk_register_region(MKDEV(disk->major, disk->first_minor),
			    disk->minors, NULL, exact_match, exact_lock, disk);
	register_disk(disk);
	blk_register_queue(disk);
}

EXPORT_SYMBOL(add_disk);
EXPORT_SYMBOL(del_gendisk);	/* in partitions/check.c */

void unlink_gendisk(struct gendisk *disk)
{
	blk_unregister_queue(disk);
	blk_unregister_region(MKDEV(disk->major, disk->first_minor),
			      disk->minors);
}

#define to_disk(obj) container_of(obj,struct gendisk,kobj)

/**
 * get_gendisk - get partitioning information for a given device
 * @dev: device to get partitioning information for
 *
 * This function gets the structure containing partitioning
 * information for the given device @dev.
 */
struct gendisk *get_gendisk(dev_t dev, int *part)
{
	struct kobject *kobj = kobj_lookup(bdev_map, dev, part);
	return  kobj ? to_disk(kobj) : NULL;
}

/*
 * print a full list of all partitions - intended for places where the root
 * filesystem can't be mounted and thus to give the victim some idea of what
 * went wrong
 */
void __init printk_all_partitions(void)
{
	int n;
	struct gendisk *sgp;

	mutex_lock(&block_subsys_lock);
	/* For each block device... */
	list_for_each_entry(sgp, &block_subsys.list, kobj.entry) {
		char buf[BDEVNAME_SIZE];
		/*
		 * Don't show empty devices or things that have been surpressed
		 */
		if (get_capacity(sgp) == 0 ||
		    (sgp->flags & GENHD_FL_SUPPRESS_PARTITION_INFO))
			continue;

		/*
		 * Note, unlike /proc/partitions, I am showing the numbers in
		 * hex - the same format as the root= option takes.
		 */
		printk("%02x%02x %10llu %s",
			sgp->major, sgp->first_minor,
			(unsigned long long)get_capacity(sgp) >> 1,
			disk_name(sgp, 0, buf));
		if (sgp->driverfs_dev != NULL &&
		    sgp->driverfs_dev->driver != NULL)
			printk(" driver: %s\n",
				sgp->driverfs_dev->driver->name);
		else
			printk(" (driver?)\n");

		/* now show the partitions */
		for (n = 0; n < sgp->minors - 1; ++n) {
			if (sgp->part[n] == NULL)
				continue;
			if (sgp->part[n]->nr_sects == 0)
				continue;
			printk("  %02x%02x %10llu %s\n",
				sgp->major, n + 1 + sgp->first_minor,
				(unsigned long long)sgp->part[n]->nr_sects >> 1,
				disk_name(sgp, n + 1, buf));
		} /* partition subloop */
	} /* Block device loop */

	mutex_unlock(&block_subsys_lock);
	return;
}

#ifdef CONFIG_PROC_FS
/* iterator */
static void *part_start(struct seq_file *part, loff_t *pos)
{
	struct list_head *p;
	loff_t l = *pos;

	mutex_lock(&block_subsys_lock);
	list_for_each(p, &block_subsys.list)
		if (!l--)
			return list_entry(p, struct gendisk, kobj.entry);
	return NULL;
}

static void *part_next(struct seq_file *part, void *v, loff_t *pos)
{
	struct list_head *p = ((struct gendisk *)v)->kobj.entry.next;
	++*pos;
	return p==&block_subsys.list ? NULL :
		list_entry(p, struct gendisk, kobj.entry);
}

static void part_stop(struct seq_file *part, void *v)
{
	mutex_unlock(&block_subsys_lock);
}

static int show_partition(struct seq_file *part, void *v)
{
	struct gendisk *sgp = v;
	int n;
	char buf[BDEVNAME_SIZE];

	if (&sgp->kobj.entry == block_subsys.list.next)
		seq_puts(part, "major minor  #blocks  name\n\n");

	/* Don't show non-partitionable removeable devices or empty devices */
	if (!get_capacity(sgp) ||
			(sgp->minors == 1 && (sgp->flags & GENHD_FL_REMOVABLE)))
		return 0;
	if (sgp->flags & GENHD_FL_SUPPRESS_PARTITION_INFO)
		return 0;

	/* show the full disk and all non-0 size partitions of it */
	seq_printf(part, "%4d  %4d %10llu %s\n",
		sgp->major, sgp->first_minor,
		(unsigned long long)get_capacity(sgp) >> 1,
		disk_name(sgp, 0, buf));
	for (n = 0; n < sgp->minors - 1; n++) {
		if (!sgp->part[n])
			continue;
		if (sgp->part[n]->nr_sects == 0)
			continue;
		seq_printf(part, "%4d  %4d %10llu %s\n",
			sgp->major, n + 1 + sgp->first_minor,
			(unsigned long long)sgp->part[n]->nr_sects >> 1 ,
			disk_name(sgp, n + 1, buf));
	}

	return 0;
}

struct seq_operations partitions_op = {
	.start =part_start,
	.next =	part_next,
	.stop =	part_stop,
	.show =	show_partition
};
#endif


extern int blk_dev_init(void);

static struct kobject *base_probe(dev_t dev, int *part, void *data)
{
	if (request_module("block-major-%d-%d", MAJOR(dev), MINOR(dev)) > 0)
		/* Make old-style 2.4 aliases work */
		request_module("block-major-%d", MAJOR(dev));
	return NULL;
}

static int __init genhd_device_init(void)
{
	int err;

	bdev_map = kobj_map_init(base_probe, &block_subsys_lock);
	blk_dev_init();
	err = subsystem_register(&block_subsys);
	if (err < 0)
		printk(KERN_WARNING "%s: subsystem_register error: %d\n",
			__FUNCTION__, err);
#ifdef CONFIG_BOOTPROFILE
	sec_bp_start();
#endif /* CONFIG_BOOTPROFILE */
	return err;
}

subsys_initcall(genhd_device_init);



/*
 * kobject & sysfs bindings for block devices
 */
static ssize_t disk_attr_show(struct kobject *kobj, struct attribute *attr,
			      char *page)
{
	struct gendisk *disk = to_disk(kobj);
	struct disk_attribute *disk_attr =
		container_of(attr,struct disk_attribute,attr);
	ssize_t ret = -EIO;

	if (disk_attr->show)
		ret = disk_attr->show(disk,page);
	return ret;
}

static ssize_t disk_attr_store(struct kobject * kobj, struct attribute * attr,
			       const char *page, size_t count)
{
	struct gendisk *disk = to_disk(kobj);
	struct disk_attribute *disk_attr =
		container_of(attr,struct disk_attribute,attr);
	ssize_t ret = 0;

	if (disk_attr->store)
		ret = disk_attr->store(disk, page, count);
	return ret;
}

static struct sysfs_ops disk_sysfs_ops = {
	.show	= &disk_attr_show,
	.store	= &disk_attr_store,
};

static ssize_t disk_uevent_store(struct gendisk * disk,
				 const char *buf, size_t count)
{
	kobject_uevent(&disk->kobj, KOBJ_ADD);
	return count;
}
static ssize_t disk_dev_read(struct gendisk * disk, char *page)
{
	dev_t base = MKDEV(disk->major, disk->first_minor); 
	return print_dev_t(page, base);
}
static ssize_t disk_range_read(struct gendisk * disk, char *page)
{
	return sprintf(page, "%d\n", disk->minors);
}
static ssize_t disk_removable_read(struct gendisk * disk, char *page)
{
	return sprintf(page, "%d\n",
		       (disk->flags & GENHD_FL_REMOVABLE ? 1 : 0));

}
static ssize_t disk_size_read(struct gendisk * disk, char *page)
{
	return sprintf(page, "%llu\n", (unsigned long long)get_capacity(disk));
}
static ssize_t disk_capability_read(struct gendisk *disk, char *page)
{
	return sprintf(page, "%x\n", disk->flags);
}
static ssize_t disk_stats_read(struct gendisk * disk, char *page)
{
	preempt_disable();
	disk_round_stats(disk);
	preempt_enable();
	return sprintf(page,
		"%8lu %8lu %8llu %8u "
		"%8lu %8lu %8llu %8u "
		"%8u %8u %8u"
		"\n",
		disk_stat_read(disk, ios[READ]),
		disk_stat_read(disk, merges[READ]),
		(unsigned long long)disk_stat_read(disk, sectors[READ]),
		jiffies_to_msecs(disk_stat_read(disk, ticks[READ])),
		disk_stat_read(disk, ios[WRITE]),
		disk_stat_read(disk, merges[WRITE]),
		(unsigned long long)disk_stat_read(disk, sectors[WRITE]),
		jiffies_to_msecs(disk_stat_read(disk, ticks[WRITE])),
		disk->in_flight,
		jiffies_to_msecs(disk_stat_read(disk, io_ticks)),
		jiffies_to_msecs(disk_stat_read(disk, time_in_queue)));
}
static struct disk_attribute disk_attr_uevent = {
	.attr = {.name = "uevent", .mode = S_IWUSR },
	.store	= disk_uevent_store
};
static struct disk_attribute disk_attr_dev = {
	.attr = {.name = "dev", .mode = S_IRUGO },
	.show	= disk_dev_read
};
static struct disk_attribute disk_attr_range = {
	.attr = {.name = "range", .mode = S_IRUGO },
	.show	= disk_range_read
};
static struct disk_attribute disk_attr_removable = {
	.attr = {.name = "removable", .mode = S_IRUGO },
	.show	= disk_removable_read
};
static struct disk_attribute disk_attr_size = {
	.attr = {.name = "size", .mode = S_IRUGO },
	.show	= disk_size_read
};
static struct disk_attribute disk_attr_capability = {
	.attr = {.name = "capability", .mode = S_IRUGO },
	.show	= disk_capability_read
};
static struct disk_attribute disk_attr_stat = {
	.attr = {.name = "stat", .mode = S_IRUGO },
	.show	= disk_stats_read
};

#ifdef CONFIG_FAIL_MAKE_REQUEST

static ssize_t disk_fail_store(struct gendisk * disk,
			       const char *buf, size_t count)
{
	int i;

	if (count > 0 && sscanf(buf, "%d", &i) > 0) {
		if (i == 0)
			disk->flags &= ~GENHD_FL_FAIL;
		else
			disk->flags |= GENHD_FL_FAIL;
	}

	return count;
}
static ssize_t disk_fail_read(struct gendisk * disk, char *page)
{
	return sprintf(page, "%d\n", disk->flags & GENHD_FL_FAIL ? 1 : 0);
}
static struct disk_attribute disk_attr_fail = {
	.attr = {.name = "make-it-fail", .mode = S_IRUGO | S_IWUSR },
	.store	= disk_fail_store,
	.show	= disk_fail_read
};

#endif

static struct attribute * default_attrs[] = {
	&disk_attr_uevent.attr,
	&disk_attr_dev.attr,
	&disk_attr_range.attr,
	&disk_attr_removable.attr,
	&disk_attr_size.attr,
	&disk_attr_stat.attr,
	&disk_attr_capability.attr,
#ifdef CONFIG_FAIL_MAKE_REQUEST
	&disk_attr_fail.attr,
#endif
	NULL,
};

static void disk_release(struct kobject * kobj)
{
	struct gendisk *disk = to_disk(kobj);
	kfree(disk->random);
	kfree(disk->part);
	free_disk_stats(disk);
	kfree(disk);
}

static struct kobj_type ktype_block = {
	.release	= disk_release,
	.sysfs_ops	= &disk_sysfs_ops,
	.default_attrs	= default_attrs,
};

extern struct kobj_type ktype_part;

static int block_uevent_filter(struct kset *kset, struct kobject *kobj)
{
	struct kobj_type *ktype = get_ktype(kobj);

	return ((ktype == &ktype_block) || (ktype == &ktype_part));
}

static int block_uevent(struct kset *kset, struct kobject *kobj,
			struct kobj_uevent_env *env)
{
	struct kobj_type *ktype = get_ktype(kobj);
	struct device *physdev;
	struct gendisk *disk;
	struct hd_struct *part;

	if (ktype == &ktype_block) {
		disk = container_of(kobj, struct gendisk, kobj);
		add_uevent_var(env, "MINOR=%u", disk->first_minor);
	} else if (ktype == &ktype_part) {
		disk = container_of(kobj->parent, struct gendisk, kobj);
		part = container_of(kobj, struct hd_struct, kobj);
		add_uevent_var(env, "MINOR=%u",
			       disk->first_minor + part->partno);
	} else
		return 0;

	add_uevent_var(env, "MAJOR=%u", disk->major);

	/* add physical device, backing this device  */
	physdev = disk->driverfs_dev;
	if (physdev) {
		char *path = kobject_get_path(&physdev->kobj, GFP_KERNEL);

		add_uevent_var(env, "PHYSDEVPATH=%s", path);
		kfree(path);

		if (physdev->bus)
			add_uevent_var(env, "PHYSDEVBUS=%s", physdev->bus->name);

		if (physdev->driver)
			add_uevent_var(env, physdev->driver->name);
	}

	return 0;
}

static struct kset_uevent_ops block_uevent_ops = {
	.filter		= block_uevent_filter,
	.uevent		= block_uevent,
};

decl_subsys(block, &ktype_block, &block_uevent_ops);

/*
 * aggregate disk stat collector.  Uses the same stats that the sysfs
 * entries do, above, but makes them available through one seq_file.
 * Watching a few disks may be efficient through sysfs, but watching
 * all of them will be more efficient through this interface.
 *
 * The output looks suspiciously like /proc/partitions with a bunch of
 * extra fields.
 */

/* iterator */
static void *diskstats_start(struct seq_file *part, loff_t *pos)
{
	loff_t k = *pos;
	struct list_head *p;

	mutex_lock(&block_subsys_lock);
	list_for_each(p, &block_subsys.list)
		if (!k--)
			return list_entry(p, struct gendisk, kobj.entry);
	return NULL;
}

static void *diskstats_next(struct seq_file *part, void *v, loff_t *pos)
{
	struct list_head *p = ((struct gendisk *)v)->kobj.entry.next;
	++*pos;
	return p==&block_subsys.list ? NULL :
		list_entry(p, struct gendisk, kobj.entry);
}

static void diskstats_stop(struct seq_file *part, void *v)
{
	mutex_unlock(&block_subsys_lock);
}

static int diskstats_show(struct seq_file *s, void *v)
{
	struct gendisk *gp = v;
	char buf[BDEVNAME_SIZE];
	int n = 0;

	/*
	if (&sgp->kobj.entry == block_subsys.kset.list.next)
		seq_puts(s,	"major minor name"
				"     rio rmerge rsect ruse wio wmerge "
				"wsect wuse running use aveq"
				"\n\n");
	*/
 
	preempt_disable();
	disk_round_stats(gp);
	preempt_enable();
	seq_printf(s, "%4d %4d %s %lu %lu %llu %u %lu %lu %llu %u %u %u %u\n",
		gp->major, n + gp->first_minor, disk_name(gp, n, buf),
		disk_stat_read(gp, ios[0]), disk_stat_read(gp, merges[0]),
		(unsigned long long)disk_stat_read(gp, sectors[0]),
		jiffies_to_msecs(disk_stat_read(gp, ticks[0])),
		disk_stat_read(gp, ios[1]), disk_stat_read(gp, merges[1]),
		(unsigned long long)disk_stat_read(gp, sectors[1]),
		jiffies_to_msecs(disk_stat_read(gp, ticks[1])),
		gp->in_flight,
		jiffies_to_msecs(disk_stat_read(gp, io_ticks)),
		jiffies_to_msecs(disk_stat_read(gp, time_in_queue)));

	/* now show all non-0 size partitions of it */
	for (n = 0; n < gp->minors - 1; n++) {
		struct hd_struct *hd = gp->part[n];

		if (hd && hd->nr_sects)
			seq_printf(s, "%4d %4d %s %u %u %u %u\n",
				gp->major, n + gp->first_minor + 1,
				disk_name(gp, n + 1, buf),
				hd->ios[0], hd->sectors[0],
				hd->ios[1], hd->sectors[1]);
	}
 
	return 0;
}

struct seq_operations diskstats_op = {
	.start	= diskstats_start,
	.next	= diskstats_next,
	.stop	= diskstats_stop,
	.show	= diskstats_show
};

static void media_change_notify_thread(struct work_struct *work)
{
	struct gendisk *gd = container_of(work, struct gendisk, async_notify);
	char event[] = "MEDIA_CHANGE=1";
	char *envp[] = { event, NULL };

	/*
	 * set enviroment vars to indicate which event this is for
	 * so that user space will know to go check the media status.
	 */
	kobject_uevent_env(&gd->kobj, KOBJ_CHANGE, envp);
	put_device(gd->driverfs_dev);
}

void genhd_media_change_notify(struct gendisk *disk)
{
	get_device(disk->driverfs_dev);
	schedule_work(&disk->async_notify);
}
EXPORT_SYMBOL_GPL(genhd_media_change_notify);

struct gendisk *alloc_disk(int minors)
{
	return alloc_disk_node(minors, -1);
}

struct gendisk *alloc_disk_node(int minors, int node_id)
{
	struct gendisk *disk;

	disk = kmalloc_node(sizeof(struct gendisk),
				GFP_KERNEL | __GFP_ZERO, node_id);
	if (disk) {
		if (!init_disk_stats(disk)) {
			kfree(disk);
			return NULL;
		}
#if defined (CONFIG_MIPS_BCM_NDVD)
        /*
         * Initialize the disk's sector size
         * to a typical value for hard disks.
         * This will be overridden later on
         * an as-needed basis.
         */
        disk->sector_size = 512;
#endif
		if (minors > 1) {
			int size = (minors - 1) * sizeof(struct hd_struct *);
			disk->part = kmalloc_node(size,
				GFP_KERNEL | __GFP_ZERO, node_id);
			if (!disk->part) {
				free_disk_stats(disk);
				kfree(disk);
				return NULL;
			}
		}
		disk->minors = minors;
		kobj_set_kset_s(disk,block_subsys);
		kobject_init(&disk->kobj);
		rand_initialize_disk(disk);
		INIT_WORK(&disk->async_notify,
			media_change_notify_thread);
	}
	return disk;
}

EXPORT_SYMBOL(alloc_disk);
EXPORT_SYMBOL(alloc_disk_node);

struct kobject *get_disk(struct gendisk *disk)
{
	struct module *owner;
	struct kobject *kobj;

	if (!disk->fops)
		return NULL;
	owner = disk->fops->owner;
	if (owner && !try_module_get(owner))
		return NULL;
	kobj = kobject_get(&disk->kobj);
	if (kobj == NULL) {
		module_put(owner);
		return NULL;
	}
	return kobj;

}

EXPORT_SYMBOL(get_disk);

void put_disk(struct gendisk *disk)
{
	if (disk)
		kobject_put(&disk->kobj);
}

EXPORT_SYMBOL(put_disk);

void set_device_ro(struct block_device *bdev, int flag)
{
	if (bdev->bd_contains != bdev)
		bdev->bd_part->policy = flag;
	else
		bdev->bd_disk->policy = flag;
}

EXPORT_SYMBOL(set_device_ro);

void set_disk_ro(struct gendisk *disk, int flag)
{
	int i;
	disk->policy = flag;
	for (i = 0; i < disk->minors - 1; i++)
		if (disk->part[i]) disk->part[i]->policy = flag;
}

EXPORT_SYMBOL(set_disk_ro);

int bdev_read_only(struct block_device *bdev)
{
	if (!bdev)
		return 0;
	else if (bdev->bd_contains != bdev)
		return bdev->bd_part->policy;
	else
		return bdev->bd_disk->policy;
}

EXPORT_SYMBOL(bdev_read_only);

int invalidate_partition(struct gendisk *disk, int index)
{
	int res = 0;
	struct block_device *bdev = bdget_disk(disk, index);
	if (bdev) {
		fsync_bdev(bdev);
		res = __invalidate_device(bdev);
		bdput(bdev);
	}
	return res;
}

EXPORT_SYMBOL(invalidate_partition);

#ifdef CONFIG_KDEBUGD_COUNTER_MONITOR

int sec_diskstats_dump_entry(struct gendisk* gd,
		long *nsect_read,
		long *nsect_write,
		unsigned long *io_ticks )
{
	if ( (NULL == gd) ||
			(NULL == nsect_read) ||
			(NULL == nsect_write) ||
			(NULL == io_ticks) )
		return 0;

	preempt_disable();
	disk_round_stats(gd);
	preempt_enable();

	(*nsect_read) += disk_stat_read(gd, sectors[0]);
	(*nsect_write) += disk_stat_read(gd, sectors[1]);
	(*io_ticks) += disk_stat_read(gd, io_ticks);

	return 1;
}

#define SECT_TO_KBYTE(x)	((x)*512/1024)

extern void diskusage_show_header(void);

int sec_diskstats_dump(void)
{
	struct list_head *p= NULL;
	struct gendisk* disk= NULL;

	static long old_nsect_read=0, old_nsect_write=0;
	static unsigned long old_io_ticks=0, old_rticks=0;

	long nsect_read=0, nsect_write=0;
	unsigned long io_ticks=0, rticks=jiffies;

	int index = (sec_diskstats_idx % SEC_DISKSTATS_NR_ENTRIES);

	mutex_lock(&block_subsys_lock);

	/* collect whole disk usage data in here */
	list_for_each(p, &block_subsys.list) {
		disk = list_entry(p, struct gendisk, kobj.entry);
		BUG_ON(!disk);
		sec_diskstats_dump_entry(disk, &nsect_read, &nsect_write, &io_ticks);
	}

	/* printing gnuplot grammar need too much time (about 1~2sec)
	 * So don't save the performance data at this time to synchronize the buffer
	 */
	if( !mutex_trylock(&diskdump_lock) )
	{
		goto diskdump_out;
	}
	sec_diskstats_table[index].sec = kdbg_get_uptime();
	sec_diskstats_table[index].nkbyte_read = SECT_TO_KBYTE(nsect_read - old_nsect_read);
	sec_diskstats_table[index].nkbyte_write = SECT_TO_KBYTE(nsect_write - old_nsect_write);

	BUG_ON(rticks == old_rticks);

	sec_diskstats_table[index].utilization = (io_ticks - old_io_ticks) * 100 / (rticks-old_rticks);
	mutex_unlock(&diskdump_lock);

	/* print turn on */
	if (sec_diskusage_status)
	{
		printk("%4ld Sec       %3lu %%    %5lu KB  %5lu KB  %5lu KB\n", 
				sec_diskstats_table[index].sec,
				sec_diskstats_table[index].utilization, 
				sec_diskstats_table[index].nkbyte_read 
				+ sec_diskstats_table[index].nkbyte_write, 
				sec_diskstats_table[index].nkbyte_read, 
				sec_diskstats_table[index].nkbyte_write );

		if ((index % 20) == 0)
			diskusage_show_header();
	}

	sec_diskstats_idx++;

	if (sec_diskstats_idx == SEC_DISKSTATS_NR_ENTRIES) {
		sec_diskusage_is_buffer_full = 1;
	}

diskdump_out:
	/* save the current data to compare with new data */
	old_nsect_read = nsect_read;
	old_nsect_write = nsect_write;
	old_io_ticks = io_ticks;
	old_rticks = rticks;

	mutex_unlock(&block_subsys_lock);

	return 1;
}

#endif


/**********************************************************************/
/*                                                                    */
/*     sec boot profile collection defines and declarations           */
/*                                                                    */
/**********************************************************************/

#include <linux/delay.h>
#include <linux/vmalloc.h>
#include <linux/proc_fs.h>
#include <linux/kernel_stat.h>
#include <linux/version.h>
#include <linux/utsname.h>
#include <linux/init.h>
#include <linux/cpumask.h>


/* Boot Profile states */

#define SEC_BP_STATE_UNINITED     0  /* not running, no memory
                                      * allocated */
#define SEC_BP_STATE_STARTING     1  /* starting, preparing for
                                      * sampling */
#define SEC_BP_STATE_RUNNING      2  /* running, sample collection
                                      * started */
#define SEC_BP_STATE_COMPLETING   3  /* closing and preparing for
                                      * further actions */
#define SEC_BP_STATE_COMPLETED    4  /* sampling done, profile data is
                                      * available in previously
                                      * allocated memory*/


/* Change it to enable / disable the debug trace prints for boot
 * profile solution.  It is only alocal setting for debugging. */
#define SEC_BP_TRACE_ON    0

/* Optimize the memory usage, calculations are done in the kernel code
 * and summarized results are prepared for report, e.g. all diskstats
 * can be summed up and just one entry per sample can be put in output
 * report */
#define SEC_DS_USE_SUMMARY  0

#if (SEC_BP_TRACE_ON == 0)
#define SEC_BP_TRACE(fmt...) do { } while(0)
#else
#define SEC_BP_TRACE(fmt,args...) do {		\
  printk(KERN_INFO "[BP] %s: " fmt,__FUNCTION__ ,##args); \
  } while(0)
#endif /* #if (SEC_BP_TRACE_ON == 0) */


#define SEC_BP_COMM_NAME  "boot_profile"

#define SEC_BP_START_DELAY_MS  200 /* initial start delay ms */
#define SEC_BP_SAMPLE_INTERVAL_MS     200  /* sampling interval ms */

#define SEC_BP_NR_SAMPLES    240  /* at 0.2 sec sampling interval it
                                     would cover 48 seconds */

#if (SEC_DS_USE_SUMMARY == 0)
/* assume 40 diskstats lines of entries per sample */
#define SEC_DS_NR_ENTRIES    (SEC_BP_NR_SAMPLES * 40)
#else
/* only 1 summarized entry is used */
#define SEC_DS_NR_ENTRIES    (SEC_BP_NR_SAMPLES)
#endif /* #if (SEC_DS_USE_SUMMARY == 0) */

/* assume on average 64 threads per sample */
#define SEC_PS_NR_ENTRIES (SEC_BP_NR_SAMPLES * 128)

#define PROC_STAT_LOG       "proc_stat.log"
#define PROC_PS_LOG         "proc_ps.log"
#define PROC_DISKSTATS_LOG  "proc_diskstats.log"

#define SEC_BP_DIRECT_DUMP           0

#if (SEC_BP_DIRECT_DUMP != 0)
#define SEC_BP_DIRECT_DUMP_START_DELAY_MS  6000
#endif
/* for slowing down the serial output of printk when directly dumping
 * the report on console */
#define DIRECT_DUMP_SLOW_MS  10

#define SEC_BP_LOG_DONE_COUNT   3 /* 3 types of logs are collected
                                   * during sampling */
#define SEC_BP_WAIT_UPTIME_SEC  0 /* start profiling at the specified
                                   * uptime */

#define SEC_BP_PROC_MIN_READ_LEN  (128 + 64) /* minimum supported proc
                                              * read len */
#define SEC_BP_WARN_SMALL_BUF_LEN  0

/* For 'who' entries while filling the report for proc read */
#define SEC_BP_FILL_HEADER      0
#define SEC_BP_FILL_CPUSTAT     1
#define SEC_BP_FILL_PROC_PS     2
#define SEC_BP_FILL_DISKSTATS   3
#define SEC_BP_FILL_DONE        4

/* used for creating the proc buffer (line wise) */
#define SEC_BP_LINE_CACHE_SIZE  1024
/* useful for matching uptime with normal application logs / trace prints */
#define SEC_BP_TRACE_UPTIME     1

#define SEC_BP_COMM_WITH_PID    1
#if (SEC_BP_COMM_WITH_PID != 0)
#define SEC_BP_PROC_PS_FMT_STR  "%d (%s-%u) %c %d 0 0 0 0 0 0 " \
                                "0 0 0 %lu %lu 0 0 0 0 "        \
                                "0 0 %llu 0 0 0 0 0 0 0 "       \
                                "0 0 0 0 0 0 0 0 0 0 0 0 0\n"
#else
#define SEC_BP_PROC_PS_FMT_STR  "%d (%s) %c %d 0 0 0 0 0 0 "    \
                                "0 0 0 %lu %lu 0 0 0 0 "        \
                                "0 0 %llu 0 0 0 0 0 0 0 "       \
                                "0 0 0 0 0 0 0 0 0 0 0 0 0\n"
#endif

#define BOOTCHART_VER_STR   "0.8"
#define BOOTSTATS_HEADER_SIZE  512

    
struct sec_uptime_table
{
    unsigned long uptime;
    int start_idx;
};

struct sec_cpustats
{
    unsigned long uptime; /* uptime in units of 1/100sec*/
    u32 user; /* cputime64_t: only 32 bit value is stored, enough for
               * 462 days = (4000000000/(100 * 60 * 60 * 24))*/
    u32 nice;
    u32 system;
    u32 idle;
    u32 iowait;
    u32 irq;
    u32 softirq;
    u32 steal;
    u32 user_rt;
    u32 system_rt;
};

struct sec_process_stat
{
    pid_t pid;
    char tcomm[FIELD_SIZEOF(struct task_struct,comm)];
    char state;
    pid_t ppid;
    cputime_t utime;
    cputime_t stime;
    unsigned long start_time; /* 32-bit value would not wrap till 462
                               * days */
};

/* BDEVNAME_SIZE is currently 32, we have only small device names, so
 * we can save some space */
#define SEC_DS_BDEVNAME  12

/* stores the disk-stats required for disk utilization and throughput
 * along with the actual jiffy at that moment. */
struct sec_ds_struct
{
    /* As we just store the summarized result, we can pretend it to be
     * a dummy device (just for reporting), so generate the dummy name
     * during report generation, no need to store here in:
     * sec_ds_table[sec_ds_idx].name */
#if (SEC_DS_USE_SUMMARY == 0)
    char name[SEC_DS_BDEVNAME];
#endif
    long nsect_read;
    long nsect_write;
    unsigned io_ticks;
};

static int sec_bp_state = SEC_BP_STATE_UNINITED;

static struct sec_cpustats* sec_cs_table= NULL;
static int sec_cs_idx= 0;

static struct sec_process_stat* sec_ps_table = NULL;
static int sec_ps_idx = 0;
static struct sec_uptime_table* sec_ps_ut_table = NULL;
static int sec_ps_ut_idx = 0;

static struct sec_ds_struct* sec_ds_table = NULL;
static int sec_ds_idx = 0;
static struct sec_uptime_table* sec_ds_ut_table = NULL;
static int sec_ds_ut_idx = 0;

#if (SEC_DS_USE_SUMMARY != 0)
static long ds_sum_nsect_read;
static long ds_sum_nsect_write;
static unsigned ds_sum_io_ticks;
#endif /* #if (SEC_DS_USE_SUMMARY != 0) */

static char *bs_header;

/* added in arch/arm/kernel/setup.c */
extern const char* bc_get_cpu_name(void);

static void sec_bp_start__(void);
inline static void sec_bp_reset(void);
static int sec_bp_alloc(void);
static void sec_bp_dealloc(void);
static int sec_bp_thread(void* arg);
#if (SEC_BP_WAIT_UPTIME_SEC != 0)
inline static void sec_uptime_wait(int nsec);
#endif

inline static int sec_bp_ds_filter(const char* str);
static void sec_bp_log_header(void);
static int sec_bp_log_cpu_stat(void);
static int sec_bp_log_process_stat(void);
static int sec_bp_log_diskstats(void);
inline static void sec_bp_log_ds_entry(struct gendisk* gd);
static void sec_bp_meminfo_dump(void);

static void sec_bp_direct_dump(void);
static void sec_bp_dump_header(void);
static void sec_bp_dump_cpu_stat(void);
static void sec_bp_dump_process_stat(void);
static void sec_bp_dump_diskstats(void);

static int find_usb_device(char *dev);
static int save_to_usb(void);

static inline const char * get_task_state(struct task_struct *tsk);

inline static void sec_bp_state_change(int new_state)
{
    SEC_BP_TRACE("state_change: %d -> %d\n", sec_bp_state, new_state);
    sec_bp_state = new_state;
}

/**********************************************************************
 *
 *  sec_bootstats:  proc related implementation.
 *
 **********************************************************************/

struct bp_stats_proc_filled_info
{
    int who; /* 0= header, 1= cpu_stat, 2= process_stat, 3= diskstats,
                other_value= done */
    int fname_is_done; /* #@fname printed before the stats */
    int next_idx; /* Any value out of range, i.e. < 0 or > available
                   * means end of data. starts with 0. */
    int next_sub_idx; /* index of sub table, if any. */
    int next_idx_is_done;
    off_t next_expected_seq_offset;
    int is_no_more_data;
    char* line_cache;
};

static struct bp_stats_proc_filled_info bp_stats_proc_read_ctx;

static int sec_bp_proc_fill_report(char *buffer, off_t offset,
                                   int buffer_length);
static int sec_bp_proc_fill_header(char* buffer, int buffer_length);
static int sec_bp_proc_fill_cpu_stat(char* buffer, int buffer_length);
static int sec_bp_proc_fill_process_stat(char* buffer, int buffer_length);
static int sec_bp_proc_fill_diskstats(char* buffer, int buffer_length);

static inline int sec_bp_proc_check_length(int buffer_length);


/**********************************************************************
 *                                                                    *
 *              proc related functions                                *
 *                                                                    *
 **********************************************************************/
static void sec_bp_proc_activate(void);

/* for /proc/bootstats writing */
static int sec_bp_proc_write(struct file *file, const char *buffer,
                             unsigned long count, void *data);

/* for /proc/bootstats reading */
static int sec_bp_proc_read(char *buffer, char **buffer_location,
                            off_t offset, int buffer_length, int *eof,
                            void *data);

/* Filter the device names that should be accounted for disk-stats
 * For Chelsea:
 *
 *   skip:     ram0 ... ram 15
 *   consider: tbmlc, tbml1 ... tbml13
 *   consider: bml0/c, bml1 ... bml13
 *   consider: stl1 ... stl13
 *   consider: sda, sdb, sdc, ...
 *   skip:     sda1, sdb1, ...
 *   consider: hda, hdb (hard-disk?)
 */
inline static int sec_bp_ds_filter(const char* str)
{
    /* just do a quick check */
    return ((str[0] == 't' && str[1] == 'b') /* tbml... */
            || (str[0] == 'b' && str[1] == 'm') /* bml... */
            || (str[0] == 's' && str[1] == 't') /* stl.... */
            /* sda, sdb, sdc... but not sda1,... */
            || (str[0] == 's' && str[1] == 'd' && str[3] == '\0')
            /* hda, hdb, hdc... but not hda1,... */
            || (str[0] == 'h' && str[1] == 'd' && str[3] == '\0'));
}


/**********************************************************************
 * some user interace functions (called writing of /proc/bootstats)
 **********************************************************************/

static void sec_bp_print_help(void);
static void sec_bp_print_status(void);
inline static void sec_bp_start(void);


/**********************************************************************/
/*                                                                    */
/*     sec boot profile functions definitions                         */
/*                                                                    */
/**********************************************************************/

void sec_bp_start__(void)
{
    pid_t pid = 0;

    SEC_BP_TRACE("enter\n");
    BUG_ON(sec_bp_state == SEC_BP_STATE_STARTING);
    BUG_ON(sec_bp_state == SEC_BP_STATE_RUNNING);
    sec_bp_state_change(SEC_BP_STATE_STARTING);

    /* changing state to running is done by thread function, if
     * success in creating thread */
	pid = kernel_thread(sec_bp_thread, NULL, CLONE_KERNEL);
	BUG_ON(pid < 0);

	if(likely(pid >= 0)) {
	    struct task_struct* tsk = NULL;

	    read_lock(&tasklist_lock);
	    tsk = find_task_by_pid(pid);
	    if(likely(tsk)) {
            task_lock(tsk);
            strncpy(tsk->comm, SEC_BP_COMM_NAME, sizeof(tsk->comm));
            tsk->comm[sizeof(tsk->comm) - 1] = '\0'; /* for boundary cond. */
            printk("--- [BP] --- pid= %u: " SEC_BP_COMM_NAME
                   " task= (%.20s)\n", pid, tsk->comm);
            task_unlock(tsk);
        }
	    read_unlock(&tasklist_lock);
    } else {
        BUG_ON(sec_bp_state != SEC_BP_STATE_STARTING);
        sec_bp_state_change(SEC_BP_STATE_UNINITED);
    }

    SEC_BP_TRACE("pid= %u\n", pid);
}

/* Note: There is a potential race condition of proc being read
 * when profiling is re-started.  It is not supported to
 * simultaneous read the proc and start the profiling in parallel
 * when proc read is in happening. */
void sec_bp_reset(void)
{
    BUG_ON(sec_bp_state != SEC_BP_STATE_STARTING);

    sec_cs_idx= 0;
    sec_ps_idx = 0;
    sec_ps_ut_idx = 0;
    sec_ds_idx = 0;
    sec_ds_ut_idx = 0;

#if (SEC_DS_USE_SUMMARY != 0)
    ds_sum_nsect_read = 0;
    ds_sum_nsect_write = 0;
    ds_sum_io_ticks = 0;
#endif /* #if (SEC_DS_USE_SUMMARY != 0) */

    memset(&bp_stats_proc_read_ctx, 0, sizeof(bp_stats_proc_read_ctx));
}

int sec_bp_alloc(void)
{
    int ret = 1;
    int len = 0;

    SEC_BP_TRACE("enter\n");
    BUG_ON(sec_bp_state != SEC_BP_STATE_STARTING);

    /* cpu stat */
    if(!sec_cs_table) {
        len = (SEC_BP_NR_SAMPLES
               * sizeof(*sec_cs_table));
        sec_cs_table = vmalloc(len);
        if(!sec_cs_table) {
            printk(KERN_WARNING "%s: cpustat: memory alloc error\n",
                   __FUNCTION__);
            goto alloc_out;
        }
        memset(sec_cs_table, 0, len);
    }

    /* process stat */
    if(!sec_ps_ut_table) {
        len = (SEC_BP_NR_SAMPLES
               * sizeof(*sec_ps_ut_table));
        sec_ps_ut_table = vmalloc(len);
        if(!sec_ps_ut_table) {
            printk(KERN_WARNING "%s: process_stat_ut: memory alloc error\n",
                   __FUNCTION__);
            goto alloc_out;
        }
        memset(sec_ps_ut_table, 0, len);
    }

    if(!sec_ps_table) {
        len = ( SEC_PS_NR_ENTRIES
                * sizeof(*sec_ps_table));
        sec_ps_table = vmalloc(len);
        if(!sec_ps_table) {
            printk(KERN_WARNING "%s: process_stat: memory alloc error\n",
                   __FUNCTION__);
            goto alloc_out;
        }
        memset(sec_ps_table, 0, len);
    }

    /* disk stat */
    if(!sec_ds_ut_table) {
        len = (SEC_BP_NR_SAMPLES
               * sizeof(*sec_ds_ut_table));
        sec_ds_ut_table = vmalloc(len);
        if(!sec_ds_ut_table) {
            printk(KERN_WARNING "%s: diskstats_ut: memory alloc error\n",
                   __FUNCTION__);
            goto alloc_out;
        }
        memset(sec_ds_ut_table, 0, len);
    }

    if(!sec_ds_table) {
        len = ( SEC_DS_NR_ENTRIES
                * sizeof(*sec_ds_table));
        sec_ds_table = vmalloc(len);
        if(!sec_ds_table) {
            printk(KERN_WARNING "%s: diskstats: memory alloc error\n",
                   __FUNCTION__);
            goto alloc_out;
        }
        memset(sec_ds_table, 0, len);
    }

    ret = 0; /* alloc success */
alloc_out:

    SEC_BP_TRACE("exit: ret= %d\n", ret);
    return ret;
}

void sec_bp_dealloc(void)
{
    SEC_BP_TRACE("vfree table\n");
    printk("[BP] %s: releasing memory\n", __FUNCTION__);

    BUG_ON(sec_bp_state != SEC_BP_STATE_COMPLETED);

    /* Note: vfree(addr) performs no-operation when 'addr' is NULL. */

    vfree(sec_cs_table);
    sec_cs_table = NULL;

    vfree(sec_ps_table);
    sec_ps_table = NULL;
    vfree(sec_ps_ut_table);
    sec_ps_ut_table = NULL;

    vfree(sec_ds_table);
    sec_ds_table = NULL;
    vfree(sec_ds_ut_table);
    sec_ds_ut_table = NULL;

    vfree(bs_header);
    bs_header = NULL;

    sec_bp_state_change(SEC_BP_STATE_UNINITED);
}

static int sec_bp_thread(void* arg)
{
    int ret = 0;
    int is_done = 0;

    SEC_BP_TRACE("enter\n");
    BUG_ON(sec_bp_state != SEC_BP_STATE_STARTING);

    /* delay before start collecting the stats */
#if (SEC_BP_START_DELAY_MS != 0)
    msleep(SEC_BP_START_DELAY_MS);
#endif

    sec_bp_reset();
    SEC_BP_TRACE("alloc memory\n");
    ret = sec_bp_alloc();

    if(ret) {
        printk(KERN_WARNING "%s: memory alloc failure\n", __FUNCTION__);
        sec_bp_state_change(SEC_BP_STATE_COMPLETED); /* transition state */
        SEC_BP_TRACE("deallocing memory\n");
        sec_bp_dealloc();
        return 1;
    }

#if (SEC_BP_WAIT_UPTIME_SEC != 0)
    sec_uptime_wait(SEC_BP_WAIT_UPTIME_SEC);
#endif

    for(; !is_done; ) {

        msleep(SEC_BP_SAMPLE_INTERVAL_MS);
        is_done = sec_bp_log_cpu_stat();
        is_done += sec_bp_log_process_stat();
        is_done += sec_bp_log_diskstats();

        /* continue till all are done */
        is_done = (is_done == SEC_BP_LOG_DONE_COUNT) ? 1 : 0;
    }

    sec_bp_log_header();
    sec_bp_state_change(SEC_BP_STATE_COMPLETING); /* transition state */

#if (SEC_BP_DIRECT_DUMP != 0)
    SEC_BP_TRACE("delay for direct dump: sleeping %dms\n",
                 SEC_BP_DIRECT_DUMP_START_DELAY_MS);

    /* Dump the collected log directly to console.  Useful for
     * testing.  The reason for delay is that we can choose suitable
     * moment, to avoid mixing with normal application power up
     * logs. */
    msleep(SEC_BP_DIRECT_DUMP_START_DELAY_MS);

    SEC_BP_TRACE("direct dump\n");
    sec_bp_direct_dump();
#endif /* #if (SEC_BP_DIRECT_DUMP != 0) */

    /* create proc (first time) / reset proc for new data */
    sec_bp_proc_activate();
    sec_bp_meminfo_dump();

    sec_bp_state_change(SEC_BP_STATE_COMPLETED);

    printk("%s: ------------------------------------\n", __FUNCTION__);
    printk("%s: exiting the sec bootprof thread func\n", __FUNCTION__);
    printk("%s: ------------------------------------\n", __FUNCTION__);
    SEC_BP_TRACE("exit\n");
	
	if (!save_to_usb())
		printk("USB write bootstats file fail\n");
	else
		printk("Saving to usb complete!\n");	

    return 0;
}


/**********************************************************************
 *
 *  Boot profile information collection related functions.
 *
 **********************************************************************/

void sec_bp_log_header(void)
{
    if(!bs_header)
    {
        bs_header = vmalloc(BOOTSTATS_HEADER_SIZE);
        if(!bs_header) {
            printk (KERN_ERR "%s: vmalloc failed\n", __FUNCTION__);
            return;
        }
    }

    snprintf(bs_header, BOOTSTATS_HEADER_SIZE,
             "#@header\n"
             "version = " BOOTCHART_VER_STR "\n"
             "title = Boot chart for %s (Sun May 31 21:01:58 IST 2009)\n"
             "system.uname = %s %s %s %s\n"
             "system.release = %s\n"
             "system.cpu = model name	: %s (%u)\n"
             "system.kernel.options = %s\n\n",
             init_uts_ns.name.nodename, init_uts_ns.name.sysname,
             init_uts_ns.name.release, init_uts_ns.name.version,
             init_uts_ns.name.machine, init_uts_ns.name.release,
             bc_get_cpu_name(), num_online_cpus(),
             saved_command_line);
}

int sec_bp_log_cpu_stat(void)
{
	int i;
	cputime64_t user_rt, user, nice, system_rt, system, idle,
        iowait, irq, softirq, steal;
    struct timespec uptime;

    SEC_BP_TRACE("enter sec_bp_log_cpu_stat\n");
    if(sec_cs_idx >= SEC_BP_NR_SAMPLES)
        return 1;

	user_rt = user = nice = system_rt = system = idle = iowait =
		irq = softirq = steal = cputime64_zero;

    do_posix_clock_monotonic_gettime(&uptime);

	for_each_possible_cpu(i) {

		user = cputime64_add(user, kstat_cpu(i).cpustat.user);
		nice = cputime64_add(nice, kstat_cpu(i).cpustat.nice);
		system = cputime64_add(system, kstat_cpu(i).cpustat.system);
		idle = cputime64_add(idle, kstat_cpu(i).cpustat.idle);
		iowait = cputime64_add(iowait, kstat_cpu(i).cpustat.iowait);
		irq = cputime64_add(irq, kstat_cpu(i).cpustat.irq);
		softirq = cputime64_add(softirq, kstat_cpu(i).cpustat.softirq);
		steal = cputime64_add(steal, kstat_cpu(i).cpustat.steal);
		user_rt = cputime64_add(user_rt, kstat_cpu(i).cpustat.user_rt);
		system_rt = cputime64_add(system_rt, kstat_cpu(i).cpustat.system_rt);
	}

	user = cputime64_add(user_rt, user);
	system = cputime64_add(system_rt, system);

    sec_cs_table[sec_cs_idx].uptime
        = ((unsigned long) uptime.tv_sec * 100
           + uptime.tv_nsec / (NSEC_PER_SEC / 100));
    sec_cs_table[sec_cs_idx].user = (u32) user;
    sec_cs_table[sec_cs_idx].nice = (u32) nice;
    sec_cs_table[sec_cs_idx].system = (u32) system;
    sec_cs_table[sec_cs_idx].idle = (u32) idle;
    sec_cs_table[sec_cs_idx].iowait = (u32) iowait;
    sec_cs_table[sec_cs_idx].irq = (u32) irq;
    sec_cs_table[sec_cs_idx].softirq = (u32) softirq;
    sec_cs_table[sec_cs_idx].steal = (u32) steal;
    sec_cs_table[sec_cs_idx].user_rt = (u32) user_rt;
    sec_cs_table[sec_cs_idx].system_rt = (u32) system_rt;
#if (SEC_BP_TRACE_UPTIME != 0)
    printk("--- [BP %lu] ---\n", sec_cs_table[sec_cs_idx].uptime);
#endif
    ++sec_cs_idx;

    SEC_BP_TRACE("normal exit\n");
    return 0;
}

/* Note:
 * ====
 * We should find out why we require tty_mutex lock.
 * Also for task->group_leader is tty_mutex lock used?
 * Find if group_leader is protected by task_lock or tty_mutex.
 */
static int sec_bp_log_process_stat(void)
{
    struct task_struct* g = NULL;
    struct task_struct* p = NULL;
	unsigned long long start_time = 0;
    struct timespec uptime;

    SEC_BP_TRACE("enter\n");

    if(sec_ps_ut_idx >= SEC_BP_NR_SAMPLES
       || sec_ps_idx >= SEC_PS_NR_ENTRIES)
        return 1;

    //mutex_lock(&tty_mutex);
	read_lock(&tasklist_lock);

    do_posix_clock_monotonic_gettime(&uptime);
    sec_ps_ut_table[sec_ps_ut_idx].start_idx
        = sec_ps_idx;

    do_each_thread(g, p) {

        sec_ps_table[sec_ps_idx].pid = p->pid;

        task_lock(p);
        strncpy(sec_ps_table[sec_ps_idx].tcomm,
                p->comm, sizeof(sec_ps_table[sec_ps_idx].tcomm));
        sec_ps_table[sec_ps_idx].tcomm[
            sizeof(sec_ps_table[sec_ps_idx].tcomm) - 1] = '\0';
        task_unlock(p);

        sec_ps_table[sec_ps_idx].state =
            *get_task_state(p);
        sec_ps_table[sec_ps_idx].ppid =
            (pid_alive(p) ? p->group_leader->real_parent->tgid : 0);
        sec_ps_table[sec_ps_idx].utime = p->utime;
        sec_ps_table[sec_ps_idx].stime = p->stime;

        /* Temporary variable needed for gcc-2.96 */
        /* convert timespec -> nsec*/
        start_time = (unsigned long long)p->start_time.tv_sec * NSEC_PER_SEC
            + p->start_time.tv_nsec;
        /* convert nsec -> ticks */
        start_time = nsec_to_clock_t(start_time);
        sec_ps_table[sec_ps_idx].start_time = start_time;
        ++sec_ps_idx;
        if(sec_ps_idx >= SEC_PS_NR_ENTRIES) {
            SEC_BP_TRACE("limit reached sec_ps_idx= %d\n", sec_ps_idx);
            goto thread_out; /* break out of nested loop */
        }
    } while_each_thread(g, p);

thread_out:

    sec_ps_ut_table[sec_ps_ut_idx].uptime
        = ((unsigned long) uptime.tv_sec * 100
           + uptime.tv_nsec / (NSEC_PER_SEC / 100));
    ++sec_ps_ut_idx;

	read_unlock(&tasklist_lock);
    //mutex_unlock(&tty_mutex);

    SEC_BP_TRACE("normal exit\n");
    return 0;
}

/* Note:
 * we can try to check if we can use jiffies as uptime (then need to
 * start with 0 or some near to 0 value, otherwise BootChart will show
 * misleading timeline). */
static int sec_bp_log_diskstats(void)
{
	struct list_head *p= NULL;
    struct gendisk* disk= NULL;
    struct timespec uptime;

    SEC_BP_TRACE("enter\n");

    if(sec_ds_ut_idx >= SEC_BP_NR_SAMPLES
       || sec_ds_idx >= SEC_DS_NR_ENTRIES)
        return 1;

#if (SEC_DS_USE_SUMMARY != 0)
    ds_sum_nsect_read = 0;
    ds_sum_nsect_write = 0;
    ds_sum_io_ticks = 0;
#endif

    sec_ds_ut_table[sec_ds_ut_idx].start_idx = sec_ds_idx;
	mutex_lock(&block_subsys_lock);
    do_posix_clock_monotonic_gettime(&uptime);

	list_for_each(p, &block_subsys.list) {
        disk = list_entry(p, struct gendisk, kobj.entry);
        BUG_ON(!disk);
        sec_bp_log_ds_entry(disk);
        if(sec_ds_idx >= SEC_DS_NR_ENTRIES) {
            SEC_BP_TRACE("limit reached sec_ds_ut_idx= %d\n", sec_ds_idx);
            break;
        }
    }

	mutex_unlock(&block_subsys_lock);

#if (SEC_DS_USE_SUMMARY != 0)
    sec_ds_table[sec_ds_idx].nsect_read
        = ds_sum_nsect_read;
    sec_ds_table[sec_ds_idx].nsect_write
        = ds_sum_nsect_write;
    sec_ds_table[sec_ds_idx].io_ticks
        = ds_sum_io_ticks;
    ++sec_ds_idx;
#endif

    sec_ds_ut_table[sec_ds_ut_idx].uptime
        = ((unsigned long) uptime.tv_sec * 100
           + uptime.tv_nsec / (NSEC_PER_SEC / 100));
    ++sec_ds_ut_idx;

    SEC_BP_TRACE("normal exit\n");
    return 0;
}

inline static void sec_bp_log_ds_entry(struct gendisk* gd)
{
	char buf[BDEVNAME_SIZE];

    SEC_BP_TRACE("enter\n");

    SEC_BP_TRACE("preempt_disable()\n");
	preempt_disable();
	disk_round_stats(gd);
	preempt_enable();
    SEC_BP_TRACE("preempt_enable()\n");

    /* limit of input char array is properly handled in disk_name(),
     * so no need to check overlow case */
    disk_name(gd, 0, buf);

    /* check if does not need to be considered for disk-stats */
    if(sec_bp_ds_filter(buf)) {
#if (SEC_DS_USE_SUMMARY != 0)
        ds_sum_nsect_read += disk_stat_read(gd, sectors[0]);
        ds_sum_nsect_write += disk_stat_read(gd, sectors[1]);
        ds_sum_io_ticks += disk_stat_read(gd, io_ticks);
#else
        strncpy(sec_ds_table[sec_ds_idx].name,
                buf, SEC_DS_BDEVNAME);
        sec_ds_table[sec_ds_idx].name[SEC_DS_BDEVNAME - 1] = '\0';

        sec_ds_table[sec_ds_idx].nsect_read
            = disk_stat_read(gd, sectors[0]);

        sec_ds_table[sec_ds_idx].nsect_write
            = disk_stat_read(gd, sectors[1]);

        sec_ds_table[sec_ds_idx].io_ticks
            = disk_stat_read(gd, io_ticks);
        ++sec_ds_idx;
#endif /* #if (SEC_DS_USE_SUMMARY == 0) */
    }
}


/**********************************************************************
 *
 *  Dump boot profile information to console related functions.
 *
 **********************************************************************/


/* Dump the memory used and actually allocated memory by the internal
 * tables of boot profile soluton.  It is only for debugging and
 * information. */
static void sec_bp_meminfo_dump(void)
{
    const int cpu_used_mem = (sec_cs_idx
                              * sizeof(*sec_cs_table));
    const int cpu_alloc_mem = (SEC_BP_NR_SAMPLES
                               * sizeof(*sec_cs_table));

    const int ps_ut_used_mem = (sec_ps_ut_idx
                                * sizeof(*sec_ps_ut_table));
    const int ps_ut_alloc_mem = (SEC_BP_NR_SAMPLES
                                 * sizeof(*sec_ps_ut_table));
    const int ps_used_mem = (sec_ps_idx
                             * sizeof(*sec_ps_table));
    const int ps_alloc_mem = (SEC_PS_NR_ENTRIES
                              * sizeof(*sec_ps_table));

    const int ds_ut_used_mem = (sec_ds_ut_idx
                                * sizeof(*sec_ds_ut_table));
    const int ds_ut_alloc_mem = (SEC_BP_NR_SAMPLES
                                 * sizeof(*sec_ds_ut_table));
    const int ds_used_mem = (sec_ds_idx
                             * sizeof(*sec_ds_table));
    const int ds_alloc_mem = (SEC_DS_NR_ENTRIES
                              * sizeof(*sec_ds_table));

    BUG_ON(sec_bp_state != SEC_BP_STATE_COMPLETING
           && sec_bp_state != SEC_BP_STATE_COMPLETED);

    printk("\n");
    printk("#   boot prof meminfo\n");
    printk("#\n");
    printk("# used memory   /  allocated memory\n");
    printk("# cpustat memory:  %d x %d = %d / %d\n",
           sec_cs_idx, sizeof(*sec_cs_table),
           cpu_used_mem, cpu_alloc_mem);

    printk("# process stat ut memory:  %d x %d = %d / %d\n",
           sec_ps_ut_idx, sizeof(*sec_ps_ut_table),
           ps_ut_used_mem, ps_ut_alloc_mem);

    printk("# process stat memory: %d x %d = %d / %d\n", 
           sec_ps_idx, sizeof(*sec_ps_table),
           ps_used_mem, ps_alloc_mem);

    printk("# diskstats ut memory: %d x %d = %d / %d\n",
           sec_ds_ut_idx, sizeof(*sec_ds_ut_table),
           ds_ut_used_mem, ds_ut_alloc_mem);

    printk("# diskstats memory: %d x %d = %d / %d\n",
           sec_ds_idx, sizeof(*sec_ds_table),
           ds_used_mem, ds_alloc_mem);

    printk("# Total memory: %d / %d\n",
           (cpu_used_mem + ps_ut_used_mem + ps_used_mem
            + ds_ut_used_mem + ds_used_mem),
           (cpu_alloc_mem + ps_ut_alloc_mem + ps_alloc_mem
            + ds_ut_alloc_mem + ds_alloc_mem));

    printk("#\n");
    printk("\n");
}

static void sec_bp_direct_dump(void)
{
    SEC_BP_TRACE("enter\n");

    BUG_ON(sec_bp_state != SEC_BP_STATE_COMPLETING
           && sec_bp_state != SEC_BP_STATE_COMPLETED);

    sec_bp_dump_header();

    printk("\n");
    printk("# cpu stat start\n");
    printk("\n");
    sec_bp_dump_cpu_stat();

    printk("\n");
    printk("# process stat start\n");
    printk("\n");
    sec_bp_dump_process_stat();

    printk("\n");
    printk("# diskstats start\n");
    printk("\n");
    sec_bp_dump_diskstats();

    printk("\n");
    printk("# boot prof report end\n");
    printk("\n");

}

static void sec_bp_dump_header(void)
{
    printk("#   boot prof header\n");
    printk("#\n");
    printk("\n");
    if(bs_header)
        printk("%s", bs_header);
}

/* for /proc/stat */
static void sec_bp_dump_cpu_stat(void)
{
	int ii = 0;

    BUG_ON(!sec_cs_table);
    printk("#@%s\n", PROC_STAT_LOG);
    for(ii = 0; ii < sec_cs_idx; ++ii) {

        printk("%lu\n", sec_cs_table[ii].uptime);

        printk("cpu  %llu %llu %llu %llu %llu %llu %llu %llu %llu %llu\n",
               (unsigned long long)
               cputime64_to_clock_t(sec_cs_table[ii].user),
               (unsigned long long)
               cputime64_to_clock_t(sec_cs_table[ii].nice),
               (unsigned long long)
               cputime64_to_clock_t(sec_cs_table[ii].system),
               (unsigned long long)
               cputime64_to_clock_t(sec_cs_table[ii].idle),
               (unsigned long long)
               cputime64_to_clock_t(sec_cs_table[ii].iowait),
               (unsigned long long)
               cputime64_to_clock_t(sec_cs_table[ii].irq),
               (unsigned long long)
               cputime64_to_clock_t(sec_cs_table[ii].softirq),
               (unsigned long long)
               cputime64_to_clock_t(sec_cs_table[ii].steal),
               (unsigned long long)
               cputime64_to_clock_t(sec_cs_table[ii].user_rt),
               (unsigned long long)
               cputime64_to_clock_t(sec_cs_table[ii].system_rt));
        printk("\n");
    }
}


/* Note:
 * =====
 *
 *  Do we need to add the time of all the threads also for the
 *  main thread?  refer to array.c, I think it is not required.
 *  We should keep eye on 'whole' argument, when received as one
 *  in array.c function? */
static void sec_bp_dump_process_stat(void)
{
	int ii = 0;
    int jj = 0;
    int end_idx = 0;

    BUG_ON(!sec_ps_ut_table);
    BUG_ON(!sec_ps_table);

    printk("#@%s\n", PROC_PS_LOG);
    for(ii = 0; ii < sec_ps_ut_idx; ++ii) {

        printk("%lu\n", sec_ps_ut_table[ii].uptime);

        if(ii == sec_ps_ut_idx - 1)
            end_idx = sec_ps_idx; /* last record limit */
        else
            end_idx = sec_ps_ut_table[ii+1].start_idx;

        for(jj = sec_ps_ut_table[ii].start_idx; jj < end_idx; ++jj)
            printk(SEC_BP_PROC_PS_FMT_STR,
                   sec_ps_table[jj].pid,
                   sec_ps_table[jj].tcomm,
#if (SEC_BP_COMM_WITH_PID != 0)
                   sec_ps_table[jj].pid, /* append to tcomm */
#endif
                   sec_ps_table[jj].state,
                   sec_ps_table[jj].ppid,
                   cputime_to_clock_t(sec_ps_table[jj].utime),
                   cputime_to_clock_t(sec_ps_table[jj].stime),
                   (unsigned long long)sec_ps_table[jj].start_time);

        printk("\n");
        msleep(DIRECT_DUMP_SLOW_MS);
    }
}

static void sec_bp_dump_diskstats(void)
{
	int ii = 0;
    int jj = 0;
    int end_idx = 0;

    BUG_ON(!sec_ds_ut_table);
    BUG_ON(!sec_ds_table);

    printk("#@%s\n", PROC_DISKSTATS_LOG);
    for(ii = 0; ii < sec_ds_ut_idx; ++ii) {

        printk("%lu\n", sec_ds_ut_table[ii].uptime);

        if(ii == sec_ds_ut_idx - 1)
            end_idx = sec_ds_idx; /* last record limit */
        else
            end_idx = sec_ds_ut_table[ii+1].start_idx;

        for(jj = sec_ds_ut_table[ii].start_idx; jj < end_idx; ++jj)
            /* major first_minor name ios[0] merges[0] nsect_read
               ticks[0] ios[1] merges[1] nsect_read sect_write
               ticks[1] in_flight io_ticks time_in_queue */
            /* "%4d %4d %s %lu %lu %llu %u %lu %lu %llu %u %u %u %u\n" */
            printk("0 0 %s 0 0 %llu 0 0 0 %llu 0 0 %u 0\n",
#if (SEC_DS_USE_SUMMARY == 0)
                   sec_ds_table[jj].name,
#else
                   "sum_bdev",
#endif
                   (unsigned long long)sec_ds_table[jj].nsect_read,
                   (unsigned long long)sec_ds_table[jj].nsect_write,
                   jiffies_to_msecs(sec_ds_table[jj].io_ticks));
        printk("\n");
        msleep(DIRECT_DUMP_SLOW_MS);
    }
}

/**********************************************************************/
/*                                                                    */
/*     sec boot profile utility functions                             */
/*                                                                    */
/**********************************************************************/

/*
 * The task state array is a strange "bitmap" of
 * reasons to sleep. Thus "running" is zero, and
 * you can test for combinations of others with
 * simple bit tests.
 */
static const char *task_state_array[] = {
	"R (running)",		/*  0 */
	"M (running-mutex)",	/*  1 */
	"S (sleeping)",		/*  2 */
	"D (disk sleep)",	/*  4 */
	"T (stopped)",		/*  8 */
	"T (tracing stop)",	/* 16 */
	"Z (zombie)",		/* 32 */
	"X (dead)"		/* 64 */
};

static inline const char * get_task_state(struct task_struct *tsk)
{
	unsigned int state = (tsk->state & (TASK_RUNNING |
                                        TASK_RUNNING_MUTEX |
                                        TASK_INTERRUPTIBLE |
                                        TASK_UNINTERRUPTIBLE |
                                        TASK_STOPPED |
                                        TASK_TRACED)) |
        (tsk->exit_state & (EXIT_ZOMBIE |
                            EXIT_DEAD));
	const char **p = &task_state_array[0];

	while (state) {
		p++;
		state >>= 1;
	}
	return *p;
}


/**********************************************************************
 *
 *  sec_bootstats:  proc related implementation.
 *
 **********************************************************************/

static void sec_bp_proc_activate(void)
{
    static struct proc_dir_entry *sec_bootstats_proc_dentry = NULL;
    SEC_BP_TRACE("enter\n");
    BUG_ON(sec_bp_state != SEC_BP_STATE_COMPLETING);

    if(!sec_bootstats_proc_dentry) {
        SEC_BP_TRACE("creating /proc/bootstats\n");
        sec_bootstats_proc_dentry = create_proc_entry("bootstats", 0, NULL);
        
        if(sec_bootstats_proc_dentry) {
            sec_bootstats_proc_dentry->read_proc  = sec_bp_proc_read;
            sec_bootstats_proc_dentry->write_proc = sec_bp_proc_write;
        } else
            printk(KERN_WARNING "/proc/bootstats creation failed\n");
    }
}

/* This function is called with the /proc file is written */
int sec_bp_proc_write(struct file *file, const char *buffer,
                      unsigned long count, void *data)
{
    long ilength = -EFAULT;

    SEC_BP_TRACE("(file= %p, buffer= %p, count= %lu, data= %p)\n",
                 file, buffer, count, data);

    (void) file; /* not used, well except for trace printks */
    (void) data; /* not used */

    /* we never expect it to be null, but put a condition to handle it
       is release builds */
    BUG_ON(!buffer);

    if(!buffer) {
        printk (KERN_ERR "buffer is null\n");
        ilength =  -EFAULT;

    } else if(count > 0) {
        char cinput[4];

        ilength = (count >= sizeof(cinput) ? sizeof(cinput) - 1 : count);
        /* write data to the buffer */
        if ( copy_from_user(cinput, buffer, ilength)) {
            printk(KERN_WARNING "%s: Error in copy_from_user: Returning\n",
                   __FUNCTION__);
            return -EPERM;
        }

        cinput[ilength] = '\0';
        SEC_BP_TRACE("buf@3= (%s)\n", cinput);
        switch(cinput[0]) {
            case 'h':
                sec_bp_print_help();
                break;
            case '1':
                /* (re)start the profiling. */
                if(sec_bp_state == SEC_BP_STATE_UNINITED
                   || sec_bp_state == SEC_BP_STATE_COMPLETED) {
                    printk(KERN_INFO "%s: Profiling OFF -> active\n", __FUNCTION__);
                    sec_bp_start();
                } else {
                    BUG_ON(sec_bp_state != SEC_BP_STATE_RUNNING
                           && sec_bp_state != SEC_BP_STATE_STARTING
                           && sec_bp_state != SEC_BP_STATE_COMPLETING);
                    printk(KERN_INFO "%s: Profiling already active\n", __FUNCTION__);
                }
                break;
            case 'd':
                if(sec_bp_state == SEC_BP_STATE_COMPLETED)
                    sec_bp_direct_dump();
                else
                    printk(KERN_INFO "%s: not in finished state (%d)\n",
                           __FUNCTION__, sec_bp_state);
                break;
            case 's':
                sec_bp_print_status();
                break;
            case 'c':
                if(sec_bp_state == SEC_BP_STATE_COMPLETED) {
                    printk(KERN_INFO "%s: dealloc memory\n", __FUNCTION__);
                    sec_bp_dealloc();
                    BUG_ON(sec_bp_state != SEC_BP_STATE_UNINITED);
                } else if(sec_bp_state == SEC_BP_STATE_UNINITED) {
                    printk(KERN_INFO "%s: memory is already released\n",
                           __FUNCTION__);
                } else {
                    BUG_ON(sec_bp_state != SEC_BP_STATE_STARTING
                           && sec_bp_state != SEC_BP_STATE_RUNNING
                           && sec_bp_state != SEC_BP_STATE_COMPLETING);
                    printk(KERN_INFO "%s: ERR: currently running\n",
                           __FUNCTION__);
                }

                break;
            default:
                printk(KERN_WARNING "%s: invalid choice [0x%x, %c]\n",
                       __FUNCTION__, cinput[0], cinput[0]);
        }

        /* we need to pretend that we consumed all the buffer, so that
         * the system feels ok and does not attempt to re-write
         * again */
        ilength = count;

    } else {
        printk(KERN_WARNING "%s: insufficient data, count= %ld\n",
               __FUNCTION__, count);
        ilength = -EFAULT;
    }

    return ilength;
}

/* Called when /proc/bootstats is read.
 * Note:
 * It is not re-entrant function. Right now it does not require to be
 * re-entrant, unless some user tries to access it from two shells, or
 * more than one program / thread.  Do not access this module for
 * output report simultaneous from more than one place, e.g. kdebugd
 * and user or from more than one threads.
 * 
 * We do not support a small buffer read, also we never support
 * seeking.  Users should read a big buffer say 512 or 1024 bytes and
 * typically a line size may be around 80 characters. We may actually
 * fill less data, in that case we return that as return value, but we
 * always expect a good size buffer. */
int sec_bp_proc_read(char *buffer, char **buffer_location,
                     off_t offset, int buffer_length, int *eof,
                     void *data)
{
    int len_copied = 0;
    int ret = -EFAULT;

    SEC_BP_TRACE("(buffer= %p, buffer_location= %p, "
                 "offset= %ld, buffer_length= %d, eof= %p, data= %p)\n",
                 buffer, buffer_location, offset, buffer_length, eof, data);

    /* It is required, otherwise for large proc data next read does
       not happen with proper offset. */
    *buffer_location = buffer;
    (void) data; /* not used */

    if(sec_bp_state != SEC_BP_STATE_COMPLETED) {

        printk(KERN_WARNING "ERR: boot-profiling is "
               "not completed, state= %d\n", sec_bp_state);
        ret = -ENOENT;
        goto proc_read_out;

    } else if(!sec_bp_proc_check_length(buffer_length)) {
#if (SEC_BP_WARN_SMALL_BUF_LEN != 0)
        /* It clashes with normal prints. */
        printk(KERN_WARNING "++++++++++++++++++++++++++++++++++++++\n");
        printk(KERN_WARNING "======================================\n");
        printk(KERN_WARNING "buffer_length= %d:  "
               "small length not supported\n", buffer_length);
        printk(KERN_WARNING "======================================\n");
        printk(KERN_WARNING "++++++++++++++++++++++++++++++++++++++\n");
#endif
        ret = -ENOBUFS;
        goto proc_read_out;

    } else if(!buffer) {
        printk(KERN_WARNING "input buffer is null\n");
        ret = -EFAULT;
        goto proc_read_out;

    } else if(offset == 0) {
        SEC_BP_TRACE("offset == 0: ~~~~~~~~\n");

        /* we need to reset here, e.g. if there is any previous
         * incomplete read */
        bp_stats_proc_read_ctx.next_expected_seq_offset = 0;
        bp_stats_proc_read_ctx.is_no_more_data = 0;
        bp_stats_proc_read_ctx.who = SEC_BP_FILL_HEADER; /* start */
        bp_stats_proc_read_ctx.next_idx = 0;
        bp_stats_proc_read_ctx.next_sub_idx = 0;
        bp_stats_proc_read_ctx.next_idx_is_done = 0;
        bp_stats_proc_read_ctx.fname_is_done = 0;

    } else if(bp_stats_proc_read_ctx.is_no_more_data) {
        SEC_BP_TRACE("is_eof: ~~~~~~~~~~~~~~\n");
        *eof = 1;
        ret = 0;
        goto proc_read_out;

    } else if(offset != bp_stats_proc_read_ctx.next_expected_seq_offset) {
        printk(KERN_WARNING "read in random order, not supported, "
               "offset = %ld vs %ld\n",
               bp_stats_proc_read_ctx.next_expected_seq_offset, offset);
        *eof = 1;
        ret = -ESPIPE;
        goto proc_read_out;
    }

    len_copied = sec_bp_proc_fill_report(buffer, offset, buffer_length);
    ret = len_copied;
    SEC_BP_TRACE("len_copied= %d\n", len_copied);

proc_read_out:

    return ret;
}

static int sec_bp_proc_fill_report(char *buffer, off_t offset,
                                   int buffer_length)
{
    int filled_len = 0;

    SEC_BP_TRACE("offset= %ld, buffer_length= %d\n",
                 (long)offset, buffer_length);

    if(!bp_stats_proc_read_ctx.line_cache) {

        SEC_BP_TRACE("alloca line cache\n");
        bp_stats_proc_read_ctx.line_cache =
            (char*) vmalloc(SEC_BP_LINE_CACHE_SIZE);
        if(!bp_stats_proc_read_ctx.line_cache) {
            printk(KERN_WARNING "out of memory\n");
            return -ENOMEM;
        }
    }

    BUG_ON(bp_stats_proc_read_ctx.is_no_more_data);

    /* header */
    if(bp_stats_proc_read_ctx.who == SEC_BP_FILL_HEADER) {
        SEC_BP_TRACE("who == %d\n", bp_stats_proc_read_ctx.who);
        filled_len += sec_bp_proc_fill_header(buffer, buffer_length);
    }

    /* proc_stat */
    if(bp_stats_proc_read_ctx.who == SEC_BP_FILL_CPUSTAT
       && filled_len < buffer_length) {
        SEC_BP_TRACE("who == %d\n", bp_stats_proc_read_ctx.who);
        filled_len += sec_bp_proc_fill_cpu_stat(buffer + filled_len,
                                                buffer_length - filled_len);
    }

    /* proc_ps_stat */
    if(bp_stats_proc_read_ctx.who == SEC_BP_FILL_PROC_PS
       && filled_len < buffer_length) {
        SEC_BP_TRACE("who == %d\n", bp_stats_proc_read_ctx.who);
        filled_len += sec_bp_proc_fill_process_stat(buffer + filled_len,
                                                    buffer_length - filled_len);
    }

    /* proc_diskstats */
    if(bp_stats_proc_read_ctx.who == SEC_BP_FILL_DISKSTATS
       && filled_len < buffer_length) {
        SEC_BP_TRACE("who == %d\n", bp_stats_proc_read_ctx.who);
        filled_len += sec_bp_proc_fill_diskstats(buffer + filled_len,
                                                 buffer_length - filled_len);
    }

    /* last step */
    if(bp_stats_proc_read_ctx.who == SEC_BP_FILL_DONE) {
        SEC_BP_TRACE("who == %d\n", bp_stats_proc_read_ctx.who);
        bp_stats_proc_read_ctx.is_no_more_data = 1;
        vfree(bp_stats_proc_read_ctx.line_cache);
        bp_stats_proc_read_ctx.line_cache = NULL;
    } else {
        BUG_ON(bp_stats_proc_read_ctx.who < SEC_BP_FILL_HEADER
               || bp_stats_proc_read_ctx.who >= SEC_BP_FILL_DONE);
    }

    SEC_BP_TRACE("filled_len= %d\n", filled_len);
    BUG_ON(filled_len > buffer_length);
    return filled_len;
}

static int sec_bp_proc_fill_header(char* buffer, int buffer_length)
{
    int filled_len = 0;
    int bs_header_len = 0;
    BUG_ON(bp_stats_proc_read_ctx.who != SEC_BP_FILL_HEADER);
    BUG_ON(!bs_header);

    if(!bs_header) {  /* silently skip header */
        bp_stats_proc_read_ctx.next_idx = 0;
        ++bp_stats_proc_read_ctx.who;
        return 0;
    }

    bs_header_len = strlen(bs_header);
    if(bp_stats_proc_read_ctx.next_idx < bs_header_len) {
        filled_len = (bs_header_len
                      - bp_stats_proc_read_ctx.next_idx); /* remaining len */
        filled_len = min(buffer_length, filled_len); /* adjusted len */
        memcpy(buffer, bs_header + bp_stats_proc_read_ctx.next_idx,
               filled_len);
        bp_stats_proc_read_ctx.next_idx += filled_len;
    } else {
        bp_stats_proc_read_ctx.next_idx = 0;
        ++bp_stats_proc_read_ctx.who;
    }

    bp_stats_proc_read_ctx.next_expected_seq_offset += filled_len;
    bp_stats_proc_read_ctx.is_no_more_data = 0;

    return filled_len;
}

static int sec_bp_proc_fill_cpu_stat(char* buffer, int buffer_length)
{
	int ii = 0;
    int line_len = 0;
    int len_copied = 0;
    int next_idx_done_cnt = 2;

    SEC_BP_TRACE("buffer_length= %d\n", buffer_length);

    SEC_BP_TRACE("next_idx= %d, next_sub_idx= %d, next_idx_done_cnt= %d\n",
                 bp_stats_proc_read_ctx.next_idx,
                 bp_stats_proc_read_ctx.next_sub_idx,
                 bp_stats_proc_read_ctx.next_idx_is_done);

    if(!bp_stats_proc_read_ctx.fname_is_done) {

        BUG_ON(bp_stats_proc_read_ctx.next_idx
               || bp_stats_proc_read_ctx.next_idx_is_done);

        line_len = sizeof("#@" PROC_STAT_LOG "\n") - 1;
        if(line_len > buffer_length) {
            SEC_BP_TRACE("#@%s: line_len= %d > %d: -- 00 --\n",
                         PROC_STAT_LOG, line_len, buffer_length);
            return 0;
        }

        memcpy(buffer, "#@" PROC_STAT_LOG "\n", line_len);
        SEC_BP_TRACE("ii= %d: [0  => %d]: %d\n",
                     ii, line_len, buffer_length);
        len_copied = line_len;
        next_idx_done_cnt = 0;
        bp_stats_proc_read_ctx.fname_is_done = 1;
    }       

    for(ii = bp_stats_proc_read_ctx.next_idx; ii < sec_cs_idx; ++ii) {

        next_idx_done_cnt = 0;

        if(ii != bp_stats_proc_read_ctx.next_idx
           || !bp_stats_proc_read_ctx.next_idx_is_done) {

            SEC_BP_TRACE("first idx not not complete: idx= %d\n",
                         bp_stats_proc_read_ctx.next_idx);

            line_len = sprintf(bp_stats_proc_read_ctx.line_cache, "%lu\n",
                               sec_cs_table[ii].uptime);
            
            if(len_copied + line_len > buffer_length) {
                SEC_BP_TRACE("len_copied + line_len = %d + %d > %d: -- 0 --\n",
                             len_copied, line_len, buffer_length);
                break;
            }

            memcpy(buffer + len_copied, bp_stats_proc_read_ctx.line_cache,
                   line_len);
            SEC_BP_TRACE("ii= %d: [%d + %d => %d]: %d\n",
                         ii, len_copied, line_len,
                         len_copied + line_len, buffer_length);
            len_copied += line_len;
        }

        next_idx_done_cnt = 1;

        SEC_BP_TRACE("ii= %d: fill_stats\n", ii);

        line_len = sprintf(bp_stats_proc_read_ctx.line_cache,
                           "cpu  %llu %llu %llu %llu %llu "
                           "%llu %llu %llu %llu %llu\n\n",
                           (unsigned long long)
                           cputime64_to_clock_t(sec_cs_table[ii].user),
                           (unsigned long long)
                           cputime64_to_clock_t(sec_cs_table[ii].nice),
                           (unsigned long long)
                           cputime64_to_clock_t(sec_cs_table[ii].system),
                           (unsigned long long)
                           cputime64_to_clock_t(sec_cs_table[ii].idle),
                           (unsigned long long)
                           cputime64_to_clock_t(sec_cs_table[ii].iowait),
                           (unsigned long long)
                           cputime64_to_clock_t(sec_cs_table[ii].irq),
                           (unsigned long long)
                           cputime64_to_clock_t(sec_cs_table[ii].softirq),
                           (unsigned long long)
                           cputime64_to_clock_t(sec_cs_table[ii].steal),
                           (unsigned long long)
                           cputime64_to_clock_t(sec_cs_table[ii].user_rt),
                           (unsigned long long)
                           cputime64_to_clock_t(sec_cs_table[ii].system_rt));

        if(len_copied + line_len > buffer_length) {
            break;
        }

        memcpy(buffer + len_copied, bp_stats_proc_read_ctx.line_cache,
               line_len);
        SEC_BP_TRACE("ii= %d:  [%d + %d => %d]: %d\n",
                     ii, len_copied, line_len,
                     len_copied + line_len, buffer_length);
        len_copied += line_len;

        next_idx_done_cnt = 2;
    }

    bp_stats_proc_read_ctx.next_expected_seq_offset += len_copied;
    bp_stats_proc_read_ctx.is_no_more_data = 0;


    SEC_BP_TRACE("ii= %d: next_idx= %d, "
                 "next_sub_idx= %d, next_idx_done_cnt= %d: -- Pre ---\n",
                 ii, bp_stats_proc_read_ctx.next_idx,
                 bp_stats_proc_read_ctx.next_sub_idx, next_idx_done_cnt);

    BUG_ON(!bp_stats_proc_read_ctx.fname_is_done);
    if(ii == sec_cs_idx) {
        SEC_BP_TRACE("ii= %d: Data finished\n", ii);
        ++bp_stats_proc_read_ctx.who;
        bp_stats_proc_read_ctx.next_idx = 0;
        bp_stats_proc_read_ctx.next_sub_idx = 0;
        BUG_ON(next_idx_done_cnt != 2);
        bp_stats_proc_read_ctx.next_idx_is_done = 0;
        /* bp_stats_proc_read_ctx.is_no_more_data = 1; */
        bp_stats_proc_read_ctx.fname_is_done = 0;
    }
    else {
        SEC_BP_TRACE("ii= %d: more_data\n", ii);
        BUG_ON(!(next_idx_done_cnt == 0 || next_idx_done_cnt == 1));
        BUG_ON(bp_stats_proc_read_ctx.who != SEC_BP_FILL_CPUSTAT);
        bp_stats_proc_read_ctx.next_idx = ii;
        bp_stats_proc_read_ctx.next_sub_idx = 0; /* not used */
        bp_stats_proc_read_ctx.next_idx_is_done = (next_idx_done_cnt != 0);
    }

    SEC_BP_TRACE("ii= %d: next_idx= %d, "
                 "next_sub_idx= %d, next_idx_done_cnt= %d: -- Post ---\n",
                 ii, bp_stats_proc_read_ctx.next_idx,
                 bp_stats_proc_read_ctx.next_sub_idx,
                 bp_stats_proc_read_ctx.next_idx_done_cnt);

    BUG_ON(len_copied > buffer_length);
    return len_copied;
}

static int sec_bp_proc_fill_process_stat(char* buffer, int buffer_length)
{
	int ii = 0;
    int jj = 0;
    int end_idx = 0;
    int line_len = 0;
    int len_copied = 0;
    int next_idx_is_done = 0;
    int is_loop_break = 0;

    SEC_BP_TRACE("buffer_length= %d\n", buffer_length);

    SEC_BP_TRACE("next_idx= %d, next_sub_idx= %d, next_idx_is_done= %d\n",
                 bp_stats_proc_read_ctx.next_idx,
                 bp_stats_proc_read_ctx.next_sub_idx,
                 bp_stats_proc_read_ctx.next_idx_is_done);

    if(!bp_stats_proc_read_ctx.fname_is_done) {

        BUG_ON(bp_stats_proc_read_ctx.next_idx
               || bp_stats_proc_read_ctx.next_idx_is_done);

        line_len = sizeof("#@" PROC_PS_LOG "\n") - 1;
        if(line_len > buffer_length) {
            SEC_BP_TRACE("#@%s: line_len= %d > %d: -- 00 --\n",
                         PROC_PS_LOG, line_len, buffer_length);
            return 0;
        }

        memcpy(buffer, "#@" PROC_PS_LOG "\n", line_len);
        SEC_BP_TRACE("ii= %d: [0  => %d]: %d\n",
                     ii, line_len, buffer_length);
        len_copied = line_len;
        bp_stats_proc_read_ctx.fname_is_done = 1;
    }       

    for(ii = bp_stats_proc_read_ctx.next_idx;
        !is_loop_break && ii < sec_ps_ut_idx; ++ii) {

        if(ii != bp_stats_proc_read_ctx.next_idx
           || !bp_stats_proc_read_ctx.next_idx_is_done) {

            SEC_BP_TRACE("first idx not not complete: idx= %d\n",
                         bp_stats_proc_read_ctx.next_idx);

            line_len = sprintf(bp_stats_proc_read_ctx.line_cache, "%lu\n",
                               sec_ps_ut_table[ii].uptime);
            
            if(len_copied + line_len > buffer_length) {
                SEC_BP_TRACE("len_copied + line_len = %d + %d > %d: -- 0 --\n",
                             len_copied, line_len, buffer_length);
                next_idx_is_done = 0;
                break;
            }

            memcpy(buffer + len_copied, bp_stats_proc_read_ctx.line_cache,
                   line_len);
            SEC_BP_TRACE("ii= %d: [%d + %d => %d]: %d\n",
                         ii, len_copied, line_len,
                         len_copied + line_len, buffer_length);
            len_copied += line_len;

            jj = sec_ps_ut_table[ii].start_idx;
        }
        else {
            jj = bp_stats_proc_read_ctx.next_sub_idx;
        }
        SEC_BP_TRACE("ii= %d: first jj= %d\n", ii, jj);

        next_idx_is_done = 1;
        if(ii == sec_ps_ut_idx - 1)
            end_idx = sec_ps_idx;
        else
            end_idx = sec_ps_ut_table[ii+1].start_idx;

        for(; jj < end_idx; ++jj) {
            line_len = sprintf(bp_stats_proc_read_ctx.line_cache,
                               SEC_BP_PROC_PS_FMT_STR,
                               sec_ps_table[jj].pid,
                               sec_ps_table[jj].tcomm,
#if (SEC_BP_COMM_WITH_PID != 0)
                               sec_ps_table[jj].pid, /* append to tcomm */
#endif
                               sec_ps_table[jj].state,
                               sec_ps_table[jj].ppid,
                               cputime_to_clock_t(sec_ps_table[jj].utime),
                               cputime_to_clock_t(sec_ps_table[jj].stime),
                               (unsigned long long)sec_ps_table[jj].start_time);

            /* lalit: added +1 because we have a terminating new line
               at end of the block (see below). */
            if(len_copied + line_len + 1 > buffer_length) {
                is_loop_break = 1;
                break;
            }
            
            memcpy(buffer + len_copied, bp_stats_proc_read_ctx.line_cache, line_len);
            SEC_BP_TRACE("ii= %d, jj= %d: [%d + %d => %d]: %d\n",
                         ii, jj, len_copied, line_len,
                         len_copied + line_len, buffer_length);
            len_copied += line_len;
        }

        /* Do not add new line if upper loop finished pre-maturely (we
         * had to break because of buffer full, net read call will
         * resume again */
        if(!is_loop_break) {
            SEC_BP_TRACE("ii= %d: [%d + NL => %d]: %d\n",
                         ii, len_copied,
                         len_copied + 1, buffer_length);
            BUG_ON(len_copied >= buffer_length);
            buffer[len_copied++] = '\n';
        }
    }

    bp_stats_proc_read_ctx.next_expected_seq_offset += len_copied;
    bp_stats_proc_read_ctx.is_no_more_data = 0;


    SEC_BP_TRACE("ii= %d, jj= %d: next_idx= %d, "
                 "next_sub_idx= %d, next_idx_is_done= %d: -- Pre ---\n",
                 ii, jj, bp_stats_proc_read_ctx.next_idx,
                 bp_stats_proc_read_ctx.next_sub_idx, next_idx_is_done);

    BUG_ON(!bp_stats_proc_read_ctx.fname_is_done);
    if(ii == sec_ps_ut_idx && jj == end_idx) {
        SEC_BP_TRACE("ii= %d, jj= %d: Data finished\n", ii, jj);
        ++bp_stats_proc_read_ctx.who;
        bp_stats_proc_read_ctx.next_idx = 0;
        bp_stats_proc_read_ctx.next_sub_idx = 0;
        BUG_ON(is_loop_break);
        bp_stats_proc_read_ctx.next_idx_is_done = 0;
        /* bp_stats_proc_read_ctx.is_no_more_data = 1; */
        bp_stats_proc_read_ctx.fname_is_done = 0;
    }
    else {
        SEC_BP_TRACE("ii= %d, jj= %d: more_data\n", ii, jj);
        BUG_ON(bp_stats_proc_read_ctx.who != SEC_BP_FILL_PROC_PS);
        bp_stats_proc_read_ctx.next_idx = is_loop_break ? ii - 1 : ii;
        bp_stats_proc_read_ctx.next_sub_idx = jj;
        bp_stats_proc_read_ctx.next_idx_is_done = next_idx_is_done;
    }

    SEC_BP_TRACE("ii= %d, jj= %d: next_idx= %d, "
                 "next_sub_idx= %d, next_idx_is_done= %d: -- Post ---\n",
                 ii, jj, bp_stats_proc_read_ctx.next_idx,
                 bp_stats_proc_read_ctx.next_sub_idx,
                 bp_stats_proc_read_ctx.next_idx_is_done);

    BUG_ON(len_copied > buffer_length);
    return len_copied;
}

static int sec_bp_proc_fill_diskstats(char* buffer, int buffer_length)
{
	int ii = 0;
    int jj = 0;
    int end_idx = 0;
    int line_len = 0;
    int len_copied = 0;
    int next_idx_is_done = 0;
    int is_loop_break = 0;

    SEC_BP_TRACE("buffer_length= %d\n", buffer_length);

    SEC_BP_TRACE("next_idx= %d, next_sub_idx= %d, next_idx_is_done= %d\n",
                 bp_stats_proc_read_ctx.next_idx,
                 bp_stats_proc_read_ctx.next_sub_idx,
                 bp_stats_proc_read_ctx.next_idx_is_done);

    if(!bp_stats_proc_read_ctx.fname_is_done) {

        BUG_ON(bp_stats_proc_read_ctx.next_idx
               || bp_stats_proc_read_ctx.next_idx_is_done);

        line_len = sizeof("#@" PROC_DISKSTATS_LOG "\n") - 1;
        if(line_len > buffer_length) {
            SEC_BP_TRACE("#@%s: line_len = %d  > %d: -- 00 --\n",
                         PROC_DISKSTATS_LOG, line_len, buffer_length);
            return 0;
        }

        memcpy(buffer, "#@" PROC_DISKSTATS_LOG "\n", line_len);
        SEC_BP_TRACE("ii= %d: [0  => %d]: %d\n",
                     ii, line_len, buffer_length);
        len_copied = line_len;
        bp_stats_proc_read_ctx.fname_is_done = 1;
    }

    for(ii = bp_stats_proc_read_ctx.next_idx;
        !is_loop_break && ii < sec_ds_ut_idx; ++ii) {

        if(ii != bp_stats_proc_read_ctx.next_idx
           || !bp_stats_proc_read_ctx.next_idx_is_done) {

            SEC_BP_TRACE("first idx not not complete: idx= %d\n",
                         bp_stats_proc_read_ctx.next_idx);

            line_len = sprintf(bp_stats_proc_read_ctx.line_cache, "%lu\n",
                               sec_ds_ut_table[ii].uptime);
            
            if(len_copied + line_len > buffer_length) {
                SEC_BP_TRACE("len_copied + line_len = %d + %d > %d: -- 0 --\n",
                             len_copied, line_len, buffer_length);
                next_idx_is_done = 0;
                break;
            }

            memcpy(buffer + len_copied, bp_stats_proc_read_ctx.line_cache,
                   line_len);
            SEC_BP_TRACE("ii= %d: [%d + %d => %d]: %d\n",
                         ii, len_copied, line_len,
                         len_copied + line_len, buffer_length);
            len_copied += line_len;

            jj = sec_ds_ut_table[ii].start_idx;
        } else {
            jj = bp_stats_proc_read_ctx.next_sub_idx;
        }
        SEC_BP_TRACE("ii= %d: first jj= %d\n", ii, jj);

        next_idx_is_done = 1;
        if(ii == sec_ds_ut_idx - 1)
            end_idx = sec_ds_idx;
        else
            end_idx = sec_ds_ut_table[ii+1].start_idx;

        for(; jj < end_idx; ++jj) {
            /* major first_minor name ios[0] merges[0] nsect_read
               ticks[0] ios[1] merges[1] nsect_read sect_write
               ticks[1] in_flight io_ticks time_in_queue */
            /* "%4d %4d %s %lu %lu %llu %u %lu %lu %llu %u %u %u %u\n" */
            line_len = sprintf(bp_stats_proc_read_ctx.line_cache,
                               "0 0 %s 0 0 %llu 0 0 0 %llu 0 0 %u 0\n",
#if (SEC_DS_USE_SUMMARY == 0)
                               sec_ds_table[jj].name,
#else
                               "sum_bdev",
#endif
                               (unsigned long long)
                               sec_ds_table[jj].nsect_read,
                               (unsigned long long)
                               sec_ds_table[jj].nsect_write,
                               jiffies_to_msecs(sec_ds_table[jj].io_ticks));

            /* lalit: added +1 because we have a terminating new line
               at end of the block (see below). */
            if(len_copied + line_len + 1 > buffer_length) {
                is_loop_break = 1;
                break;
            }
            
            memcpy(buffer + len_copied, bp_stats_proc_read_ctx.line_cache, line_len);
            SEC_BP_TRACE("ii= %d, jj= %d: [%d + %d => %d]: %d\n",
                         ii, jj, len_copied, line_len,
                         len_copied + line_len, buffer_length);
            len_copied += line_len;
        }

        /* Do not add new line if upper loop finished pre-maturely (we
         * had to break because of buffer full, net read call will
         * resume again */
        if(!is_loop_break) {
            SEC_BP_TRACE("ii= %d: [%d + NL => %d]: %d\n",
                         ii, len_copied,
                         len_copied + 1, buffer_length);
            BUG_ON(len_copied >= buffer_length);
            buffer[len_copied++] = '\n';
        }
    }

    bp_stats_proc_read_ctx.next_expected_seq_offset += len_copied;
    bp_stats_proc_read_ctx.is_no_more_data = 0;

    SEC_BP_TRACE("ii= %d, jj= %d: next_idx= %d, "
                 "next_sub_idx= %d, next_idx_is_done= %d: -- Pre ---\n",
                 ii, jj, bp_stats_proc_read_ctx.next_idx,
                 bp_stats_proc_read_ctx.next_sub_idx, next_idx_is_done);

    BUG_ON(!bp_stats_proc_read_ctx.fname_is_done);
    if(ii == sec_ds_ut_idx && jj == end_idx) {
        SEC_BP_TRACE("ii= %d, jj= %d: Data finished\n", ii, jj);
        ++bp_stats_proc_read_ctx.who;
        bp_stats_proc_read_ctx.next_idx = 0;
        bp_stats_proc_read_ctx.next_sub_idx = 0;
        BUG_ON(is_loop_break);
        bp_stats_proc_read_ctx.next_idx_is_done = 0;
        /* bp_stats_proc_read_ctx.is_no_more_data = 1; */
        bp_stats_proc_read_ctx.fname_is_done = 0;
    } else {
        SEC_BP_TRACE("ii= %d, jj= %d: more_data\n", ii, jj);
        BUG_ON(bp_stats_proc_read_ctx.who != SEC_BP_FILL_DISKSTATS);
        bp_stats_proc_read_ctx.next_idx = is_loop_break ? ii - 1 : ii;
        bp_stats_proc_read_ctx.next_sub_idx = jj;
        bp_stats_proc_read_ctx.next_idx_is_done = next_idx_is_done;
    }

    SEC_BP_TRACE("ii= %d, jj= %d: next_idx= %d, "
                 "next_sub_idx= %d, next_idx_is_done= %d: -- Post ---\n",
                 ii, jj, bp_stats_proc_read_ctx.next_idx,
                 bp_stats_proc_read_ctx.next_sub_idx,
                 bp_stats_proc_read_ctx.next_idx_is_done);

    BUG_ON(len_copied > buffer_length);
    return len_copied;
}

static inline int sec_bp_proc_check_length(int buffer_length)
{
    return buffer_length > SEC_BP_PROC_MIN_READ_LEN;
}


/**********************************************************************/

static void sec_bp_print_help(void)
{
    printk("Boot Profile Help:\n"
           "==========================================\n\n"
           "proc_read:    cat /proc/bootstats\n\n"
           "help:         echo h > /proc/bootstats\n"
           "start:        echo 1 > /proc/bootstats\n"
           "direct-dump:  echo d > /proc/bootstats\n"
           "clear memory: echo c > /proc/bootstats\n"
           "status print: echo s > /proc/bootstats\n\n");
}

static void sec_bp_print_status(void)
{
    if(sec_bp_state == SEC_BP_STATE_COMPLETING) {
        BUG_ON(!sec_cs_table);
        printk("Boot Profile Status: COMPLETING\n");
    } else if(sec_bp_state == SEC_BP_STATE_COMPLETED) {
        BUG_ON(!sec_cs_table);
        printk("Boot Profile Status: COMPLETED\n");
        sec_bp_meminfo_dump();
    } else if(sec_bp_state == SEC_BP_STATE_UNINITED) {
        printk("Boot Profile Status: NOT STARTED - MEMORY CLEARED\n");
    } else {
        BUG_ON(sec_bp_state != SEC_BP_STATE_RUNNING
               && sec_bp_state != SEC_BP_STATE_STARTING);
        printk("Boot Profile Status: RUNNING\n");
    }
}

inline static void sec_bp_start(void)
{
    SEC_BP_TRACE("enter\n");
    if(sec_bp_state == SEC_BP_STATE_UNINITED
       || sec_bp_state == SEC_BP_STATE_COMPLETED) {
        SEC_BP_TRACE("starting\n");
        sec_bp_start__();
    } else {
        BUG_ON(sec_bp_state != SEC_BP_STATE_RUNNING
               && sec_bp_state != SEC_BP_STATE_STARTING);
        printk(KERN_INFO "already running\n");
    }
}

/**********************************************************************/
#if (SEC_BP_WAIT_UPTIME_SEC != 0)
void sec_uptime_wait(int nsec)
{
    int sleep_delay = 0;
    struct timespec uptime;
    do_posix_clock_monotonic_gettime(&uptime);
    sleep_delay = nsec - uptime.tv_sec;

    if(sleep_delay > 0) {
        printk("%s: sleeping for %d ms\n", __FUNCTION__, sleep_delay);
        msleep(sleep_delay * 1000);
    }

    do_posix_clock_monotonic_gettime(&uptime);
    printk("%s: Uptime: %u.%u\n", __FUNCTION__,
           (unsigned)uptime.tv_sec, (unsigned)uptime.tv_nsec);
}
#endif


/************************************************************************/

static int find_usb_device(char *dev)
{
	struct file *fp;
    char buf[128];
    char *p;
    int ret = -ENOENT;

    fp = filp_open("/dtv/usb/log", O_RDONLY | O_LARGEFILE, 0600);

    if (IS_ERR(fp)) 
	{
		printk("open error \n");
		return 0;
	}
       
    ret = fp->f_op->read(fp, buf, 128, &fp->f_pos);                                                                                    
                                                                                                                                           
    if (ret < 0) 
	{
		printk("read error\n");
		return 0;
	}
        
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

static int save_to_usb()
{
	struct file *fp;
	char line[4096];
	off_t offset = 0;
	char path[120];
	int ret;
	int line_copyed = 0;
	char dev[40];

	if (!find_usb_device(dev))
	{
		printk(KERN_DEBUG "Usb open fail\n");
		return 0;
	}

	sprintf(path, "%s/%s", dev, "bootstats");	

	fp = filp_open(path, O_CREAT | O_TRUNC | O_WRONLY | O_LARGEFILE, 0600);

	while(!bp_stats_proc_read_ctx.is_no_more_data)
	{
		line_copyed = sec_bp_proc_fill_report(line, offset, sizeof(line)); 		

		ret = fp->f_op->write(fp, line, line_copyed, &fp->f_pos);	

		if (ret < 0) 
		{
			printk(KERN_DEBUG "Write proc_stat.log is fail\n");
			return 0;
		}
	}

	return 1;
}
