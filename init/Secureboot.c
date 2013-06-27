
#include <linux/Secureboot.h>

#define CIP_CRIT_PRINT(fmt, ...)		printk(KERN_CRIT "[CIP_KERNEL] " fmt,##__VA_ARGS__)
#define CIP_WARN_PRINT(fmt, ...)		printk(KERN_WARNING "[CIP_KERNEL] " fmt,##__VA_ARGS__)
#define CIP_DEBUG_PRINT(fmt, ...)		printk(KERN_DEBUG "[CIP_KERNEL] " fmt,##__VA_ARGS__)

extern char rootfs_name[64];

int getAuthUld(macAuthULd_t *mac_authuld)
{
	int fd;

	CIP_DEBUG_PRINT("%s\n", rootfs_name);
	fd = sys_open(rootfs_name, O_RDONLY, 0);
	if(fd >= 0) 
	{
		sys_lseek(fd, -4096, SEEK_END);
		sys_read(fd, (void *)mac_authuld, sizeof(MacInfo_t));
		sys_close(fd);
		return 1;
	} 
	CIP_CRIT_PRINT("%s file open error\n", rootfs_name);
	
	return 0;
}

int getPartSize(void) // 1000, 2000
{
	int fp;
	
	fp = sys_open("/etc/flash_flag", O_RDONLY, 0);
	if (fp>=0)
	{
		sys_close(fp);
		return 128;
	}
	else
	{
		return 1000;
	}
}

int getcmackey(const char *mackey_partition, cmacKey_t *cmackey_authuld)
{
	int fd;
	
	fd = sys_open(mackey_partition, O_RDONLY, 0);
	if(fd>=0)
	{
		sys_read(fd, (void *)cmackey_authuld, sizeof(cmacKey_t));
		sys_close(fd);
		return 1;
	} 
	CIP_CRIT_PRINT("%s file open error\n", mackey_partition);
	
	return 0;
}

