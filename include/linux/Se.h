#ifndef __CI_HEADER_H__
#define __CI_HEADER_H__

typedef unsigned long int UInt32;
typedef unsigned long int  *pUInt32;
typedef unsigned char *pUInt8;		
typedef unsigned char UInt8;
typedef unsigned char Bool;

/**
 * @enum R_SE_CPCFG register[3:0]
 * A list of cihper-mode values .\n
 */
typedef enum
{
	AES_128_ECB			= 0x0,
	AES_128_CBC			= 0x1,
	AES_256_ECB			= 0x2,
	CBC_MAC				= 0x3,
	AES_128_CTR			= 0x5,
	AES_256_CTR			= 0x6,
	DES_ECB				= 0x8,
	DES_CBC				= 0x9,
	TDES_ECB				= 0xa,
	TDES_CBC				= 0xb,
	SHA1					= 0xc,
	RC4						= 0xe,
	MAX_CP_MODE			= 0xf,
} eCipherMode;

/**
 * @enum R_SE_CPCFG register[4]
 * A list of selection weather it's decryption or encryption.\n
 */
typedef enum
{
	SEL_DEC = 0x00,
	SEL_ENC = 0x10,
} eSelEncDec;

/**
 * @enum R_SE_CPCFG register[8:7]
 * A list of key-mode values .\n
 */
typedef enum
{
	SE_EXT_KEY			= 0x080,
	SE_INT_ROM_KEY	= 0x500,
	SE_EFUSE_KEY		= 0x180,
}
eSeKeyMode;

#define	REG_BASE 			(0x003f0400)
#define PA_SE_CIPHER_BASE	(REG_BASE)
#define	R_SE_STATUS		(PA_SE_CIPHER_BASE)
#define	R_SE_DMACTRL		(PA_SE_CIPHER_BASE+0x04)
#define	R_SE_SWRST		(PA_SE_CIPHER_BASE+0x08)	
#define	R_SE_XFRSIZE		(PA_SE_CIPHER_BASE+0x14)
#define	R_SE_CXFRSIZE		(PA_SE_CIPHER_BASE+0x18)
#define	R_SE_TBUFSTART	(PA_SE_CIPHER_BASE+0x1c)
#define	R_SE_SBUFSTART	(PA_SE_CIPHER_BASE+0x20)
#define	R_SE_CPCFG			(PA_SE_CIPHER_BASE+0x30)
#define	R_SE_CPBLKSIZE		(PA_SE_CIPHER_BASE+0x64)

/* DMACTRL regisger values */
#define DC_CP_EN		(0x20)	// [5]
#define DC_BYTE_SWAP	(0xa)	// [4:1] = b'0101
#define DC_START_DMA	(0x1)	// [0]

#define DMACTRL_START_DMA	(DC_CP_EN | DC_START_DMA | DC_BYTE_SWAP)

UInt32 lldSe_ReadReg(UInt32 u32Addr);
void lldSe_WriteReg(UInt32 u32Addr, UInt32 u32Data);
int secure_dec(UInt32 u32SrcAddr, UInt32 u32InDataLength, UInt32 u32DstAddr, UInt32 *pu32OutDataLength);
int secure_enc(UInt32 u32SrcAddr, UInt32 u32InDataLength, UInt32 u32DstAddr, UInt32 *pu32OutDataLength);

#endif // __CI_HEADER_H__

