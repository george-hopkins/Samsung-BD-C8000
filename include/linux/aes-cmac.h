/****************************************************************/
/* AES-CMAC with AES-128 bit                                    */
/* CMAC     Algorithm described in SP800-38B                    */
/* Author: Junhyuk Song (junhyuk.song@samsung.com)              */
/*         Jicheol Lee  (jicheol.lee@samsung.com)               */
/****************************************************************/

#ifndef _AES_CMAC_H_
#define _AES_CMAC_H_

#define MY_FILE_READ  sys_read

void generate_subkey(unsigned char *key, unsigned char *K1, unsigned char *K2);
void AES_CMAC(unsigned char *key, unsigned char *input, int length, unsigned char *mac);
void AES_CMAC_with_fd_and_size(unsigned char *key, int fd, int real_size, unsigned char *mac);

#endif

