/*
 * udf_fs_i.h
 *
 * This file is intended for the Linux kernel/module. 
 *
 * COPYRIGHT
 *	This file is distributed under the terms of the GNU General Public
 *	License (GPL). Copies of the GPL can be obtained from:
 *		ftp://prep.ai.mit.edu/pub/gnu/GPL
 *	Each contributing author retains all rights to their own work.
 */

#ifndef _UDF_FS_I_H
#define _UDF_FS_I_H 1

#ifdef __KERNEL__

#ifdef CONFIG_SSIF_EXT_CACHE

typedef struct _udf_extent_position_
{
	struct buffer_head* bh;
	uint32_t offset;
	kernel_lb_addr block;
	int block_id;
}
udf_extent_position;

typedef struct _udf_extent_cache_
{
	udf_extent_position udf_pos;
	loff_t udf_lbcount;
	int sanity;
	kernel_lb_addr inode_location;
}
udf_extent_cache;

#define UDF3D_MAGIC_NUM 0xcafe7788
#define UDF3D_MAXN_PRELOAD_BLOCKS 512

typedef struct _udf_extent_descriptor_cache_
{
	void* descriptor_blocks[UDF3D_MAXN_PRELOAD_BLOCKS];
	int n_descriptors;
	int sanity;
}
udf_extent_descriptor_cache;

#endif

struct udf_inode_info
{
	struct timespec		i_crtime;
	/* Physical address of inode */
	kernel_lb_addr		i_location;
	__u64			i_unique;
	__u32			i_lenEAttr;
	__u32			i_lenAlloc;
	__u64			i_lenExtents;
	__u32			i_next_alloc_block;
	__u32			i_next_alloc_goal;
	unsigned		i_alloc_type : 3;
	unsigned		i_efe : 1;
	unsigned		i_use : 1;
	unsigned		i_strat4096 : 1;
	unsigned		reserved : 26;
	union
	{
		short_ad	*i_sad;
		long_ad		*i_lad;
		__u8		*i_data;
	} i_ext;

#ifdef CONFIG_SSIF_EXT_CACHE

	udf_extent_cache recent_access; /*Key element-1 : bgbak@samsung.com*/
	udf_extent_descriptor_cache extent_desc_cache; /*Key element-2 : bgbak@samsung.com*/

	/* Reserved for future optimization to do faster random access.*/
	/*udf_extent_cache mid_points[UDF3D_SZ_EXT_CACHE]; Obselete*/
	/* Reserved for future optimization to do faster random access.*/

#endif

	struct inode vfs_inode;
};

#endif

/* exported IOCTLs, we have 'l', 0x40-0x7f */

#define UDF_GETEASIZE   _IOR('l', 0x40, int)
#define UDF_GETEABLOCK  _IOR('l', 0x41, void *)
#define UDF_GETVOLIDENT _IOR('l', 0x42, void *)
#define UDF_RELOCATE_BLOCKS _IOWR('l', 0x43, long)

#endif /* _UDF_FS_I_H */
