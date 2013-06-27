/*
 * file.c
 *
 * PURPOSE
 *  File handling routines for the OSTA-UDF(tm) filesystem.
 *
 * CONTACTS
 *  E-mail regarding any portion of the Linux UDF file system should be
 *  directed to the development team mailing list (run by majordomo):
 *    linux_udf@hpesjro.fc.hp.com
 *
 * COPYRIGHT
 *  This file is distributed under the terms of the GNU General Public
 *  License (GPL). Copies of the GPL can be obtained from:
 *    ftp://prep.ai.mit.edu/pub/gnu/GPL
 *  Each contributing author retains all rights to their own work.
 *
 *  (C) 1998-1999 Dave Boynton
 *  (C) 1998-2004 Ben Fennema
 *  (C) 1999-2000 Stelias Computing Inc
 *
 * HISTORY
 *
 *  10/02/98 dgb  Attempt to integrate into udf.o
 *  10/07/98      Switched to using generic_readpage, etc., like isofs
 *                And it works!
 *  12/06/98 blf  Added udf_file_read. uses generic_file_read for all cases but
 *                ICBTAG_FLAG_AD_IN_ICB.
 *  04/06/99      64 bit file handling on 32 bit systems taken from ext2 file.c
 *  05/12/99      Preliminary file write support
 */

#include "udfdecl.h"
#include <linux/fs.h>
#include <linux/udf_fs.h>
#include <asm/uaccess.h>
#include <linux/kernel.h>
#include <linux/string.h> /* memset */
#include <linux/errno.h>
#include <linux/smp_lock.h>
#include <linux/pagemap.h>
#include <linux/buffer_head.h>

#include "udf_i.h"
#include "udf_sb.h"

static int udf_adinicb_readpage(struct file *file, struct page * page)
{
	struct inode *inode = page->mapping->host;
	char *kaddr;

	BUG_ON(!PageLocked(page));

	kaddr = kmap(page);
	memset(kaddr, 0, PAGE_CACHE_SIZE);
	memcpy(kaddr, UDF_I_DATA(inode) + UDF_I_LENEATTR(inode), inode->i_size);
	flush_dcache_page(page);
	SetPageUptodate(page);
	kunmap(page);
	unlock_page(page);
	return 0;
}

static int udf_adinicb_writepage(struct page *page, struct writeback_control *wbc)
{
	struct inode *inode = page->mapping->host;
	char *kaddr;

	BUG_ON(!PageLocked(page));

	kaddr = kmap(page);
	memcpy(UDF_I_DATA(inode) + UDF_I_LENEATTR(inode), kaddr, inode->i_size);
	mark_inode_dirty(inode);
	SetPageUptodate(page);
	kunmap(page);
	unlock_page(page);
	return 0;
}

static int udf_adinicb_prepare_write(struct file *file, struct page *page, unsigned offset, unsigned to)
{
	kmap(page);
	return 0;
}

static int udf_adinicb_commit_write(struct file *file, struct page *page, unsigned offset, unsigned to)
{
	struct inode *inode = page->mapping->host;
	char *kaddr = page_address(page);

	memcpy(UDF_I_DATA(inode) + UDF_I_LENEATTR(inode) + offset,
		kaddr + offset, to - offset);
	mark_inode_dirty(inode);
	SetPageUptodate(page);
	kunmap(page);
	/* only one page here */
	if (to > inode->i_size)
		inode->i_size = to;
	return 0;
}

struct address_space_operations udf_adinicb_aops = {
	.readpage		= udf_adinicb_readpage,
	.writepage		= udf_adinicb_writepage,
	.sync_page		= block_sync_page,
	.prepare_write		= udf_adinicb_prepare_write,
	.commit_write		= udf_adinicb_commit_write,
};

static ssize_t udf_file_aio_write(struct kiocb *iocb, const struct iovec *iov,
				  unsigned long nr_segs, loff_t ppos)
{
	ssize_t retval;
	struct file *file = iocb->ki_filp;
	struct inode *inode = file->f_path.dentry->d_inode;
	int err, pos;
	size_t count = iocb->ki_left;

	if (UDF_I_ALLOCTYPE(inode) == ICBTAG_FLAG_AD_IN_ICB) {
		if (file->f_flags & O_APPEND)
			pos = inode->i_size;
		else
			pos = ppos;

		if (inode->i_sb->s_blocksize < (udf_file_entry_alloc_offset(inode) +
						pos + count)) {

			udf_expand_file_adinicb(inode, pos + count, &err);
			if (UDF_I_ALLOCTYPE(inode) == ICBTAG_FLAG_AD_IN_ICB) {

				udf_debug("udf_expand_adinicb: err=%d\n", err);
				return err;
			}
		} else {


			if (pos + count > inode->i_size)
				UDF_I_LENALLOC(inode) = pos + count;
			else
				UDF_I_LENALLOC(inode) = inode->i_size;
		}
	}

	retval = generic_file_aio_write(iocb, iov, nr_segs, ppos);

	if (retval > 0)
		mark_inode_dirty(inode);

	return retval;
}

/*
 * udf_ioctl
 *
 * PURPOSE
 *	Issue an ioctl.
 *
 * DESCRIPTION
 *	Optional - sys_ioctl() will return -ENOTTY if this routine is not
 *	available, and the ioctl cannot be handled without filesystem help.
 *
 *	sys_ioctl() handles these ioctls that apply only to regular files:
 *		FIBMAP [requires udf_block_map()], FIGETBSZ, FIONREAD
 *	These ioctls are also handled by sys_ioctl():
 *		FIOCLEX, FIONCLEX, FIONBIO, FIOASYNC
 *	All other ioctls are passed to the filesystem.
 *
 *	Refer to sys_ioctl() in fs/ioctl.c
 *	sys_ioctl() -> .
 *
 * PRE-CONDITIONS
 *	inode			Pointer to inode that ioctl was issued on.
 *	filp			Pointer to file that ioctl was issued on.
 *	cmd			The ioctl command.
 *	arg			The ioctl argument [can be interpreted as a
 *				user-space pointer if desired].
 *
 * POST-CONDITIONS
 *	<return>		Success (>=0) or an error code (<=0) that
 *				sys_ioctl() will return.
 *
 * HISTORY
 *	July 1, 1997 - Andrew E. Mileski
 *	Written, tested, and released.
 */
int udf_ioctl(struct inode *inode, struct file *filp, unsigned int cmd,
	unsigned long arg)
{
	int result = -EINVAL;

	if ( permission(inode, MAY_READ, NULL) != 0 )
	{
		udf_debug("no permission to access inode %lu\n",
						inode->i_ino);
		return -EPERM;
	}

	if ( !arg )
	{
		udf_debug("invalid argument to udf_ioctl\n");
		return -EINVAL;
	}

	switch (cmd)
	{
		case UDF_GETVOLIDENT:
			return copy_to_user((char __user *)arg,
				UDF_SB_VOLIDENT(inode->i_sb), 32) ? -EFAULT : 0;
		case UDF_RELOCATE_BLOCKS:
		{
			long old, new;

			if (!capable(CAP_SYS_ADMIN)) return -EACCES;
			if (get_user(old, (long __user *)arg)) return -EFAULT;
			if ((result = udf_relocate_blocks(inode->i_sb,
					old, &new)) == 0)
				result = put_user(new, (long __user *)arg);

			return result;
		}
		case UDF_GETEASIZE:
			result = put_user(UDF_I_LENEATTR(inode), (int __user *)arg);
			break;

		case UDF_GETEABLOCK:
			result = copy_to_user((char __user *)arg, UDF_I_DATA(inode),
				UDF_I_LENEATTR(inode)) ? -EFAULT : 0;
			break;
	}

	return result;
}

void udf_extent_descriptor_cache_clear(udf_extent_descriptor_cache* p_this)
{
	int n;
	p_this->sanity = ~UDF3D_MAGIC_NUM;
	p_this->n_descriptors = 0;
	for( n = 0; n < UDF3D_MAXN_PRELOAD_BLOCKS; n++ )
	{
		p_this->descriptor_blocks[n] = (void*)0;
	}
	return;
}

int udf_extent_descriptor_cache_release(udf_extent_descriptor_cache* p_this)
{
	int n;
	if( p_this->sanity != UDF3D_MAGIC_NUM )
	{
		printk("udf_extent_descriptor_cache_release::Error-1\n");
		return -1;
	}
//	printk("udf_extent_descriptor_cache_release::p_this->n_descriptors(%d)\n", p_this->n_descriptors);
	for( n = 0; n < p_this->n_descriptors; n++ )
	{
		if( p_this->descriptor_blocks[n] != (void*)0 )
		{
			udf_release_data((struct buffer_head*)p_this->descriptor_blocks[n]);
			p_this->descriptor_blocks[n] = (void*)0;
		}
	}
	p_this->sanity = ~(UDF3D_MAGIC_NUM);
	p_this->n_descriptors = 0;
	return n;
}

int8_t inode_bmap_preload_extent_blocks(
	struct inode *inode, kernel_lb_addr *bloc, uint32_t *extoffset,
	uint32_t *elen, struct buffer_head **bh, udf_extent_descriptor_cache* p_ext_desc_cache);

int udf_extent_descriptor_cache_open_inode(
	udf_extent_descriptor_cache* p_this, struct inode *inode
){
	int n_blocks;
	kernel_lb_addr bloc;
	uint32_t extoffset;
	uint32_t elen;
	struct buffer_head* bh = NULL;

//	printk("udf_extent_descriptor_cache_open_inode(%d)\n", (int)inode);
	n_blocks = inode_bmap_preload_extent_blocks(inode, &bloc, &extoffset, &elen, &bh, p_this);
//	printk("udf_extent_descriptor_cache_open_inode(%d), n_descriptors(%d)\n", (int)inode, p_this->n_descriptors);
	p_this->sanity = UDF3D_MAGIC_NUM;
	return n_blocks;
}

/*
 * udf_release_file
 *
 * PURPOSE
 *  Called when all references to the file are closed
 *
 * DESCRIPTION
 *  Discard prealloced blocks
 *
 * HISTORY
 *
 */
static int udf_release_file(struct inode * inode, struct file * filp)
{
#ifdef CONFIG_SSIF_EXT_CACHE
	struct udf_inode_info *iinfo = NULL;
	iinfo = UDF_I(inode);
//	printk("udf_release_file(%d)::1\n", (int)inode);
	udf_extent_descriptor_cache_release(&(iinfo->extent_desc_cache));
#endif

	if (filp->f_mode & FMODE_WRITE)
	{
		lock_kernel();
		udf_discard_prealloc(inode);
		unlock_kernel();
	}

#ifdef CONFIG_SSIF_EXT_CACHE
	iinfo->recent_access.sanity = ~(UDF3D_MAGIC_NUM);
#endif
	
	udf_extent_descriptor_cache_clear(&(iinfo->extent_desc_cache));
//	printk("udf_release_file(%d)::2\n", (int)inode);
	
	return 0;
}

static int udf_file_open(struct inode *inode, struct file *filp)
{
	struct udf_inode_info *iinfo;
	int n;
	iinfo = UDF_I(inode);
	udf_extent_descriptor_cache_open_inode(&(iinfo->extent_desc_cache), inode);
	//printk("udf_file_open(%d)\n", (int)inode);
	
	return generic_file_open(inode, filp);
}

struct file_operations udf_file_operations = {
	.read			= do_sync_read,
	.aio_read		= generic_file_aio_read,
	.ioctl			= udf_ioctl,
	.open			= udf_file_open,
	.mmap			= generic_file_mmap,
	.write			= do_sync_write,
	.aio_write		= udf_file_aio_write,
	.release		= udf_release_file,
	.fsync			= udf_fsync_file,
	.splice_read		= generic_file_splice_read,
};

struct inode_operations udf_file_inode_operations = {
	.truncate		= udf_truncate,
};
