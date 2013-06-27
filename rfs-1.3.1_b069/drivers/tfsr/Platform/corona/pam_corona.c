/**
 *   @mainpage   Flex Sector Remapper : RFS_1.3.1_b060-LinuStoreIII_1.1.0_b022-FSR_1.1.1_b112_RC
 *
 *   @section Intro
 *       Flash Translation Layer for Flex-OneNAND and OneNAND
 *    
 *    @section  Copyright
 *            COPYRIGHT. 2007 - 2008 SAMSUNG ELECTRONICS CO., LTD.               
 *                            ALL RIGHTS RESERVED                              
 *                                                                             
 *     Permission is hereby granted to licensees of Samsung Electronics        
 *     Co., Ltd. products to use or abstract this computer program for the     
 *     sole purpose of implementing a product based on Samsung                 
 *     Electronics Co., Ltd. products. No other rights to reproduce, use,      
 *     or disseminate this computer program, whether in part or in whole,      
 *     are granted.                                                            
 *                                                                             
 *     Samsung Electronics Co., Ltd. makes no representation or warranties     
 *     with respect to the performance of this computer program, and           
 *     specifically disclaims any responsibility for any damages,              
 *     special or consequential, connected with the use of this program.       
 *
 *     @section Description
 *
 */

/**
 * @file      FSR_PAM_ApollonPLUS.c
 * @brief     This file contain the Platform Adaptation Modules for ApollonPLUS
 * @author    SongHo Yoon
 * @date      15-MAR-2007
 * @remark
 * REVISION HISTORY
 * @n  15-MAR-2007 [SongHo Yoon] : first writing
 *
 */

#include "FSR.h"
#include <linux/fsr_if.h> // byjins
#include <asm/cacheflush.h>

/*****************************************************************************/
/* [PAM customization]                                                       */
/* The following 5 parameters can be customized                              */
/*                                                                           */
/* - FSR_ONENAND_PHY_BASE_ADDR                                               */
/* - FSR_ENABLE_WRITE_DMA                                                    */
/* - FSR_ENABLE_READ_DMA                                                     */
/* - FSR_ENABLE_FLEXOND_LFT                                                  */
/* - FSR_ENABLE_ONENAND_LFT                                                  */
/*                                                                           */
/*****************************************************************************/
/**< if FSR_ENABLE_FLEXOND_LFT is defined, 
     Low level function table is linked with Flex-OneNAND LLD */
#define     FSR_ENABLE_FLEXOND_LFT

/**< if FSR_ENABLE_ONENAND_LFT is defined, 
     Low level function table is linked with OneNAND LLD */
#define     FSR_ENABLE_ONENAND_LFT

/**< if FSR_ENABLE_4K_ONENAND_LFT is defined, 
     Low level function table is linked with OneNAND LLD */
#define     FSR_ENABLE_4K_ONENAND_LFT

#if defined(FSR_WINCE_OAM)

    //#include <oal_memory.h>
    #include <omap2420_irq.h>

    #define     FSR_ONENAND_PHY_BASE_ADDR           ONENAND_BASEADDR
    //#define     FSR_ONENAND_VIR_BASE_ADDR           ((UINT32)OALPAtoVA(ONENAND_BASEADDR,FALSE))
    #define     FSR_ONENAND_VIR_BASE_ADDR           0xA0000000

    /**< if FSR_ENABLE_WRITE_DMA is defined, write DMA is enabled */
    #undef      FSR_ENABLE_WRITE_DMA
    /**< if FSR_ENABLE_READ_DMA is defined, read DMA is enabled */
    #undef      FSR_ENABLE_READ_DMA

    #define     IRQ_GPIO_72                         (IRQ_GPIO_64 + 8 )

#elif defined(FSR_SYMOS_OAM)

    #include <Shared_gpio.h>

    #define     FSR_ONENAND_PHY_BASE_ADDR           (0x00000000)

    /**< if FSR_ENABLE_WRITE_DMA is defined, write DMA is enabled */
    #undef      FSR_ENABLE_WRITE_DMA
    /**< if FSR_ENABLE_READ_DMA is defined, read DMA is enabled */
    #undef      FSR_ENABLE_READ_DMA

    #define     OMAP2420_GPIO_MODULE                (2)
    #define     OMAP2420_GPIO_PIN                   (72)

#elif defined(FSR_LINUX_OAM)

    #if defined(TINY_FSR)
    /* Setting at Kernel configuration */
        #define     FSR_ONENAND_PHY_BASE_ADDR       CONFIG_TINY_FLASH_PHYS_ADDR
    #else
    	#define     FSR_ONENAND_PHY_BASE_ADDR       CONFIG_FSR_FLASH_PHYS_ADDR
    #endif

    /**< if FSR_ENABLE_WRITE_DMA is defined, write DMA is enabled */
    #undef      FSR_ENABLE_WRITE_DMA
    /**< if FSR_ENABLE_READ_DMA is defined, read DMA is enabled */
    #undef      FSR_ENABLE_READ_DMA

#else /* RTOS (such as Nucleus) or OSLess */

    /* ij.jang : fnw/onboot not use standard c header files */
#if 0
    #include <stdarg.h>
    #include <stdio.h>
#endif
    //#include "../OSLessBSP/omap2420.h"
    
    //#define     FSR_ONENAND_PHY_BASE_ADDR           (0x00100000)
    #define     FSR_ONENAND_PHY_BASE_ADDR           (0x0)

    /**< if FSR_ENABLE_WRITE_DMA is defined, write DMA is enabled */
    #undef      FSR_ENABLE_WRITE_DMA
    /**< if FSR_ENABLE_READ_DMA is defined, read DMA is enabled */
    #undef      FSR_ENABLE_READ_DMA

#endif

#if defined(FSR_ENABLE_FLEXOND_LFT)
    #include "FSR_LLD_FlexOND.h"
#endif

#if defined(FSR_ENABLE_ONENAND_LFT)
    #include "FSR_LLD_OneNAND.h"
#endif

#if defined(FSR_ENABLE_4K_ONENAND_LFT)
    #include "FSR_LLD_4K_OneNAND.h"
#endif


#define USE_MMAP_CACHED // byjins 10.09
/* memcpy_corona.c */
void* memcpy32(void *pDst, const void *pSrc, unsigned int nLen);

/*****************************************************************************/
/* Global variables definitions                                              */
/*****************************************************************************/

/*****************************************************************************/
/* Local #defines                                                            */
/*****************************************************************************/
#define     ONENAND_DID_FLEX        (2)
#define     ONENAND_DID_SLC         (0)
#define     ONENAND_DID_MLC         (1)
#define     ONENAND_DIE_REG_OFFSET  (0x0001e002)
#define     FSR_FND_4K_PAGE         (0)
#define     FSR_OND_2K_PAGE         (1)
#define     FSR_OND_4K_PAGE         (2)

#define     DBG_PRINT(x)
#define     RTL_PRINT(x)


/*****************************************************************************/
/* Local typedefs                                                            */
/*****************************************************************************/

/*****************************************************************************/
/* Static variables definitions                                              */
/*****************************************************************************/

PRIVATE FsrVolParm              gstFsrVolParm[FSR_MAX_VOLS];
PRIVATE BOOL32                  gbPAMInit                   = FALSE32;
PRIVATE BOOL32                  gbUseWriteDMA               = FALSE32;
PRIVATE BOOL32                  gbUseReadDMA                = FALSE32;
PRIVATE UINT32                  gbFlexOneNAND[FSR_MAX_VOLS] = {FSR_OND_2K_PAGE, FSR_OND_2K_PAGE};
#if defined(FSR_ENABLE_ONENAND_LFT)
PRIVATE volatile OneNANDReg     *gpOneNANDReg               = (volatile OneNANDReg *) 0;
#elif defined(FSR_ENABLE_4K_ONENAND_LFT)
PRIVATE volatile OneNAND4kReg   *gpOneNANDReg               = (volatile OneNAND4kReg *) 0;
#elif defined(FSR_ENABLE_FLEXOND_LFT)
PRIVATE volatile FlexOneNANDReg *gpOneNANDReg               = (volatile FlexOneNANDReg *) 0;
#else
#error  Either FSR_ENABLE_FLEXOND_LFT or FSR_ENABLE_ONENAND_LFT should be defined
#endif

#if defined(USE_MMAP_CACHED)// byjins. 10.08
#include <asm/io.h>
PRIVATE UINT32 gcached_ioaddr_offset;
void dma_cache_maint(const void *start, size_t size, int direction);
#endif

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */
#ifdef __cplusplus
}
#endif /* __cplusplus */

/*****************************************************************************/
/* Function Implementation                                                   */
/*****************************************************************************/

VOID
FSR_PAM_Memcpy(VOID *pDst, VOID *pSrc, UINT32 nLen)
{


    memcpy32(pDst,pSrc,nLen);
	
}



/**
 * @brief           This function initializes PAM
 *                  this function is called by FSR_BML_Init
 *
 * @return          FSR_PAM_SUCCESS
 *
 * @author          SongHo Yoon
 * @version         1.0.0
 *
 */
PUBLIC INT32
FSR_PAM_Init(VOID)
{
    INT32   nRe = FSR_PAM_SUCCESS;
    UINT32  nONDVirBaseAddr;
#if defined(CONFIG_FSR_DUAL_VOLUME)
    UINT32  nONDVirBaseAddr_1;
#endif
    FSR_STACK_VAR;

    FSR_STACK_END;

    if (gbPAMInit == TRUE32)
    {
        return FSR_PAM_SUCCESS;
    }
    gbPAMInit     = TRUE32;
    
    RTL_PRINT((TEXT("[PAM:   ] ++%s\r\n"), __FSR_FUNC__));

    do
    {
#if defined(FSR_ENABLE_WRITE_DMA)
        gbUseWriteDMA = TRUE32;
#else
        gbUseWriteDMA = FALSE32;
#endif

#if defined(FSR_ENABLE_READ_DMA)
        gbUseReadDMA  = TRUE32;
#else
        gbUseReadDMA  = FALSE32;
#endif

        nONDVirBaseAddr  = FSR_OAM_Pa2Va(FSR_ONENAND_PHY_BASE_ADDR);
        RTL_PRINT((TEXT("[PAM:   ]   OneNAND physical base address       : 0x%08x\r\n"), FSR_ONENAND_PHY_BASE_ADDR));
        RTL_PRINT((TEXT("[PAM:   ]   OneNAND virtual  base address       : 0x%08x\r\n"), nONDVirBaseAddr));

#if defined(FSR_ENABLE_ONENAND_LFT)
        gpOneNANDReg  = (volatile OneNANDReg *) nONDVirBaseAddr;
#elif defined(FSR_ENABLE_4K_ONENAND_LFT)
        gpOneNANDReg  = (volatile OneNAND4kReg *) nONDVirBaseAddr;
#elif defined(FSR_ENABLE_FLEXOND_LFT)
        gpOneNANDReg  = (volatile FlexOneNANDReg *) nONDVirBaseAddr;
#endif

        /* check manufacturer ID */
        if (gpOneNANDReg->nMID != 0x00ec)
        {
            /* ERROR : OneNAND address space is not mapped with address space of ARM core */
            RTL_PRINT((TEXT("[PAM:ERR] OneNAND address space is not mapped with address space of ARM core\r\n")));
            nRe = FSR_PAM_NAND_PROBE_FAILED;
            break;
        }

        /* check whether current attached OneNAND is Flex-OneNAND or OneNAND */
        if (((gpOneNANDReg->nDID >> 8) & 0x03) == ONENAND_DID_FLEX)
        {
            gbFlexOneNAND[0] = FSR_FND_4K_PAGE;
            gbFlexOneNAND[1] = FSR_FND_4K_PAGE;

            RTL_PRINT((TEXT("[PAM:   ]   Flex-OneNAND nMID=0x%2x : nDID=0x%02x\r\n"), 
                    gpOneNANDReg->nMID, gpOneNANDReg->nDID));
        }
        else
        {
            gbFlexOneNAND[0] = FSR_OND_2K_PAGE;
            gbFlexOneNAND[1] = FSR_OND_2K_PAGE;
            
            /* FB mask for supporting Demux and Mux device. */ 
            if (((gpOneNANDReg->nDID & 0xFB) == 0x50) || ((gpOneNANDReg->nDID & 0xFB) == 0x68))
            {
                gbFlexOneNAND[0] = FSR_OND_4K_PAGE;
                gbFlexOneNAND[1] = FSR_OND_4K_PAGE;
            }

            RTL_PRINT((TEXT("[PAM:   ]   OneNAND nMID=0x%2x : nDID=0x%02x\r\n"), 
                    gpOneNANDReg->nMID, gpOneNANDReg->nDID));
        }

        gstFsrVolParm[0].nBaseAddr[0] = nONDVirBaseAddr;
#if !defined(USE_MMAP_CACHED)
        gstFsrVolParm[0].nBaseAddr[1] = FSR_PAM_NOT_MAPPED;
#else
		gstFsrVolParm[0].nBaseAddr[1] = (unsigned long) ioremap_cached(FSR_ONENAND_PHY_BASE_ADDR, SZ_128K);
		gcached_ioaddr_offset = gstFsrVolParm[0].nBaseAddr[1] - gstFsrVolParm[0].nBaseAddr[0];		
#endif        
        gstFsrVolParm[0].nIntID[0]    = FSR_INT_ID_NAND_0;
        gstFsrVolParm[0].nIntID[1]    = FSR_INT_ID_NONE;
        gstFsrVolParm[0].nDevsInVol   = 1;
        gstFsrVolParm[0].bProcessorSynchronization = FALSE32;
        gstFsrVolParm[0].pExInfo      = NULL;

#ifdef CONFIG_FSR_DUAL_VOLUME
        nONDVirBaseAddr  = FSR_OAM_Pa2Va(CONFIG_FSR_FLASH_PHYS_ADDR2);
        gstFsrVolParm[1].nBaseAddr[0] = nONDVirBaseAddr;
        gstFsrVolParm[1].nBaseAddr[1] = FSR_PAM_NOT_MAPPED;
        gstFsrVolParm[1].nIntID[0]    = FSR_INT_ID_NAND_1;;
        gstFsrVolParm[1].nIntID[1]    = FSR_INT_ID_NONE;
        gstFsrVolParm[1].nDevsInVol   = 1;
        gstFsrVolParm[1].bProcessorSynchronization = FALSE32;
        gstFsrVolParm[1].pExInfo      = NULL;
#else
        gstFsrVolParm[1].nBaseAddr[0] = FSR_PAM_NOT_MAPPED;
        gstFsrVolParm[1].nBaseAddr[1] = FSR_PAM_NOT_MAPPED;
        gstFsrVolParm[1].nIntID[0]    = FSR_INT_ID_NONE;;
        gstFsrVolParm[1].nIntID[1]    = FSR_INT_ID_NONE;
        gstFsrVolParm[1].nDevsInVol   = 0;
        gstFsrVolParm[1].bProcessorSynchronization = FALSE32;
        gstFsrVolParm[1].pExInfo      = NULL;
#endif
    } while (0);

    RTL_PRINT((TEXT("[PAM:   ] --%s\r\n"), __FSR_FUNC__));

    return nRe;
}

/**
 * @brief           This function returns FSR volume parameter
 *                  this function is called by FSR_BML_Init
 *
 * @param[in]       stVolParm[FSR_MAX_VOLS] : FsrVolParm data structure array
 *
 * @return          FSR_PAM_SUCCESS
 * @return          FSR_PAM_NOT_INITIALIZED
 *
 * @author          SongHo Yoon
 * @version         1.0.0
 *
 */
PUBLIC INT32
FSR_PAM_GetPAParm(FsrVolParm stVolParm[FSR_MAX_VOLS])
{
    FSR_STACK_VAR;

    FSR_STACK_END;

    if (gbPAMInit == FALSE32)
    {
        return FSR_PAM_NOT_INITIALIZED;
    }

    FSR_OAM_MEMCPY(&(stVolParm[0]), &gstFsrVolParm[0], sizeof(FsrVolParm));
    FSR_OAM_MEMCPY(&(stVolParm[1]), &gstFsrVolParm[1], sizeof(FsrVolParm));

    return FSR_PAM_SUCCESS;
}

/**
 * @brief           This function registers LLD function table
 *                  this function is called by FSR_BML_Open
 *
 * @param[in]      *pstLFT[FSR_MAX_VOLS] : pointer to FSRLowFuncTable data structure
 *
 * @return          none
 *
 * @author          SongHo Yoon
 * @version         1.0.0
 *
 */
PUBLIC INT32
FSR_PAM_RegLFT(FSRLowFuncTbl  *pstLFT[FSR_MAX_VOLS])
{
    UINT32  nVolIdx;
    FSR_STACK_VAR;

    FSR_STACK_END;

    if (gbPAMInit == FALSE32)
    {
        return FSR_PAM_NOT_INITIALIZED;
    }

    if (gstFsrVolParm[0].nDevsInVol > 0)
    {
        nVolIdx = 0;

        if (gbFlexOneNAND[0] == FSR_FND_4K_PAGE)
        {
#if defined(FSR_ENABLE_FLEXOND_LFT)
            pstLFT[nVolIdx]->LLD_Init               = FSR_FND_Init;
            pstLFT[nVolIdx]->LLD_Open               = FSR_FND_Open;
            pstLFT[nVolIdx]->LLD_Close              = FSR_FND_Close;
            pstLFT[nVolIdx]->LLD_Erase              = FSR_FND_Erase;
            pstLFT[nVolIdx]->LLD_ChkBadBlk          = FSR_FND_ChkBadBlk;
            pstLFT[nVolIdx]->LLD_FlushOp            = FSR_FND_FlushOp;
            pstLFT[nVolIdx]->LLD_GetDevSpec         = FSR_FND_GetDevSpec;
            pstLFT[nVolIdx]->LLD_Read               = FSR_FND_Read;
            pstLFT[nVolIdx]->LLD_ReadOptimal        = FSR_FND_ReadOptimal;
            pstLFT[nVolIdx]->LLD_Write              = FSR_FND_Write;
            pstLFT[nVolIdx]->LLD_CopyBack           = FSR_FND_CopyBack;
            pstLFT[nVolIdx]->LLD_GetPrevOpData      = FSR_FND_GetPrevOpData;
            pstLFT[nVolIdx]->LLD_IOCtl              = FSR_FND_IOCtl;
            pstLFT[nVolIdx]->LLD_InitLLDStat        = FSR_FND_InitLLDStat;
            pstLFT[nVolIdx]->LLD_GetStat            = FSR_FND_GetStat;
            pstLFT[nVolIdx]->LLD_GetBlockInfo       = FSR_FND_GetBlockInfo;
            pstLFT[nVolIdx]->LLD_GetNANDCtrllerInfo = FSR_FND_GetPlatformInfo;
#else
            RTL_PRINT((TEXT("[PAM:ERR] LowFuncTbl(FlexOneNAND) isn't linked : %s / line %d\r\n"), __FSR_FUNC__, __LINE__));
            return FSR_PAM_LFT_NOT_LINKED;
#endif
        }
        else if (gbFlexOneNAND[0] == FSR_OND_2K_PAGE)
        {
#if defined(FSR_ENABLE_ONENAND_LFT)
            pstLFT[nVolIdx]->LLD_Init               = FSR_OND_Init;
            pstLFT[nVolIdx]->LLD_Open               = FSR_OND_Open;
            pstLFT[nVolIdx]->LLD_Close              = FSR_OND_Close;
            pstLFT[nVolIdx]->LLD_Erase              = FSR_OND_Erase;
            pstLFT[nVolIdx]->LLD_ChkBadBlk          = FSR_OND_ChkBadBlk;
            pstLFT[nVolIdx]->LLD_FlushOp            = FSR_OND_FlushOp;
            pstLFT[nVolIdx]->LLD_GetDevSpec         = FSR_OND_GetDevSpec;
            pstLFT[nVolIdx]->LLD_Read               = FSR_OND_Read;
            pstLFT[nVolIdx]->LLD_ReadOptimal        = FSR_OND_ReadOptimal;
            pstLFT[nVolIdx]->LLD_Write              = FSR_OND_Write;
            pstLFT[nVolIdx]->LLD_CopyBack           = FSR_OND_CopyBack;
            pstLFT[nVolIdx]->LLD_GetPrevOpData      = FSR_OND_GetPrevOpData;
            pstLFT[nVolIdx]->LLD_IOCtl              = FSR_OND_IOCtl;
            pstLFT[nVolIdx]->LLD_InitLLDStat        = FSR_OND_InitLLDStat;
            pstLFT[nVolIdx]->LLD_GetStat            = FSR_OND_GetStat;
            pstLFT[nVolIdx]->LLD_GetBlockInfo       = FSR_OND_GetBlockInfo;
            pstLFT[nVolIdx]->LLD_GetNANDCtrllerInfo = FSR_OND_GetNANDCtrllerInfo;
#else
            RTL_PRINT((TEXT("[PAM:ERR] LowFuncTbl(OneNAND) isn't linked : %s / line %d\r\n"), __FSR_FUNC__, __LINE__));
            return FSR_PAM_LFT_NOT_LINKED;
#endif
        }
        else if (gbFlexOneNAND[0] == FSR_OND_4K_PAGE)
        {
#if defined(FSR_ENABLE_4K_ONENAND_LFT)
            pstLFT[nVolIdx]->LLD_Init               = FSR_OND_4K_Init;
            pstLFT[nVolIdx]->LLD_Open               = FSR_OND_4K_Open;
            pstLFT[nVolIdx]->LLD_Close              = FSR_OND_4K_Close;
            pstLFT[nVolIdx]->LLD_Erase              = FSR_OND_4K_Erase;
            pstLFT[nVolIdx]->LLD_ChkBadBlk          = FSR_OND_4K_ChkBadBlk;
            pstLFT[nVolIdx]->LLD_FlushOp            = FSR_OND_4K_FlushOp;
            pstLFT[nVolIdx]->LLD_GetDevSpec         = FSR_OND_4K_GetDevSpec;
            pstLFT[nVolIdx]->LLD_Read               = FSR_OND_4K_Read;
            pstLFT[nVolIdx]->LLD_ReadOptimal        = FSR_OND_4K_ReadOptimal;
            pstLFT[nVolIdx]->LLD_Write              = FSR_OND_4K_Write;
            pstLFT[nVolIdx]->LLD_CopyBack           = FSR_OND_4K_CopyBack;
            pstLFT[nVolIdx]->LLD_GetPrevOpData      = FSR_OND_4K_GetPrevOpData;
            pstLFT[nVolIdx]->LLD_IOCtl              = FSR_OND_4K_IOCtl;
            pstLFT[nVolIdx]->LLD_InitLLDStat        = FSR_OND_4K_InitLLDStat;
            pstLFT[nVolIdx]->LLD_GetStat            = FSR_OND_4K_GetStat;
            pstLFT[nVolIdx]->LLD_GetBlockInfo       = FSR_OND_4K_GetBlockInfo;
            pstLFT[nVolIdx]->LLD_GetNANDCtrllerInfo = FSR_OND_4K_GetPlatformInfo;
#else
            RTL_PRINT((TEXT("[PAM:ERR] LowFuncTbl(FlexOneNAND) isn't linked : %s / line %d\r\n"), __FSR_FUNC__, __LINE__));
            return FSR_PAM_LFT_NOT_LINKED;
#endif
        }
    }

    if (gstFsrVolParm[1].nDevsInVol > 0)
    {
        nVolIdx = 1;

        if (gbFlexOneNAND[1] == FSR_FND_4K_PAGE)
        {
#if defined(FSR_ENABLE_FLEXOND_LFT)
            pstLFT[nVolIdx]->LLD_Init               = FSR_FND_Init;
            pstLFT[nVolIdx]->LLD_Open               = FSR_FND_Open;
            pstLFT[nVolIdx]->LLD_Close              = FSR_FND_Close;
            pstLFT[nVolIdx]->LLD_Erase              = FSR_FND_Erase;
            pstLFT[nVolIdx]->LLD_ChkBadBlk          = FSR_FND_ChkBadBlk;
            pstLFT[nVolIdx]->LLD_FlushOp            = FSR_FND_FlushOp;
            pstLFT[nVolIdx]->LLD_GetDevSpec         = FSR_FND_GetDevSpec;
            pstLFT[nVolIdx]->LLD_Read               = FSR_FND_Read;
            pstLFT[nVolIdx]->LLD_ReadOptimal        = FSR_FND_ReadOptimal;
            pstLFT[nVolIdx]->LLD_Write              = FSR_FND_Write;
            pstLFT[nVolIdx]->LLD_CopyBack           = FSR_FND_CopyBack;
            pstLFT[nVolIdx]->LLD_GetPrevOpData      = FSR_FND_GetPrevOpData;
            pstLFT[nVolIdx]->LLD_IOCtl              = FSR_FND_IOCtl;
            pstLFT[nVolIdx]->LLD_InitLLDStat        = FSR_FND_InitLLDStat;
            pstLFT[nVolIdx]->LLD_GetStat            = FSR_FND_GetStat;
            pstLFT[nVolIdx]->LLD_GetBlockInfo       = FSR_FND_GetBlockInfo;
            pstLFT[nVolIdx]->LLD_GetNANDCtrllerInfo = FSR_FND_GetPlatformInfo;
#else
            RTL_PRINT((TEXT("[PAM:ERR] LowFuncTbl(FlexOneNAND) isn't linked : %s / line %d\r\n"), __FSR_FUNC__, __LINE__));
            return FSR_PAM_LFT_NOT_LINKED;
#endif
        }
        else if (gbFlexOneNAND[0] == FSR_OND_2K_PAGE)
        {   
#if defined(FSR_ENABLE_ONENAND_LFT)
            pstLFT[nVolIdx]->LLD_Init               = FSR_OND_Init;
            pstLFT[nVolIdx]->LLD_Open               = FSR_OND_Open;
            pstLFT[nVolIdx]->LLD_Close              = FSR_OND_Close;
            pstLFT[nVolIdx]->LLD_Erase              = FSR_OND_Erase;
            pstLFT[nVolIdx]->LLD_ChkBadBlk          = FSR_OND_ChkBadBlk;
            pstLFT[nVolIdx]->LLD_FlushOp            = FSR_OND_FlushOp;
            pstLFT[nVolIdx]->LLD_GetDevSpec         = FSR_OND_GetDevSpec;
            pstLFT[nVolIdx]->LLD_Read               = FSR_OND_Read;
            pstLFT[nVolIdx]->LLD_ReadOptimal        = FSR_OND_ReadOptimal;
            pstLFT[nVolIdx]->LLD_Write              = FSR_OND_Write;
            pstLFT[nVolIdx]->LLD_CopyBack           = FSR_OND_CopyBack;
            pstLFT[nVolIdx]->LLD_GetPrevOpData      = FSR_OND_GetPrevOpData;
            pstLFT[nVolIdx]->LLD_IOCtl              = FSR_OND_IOCtl;
            pstLFT[nVolIdx]->LLD_InitLLDStat        = FSR_OND_InitLLDStat;
            pstLFT[nVolIdx]->LLD_GetStat            = FSR_OND_GetStat;
            pstLFT[nVolIdx]->LLD_GetBlockInfo       = FSR_OND_GetBlockInfo;
            pstLFT[nVolIdx]->LLD_GetNANDCtrllerInfo = FSR_OND_GetNANDCtrllerInfo;
#else
            RTL_PRINT((TEXT("[PAM:ERR] LowFuncTbl(OneNAND) isn't linked : %s / line %d\r\n"), __FSR_FUNC__, __LINE__));
            return FSR_PAM_LFT_NOT_LINKED;
#endif
        }
        else if (gbFlexOneNAND[0] == FSR_OND_4K_PAGE)
        {
#if defined(FSR_ENABLE_4K_ONENAND_LFT)
            pstLFT[nVolIdx]->LLD_Init               = FSR_OND_4K_Init;
            pstLFT[nVolIdx]->LLD_Open               = FSR_OND_4K_Open;
            pstLFT[nVolIdx]->LLD_Close              = FSR_OND_4K_Close;
            pstLFT[nVolIdx]->LLD_Erase              = FSR_OND_4K_Erase;
            pstLFT[nVolIdx]->LLD_ChkBadBlk          = FSR_OND_4K_ChkBadBlk;
            pstLFT[nVolIdx]->LLD_FlushOp            = FSR_OND_4K_FlushOp;
            pstLFT[nVolIdx]->LLD_GetDevSpec         = FSR_OND_4K_GetDevSpec;
            pstLFT[nVolIdx]->LLD_Read               = FSR_OND_4K_Read;
            pstLFT[nVolIdx]->LLD_ReadOptimal        = FSR_OND_4K_ReadOptimal;
            pstLFT[nVolIdx]->LLD_Write              = FSR_OND_4K_Write;
            pstLFT[nVolIdx]->LLD_CopyBack           = FSR_OND_4K_CopyBack;
            pstLFT[nVolIdx]->LLD_GetPrevOpData      = FSR_OND_4K_GetPrevOpData;
            pstLFT[nVolIdx]->LLD_IOCtl              = FSR_OND_4K_IOCtl;
            pstLFT[nVolIdx]->LLD_InitLLDStat        = FSR_OND_4K_InitLLDStat;
            pstLFT[nVolIdx]->LLD_GetStat            = FSR_OND_4K_GetStat;
            pstLFT[nVolIdx]->LLD_GetBlockInfo       = FSR_OND_4K_GetBlockInfo;
            pstLFT[nVolIdx]->LLD_GetNANDCtrllerInfo = FSR_OND_4K_GetPlatformInfo;
#else
            RTL_PRINT((TEXT("[PAM:ERR] LowFuncTbl(FlexOneNAND) isn't linked : %s / line %d\r\n"), __FSR_FUNC__, __LINE__));
            return FSR_PAM_LFT_NOT_LINKED;
#endif
        }
    }

    return FSR_PAM_SUCCESS;
}


/* ij.jang : add sdp93 support */
/************************************************************
 * SDP93 support
 ************************************************************/
#if 0	/* u-boot */
#include <config.h>
#include <asm/arch/sdp93.h>

#if !defined(CONFIG_SDP93)
#error this PAM is dedicated for SDP83 chelsea
#endif
#endif

#if 0	/* onboot */
#include "sdp93.h"
#endif

#if 1	/* linux */
#include <asm/arch/sdp93.h>
#endif

/* onenand clock disable/enable */
static inline void sdp92_smc_clock(int flag, unsigned int addr)
{
#ifndef CONFIG_FSR_DUAL_VOLUME
	if (flag) {
		R_SMC_SYNCEN |= 1;
	} else {
		R_SMC_SYNCEN &= ~1;
	}
#else
	unsigned int shift=0;
	if(gstFsrVolParm[0].nBaseAddr[0] != (addr & ~(SZ_128K-1))) 
		shift++;
	if (flag) {
		R_SMC_SYNCEN |= (1<<shift);
	} else {
		R_SMC_SYNCEN &= ~(1<<shift);
	}
#endif
}
void FSR_PAM_WRITE16(volatile unsigned short *reg, unsigned short v)
{
	FSR_STACK_VAR;
	FSR_STACK_END;

	//FSR_ASSERT( (((unsigned int)reg) & 3) == 0);

	sdp92_smc_clock(0, (UINT32)reg);
	*reg = v;
	sdp92_smc_clock(1, (UINT32)reg);
}

void FSR_PAM_MEMSET_DATARAM(void * s,int c, unsigned int count)
{
	FSR_STACK_VAR;
	FSR_STACK_END;

	sdp92_smc_clock(0, (UINT32)s);
	FSR_OAM_MEMSET(s, c, count);
	sdp92_smc_clock(1, (UINT32)s);
}

/**
 * @brief           This function transfers data to NAND
 *
 * @param[in]      *pDst  : Destination array Pointer to be copied
 * @param[in]      *pSrc  : Source data allocated Pointer
 * @param[in]      *nSize : length to be transferred
 *
 * @return          none
 *
 * @author          SongHo Yoon
 * @version         1.0.0
 * @remark          pDst / pSrc address should be aligned by 4 bytes.
 *                  DO NOT USE memcpy of standard library 
 *                  memcpy (RVDS2.2) can do wrong memory copy operation.
 *                  memcpy32 is optimized by using multiple load/store instruction
 *
 */
PUBLIC VOID
FSR_PAM_TransToNAND(volatile VOID *pDst,
                    VOID          *pSrc,
                    UINT32        nSize)
{
    FSR_STACK_VAR;

    FSR_STACK_END;

    FSR_ASSERT(((UINT32) pSrc & 0x03) == 0x00000000);
    FSR_ASSERT(((UINT32) pDst & 0x03) == 0x00000000);
    FSR_ASSERT(nSize > sizeof(UINT32));
   
    sdp92_smc_clock(0, (UINT32)pDst);
    memcpy32((void *) pDst, pSrc, nSize);
    sdp92_smc_clock(1, (UINT32)pDst);
}

/**
 * @brief           This function transfers data from NAND
 *
 * @param[in]      *pDst  : Destination array Pointer to be copied
 * @param[in]      *pSrc  : Source data allocated Pointer
 * @param[in]      *nSize : length to be transferred
 *
 * @return          none
 *
 * @author          SongHo Yoon
 * @version         1.0.0
 * @remark          pDst / pSrc address should be aligned by 4 bytes
 *                  DO NOT USE memcpy of standard library 
 *                  memcpy (RVDS2.2) can do wrong memory copy operation
 *                  memcpy32 is optimized by using multiple load/store instruction
 *
 */
VOID
FSR_PAM_TransFromNAND(VOID          *pDst,
                      volatile VOID *pSrc,
                      UINT32         nSize)
{
    FSR_STACK_VAR;

    FSR_STACK_END;

    FSR_ASSERT(((UINT32) pSrc & 0x03) == 0x00000000);
    FSR_ASSERT(((UINT32) pDst & 0x03) == 0x00000000);
    FSR_ASSERT(nSize > sizeof(UINT32));

#if !defined(USE_MMAP_CACHED)
    memcpy32(pDst, (void *) pSrc, nSize);
#else
	dmb();		
	dma_cache_maint((void*)(gstFsrVolParm[0].nBaseAddr[1]+0x400), 0x1000, 2/*DMA_FROM_DEVICE*/);
    memcpy32(pDst, (void *)((unsigned long)pSrc+gcached_ioaddr_offset), nSize);    
	__cpuc_coherent_kern_range(pDst, pDst+nSize);
#endif

}

/**
 * @brief           This function initializes the specified logical interrupt.
 *
 * @param[in]       nLogIntId : Logical interrupt id
 * 
 * @return          FSR_PAM_SUCCESS
 *
 * @author          SongHo Yoon
 *
 * @version         1.0.0
 *
 * @remark          this function is used to support non-blocking I/O feature of FSR
 *
 */
PUBLIC INT32
FSR_PAM_InitInt(UINT32 nLogIntId)
{
    return FSR_PAM_SUCCESS;
}

/**
 * @brief           This function deinitializes the specified logical interrupt.
 *
 * @param[in]       nLogIntId : Logical interrupt id
 * 
 * @return          FSR_PAM_SUCCESS
 *
 * @author          SongHo Yoon
 *
 * @version         1.0.0
 *
 * @remark          this function is used to support non-blocking I/O feature of FSR
 *
 */
PUBLIC INT32
FSR_PAM_DeinitInt(UINT32 nLogIntId)
{
    return FSR_PAM_SUCCESS;
}

/**
 * @brief           This function returns the physical interrupt ID from the logical interrupt ID
 *
 * @param[in]       nLogIntID : Logical interrupt id
 *
 * @return          physical interrupt ID
 *
 * @author          SongHo Yoon
 *
 * @version         1.0.0
 *
 */
UINT32
FSR_PAM_GetPhyIntID(UINT32  nLogIntID)
{
    return 0;
}

/**
 * @brief           This function enables the specified interrupt.
 *
 * @param[in]       nLogIntID : Logical interrupt id
 *
 * @return          FSR_PAM_SUCCESS
 *
 * @author          Seunghyun Han
 *
 * @version         1.0.0
 *
 */
INT32
FSR_PAM_ClrNEnableInt(UINT32 nLogIntID)
{
    switch (nLogIntID)
    {
    case FSR_INT_ID_NAND_0:

#if defined(FSR_SYMOS_OAM)
        pNandGpioInterrupt->AckInterruptNotif(); /* Interrupt Clear   */
        pNandGpioInterrupt->Enable();            /* Interrupt Enable  */
#elif defined(FSR_WINCE_OAM)

#elif defined(FSR_LINUX_OAM)

#else /* for RTOS or OSLess*/

#endif
        break;
    default:
        break;
    }

    return FSR_PAM_SUCCESS;
}

/**
 * @brief           This function disables the specified interrupt.
 *
 * @param[in]       nLogIntID : Logical interrupt id
 *
 * @return          FSR_PAM_SUCCESS
 *
 * @author          Seunghyun Han
 *
 * @version         1.0.0
 *
 */
INT32
FSR_PAM_ClrNDisableInt(UINT32 nLogIntID)
{
    switch (nLogIntID)
    {
    case FSR_INT_ID_NAND_0:

#if defined(FSR_SYMOS_OAM)
        pNandGpioInterrupt->AckInterruptNotif(); /* Interrupt Clear   */
        pNandGpioInterrupt->Enable(EFalse);      /* Interrupt Disable */
#elif defined(FSR_WINCE_OAM)

#elif defined(FSR_LINUX_OAM)

#else /* for RTOS or OSLess*/

#endif
        break;
    default:
        break;
    }

    return FSR_PAM_SUCCESS;
}


#if defined(FSR_SYMOS_OAM)
/**
 * @brief           This function is used only for SymOS.
 *
 * @param[in]       aPtr
 * @param[in]       aHandle
 *
 * @return          none
 *
 * @author          WooYoungYang
 *
 * @version         1.0.0
 *
 */
void TNandGpioInterrupt::NandInterrupt(TAny* aPtr, TOmapGpioPinHandle aHandle)
{
    FSR_PAM_ClrNDisableInt(FSR_INT_ID_NAND_0);
	pMDNand->iNandIreqDfc.Add();
}

/**
 * @brief           This function is used only for SymOS.
 *
 * @param[in]       aPinId
 *
 * @return          TInt
 *
 * @author          WooYoungYang
 *
 * @version         1.0.0
 *
 */
TInt TNandGpioInterrupt::Bind(TInt aPinId)
{
    iPinHandle = OmapGpioPinMgr::AcquirePin(aPinId);

    if (iPinHandle == KOmapGpioInvalidHandle)
        return KErrBadHandle;

	OmapGpioPinMgr::SetPinAsInput(iPinHandle);

    iPinWrapper = new TOmapGpioInputPinWrapper(iPinHandle);
    if (!iPinWrapper)
    {
        return KErrNoMemory;
    }

    // Configure the Intr on the falling edge
    iPinWrapper->SetEventDetection(EOmapGpioEDTEdge, EOmapGpioEDSUp);

    // Enable module wake-up on this pin (interrupt is also enabled)
    iPinWrapper->EnableWakeUpNotif();

    // Disable interrupt generation on the input pin
    iPinWrapper->EnableInterruptNotif(EFalse);

    return iPinWrapper->BindIsr(NandInterrupt, this);
}
#endif

/**
 * @brief           This function creates spin lock for dual core.
 *
 * @param[out]     *pHandle : Handle of semaphore
 * @param[in]       nLayer  : 0 : FSR_OAM_SM_TYPE_BDD
 *                            0 : FSR_OAM_SM_TYPE_STL
 *                            1 : FSR_OAM_SM_TYPE_BML
 *                            2 : FSR_OAM_SM_TYPE_LLD
 * 
 * @return          TRUE32   : this function creates spin lock successfully
 * @return          FALSE32  : fail
 *
 * @author          DongHoon Ham
 * @version         1.0.0
 * @remark          An initial count of spin lock object is 1.
 *                  An maximum count of spin lock object is 1.
 *
 */
BOOL32  
FSR_PAM_CreateSL(UINT32  *pHandle, UINT32  nLayer)
{
    return TRUE32;
}

/**
 * @brief          This function acquires spin lock for dual core.
 *
 * @param[in]       nHandle : Handle of semaphore to be acquired
 * @param[in]       nLayer  : 0 : FSR_OAM_SM_TYPE_BDD
 *                            0 : FSR_OAM_SM_TYPE_STL
 *                            1 : FSR_OAM_SM_TYPE_BML
 *                            2 : FSR_OAM_SM_TYPE_LLD
 * 
 * @return          TRUE32   : this function acquires spin lock successfully
 * @return          FALSE32  : fail
 *
 * @author          DongHoon Ham
 * @version         1.0.0
 *
 */
BOOL32  
FSR_PAM_AcquireSL(UINT32  nHandle, UINT32  nLayer)
{
    return TRUE32;
}

/**
 * @brief           This function releases spin lock for dual core.
 *
 * @param[in]       nHandle : Handle of semaphore to be released
 * @param[in]       nLayer  : 0 : FSR_OAM_SM_TYPE_BDD
 *                            0 : FSR_OAM_SM_TYPE_STL
 *                            1 : FSR_OAM_SM_TYPE_BML
 *                            2 : FSR_OAM_SM_TYPE_LLD
 *
 * @return          TRUE32   : this function releases spin lock successfully
 * @return          FALSE32  : fail
 *
 * @author          DongHoon Ham
 * @version         1.0.0
 *
 */
BOOL32
FSR_PAM_ReleaseSL(UINT32  nHandle, UINT32  nLayer)
{
    return TRUE32;
}
