
#include <linux/proc_fs.h>
#include <linux/syscalls.h>

#define CI_CMAC_SIZE		(16)

// 1GB
#define CMACKEY_PARTITION_1000	"/dev/bml0/11"
// 2GB
#define CMACKEY_PARTITION_128	"/dev/bml0/14"

typedef struct {
	unsigned char au8PreCmacResult[CI_CMAC_SIZE];	// MAC
	int msgLen;	// the length of a message to be authenticated
} MacInfo_t;

typedef struct {
    	MacInfo_t macAuthULD;
} macAuthULd_t;

typedef struct {
	int firstBoot;
	unsigned char key[CI_CMAC_SIZE];
} cmacKey_t;

int getAuthUld(macAuthULd_t *authuld);
int getcmackey(const char *mackey_partition, cmacKey_t *cmackey_authuld);
int getPartSize(void);

