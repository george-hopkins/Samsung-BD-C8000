/*
 *  arch/arm/oprofile/aop_read_dir.c 
 *
 *  reading files in directory and no of usb conected along with Path 
 *
 *  Copyright (C) 2009  Samsung 
 * 
 *  2009-08-11  Created by Namit Gupta(gupta.namit@samsung.com)
 * 
 */

#include <linux/proc_fs.h>
#include <linux/kdebugd.h>

#include <linux/fs.h>
#include <linux/dirent.h>
#include <asm/uaccess.h>
#include <linux/file.h>
#include <linux/vmalloc.h>

#include "aop_sym_db.h"

extern asmlinkage long sys_close(unsigned int fd);
extern asmlinkage long sys_getdents64(unsigned int fd,
				struct linux_dirent64 __user *dirent,
				unsigned int count);
/*
 *   The fucntion prototype for reading the directory in given path 
 */
static int aop_read_dir_raw(char *filename, unsigned char *buf,int buff_size);

#define AOP_DIR_READ_BUFF_SIZE 	4096 
#define AOP_FILE_READ_BUFF_SIZE	1024

extern asmlinkage long sys_open(const char __user *filename, 
			int flags, int mode);

/*
 * init function for initilizing directory_read 
 *  
 */ 

//int directory_read(char *dirname)
int aop_dir_read(struct aop_dir_list *pdir_list)
{

	char *pbuff = NULL;
	char * dirname = NULL;
	int i=0;
	int n = 0;
	int ret = 0;
	int total_files = 0;

	if (!pdir_list)
		goto out1;

	pbuff = (char*)vmalloc(AOP_DIR_READ_BUFF_SIZE);
	if(!pbuff) {
		printk("%s: pbuff: no memory\n", __FUNCTION__);
		ret = -1;
		goto out1; /* no memory!! */
	}
	dirname =pdir_list->path_dir;

	printk("dirname %s\n",dirname);

	if((n = aop_read_dir_raw(dirname,pbuff,AOP_DIR_READ_BUFF_SIZE)) < 0 )
	{
		printk(" error in opening diractory  \n");	
		ret = -1;
		goto out1;
	}

	for(i =0; i < n; i += pdir_list->dirent[total_files++].d_reclen )
	{
		if(total_files == AOP_MAX_FILES)	{
			printk("Number of files TOTAL FILE(%d)exceeds AOP_MAX_FILES(%d)\n", total_files, AOP_MAX_FILES);
			break;
		}
		memcpy(&pdir_list->dirent[total_files], &pbuff[i], sizeof(struct ___dirent64));
	}

	pdir_list->num_files=total_files;	

out1:
	if(pbuff)
		vfree(pbuff);
	
	return ret;

}

/*
 * init function for  initilizing  usb_read 
 * to  identify  where usb has been mounted
 * and how many files are there in that usb
 *  
 */ 

int aop_usb_detect(struct aop_usb_path *usb_path)
{
	unsigned char *read_buf = NULL;	
	struct file *filp = NULL;
	int ret = 0;
	int bytesread = 0;

	if (!usb_path){
		ret = -1;
		goto out;
	}


	read_buf = (char *)vmalloc(AOP_FILE_READ_BUFF_SIZE);
	if(!read_buf) {
		printk("%s: read_buf: no memory\n", __FUNCTION__);
		ret = -1;
		goto out; /* no memory!! */
	}
	/* 
	 *  opening file to know where usb has mounted 
	 */

	filp = filp_open("/proc/mounts", O_RDONLY | O_LARGEFILE, 0);
	if (IS_ERR(filp)||(filp==NULL))
	{
		printk("error opening file \n");
		ret = -1;
		goto out;
	}

	if (filp->f_op->read==NULL)
	{
		printk("read not allowed\n");
		ret = -1;
		goto out_close;
	}

	filp->f_pos = 0;

	printk(" USB MOUNT PATH(s): \n");
	while(1)
	{

		unsigned char *pbuf[10], *pname_buf;      // 10 string temporary will put put 
		int j=0,i=0;
		unsigned char name_buf[AOP_MAX_PATH_LEN];

		/* 
		 *  reading a /proc/mount file here (to know where usb has mounted ) 
		 */


		if((bytesread = filp->f_op->read(
				filp,read_buf,AOP_FILE_READ_BUFF_SIZE,&filp->f_pos)) <= 0 )
		{
			printk("error reading file --1  = %p\n", filp->f_op->read);
			ret = -1;
			break;
		}
		read_buf[bytesread-1] = 0;

		pbuf[0] = read_buf;

		/*
		 * The idea for this loop is to take all the string 
		 * started  with  "/dev/sd"  (usb  make dev node in 
		 * /dev/sd** ) 
		 *  the format of that file is 
		 *  "devnode"  "moount dir" "option" ...............
		 *  by this way we can find the directory where usb 
		 *  has mounted 
		 */


		for(j=0;;j++)
		{
			pbuf[j+1] = NULL;
			/*
			 * pbuf[j]+1 to avoid traping in infinite loop 
			 */

			pbuf[j+1] = strstr((pbuf[j]+1),"/dev/sd");   

			if(pbuf[j+1])
			{
				memset(name_buf,0,AOP_MAX_PATH_LEN);
				pname_buf =strstr(pbuf[j+1]," ");
				for(i=1;;i++)	
				{
					if(*(pname_buf + i) == '\n' || \
						*(pname_buf + i) == '\0' || *(pname_buf + i) == ' ')
					{
						name_buf[i] = 0;
						printk("\t %d:%s \n",j+1,name_buf);
						snprintf(usb_path->name[j], 
							AOP_MAX_PATH_LEN, "%s", name_buf);
						usb_path->num_usb = j+1;
						break;
					}
					name_buf[i-1] = *(pname_buf+i) ;
				}
			}
			else
				break;
		}

		/* file reading compleated */
		if(bytesread < AOP_FILE_READ_BUFF_SIZE ) 
			break;
	}
	
out_close:
	filp_close(filp, NULL);

out:
	if(!read_buf)
		vfree(read_buf);
	return ret;

}

/*
 *   The fucntion for reading the directory in given path 
 */

static int aop_read_dir_raw(char *filename, unsigned char *buf,int buff_size)
{
	int ret=0;
	int fd = -1;

	fd = sys_open(filename, O_RDONLY|O_NONBLOCK|O_LARGEFILE|O_DIRECTORY, 0);
	if(fd == -1)
	{
		printk(" Unable to open file  \n");	
		ret = -1;
		goto out1;
	}

	/*TODO: It should be done in a clean way ie use struct dirent64 for buff*/
	ret = sys_getdents64(fd,(struct linux_dirent64 __user *)buf,buff_size);	
	if(ret < 0)
	{
		printk(" error in sys_getdents64 \n");
		goto out2;
	}
out2:
	/* close file here */
	sys_close(fd);
out1:
	return ret;
}

