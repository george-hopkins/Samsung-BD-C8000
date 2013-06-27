/*
 * kernel/lspinfo.c
 *
 * Linux support package info to be exported in to /sys/selp/vd
 *
 * Author: MontaVista Software, Inc. <source@mvista.com>
 * Modified by VD SPTeam Linux Part on April-18-2009. <linux.sec@samsung.com>
 *
 * sysfs code based on drivers/base/firmware.c, drivers/firmware/edd.c
 * and drivers/pci/pci-sysfs.c
 *
 * 2003-2005 (c) MontaVista Software, Inc. This file is licensed under
 * the terms of the GNU General Public License version 2. This program
 * is licensed "as is" without any warranty of any kind, whether express
 * or implied.
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/sysfs.h>
#include <linux/lspinfo.h>
#include <linux/lsppatchlevel.h>
#include <linux/mvl_patch.h>

#include <asm/bug.h>

/* VDLinux, based SELP.4.3.1.x default patch No.9, 
   SELP version print, SP Team 2009-05-08 */
struct vd_info_dev {
	struct kobject kobj;
};

struct vd_attribute {
	struct attribute attr;
	ssize_t(*show) (char *buf);
};

static struct vd_info_dev *vd_info_devs;

#define SELP_INFO_DEV_ATTR(_name)				\
static struct vd_attribute vd_info_dev_attr_##_name = __ATTR_RO(_name);

/*
 * Create a selp top-level sysfs dir
 */
static decl_subsys(selp, NULL, NULL);

static int selp_register(struct kset * s)
{
	kobj_set_kset_s(s, selp_subsys);
	return subsystem_register(s);
}

static void selp_unregister(struct kset * s)
{
	subsystem_unregister(s);
}

static int __init selp_init(void)
{
	return subsystem_register(&selp_subsys);
}

module_init(selp_init);

/* Standard and very simple bits of information we need to export */
#define vd_info_dev_attr(_type, _value)			\
static ssize_t							\
_type##_show(char *buf)						\
{								\
	return sprintf(buf, "%s\n", _value);			\
}
vd_info_dev_attr(board_name, LSP_BOARD_NAME);
SELP_INFO_DEV_ATTR(board_name);
vd_info_dev_attr(build_id, LSP_BUILD_DATE);
SELP_INFO_DEV_ATTR(build_id);
vd_info_dev_attr(name, LSP_NAME);
SELP_INFO_DEV_ATTR(name);
vd_info_dev_attr(selp_arch, LSP_SELP_ARCH);
SELP_INFO_DEV_ATTR(selp_arch);
vd_info_dev_attr(revision, LSP_REVISION);
SELP_INFO_DEV_ATTR(revision);
vd_info_dev_attr(selp_patches, SELP_PATCHES);
SELP_INFO_DEV_ATTR(selp_patches);

static ssize_t
summary_show(char *buf)
{
	return sprintf(buf,
			"SELP Version		: %s\n"
			"Board Name		: %s\n"
			"LSP Name		: %s\n"
			"LSP Revision		: %s.%s\n"
			"SELP Architecture	: %s\n"
			"Patch Level		: %s\n",
            LSP_SELP_VERSION, LSP_BOARD_NAME, LSP_NAME,
            LSP_BUILD_DATE, LSP_REVISION, LSP_SELP_ARCH,
            SELP_PATCHES);
}
SELP_INFO_DEV_ATTR(summary);

/* Infrastructure for listing patches which have been applied post
 * release.
 */
typedef struct mvl_patchlist_s
{
	int                    common;
	struct mvl_patchlist_s *next;
} mvl_patchlist_t;

static mvl_patchlist_t *mvl_patchlist, *mvl_patchlist_tail;

static ssize_t show_mvl_patches(struct kobject *kobj, struct bin_attribute *b,
				char *buf, loff_t off, size_t count);

static struct bin_attribute mvl_patches = {
	.attr = {
		.name = "mvl_patches",
		.mode = 0444,
		.owner = THIS_MODULE,
	},
	.size = 0,
	.read = show_mvl_patches,
};

int
mvl_register_patch(int common)
{
	mvl_patchlist_t *p;

	p = kmalloc(sizeof(*p), GFP_KERNEL);
	if (!p)
		return -ENOMEM;

	p->common = common;
	p->next = NULL;
	if (mvl_patchlist) {
		mvl_patchlist_tail->next = p;
		mvl_patchlist_tail = p;
	} else {
		mvl_patchlist = p;
		mvl_patchlist_tail = p;
	}
	mvl_patches.size += 5;

	return 0;
}
EXPORT_SYMBOL(mvl_register_patch);

static ssize_t
show_mvl_patches(struct kobject *kobj, struct bin_attribute *b, char *buf,
		 loff_t off, size_t count)
{
	int             len = 0;
	mvl_patchlist_t *p = mvl_patchlist;
	char            name[6];
	off_t           left = off;

	while (p && left) {
		int l = snprintf(name, sizeof(name), "%4.4d\n", p->common);
		p = p->next;
		if (l > left) {
			len = l - left;
			if (len > count)
				len = count;
			memcpy(buf, name+left, len);
			count -= len;
			buf += len;
			break;
		}
		left -= l;
	}

	while (p && count) {
		int l = snprintf(name, sizeof(name), "%4.4d\n", p->common);
		p = p->next;
		if (l > count) {
			memcpy(buf, name, count);
			len += count;
			count = 0;
		} else {
			memcpy(buf, name, l);
			count -= l;
			len += l;
			buf += l;
		}
	}

	return len;
}

/* On top of that create an vd dir to register all of our things
 * into.
 */
#define to_vd_info_dev_attr(_attr)				\
	container_of(_attr, struct vd_attribute, attr)

static ssize_t
vd_info_dev_attr_show(struct kobject * kobj, struct attribute *attr,
		char *buf)
{
	struct vd_attribute *vd_info_dev_attr =
		to_vd_info_dev_attr(attr);

	return vd_info_dev_attr->show(buf);
}

static struct sysfs_ops vd_info_dev_attr_ops = {
	.show = vd_info_dev_attr_show,
};

/* Theser are the default attributes that exist on every LSP and are
 * unchanged from initial release. */
static struct attribute * def_attrs[] = {
	&vd_info_dev_attr_board_name.attr,
	&vd_info_dev_attr_build_id.attr,
	&vd_info_dev_attr_name.attr,
	&vd_info_dev_attr_selp_arch.attr,
	&vd_info_dev_attr_revision.attr,
	&vd_info_dev_attr_summary.attr,
	&vd_info_dev_attr_selp_patches.attr,
	NULL,
};

static struct kobj_type ktype_vd = {
	.sysfs_ops	= &vd_info_dev_attr_ops,
	.default_attrs	= def_attrs,
};

static decl_subsys(vd, &ktype_vd, NULL);

static int __init
vd_info_dev_init(void)
{
	int rc;

	rc = selp_register(&vd_subsys);
	if (rc)
		return rc;

	if (!(vd_info_devs = kmalloc(sizeof (struct vd_info_dev),
					GFP_KERNEL)))
		return -ENOMEM;

	memset(vd_info_devs, 0, sizeof (struct vd_info_dev));
	kobject_set_name(&vd_info_devs->kobj, "lspinfo");
	kobj_set_kset_s(vd_info_devs, vd_subsys);
	rc = kobject_register(&vd_info_devs->kobj);
	if (rc)
		goto error;

	rc = sysfs_create_bin_file(&vd_info_devs->kobj, &mvl_patches);
	if (rc)
		goto error;

	return 0;
error:
	kfree(vd_info_devs);
	selp_unregister(&vd_subsys);
	return rc;
}

late_initcall(vd_info_dev_init);
