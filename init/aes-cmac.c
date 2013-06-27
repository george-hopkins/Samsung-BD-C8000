/****************************************************************/
/* AES-CMAC with AES-128 bit                                    */
/* CMAC     Algorithm described in SP800-38B                    */
/* Author: Junhyuk Song (junhyuk.song@samsung.com)              */
/*         Jicheol Lee  (jicheol.lee@samsung.com)               */
/****************************************************************/

#include <linux/aes-cmac.h>
#include <linux/rijndael-alg-fst.h>

extern long sys_read(unsigned int fd, char *buf, long count);
extern int printk(const char * fmt, ...);
extern void * memcpy(void *, const void *, unsigned int);
extern long sys_close(unsigned int fd);
extern long sys_open(const char *filename, int flags, int mode);
extern long sys_write(unsigned int fd, const char __attribute__((noderef, address_space(1))) *buf, long count);                              
extern void msleep(unsigned int msecs);  

/* For CMAC Calculation */
unsigned char const_Rb[16] = {
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x87
};

// file에서 더이상 가져올 수 孃坪?경우 FF로 적어서 return한다.
int read_from_file_or_ff (int fd, unsigned char *buf, int size)
{
	int read_cnt;
	int i;

	read_cnt = sys_read(fd, buf, size);

	if( read_cnt <= 0 ) 
	{
		read_cnt = 0;
	}
	
	for(i=read_cnt; i<size; i++) 
	{
		buf[i] = 0xff;
	}
	
	return size;
}

/* Basic Functions */
void xor_128(unsigned char *a, unsigned char *b, unsigned char *out)
{
	int i;
	for (i=0;i<16; i++)
	{
		out[i] = a[i] ^ b[i];
	}
	
	return;
}

/* AES-CMAC Generation Function */
void leftshift_onebit(unsigned char *input,unsigned char *output)
{
	int i;
	unsigned char overflow = 0;

	for (i=15; i>=0; i--) 
	{
		output[i] = input[i] << 1;
		output[i] |= overflow;
		overflow = (input[i] & 0x80)?1:0;
	}
	
	return;
}

void generate_subkey(unsigned char *key, unsigned char *K1, unsigned char *K2)
{
	unsigned char L[16];
	unsigned char Z[16];
	unsigned char tmp[16];
	int i;

	for ( i=0; i<16; i++ ) Z[i] = 0;

	AES_128(key,Z,L);

	if ( (L[0] & 0x80) == 0 ) 
	{ /* If MSB(L) = 0, then K1 = L << 1 */
		leftshift_onebit(L,K1);
	} 
	else 
	{    /* Else K1 = ( L << 1 ) (+) Rb */
		leftshift_onebit(L,tmp);
		xor_128(tmp,const_Rb,K1);
	}

	if ( (K1[0] & 0x80) == 0 ) 
	{
		leftshift_onebit(K1,K2);
	} 
	else 
	{
		leftshift_onebit(K1,tmp);
		xor_128(tmp,const_Rb,K2);
	}
	
	return;
}

void padding(unsigned char *lastb, unsigned char *pad, int length)
{
	int j;

	/* original last block */
	for(j=0; j<16; j++) 
	{
		if(j < length) 
		{
			pad[j] = lastb[j];
		} 
		else if ( j == length ) {
			pad[j] = 0x80;
		} 
		else 
		{
			pad[j] = 0x00;
		}
	}

	return;
}

void AES_CMAC(unsigned char *key, unsigned char *input, int length, unsigned char *mac)
{
	unsigned char X[16],Y[16], M_last[16], padded[16];
	unsigned char K1[16], K2[16];
	int n, i, flag;
	generate_subkey(key,K1,K2);

	n = (length+15) / 16;       /* n is number of rounds */

	if(n == 0) 
	{
		n = 1;
		flag = 0;
	} 
	else 
	{
		if ( (length%16) == 0 ) 
		{ /* last block is a complete block */
			flag = 1;
		} 
		else 
		{ /* last block is not complete block */
			flag = 0;
		}
	}

	for ( i=0; i<16; i++ ) X[i] = 0;
	for ( i=0; i<n-1; i++ ) 
	{
		xor_128(X,&input[16*i],Y);	/* Y := Mi (+) X  */
		AES_128(key,Y,X);		/* X := AES-128(KEY, Y); */
	}

	if(flag) 
	{ /* last block is complete block */
		xor_128(&input[16*(n-1)],K1,M_last);
	} 
	else 
	{
		padding(&input[16*(n-1)],padded,length%16);
		xor_128(padded,K2,M_last);
	}

	xor_128(X,M_last,Y);
	AES_128(key,Y,X);

	for ( i=0; i<16; i++ ) 
	{
		mac[i] = X[i];
	}

	return;
}

void AES_CMAC_with_fd_and_size(unsigned char *key, int fd, int real_size, unsigned char *mac)
{
	unsigned char A[16];
	unsigned char B[16];
	unsigned char X[16],Y[16], M_last[16], padded[16];
	unsigned char K1[16], K2[16];
	int i;
	unsigned long int total_read_length;
	int A_length;
	int B_length;
    
	generate_subkey(key,K1,K2);

	total_read_length = 0;

	for ( i=0; i<16; i++ ) X[i] = 0;

	A_length = read_from_file_or_ff (fd, A, 16);
	if( A_length > real_size ) {
		A_length = real_size;
	}

	total_read_length += A_length;	

   	if( A_length < 16 ) 
	{ /* in case of using K2 */
	       padding(A,padded,A_length%16);
       	xor_128(padded,K2,M_last);
       	goto kernel_final_overflowed;
   	}

	while(1) 
	{
		B_length = read_from_file_or_ff (fd, B, 16);

		if( total_read_length+B_length > real_size ) 
		{
			B_length = real_size - total_read_length;
		}

		total_read_length += B_length;

		if( B_length > 0 ) 
		{
		        xor_128(X,A,Y);		 /* Y := Mi (+) X  */
			AES_128(key,Y,X);      /* X := AES-128(KEY, Y); */

      	 		if( B_length < 16 ) 
			{
				/* in case of K2 */
		       	padding(B,padded,B_length%16);
       			xor_128(padded,K2,M_last);
				goto kernel_final_overflowed;
      	 		}
      	 		memcpy(A, B, 16);
		} 
		else 
		{
			/* in case of K1 */
       	 	xor_128(A,K1,M_last);
       	 	goto kernel_final_overflowed;
		}
	}

kernel_final_overflowed:
	//printk("[CIP_KERNEL] total read length = %lu\n", total_read_length);

	xor_128(X,M_last,Y);
	AES_128(key,Y,X);

	for ( i=0; i<16; i++ ) 
	{
		mac[i] = X[i];
	}

	return;
}

