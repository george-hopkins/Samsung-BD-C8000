/*----------------------------------------------------------------------*/
/*                 [ SE ]														*/
/*----------------------------------------------------------------------*/

#include <linux/dma-mapping.h>
#include <linux/Se.h>

UInt32 lldSe_ReadReg(UInt32 u32Addr)
{
	volatile UInt32 tmp = 0;
	tmp = *(volatile pUInt32)(u32Addr +0xF8000000UL);
	return(tmp);
}

void lldSe_WriteReg(UInt32 u32Addr, UInt32 u32Data)
{
	*(volatile pUInt32)(u32Addr +0xF8000000UL) = u32Data;
}

//MS.PARK 0902
int secure_dec(UInt32 u32SrcAddr, UInt32 u32InDataLength, UInt32 u32DstAddr, UInt32 *pu32OutDataLength)
{
#if 0//def DEBUG_SE
	unsigned int *t=u32SrcAddr;
	unsigned int *p=u32DstAddr;
#endif
	lldSe_WriteReg(R_SE_CPCFG, 0);	
	lldSe_WriteReg(R_SE_CPCFG,  (AES_128_ECB)|(SE_EFUSE_KEY)|(SEL_DEC));	
	lldSe_WriteReg(R_SE_SBUFSTART,virt_to_phys((void*)u32SrcAddr));	
	lldSe_WriteReg(R_SE_TBUFSTART, virt_to_phys((void*)u32DstAddr));	
	lldSe_WriteReg(R_SE_XFRSIZE, u32InDataLength);	
	lldSe_WriteReg(R_SE_CPBLKSIZE, u32InDataLength);	
	dma_cache_maint((void *)u32SrcAddr,u32InDataLength,DMA_TO_DEVICE);
	lldSe_WriteReg(R_SE_DMACTRL, DMACTRL_START_DMA); ///(DC_CP_EN | DC_START_DMA | DC_BYTE_SWAP) 
#if 0//def DEBUG_SEdef DEBUG_SE
	printk("R_SE_CPCFG=%x\n",lldSe_ReadReg(R_SE_CPCFG));
	printk("R_SE_SBUFSTART=%x\n",lldSe_ReadReg(R_SE_SBUFSTART));
	printk("u32SrcAddr=%x\n",*t);
	printk("u32DstAddr=%x\n",*p);
	printk("R_SE_TBUFSTART=%x\n",lldSe_ReadReg(R_SE_TBUFSTART));
	printk("R_SE_XFRSIZE=%x\n",lldSe_ReadReg(R_SE_XFRSIZE));
	printk("R_SE_CPBLKSIZE=%x\n",lldSe_ReadReg(R_SE_CPBLKSIZE));
	printk("R_SE_DMACTRL=%x\n",lldSe_ReadReg(R_SE_DMACTRL));
#endif
	while(lldSe_ReadReg(R_SE_STATUS) != 0)
	dma_cache_maint((void *)u32DstAddr,u32InDataLength,DMA_FROM_DEVICE); //MS.PARK 0810
	lldSe_WriteReg(R_SE_DMACTRL, DMACTRL_START_DMA & (~DC_START_DMA));   
#if 0//def DEBUG_SEdef DEBUG_SE
	printk("dma_cache_maint u32DstAddr=%x\n",*p);
#endif

	return 0;
}

int secure_enc(UInt32 u32SrcAddr, UInt32 u32InDataLength, UInt32 u32DstAddr, UInt32 *pu32OutDataLength)
{
#ifdef DEBUG_SE
	unsigned int *t=u32SrcAddr;
	unsigned int *p=u32DstAddr;
#endif
	lldSe_WriteReg(R_SE_CPCFG, 0);	
	lldSe_WriteReg(R_SE_CPCFG,  (AES_128_ECB)|(SE_INT_ROM_KEY)|(SEL_ENC));	
	lldSe_WriteReg(R_SE_SBUFSTART,virt_to_phys((void*)u32SrcAddr));	
	lldSe_WriteReg(R_SE_TBUFSTART,virt_to_phys((void*)u32DstAddr));	
	lldSe_WriteReg(R_SE_XFRSIZE, u32InDataLength);	
	lldSe_WriteReg(R_SE_CPBLKSIZE, u32InDataLength);	
	dma_cache_maint((void *)u32SrcAddr,u32InDataLength,DMA_TO_DEVICE);
	lldSe_WriteReg(R_SE_DMACTRL, DMACTRL_START_DMA); ///(DC_CP_EN | DC_START_DMA | DC_BYTE_SWAP) 
#ifdef DEBUG_SE
	printk("R_SE_CPCFG=%x\n",lldSe_ReadReg(R_SE_CPCFG));
	printk("R_SE_SBUFSTART=%x\n",lldSe_ReadReg(R_SE_SBUFSTART));
	printk("u32SrcAddr=%x\n",*t);
	printk("u32DstAddr=%x\n",*p);
	printk("R_SE_TBUFSTART=%x\n",lldSe_ReadReg(R_SE_TBUFSTART));
	printk("R_SE_XFRSIZE=%x\n",lldSe_ReadReg(R_SE_XFRSIZE));
	printk("R_SE_CPBLKSIZE=%x\n",lldSe_ReadReg(R_SE_CPBLKSIZE));
	printk("R_SE_DMACTRL=%x\n",lldSe_ReadReg(R_SE_DMACTRL));
#endif
	while(lldSe_ReadReg(R_SE_STATUS) != 0)
	dma_cache_maint((void *)u32DstAddr,u32InDataLength,DMA_FROM_DEVICE); 
	lldSe_WriteReg(R_SE_DMACTRL, DMACTRL_START_DMA & (~DC_START_DMA));   
	return 0;
}

