/*
 *  sata_aspen.c - Aspen SATA
 *
 *  Maintained by:  Rudrajit Sengupta <r.sengupta@samsung.com>
 				Sunil M <sunilm@samsung.com>
 *
 * Copyright 2009 Samsung Electronics, Inc.,
 
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  libata documentation is available via 'make {ps|pdf}docs',
 *  as Documentation/DocBook/libata.*
 *
 *  Other errata and documentation available under NDA.
 *
 */
 
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/pci.h>
#include <linux/init.h>
#include <linux/blkdev.h>
#include <linux/delay.h>
#include <linux/interrupt.h>
#include <linux/device.h>
#include <scsi/scsi_host.h>
#include <linux/libata.h>
#include <linux/platform_device.h>
#include <scsi/scsi_cmnd.h>
#include <scsi/scsi.h>
#include <linux/cdrom.h>
#include "sata_aspen.h"

#define SDP_SHORT_PAUSE	100 /*100 usec*/
#define BM_STATUS_DMAING 	0x01
#define BM_STATUS_ERROR 	0x02
#define BM_STATUS_INT    	0x04
#define BM_STATUS_DMA_RETRY  0x08
#define BM_STATUS_PIO_RETRY  	0x10

#ifdef  CONFIG_SATA_SDP93_DEVICE_0
#define START_SATA_NUM	0
#else
#define START_SATA_NUM	1
#endif

#ifdef CONFIG_SATA_SDP93_DEVICE_1
#define LAST_SATA_NUM	2
#else
#define LAST_SATA_NUM	1
#endif


//#define ASPEN_DEBUG

/* Debug */
#ifdef ASPEN_DEBUG
#define ASPEN_DBG(args...) printk(args)
#else
#define ASPEN_DBG(args...)
#endif

#undef MAKE_DATA_ZERO

static struct _host{
	struct platform_device *pdevice;
	char	   name[25];
}sdp93host[NR_DEVS];


static int aspen_probe(struct platform_device *pdev);
static int aspen_remove(struct platform_device *pdev);
static void barc_sata_phy0_init(void);
static void barc_sata_phy1_init(void);


/*bmdma Register description*/
typedef struct dma_regs{
	void __iomem* DMASRC;
	void __iomem* DMADST;	
	void __iomem* DMACON;	
	void __iomem* DMASTS;	
	void __iomem* DMACSRC;	
	void __iomem* DMACDST;	
	void __iomem* DMAMASK;
	void __iomem* DMAINT;	
	void __iomem* DMACR;	
	void __iomem* DBTSR;	
} dma_regs_t;
static dma_regs_t bmdmareg[NR_DEVS]={
		{
			/*SATA0 RAPTOR*/
			.DMASRC=((void __iomem*)(SATA_DMA_BASE_HOST_0 + 0x00)),
			.DMADST=((void __iomem*)(SATA_DMA_BASE_HOST_0 + 0x04)),
			.DMACON=((void __iomem*)(SATA_DMA_BASE_HOST_0 + 0x08)),
			.DMASTS=((void __iomem*)(SATA_DMA_BASE_HOST_0 + 0x0c)),
			.DMACSRC=((void __iomem*)(SATA_DMA_BASE_HOST_0 + 0x10)),
			.DMACDST=((void __iomem*)(SATA_DMA_BASE_HOST_0 + 0x14)),
			.DMAMASK=((void __iomem*)(SATA_DMA_BASE_HOST_0 + 0x18)),
			.DMAINT=((void __iomem*)(SATA_DMA_BASE_HOST_0 + 0x20)),
			.DMACR=((void __iomem*)(SATA_REG_BASE_HOST_0 + 0x70)),			
			.DBTSR=((void __iomem*)(SATA_REG_BASE_HOST_0 + 0x74)),
		},
		{
			/*SATA1 NEC*/		
			.DMASRC=((void __iomem*)(SATA_DMA_BASE_HOST_1 + 0x00)),
			.DMADST=((void __iomem*)(SATA_DMA_BASE_HOST_1 + 0x04)),
			.DMACON=((void __iomem*)(SATA_DMA_BASE_HOST_1 + 0x08)),
			.DMASTS=((void __iomem*)(SATA_DMA_BASE_HOST_1 + 0x0c)),
			.DMACSRC=((void __iomem*)(SATA_DMA_BASE_HOST_1 + 0x10)),
			.DMACDST=((void __iomem*)(SATA_DMA_BASE_HOST_1 + 0x14)),
			.DMAMASK=((void __iomem*)(SATA_DMA_BASE_HOST_1 + 0x18)),
			.DMAINT=((void __iomem*)(SATA_DMA_BASE_HOST_1 + 0x20)),
			.DMACR=((void __iomem*)(SATA_REG_BASE_HOST_1 + 0x70)),			
			.DBTSR=((void __iomem*)(SATA_REG_BASE_HOST_1 + 0x74)),			
		},
};


static struct platform_driver  aspen_platform_driver[2] = {
{
  .probe                = aspen_probe,
  .remove               = aspen_remove,
  .driver               = {
//    .name       = DEV_NAME_SATA0,
    .owner      = THIS_MODULE,
  },
},
{
  .probe                = aspen_probe,
  .remove               = aspen_remove,
  .driver               = {
//    .name       = DEV_NAME_SATA0,
    .owner      = THIS_MODULE,
  },
},
};

static int sata_aspen_scr_read(struct ata_port *ap, unsigned int sc_reg, u32 *val);
static int sata_aspen_scr_write(struct ata_port *ap, unsigned int sc_reg, u32 val);
static unsigned int sata_aspen_qc_issue_prot(struct ata_queued_cmd *qc);

/* DMA ctrl Register, Read/ write */
struct SDMACON_t {
  u32 beatCount        :20;
  u32 transferDataSize :2;              
  u32 burstControl     :2;
  u32 hLock            :1;
  u32                  :7;      
};

/* DMA dst Register, Read/ write */
struct SDMADST_t {
  u32 dstaddr   :32;
};

/* DMA Src Register, Read/ write */
struct SDMASRC_t {
  u32 srcaddr                   :32;
};

/* DMA status Register, Read/ write */
struct SDMASTS_t {
  u32 currentTransCnt  :20;
  u32 state            :2;
  u32                  :11;     
};

/* DMA interrupt pending Register, Read/ write */
struct SDMAINT_t {
  u32 intp                      :32;
};

/* DMA Control Register, Read / Write */
struct DMACR_t {
  u32 TXCHEN                    :1;
  u32 RXCHEN                    :1;
u32                                     :30;// MSB            
};

/* DMA interrup mask Register, Read/ write */
struct SDMAMASK_t {
  u32                   :1;     
  u32 DMAEnable    :1;
  u32 DMADisable   :1;  
  u32                   :29;    
};

typedef union {
  unsigned int       rValue;
  struct SDMADST_t   rSata_DMADST;
  struct SDMASRC_t   rSata_DMASRC;      
  struct SDMACON_t   rSata_DMACON;
  struct SDMASTS_t   rSata_DMASTS;      
  struct SDMAINT_t   rSata_DMAINT;      
  struct DMACR_t     rSata_DMACR;       
  struct SDMAMASK_t  rSata_DMAMASK;     
}SATA_REG;

typedef enum _SATA_ONOFF{
  SATA_OFF,
  SATA_ON
}SATA_ONOFF;

typedef enum _SATA_CHANNEL{
  SATA_RX,
  SATA_TX
}SATA_CHANNEL;

#define MULTPLE_SG_BUFFERSIZE	(0x80000)

static void*        pvSataDmaBufVA[NR_DEVS] = {
	(void*)(MACH_BD_SUB1_BASE+(MACH_MEM1_SIZE-SYS_MEM1_SIZE)-MULTPLE_SG_BUFFERSIZE*2),
	(void*)(MACH_BD_SUB1_BASE+(MACH_MEM1_SIZE-SYS_MEM1_SIZE)-MULTPLE_SG_BUFFERSIZE)
};
static unsigned int uiSataDmaBufPA[NR_DEVS] = {
	(unsigned int)(MACH_MEM1_BASE+MACH_MEM1_SIZE-MULTPLE_SG_BUFFERSIZE*2),
	(unsigned int)(MACH_MEM1_BASE+MACH_MEM1_SIZE-MULTPLE_SG_BUFFERSIZE)
};

#if defined (MAKE_DATA_ZERO)
static unsigned int*	puiSataSetVA = (unsigned int*)(MACH_BD_SUB1_BASE+(MACH_MEM1_SIZE-SYS_MEM1_SIZE)-0x1000);
static unsigned int	uiSataSetPA = (MACH_MEM1_BASE+MACH_MEM1_SIZE-0x1000);
#endif

typedef struct
{
	dma_addr_t   addr;
	unsigned int uSize;
} BarScatter_t;

typedef struct __Barcelona_private_t
{
	unsigned int uiDevno;
	unsigned int uiIrqNum;
	void*			pvDmaBufVA;
	unsigned int	uiDmaBufPA;
	BarScatter_t tDma_ReadAddress[2048];
	BarScatter_t tDma_WriteAddress[2048];
	unsigned int uiReadSize;
	unsigned int uiWriteSize;
	unsigned int uiReadCount;
	unsigned int uiWriteCount;
	unsigned int uiDmaStarted;
	void 	(*ResetFuntion)(unsigned int uiIrq);
	char*	pcDeviceName;
	struct __Barcelona_private_t* ptasklet_FuncPrivate;
} Barcelona_private_t;

static void Bar_iowrite32(u32 val, void __iomem* addr)
{
	iowrite32(val, addr);
	
	/* To bug fix of barcelona dma problem , dummy wirte ahb register one more time*/
	iowrite32(val, 0x300dffff + DIFF_IO_BASE0);
}

static void barc_sata_reg_set(unsigned int addr, unsigned int val)
{
	iowrite32(val, addr + DIFF_IO_BASE0);
}

static void barc_Dma_reg_set(unsigned int addr, unsigned int val)
{
	Bar_iowrite32(val, ( void __iomem*)(addr + DIFF_IO_BASE0));
}

static unsigned int barc_sata_reg_get(unsigned int addr)
{
	return ioread32(addr + DIFF_IO_BASE0);
}


/**
 *This function sets the DMA src register
 *@function     lldSataSetDMASource
 *@return       void   
 *@param        void
 */
static void \
lldSataSetDMASource(void __iomem* addr, u32 nValue){
  SATA_REG sataReg;
  sataReg.rValue = ioread32(addr);
  sataReg.rSata_DMASRC.srcaddr = nValue;
  Bar_iowrite32(sataReg.rValue,addr);
}


/**
 *This function sets the DMA dst register
 *@function     lldSataSetDMADestination
 *@return       void   
 *@param        void
 */
static void \
lldSataSetDMADestination(void __iomem* addr, unsigned int nValue){
  SATA_REG sataReg;
  sataReg.rValue = ioread32(addr);
  sataReg.rSata_DMADST.dstaddr = nValue;
  Bar_iowrite32(sataReg.rValue, addr);
}

/**
 *This function sets the DMA dst register
 *@function     lldSataSetDMATransferByteSize
 *@return       void   
 *@param        void
 */
static void \
lldSataSetDMAHLock(void __iomem* addr, SATA_ONOFF onoff){
  SATA_REG sataReg;
  sataReg.rValue = ioread32(addr);
  sataReg.rSata_DMACON.hLock = onoff;
  Bar_iowrite32(sataReg.rValue, addr);
}

/**
 *This function sets the DMA dst register
 *@function     lldSataSetDMATransferByteSize
 *@return       void   
 *@param        void
 */
static void \
lldSataSetDMATransferByteSize(void __iomem* addr, u8 nValue){
  SATA_REG sataReg;
  sataReg.rValue = ioread32(addr);
  sataReg.rSata_DMACON.transferDataSize = nValue;
  Bar_iowrite32(sataReg.rValue, addr);
}

/**
 *This function sets the DMA dst register
 *@function     lldSataSetDMABeatCount
 *@return       void   
 *@param        u32
 */
static void \
lldSataSetDMABeatCount(void __iomem* addr, unsigned int nValue){
  SATA_REG sataReg;
  sataReg.rValue = ioread32(addr);
  sataReg.rSata_DMACON.beatCount = nValue;
  Bar_iowrite32(sataReg.rValue, addr);
}

/**
 *This function sets the DMA dst register
 *@function     lldSataSetDMABeatCount
 *@return       void   
 *@param        u32
 */
static u32 \
lldSataGetDMABeatCount(void __iomem* addr){
  SATA_REG sataReg;
  sataReg.rValue = ioread32(addr);
  return sataReg.rSata_DMACON.beatCount;
}

/**
 *This function sets the DMA dst register
 *@function     lldSataSetDMABurstSize
 *@return       void   
 *@param        void
 */
static void \
lldSataSetDMABurstSize(void __iomem* addr, u8 nValue){
  SATA_REG sataReg;
  sataReg.rValue = ioread32(addr);
  sataReg.rSata_DMACON.burstControl = nValue;
  Bar_iowrite32(sataReg.rValue, addr);
}

/**
 *This function set the DBTSR register
 *@function     lldSataSetDBTSR
 *@return       void   
 *@param        void
 */
static void
lldSataSetDBTSR(void __iomem* addr, u32 nValue){
  SATA_REG sataReg;

  sataReg.rValue = ioread32(addr);
  sataReg.rValue = nValue;
  Bar_iowrite32(sataReg.rValue,addr);
  return;
}

/**
 *This function gets the DMA Status
 *@function     lldSataGetDMAStatus
 *@return       u32   
 *@param        void
 */
static u32 \
lldSataGetDMAStatus(void __iomem* addr){
  SATA_REG sataReg;
        
  sataReg.rValue= ioread32(addr);
  //printk(KERN_ERR"\nlldSataGetDMAStatus: 0x%x ", sataReg.rSata_DMASTS.state);
  return sataReg.rSata_DMASTS.state;
}

/**
 *This function gets the DMA Status
 *@function     lldSataGetDMAStatus
 *@return       u32   
 *@param        void
 */
static u32 \
lldSataGetDMAStatusCount(void __iomem* addr){
  SATA_REG sataReg;
        
  sataReg.rValue= ioread32(addr);
  //printk(KERN_ERR"\nlldSataGetDMAStatus: 0x%x ", sataReg.rSata_DMASTS.state);
  return sataReg.rSata_DMASTS.currentTransCnt;
}

#if 0
/**
 *This function gets the DMA Current source or destination
 *@function     lldSataGetDMACurrentDst
 *@return       u32   
 *@param        void
 */
static u32 \
lldSataGetDMACurrentDst(void __iomem* addr){
       
  return ioread32(addr);
}
#endif

/**
 *This function sets the DMA interrrupt pending register
 *@function     lldSataSetDMAInterrupPending
 *@return       void   
 *@param        u32
 */
static void \
lldSataSetDMAInterrupPending(void __iomem* addr, u32 nValue){
  SATA_REG sataReg;

  sataReg.rValue = ioread32(addr);
  sataReg.rSata_DMAINT.intp = nValue;
  Bar_iowrite32(sataReg.rValue,addr);
  ASPEN_DBG(KERN_DEBUG"\nIntr Pending Cleared%s Line %d", __FUNCTION__, __LINE__);

}

/**
 *This function gets the DMA interrrupt pending register
 *@function     lldSataSetDMAInterrupPending
 *@return       int status - on/off   
 *@param        void
 */

static u32 \
lldSataGetDMAInterrupPending(void __iomem* addr){
  return ioread32(addr);
}

/**
 *This function gets the status of SDMA - Locked/Fine
 *@function     lldSataSetDMAIsLocked
 *@return       int status - Locked/Fine
 *@param        SDMA DMA mask register address
 */
static int \
lldSataSetDMAIsLocked(void __iomem* addr){
  SATA_REG sataReg;
  sataReg.rValue = ioread32(addr);
  
  return  sataReg.rSata_DMAMASK.DMAEnable & sataReg.rSata_DMAMASK.DMADisable;
}

/**
 *This function sets the DMA Mask register
 *@function     lldSataSetDMAMask
 *@return       void   
 *@param        void
 */
static void \
lldSataSetDMAMask(void __iomem* addr, u32 onoff){
  SATA_REG sataReg;
    
  sataReg.rValue = ioread32(addr);
    
  if(onoff){
    sataReg.rSata_DMAMASK.DMADisable = 0;
    sataReg.rSata_DMAMASK.DMAEnable  = 1;
  } else{
    sataReg.rSata_DMAMASK.DMAEnable  = 0;
    sataReg.rSata_DMAMASK.DMADisable = 1;
  }
        
  Bar_iowrite32(sataReg.rValue,addr);
  ASPEN_DBG(KERN_DEBUG"\nIntr Mask Val=%d %s Line %d", onoff, __FUNCTION__, __LINE__);  
}


/**
 *This function set the DMA control register
 *@function     lldSataSetDMACtrl
 *@return       void   
 *@param        void
 */
static void \
lldSataSetDMACtrl(void __iomem* addr, SATA_CHANNEL nDirection, SATA_ONOFF onoff){
  SATA_REG sataReg;

  sataReg.rValue = ioread32(addr);
  if(nDirection == SATA_TX){
    sataReg.rSata_DMACR.TXCHEN = onoff;
  } else if(nDirection == SATA_RX){
    sataReg.rSata_DMACR.RXCHEN = onoff;
  }
  Bar_iowrite32(sataReg.rValue,addr);
  ASPEN_DBG(KERN_ERR"\nnDirection=%s", (nDirection==SATA_RX)?"Rx":"Tx");
  return;  
}

/**
 *This function Task File load
 *@function sata_tf_load
 *@return void
 *@param ap, tf
 */
static void \
sata_tf_load(struct ata_port *ap, const struct ata_taskfile *tf){
  ASPEN_DBG(KERN_DEBUG"\n%s Line %d", __FUNCTION__, __LINE__);
  ata_tf_load(ap, tf);
}

/**
 *This function Reads taskfile
 *@function sata_tf_read
 *@return void
 *@param ap, tf
 */
static void \
sata_tf_read(struct ata_port *ap, struct ata_taskfile *tf){
  ASPEN_DBG(KERN_DEBUG"\n%s Line %d", __FUNCTION__, __LINE__);
  ata_tf_read(ap, tf);
}

/**
 *This function checks the status register
 *@function sata_check_status
 *@return u8
 *@param ap
 */
static u8 \
sata_check_status(struct ata_port *ap){
  u8 val;

  val=ata_check_status(ap);
  ASPEN_DBG(KERN_DEBUG"\n Status=%x %s Line %d",val, __FUNCTION__, __LINE__);
  return val;
  
}

/**
 *This function  ATA/ATAPI  protocol command execution
 *@function sata_exec_command
 *@return void
 *@param ap , tf
 */
static void \
sata_exec_command(struct ata_port *ap, const struct ata_taskfile *tf){
  ASPEN_DBG(KERN_DEBUG"\n Command=%x %s Line %d", tf->command, __FUNCTION__, __LINE__);
  ata_exec_command(ap,tf);
}

void  \
sata_qc_prep(struct ata_queued_cmd *qc) {

   /*There seems to be an issue with SDMA controller. SDMA controller expects to read/write 
       exactly as much data as written by the host software (driver) in the SDMACON.
       In ATAPI, few commands write maximum data expected from device and not the actual. 
       In those commands, the actual data from device may be less than the maximum count 
       written in SDMACON. SDMA is unable to handle such request and keep waiting for more data 
       from the device. This workaround detects such functions and changes the protocol to PIO
   */
   struct scsi_cmnd *cmd = qc->scsicmd;

   if(cmd == NULL){
   	return;
   }

   /* These commands fall in problematic category. Handle by PIO
   */
    if(qc->tf.protocol==ATA_PROT_ATAPI_DMA)
    {
	    /* Check for mode sense */
	   if(cmd->cmnd[0] == MODE_SENSE_10){
		  qc->tf.protocol = ATA_PROT_ATAPI;
	      qc->tf.feature &= ~ATAPI_PKT_DMA;
	   }

	   /* Check for request sense */
	   if(cmd->cmnd[0] == REQUEST_SENSE){
		  qc->tf.protocol = ATA_PROT_ATAPI;
	      qc->tf.feature &= ~ATAPI_PKT_DMA;
	   }

	   /* Check for Inquiry */
	   if(cmd->cmnd[0] == INQUIRY){
		  qc->tf.protocol = ATA_PROT_ATAPI;
	      qc->tf.feature &= ~ATAPI_PKT_DMA;
	   }

	   /* Check for Read TOC */
	   if(cmd->cmnd[0] == READ_TOC){
		  qc->tf.protocol = ATA_PROT_ATAPI;
	      qc->tf.feature &= ~ATAPI_PKT_DMA;
	   } 		
	   
	   if(cmd->cmnd[0] == GPCMD_MECHANISM_STATUS){
		  qc->tf.protocol = ATA_PROT_ATAPI;
	      qc->tf.feature &= ~ATAPI_PKT_DMA;
	   } 	   

	   if(cmd->cmnd[0] == GPCMD_READ_HEADER){
		  qc->tf.protocol = ATA_PROT_ATAPI;
	      qc->tf.feature &= ~ATAPI_PKT_DMA;
	   } 	

	   if(cmd->cmnd[0] == GPCMD_READ_SUBCHANNEL){
		  qc->tf.protocol = ATA_PROT_ATAPI;
	      qc->tf.feature &= ~ATAPI_PKT_DMA;
	   }      

	   if(cmd->cmnd[0] == GPCMD_GET_CONFIGURATION){
		  qc->tf.protocol = ATA_PROT_ATAPI;
	      qc->tf.feature &= ~ATAPI_PKT_DMA;
	   }	   

	   if(cmd->cmnd[0] == GPCMD_GET_EVENT_STATUS_NOTIFICATION){
		  qc->tf.protocol = ATA_PROT_ATAPI;
	      qc->tf.feature &= ~ATAPI_PKT_DMA;
	   }	   

	   if(cmd->cmnd[0] == GPCMD_GET_PERFORMANCE){
		  qc->tf.protocol = ATA_PROT_ATAPI;
	      qc->tf.feature &= ~ATAPI_PKT_DMA;
	   }	

	   if(cmd->cmnd[0] == GPCMD_READ_BUFFER_CAPACITY){
		  qc->tf.protocol = ATA_PROT_ATAPI;
	      qc->tf.feature &= ~ATAPI_PKT_DMA;
	   }	

	   if(cmd->cmnd[0] == GPCMD_READ_DISC_INFO){
		  qc->tf.protocol = ATA_PROT_ATAPI;
	      qc->tf.feature &= ~ATAPI_PKT_DMA;
	   } 

	   if(cmd->cmnd[0] == GPCMD_READ_DVD_STRUCTURE){
		  qc->tf.protocol = ATA_PROT_ATAPI;
	      qc->tf.feature &= ~ATAPI_PKT_DMA;
	   } 	

	   if(cmd->cmnd[0] == GPCMD_READ_FORMAT_CAPACITIES){
		  qc->tf.protocol = ATA_PROT_ATAPI;
	      qc->tf.feature &= ~ATAPI_PKT_DMA;
	   } 	

	   if(cmd->cmnd[0] == GPCMD_READ_TRACK_RZONE_INFO){
		  qc->tf.protocol = ATA_PROT_ATAPI;
	      qc->tf.feature &= ~ATAPI_PKT_DMA;
	   }

	   if(cmd->cmnd[0] == GPCMD_READ_SUBCHANNEL){
		  qc->tf.protocol = ATA_PROT_ATAPI;
	      qc->tf.feature &= ~ATAPI_PKT_DMA;
	   }	

	   if(cmd->cmnd[0] == GPCMD_READ_TOC_PMA_ATIP){
		  qc->tf.protocol = ATA_PROT_ATAPI;
	      qc->tf.feature &= ~ATAPI_PKT_DMA;
	   }	

	   if(cmd->cmnd[0] == GPCMD_REPORT_KEY){
		  qc->tf.protocol = ATA_PROT_ATAPI;
	      qc->tf.feature &= ~ATAPI_PKT_DMA;
	   } 	   

	   if(cmd->cmnd[0] == GPCMD_SEND_KEY){
		  qc->tf.protocol = ATA_PROT_ATAPI;
	      qc->tf.feature &= ~ATAPI_PKT_DMA;
	   }	   

	   if(cmd->cmnd[0] == GPCMD_TEST_UNIT_READY){
		  qc->tf.protocol = ATA_PROT_ATAPI;
	      qc->tf.feature &= ~ATAPI_PKT_DMA;
	   }	   

	   if(cmd->cmnd[0] == GPCMD_READ_CD){
		  qc->tf.protocol = ATA_PROT_ATAPI;
	      qc->tf.feature &= ~ATAPI_PKT_DMA;
	   }	   

	   /* vendor specific command */
	   if(cmd->cmnd[0] == CMD_VENDER_LOG){
		  qc->tf.protocol = ATA_PROT_ATAPI;
	      qc->tf.feature &= ~ATAPI_PKT_DMA;
	   }	   

	}

	//printk(" Command = %x\n", cmd->cmnd[0]);


}


static void \
sata_reg_dump(u32 devNum)
{
	if ( devNum == 0 )
	{
//		printk("0x30010000 SDMASRC	= 0x%x\n", barc_sata_reg_get(0x30010000));
		printk("0x30014004 SDMADST	= 0x%x\n", barc_sata_reg_get(0x30014004));
		printk("0x30014008 SDMACON	= 0x%x\n", barc_sata_reg_get(0x30014008));
		printk("0x3001400C SDMASTS	= 0x%x\n", barc_sata_reg_get(0x3001400C));
		printk("0x30014010 SDMACSRC = 0x%x\n", barc_sata_reg_get(0x30014010));
		printk("0x30014014 SDMACDST = 0x%x\n", barc_sata_reg_get(0x30014014));
		printk("0x30014018 SDMAMASK = 0x%x\n", barc_sata_reg_get(0x30014018));
		printk("0x30010020 SATA_CLR0 	= 0x%x\n", barc_sata_reg_get(0x30010020));
		printk("0x30010024 SATA_SCR0 	= 0x%x\n", barc_sata_reg_get(0x30010024));
		printk("0x30010028 SATA_SCR1 	= 0x%x\n", barc_sata_reg_get(0x30010028));
		printk("0x30010030 SATA_SCR3 	= 0x%x\n", barc_sata_reg_get(0x30010030));
		printk("0x30010070 SATA_DMACR	= 0x%x\n", barc_sata_reg_get(0x30010070));  
		printk("0x30010078 SATA_INTPR	= 0x%x\n", barc_sata_reg_get(0x30010078));
		printk("0x3001007C SATA_INTMR	= 0x%x\n", barc_sata_reg_get(0x3001007C));
		printk("0x30010080 SATA_ERRMR	= 0x%x\n", barc_sata_reg_get(0x30010080));
	}
	else
	{
//		printk("0x30018000 SDMASRC	= 0x%x\n", barc_sata_reg_get(0x30018000));
		printk("0x3001c004 SDMADST	= 0x%x\n", barc_sata_reg_get(0x3001c004));
		printk("0x3001c008 SDMACON	= 0x%x\n", barc_sata_reg_get(0x3001c008));
		printk("0x3001c00C SDMASTS	= 0x%x\n", barc_sata_reg_get(0x3001c00C));
		printk("0x3001c010 SDMACSRC = 0x%x\n", barc_sata_reg_get(0x3001c010));
		printk("0x3001c014 SDMACDST = 0x%x\n", barc_sata_reg_get(0x3001c014));
		printk("0x3001c018 SDMAMASK = 0x%x\n", barc_sata_reg_get(0x3001c018));
		printk("0x30018020 SATA_CLR0 	= 0x%x\n", barc_sata_reg_get(0x30018020));
		printk("0x30018024 SATA_SCR0 	= 0x%x\n", barc_sata_reg_get(0x30018024));
		printk("0x30018028 SATA_SCR1 	= 0x%x\n", barc_sata_reg_get(0x30018028));
		printk("0x30018030 SATA_SCR3 	= 0x%x\n", barc_sata_reg_get(0x30018030));
		printk("0x30018070 SATA_DMACR	= 0x%x\n", barc_sata_reg_get(0x30018070));  
		printk("0x30018078 SATA_INTPR	= 0x%x\n", barc_sata_reg_get(0x30018078));
		printk("0x3001807C SATA_INTMR	= 0x%x\n", barc_sata_reg_get(0x3001807C));
		printk("0x30018080 SATA_ERRMR	= 0x%x\n", barc_sata_reg_get(0x30018080));
	}
}

/**
 *This function
 *@function
 *@return
 *@param
 */
static void \
apen_tasklet_func(unsigned long data){
  int i;
  Barcelona_private_t* ptBar_priv=(Barcelona_private_t*)data;
//  spinlock_t queue_lock;
//  unsigned long flags;
  char* pcData;
  
  if (!ptBar_priv)
  {
  	return;
  }

//  spin_lock_init(&queue_lock);

//  spin_lock_irqsave(&queue_lock, flags);

  if ( ptBar_priv->uiReadSize > MULTPLE_SG_BUFFERSIZE)
  {
  	printk("Error, Total Read Size 0x%x\n", ptBar_priv->uiReadSize);
  	return;
  }

  disable_irq(ptBar_priv->uiIrqNum);

  /* Increase address */
  barc_Dma_reg_set(0x3004001C, 0);

  pcData = (char*)ptBar_priv->uiDmaBufPA;
  
  for ( i = 0 ; i < ptBar_priv->uiReadCount; i++)
  {
	/* Src */
	barc_Dma_reg_set(0x30040000, (u32)pcData);	

  	/* Dest */
	barc_Dma_reg_set(0x30040004, (u32)ptBar_priv->tDma_ReadAddress[i].addr); 

	/*Aspen specific settings*/
	barc_Dma_reg_set(0x30040008, (ptBar_priv->tDma_ReadAddress[i].uSize/4)|(3<<20)|(3<<24)|(1<<26)|(1<<27)|(1<<28)|(1<<30));

	/*Start DMA */
	barc_Dma_reg_set(0x30040018, 3);

	while(1)
	{
	  if(barc_sata_reg_get(0x30040020) & 0x1 )
	  {
		  /* Clear SDMA interrupt */		  
		  barc_Dma_reg_set(0x30040020, 1);  
		  break;
	  }
	}

	/*Mask DMA Intr*/
	barc_Dma_reg_set(0x30040018, 4);

	pcData += ptBar_priv->tDma_ReadAddress[i].uSize;

  }

  ptBar_priv->uiReadSize = 0;
  ptBar_priv->uiReadCount = 0;

  enable_irq(ptBar_priv->uiIrqNum);

//  spin_unlock_irqrestore(&queue_lock, flags);

}

#if defined(MAKE_DATA_ZERO)
/**
 *This function
 *@function
 *@return
 *@param
 */
static void \
apen_tasklet_func_Cpu(unsigned long data){
  int i;
  void*	pvAddr;
  char*	pcData;
  Barcelona_private_t* ptBar_priv=(Barcelona_private_t*)data;
//  spinlock_t queue_lock;
//  unsigned long flags;

  if (!ptBar_priv)
  {
  	return;
  }

//  spin_lock_init(&queue_lock);

//  spin_lock_irqsave(&queue_lock, flags);

  if ( ptBar_priv->uiReadSize > MULTPLE_SG_BUFFERSIZE)
  {
	printk("Error, Total Read Size 0x%x\n", ptBar_priv->uiReadSize);
	return;
  }

  disable_irq(ptBar_priv->uiIrqNum);

  /* check read mode & prviate  data */
  pcData = ptBar_priv->pvDmaBufVA;
  for ( i = 0 ; i < ptBar_priv->uiReadCount; i++)
  {
  	pvAddr = phys_to_virt(ptBar_priv->tDma_ReadAddress[i].addr); 
  	if ( !pvAddr || !pcData)
  	{
  		printk("Error Read:%d-th~~~~~status %08x %08x %08x\n", i, (int)pvAddr, (int)pcData, ptBar_priv->tDma_ReadAddress[i].uSize);
  		break;
  	}
  	
	/* When disable case, copy data to buffer*/
	if (*(volatile unsigned int*)(puiSataSetVA) == 0 )
	{
		memcpy(pvAddr, pcData, ptBar_priv->tDma_ReadAddress[i].uSize);
	}
	else
	{
		if ( *(volatile unsigned int*)(puiSataSetVA+1) == 0 )
		{
			if ( jiffies%2 )
			{
			  memcpy(pvAddr, pcData, ptBar_priv->tDma_ReadAddress[i].uSize);
			}
			else
			{
			  memset(pvAddr, 0, ptBar_priv->tDma_ReadAddress[i].uSize );
			}
		}
		else
		{
			static int j =0;
	
			if ( j++ % (*(volatile unsigned int*)(puiSataSetVA+1)))
			{
			  memcpy(pvAddr, pcData, ptBar_priv->tDma_ReadAddress[i].uSize);
			}
			else
			{
			  memset(pvAddr, 0, ptBar_priv->tDma_ReadAddress[i].uSize );
			}
		}
	}

  	pcData += ptBar_priv->tDma_ReadAddress[i].uSize;
  }
  
  ptBar_priv->uiReadSize = 0;
  ptBar_priv->uiReadCount = 0;

  enable_irq(ptBar_priv->uiIrqNum);

//  spin_unlock_irqrestore(&queue_lock, flags);

}

#else

/**
 *This function
 *@function
 *@return
 *@param
 */
static void \
apen_tasklet_func_Cpu(unsigned long data){
  int i;
  void*	pvAddr;
  char*	pcData;
  Barcelona_private_t* ptBar_priv=(Barcelona_private_t*)data;
//  spinlock_t queue_lock;
//  unsigned long flags;

  if (!ptBar_priv)
  {
  	return;
  }

//  spin_lock_init(&queue_lock);

//  spin_lock_irqsave(&queue_lock, flags);

  if ( ptBar_priv->uiReadSize > MULTPLE_SG_BUFFERSIZE)
  {
  	printk("Error, Total Read Size 0x%x\n", ptBar_priv->uiReadSize);
  	return;
  }

  disable_irq(ptBar_priv->uiIrqNum);

  /* check read mode & prviate  data */
  pcData = ptBar_priv->pvDmaBufVA;
  for ( i = 0 ; i < ptBar_priv->uiReadCount; i++)
  {
  	pvAddr = phys_to_virt(ptBar_priv->tDma_ReadAddress[i].addr); 
  	if ( !pvAddr || !pcData)
  	{
  		printk("Error Read:%d-th~~~~~status %08x %08x %08x\n", i, (int)pvAddr, (int)pcData, ptBar_priv->tDma_ReadAddress[i].uSize);
  		break;
  	}
  	memcpy(pvAddr, pcData, ptBar_priv->tDma_ReadAddress[i].uSize);
  	pcData += ptBar_priv->tDma_ReadAddress[i].uSize;
  }
  
  ptBar_priv->uiReadSize = 0;
  ptBar_priv->uiReadCount = 0;

  enable_irq(ptBar_priv->uiIrqNum);

//  spin_unlock_irqrestore(&queue_lock, flags);

}
#endif

static DECLARE_TASKLET(aspen_tasklet, apen_tasklet_func, 0);
static DECLARE_TASKLET(aspen_taskletCpu, apen_tasklet_func_Cpu, 0);

/**
 *This function DMA Fire
 *@function sata_bmdma_start
 *@return void
 *@param qc
 */
static void \
sata_bmdma_start(struct ata_queued_cmd *qc){
  struct ata_port *ap = qc->ap;    
  Barcelona_private_t* ptBar_priv=NULL;
  unsigned int write = qc->tf.flags & ATA_TFLAG_WRITE;
  u32 val;
        
  ASPEN_DBG(KERN_DEBUG"\n%s Line %d", __FUNCTION__, __LINE__);
//  printk("Ata port no %d\n", ap->print_id);

  if ( !ap->private_data )
  {
	  printk("There is no Barcelona private data\n");
	  return;
  }

//   printk("\n%s...................", __FUNCTION__);
  ptBar_priv = ap->private_data;

  if ( write )
  {
      lldSataSetDMASource(((dma_regs_t*)qc->ap->ioaddr.bmdma_addr)->DMASRC, (u32)ptBar_priv->uiDmaBufPA);	
  }
  else
  {
  	  if ( ptBar_priv->uiReadCount == 1 )
  	  {
		  lldSataSetDMADestination(((dma_regs_t*)qc->ap->ioaddr.bmdma_addr)->DMADST, (u32)ptBar_priv->tDma_ReadAddress[0].addr);
//		  printk("\n%s...................,Addr = 0x%x...Size = 0x%x...", __FUNCTION__, ptBar_priv->tDma_ReadAddress[0].addr, (u32)ptBar_priv->uiReadSize);

	  }
	  else
	  {
		  lldSataSetDMADestination(((dma_regs_t*)qc->ap->ioaddr.bmdma_addr)->DMADST, (u32)ptBar_priv->uiDmaBufPA);
//		  printk("\n%s...................,Addr = 0x%x...Size = 0x%x...", __FUNCTION__, (u32)ptBar_priv->uiDmaBufPA, (u32)ptBar_priv->uiReadSize);
	  }
  }

  lldSataSetDMATransferByteSize(((dma_regs_t*)qc->ap->ioaddr.bmdma_addr)->DMACON, 0x3); 

  if ( write )
  {
	lldSataSetDMABeatCount(((dma_regs_t*)qc->ap->ioaddr.bmdma_addr)->DMACON, ptBar_priv->uiWriteSize/4);
	if ( ptBar_priv->uiWriteSize > MULTPLE_SG_BUFFERSIZE)
	{
		printk("Error, Total Write Size 0x%x\n", ptBar_priv->uiWriteSize);
	}

	ptBar_priv->uiWriteSize = 0;
	ptBar_priv->uiWriteCount = 0;
  }
  else
  {
	lldSataSetDMABeatCount(((dma_regs_t*)qc->ap->ioaddr.bmdma_addr)->DMACON, ptBar_priv->uiReadSize/4);	
  }

  //Burst size 16.
  lldSataSetDMABurstSize(((dma_regs_t*)qc->ap->ioaddr.bmdma_addr)->DMACON, 0x03);
  
  //Burst size 16.
  lldSataSetDBTSR(((dma_regs_t*)qc->ap->ioaddr.bmdma_addr)->DBTSR, 0x00100010);
  
  /* Clear SErr */
  sata_aspen_scr_read(ap, SCR_ERROR, &val); 
  val=0xFFFFFFFF;
  sata_aspen_scr_write(ap, SCR_ERROR, val);
			  
  /* Clear SDMA interrupt */		  
  lldSataSetDMAInterrupPending(((dma_regs_t*)qc->ap->ioaddr.bmdma_addr)->DMAINT, 1);  

  /*Unmask DMA Intr*/
  lldSataSetDMAMask(((dma_regs_t*)qc->ap->ioaddr.bmdma_addr)->DMAMASK, SATA_ON);

  /*Aspen specific settings*/
  lldSataSetDMAHLock(((dma_regs_t*)qc->ap->ioaddr.bmdma_addr)->DMACON, SATA_ON);

  /*Enable DMA Tx/Rx Channel*/
  lldSataSetDMACtrl(((dma_regs_t*)qc->ap->ioaddr.bmdma_addr)->DMACR, (write?SATA_TX:SATA_RX), SATA_ON);  

  /* Set DMA flag */
  ptBar_priv->uiDmaStarted = 1;

}

/**
 *This function DMA setup
 *@function sata_bmdma_setup
 *@return void
 *@param qc
 */
static void \
sata_bmdma_setup(struct ata_queued_cmd *qc){
  struct scatterlist *sg = NULL;
  struct ata_port *ap = qc->ap;    
  Barcelona_private_t* ptBar_priv=NULL;
  unsigned int write = qc->tf.flags & ATA_TFLAG_WRITE;
  void* pvAddr;
  char*	pcData;

  ASPEN_DBG(KERN_DEBUG"\n%s Line %d", __FUNCTION__, __LINE__);  

  if ( !ap->private_data )
  {
	  printk("There is no Barcelona private data\n");
	  return;
  }

   // printk("\n%s...................", __FUNCTION__);

  ptBar_priv = ap->private_data;

  ptBar_priv->uiWriteSize = ptBar_priv->uiWriteCount = 0;
	  
  ata_for_each_sg(sg, qc) {

    /* Configure the direction */
    if(write){
      /* Writing DATA */       
      ASPEN_DBG(KERN_ERR"\nController write to device");	  

	  pvAddr = phys_to_virt(sg->dma_address);
      pcData = (char*)ptBar_priv->pvDmaBufVA + ptBar_priv->uiWriteSize;
	  if (!pvAddr || !pcData )
	  {
      	printk("Error Write:%d-th~~~~~status %08x %08x %08x\n", ptBar_priv->uiWriteCount, (int)pvAddr, (int)pcData, (int)sg->length);
		  return;
	  }

	  memcpy(pcData, pvAddr, sg->length);

	  ptBar_priv->tDma_WriteAddress[ptBar_priv->uiWriteCount].addr = sg->dma_address;
	  ptBar_priv->tDma_WriteAddress[ptBar_priv->uiWriteCount].uSize = sg->length;
	  ptBar_priv->uiWriteSize += sg->length;
	  ptBar_priv->uiWriteCount++;
	
  	}
    else
    {
      ASPEN_DBG(KERN_ERR"\nController read from  device. DST Addr = 0x%x sg->length=%d", sg->dma_address, sg->length);	      
	  ptBar_priv->tDma_ReadAddress[ptBar_priv->uiReadCount].addr = sg->dma_address;
	  ptBar_priv->tDma_ReadAddress[ptBar_priv->uiReadCount].uSize= sg->length;
	  ptBar_priv->uiReadSize += sg->length;
	  ptBar_priv->uiReadCount++;
    }
  }

#if 0
  {
	  struct scsi_cmnd *cmd = qc->scsicmd;

	  if (cmd)
	  {
		 printk("Setup.orig.command(0x%x:0x%x), size 0x%x, count %d", qc->tf.command, cmd->cmnd[0],ptBar_priv->uiReadSize, ptBar_priv->uiReadCount);
	  }
	  else
	  {
		 printk("Setup.orig.command(0x%x:0x%x), size 0x%x, count %d", qc->tf.command, 0x0, ptBar_priv->uiReadSize, ptBar_priv->uiReadCount);
	  }
  }
#endif

  /* In Aspen we cannot support multiple SG BUFS. FIXED - Please look at the SCSI HOST Struct */
  /* issue r/w command */
  ap->ops->exec_command(ap, &qc->tf);
}

/**
 *This function DMA Stop
 *@function  sata_bmdma_stop
 *@return void
 *@param qc
 */
static void \
sata_bmdma_stop(struct ata_queued_cmd *qc){
  unsigned int write = qc->tf.flags & ATA_TFLAG_WRITE;
  struct ata_port *ap = qc->ap;    
  Barcelona_private_t* ptBar_priv;

  ASPEN_DBG(KERN_DEBUG"\n%s Line %d", __FUNCTION__, __LINE__);

   // printk("\n%s...................", __FUNCTION__);

  /* check read mode & prviate  data */
  if ( !ap->private_data)
  {
  	return;
  }

  /* Get private data */
  ptBar_priv = ap->private_data;

  if ( 0 == ptBar_priv->uiDmaStarted )
  {
  	  /* see ata_bmdma_stop() */
	  ata_altstatus(qc->ap);        /* dummy read */  
	  return;
  }
  
  /*Disable DMA Tx/Rx Channel*/
  lldSataSetDMACtrl(((dma_regs_t*)qc->ap->ioaddr.bmdma_addr)->DMACR, (write?SATA_TX:SATA_RX), SATA_OFF);

  /*Mask DMA Intr*/
  lldSataSetDMAMask(((dma_regs_t*)qc->ap->ioaddr.bmdma_addr)->DMAMASK, SATA_OFF); 

  if(unlikely(lldSataSetDMAIsLocked(((dma_regs_t*)qc->ap->ioaddr.bmdma_addr)->DMAMASK))){
    /* SUNIL: Detected a SDMA Hardware issue. Only solution is reset */
//    qc->flags |= ATA_QCFLAG_FAILED;
//    qc->err_mask |= AC_ERR_HOST_BUS;
    printk("\n\e[41mDMA Locked........................\e[0m");
  }

  /* see ata_bmdma_stop() */
  ata_altstatus(qc->ap);        /* dummy read */  

  /* check read mode & prviate  data */
  if ( !write )
  {
	if ( qc->flags & ATA_QCFLAG_FAILED )
	{
		/* no copy data */
	}
	else if ( ptBar_priv->uiReadCount <= 1 )
	{
		/* direct dma mode */
	}
	else
	{
		/* Check tasklet private data */
		if ( !ptBar_priv->ptasklet_FuncPrivate)
		{
			return;
		}

//		apen_tasklet_func_Cpu((unsigned long)ptBar_priv);

		/* tasklet mode */
		memcpy(&ptBar_priv->ptasklet_FuncPrivate->tDma_ReadAddress[0], &ptBar_priv->tDma_ReadAddress[0], sizeof(BarScatter_t)*ptBar_priv->uiReadCount);
		ptBar_priv->ptasklet_FuncPrivate->uiReadSize = ptBar_priv->uiReadSize;
		ptBar_priv->ptasklet_FuncPrivate->uiReadCount = ptBar_priv->uiReadCount;

#if defined (MAKE_DATA_ZERO)
		if ( ptBar_priv->uiIrqNum == INT_SATA0 )
		{
			aspen_taskletCpu.data = (unsigned long)ptBar_priv->ptasklet_FuncPrivate;
			tasklet_hi_schedule(&aspen_taskletCpu);
		}
		else
		{
			aspen_taskletCpu.data = (unsigned long)ptBar_priv->ptasklet_FuncPrivate;
			tasklet_hi_schedule(&aspen_taskletCpu);
		}
#else
		if ( ptBar_priv->uiIrqNum == INT_SATA0 )
		{
			aspen_tasklet.data = (unsigned long)ptBar_priv->ptasklet_FuncPrivate;
			tasklet_hi_schedule(&aspen_tasklet);
		}
		else
		{
			aspen_taskletCpu.data = (unsigned long)ptBar_priv->ptasklet_FuncPrivate;
			tasklet_hi_schedule(&aspen_taskletCpu);
		}
#endif
	}

    ptBar_priv->uiReadSize=0;
    ptBar_priv->uiReadCount=0;

  }

  /* Clear flag */
  ptBar_priv->uiDmaStarted = 0;

}

/**
 *This function DMA Status
 *@function sata_bmdma_status
 *@return u8
 *@param ap
 */
static u8 \
sata_bmdma_status(struct ata_port *ap){
 u32 scr1=0;
#if 0
  u32 timeout=5; /* timeout*SDP_SHORT_PAUSE=1ms */
#endif
  u32 cdr7;
  Barcelona_private_t* ptBar_priv=NULL;

  ptBar_priv = ap->private_data;

  /* We need to populate the status to the caller 
     BM_STATUS_DMAING 0x01
     BM_STATUS_ERROR  0x02
     BM_STATUS_INT    0x04
     BM_STATUS_DMA_RETRY  0x08
     BM_STATUS_PIO_RETRY  0x10
  */

//	printk("\n%s...................", __FUNCTION__);
	sata_aspen_scr_read(ap, SCR_ERROR, &scr1); 
	cdr7 = sata_check_status(ap);

	if ( (cdr7&0x80)) // while 'NoError' and 'Device Busy' and 'DMA working'
	{
		/*DMA Active*/
		printk("Waiting.......1, %d\n", scr1);	
		sata_reg_dump(ptBar_priv->uiDevno);		// don't remove
		return (BM_STATUS_INT);		
	}
    else if((lldSataGetDMAStatus(((dma_regs_t*)ap->ioaddr.bmdma_addr)->DMASTS) & 0x01))
    {
		/*DMA Active*/
		ASPEN_DBG("Dmaing.......2\n");
		return BM_STATUS_DMAING;		
    }
    else if(lldSataGetDMAInterrupPending(((dma_regs_t*)ap->ioaddr.bmdma_addr)->DMAINT) & 0x01)
    {
		/*DMA Finished*/
		return ((scr1)?(BM_STATUS_ERROR|BM_STATUS_INT):BM_STATUS_INT);   
    }
    else if (lldSataGetDMAStatusCount(((dma_regs_t*)ap->ioaddr.bmdma_addr)->DMASTS))
    {
    	int i, waitCount;
//    	printk("DMA size is remained........0x%x\n", lldSataGetDMAStatusCount(((dma_regs_t*)ap->ioaddr.bmdma_addr)->DMASTS)*4);

    	waitCount = lldSataGetDMAStatusCount(((dma_regs_t*)ap->ioaddr.bmdma_addr)->DMASTS)/0x800;
    	waitCount = (waitCount>1)?(waitCount):(1);
    	
		printk("\n\e[43mWaiting.....DMA status done...%d.\e[0m", waitCount);
		for ( i = 0 ; i <waitCount ; i++)
		{
			udelay(SDP_SHORT_PAUSE);
		}

		/*DMA Finished*/
		return ((scr1)?(BM_STATUS_ERROR|BM_STATUS_INT):BM_STATUS_INT);   
    }
#if 0
    /*SDMA might take little time to assert INTR PNDG. This Intr generated by host controller*/	
   while(timeout && !(lldSataGetDMAInterrupPending(((dma_regs_t*)ap->ioaddr.bmdma_addr)->DMAINT) & 0x01)){
   	udelay(SDP_SHORT_PAUSE);
   	printk("Short pause....%d\n", timeout);
	timeout--;
}
#endif

	printk("\n\e[43mWaiting.......3, 0x%x..\e[0m\n", scr1 );	
	sata_reg_dump(ptBar_priv->uiDevno);	// don't remove

	return ((scr1)?(BM_STATUS_ERROR|BM_STATUS_INT):BM_STATUS_INT);

}


/**
 *This function ;Clear interrupt and error flags.
 *@function  sata_bmdma_irq_clear
 *@return void
 *@param ap
 */
static void \
sata_bmdma_irq_clear(struct ata_port *ap){
  u32 val;
  u32 i=0;
  
  ASPEN_DBG(KERN_DEBUG"\n%s Line %d", __FUNCTION__, __LINE__);

  sata_aspen_scr_read(ap, SCR_ERROR, &val); 
  val=0xFFFFFFFF;
  sata_aspen_scr_write(ap, SCR_ERROR, val);

  while (lldSataGetDMAInterrupPending(((dma_regs_t*)ap->ioaddr.bmdma_addr)->DMAINT) & 0x01)
    {
    if ( i > 0 )
    {
		printk("Clear...Isr %d\n", i);
    }
	i++;
	/* Clear SDMA interrupt */          
	lldSataSetDMAInterrupPending(((dma_regs_t*)ap->ioaddr.bmdma_addr)->DMAINT, 1);    
   }
}

/**
 *This function ATA/ATAPI PIO data transfer
 *@function sata_data_xfer
 *@return
 *@param
 */
static void \
sata_data_xfer(struct ata_device *adev, unsigned char *buf, unsigned int buflen, 
                                int write_data){
  ata_data_xfer(adev, buf, buflen, write_data);
ASPEN_DBG(KERN_DEBUG "\nbuf= %x ",  buf[0]); 
ASPEN_DBG(KERN_DEBUG "%x ",  buf[1]); 
ASPEN_DBG(KERN_DEBUG "%x ",  buf[2]); 
ASPEN_DBG(KERN_DEBUG "%x ",  buf[3]); 
ASPEN_DBG(KERN_DEBUG "%x ",  buf[4]); 
ASPEN_DBG(KERN_DEBUG "%x ",  buf[5]); 
ASPEN_DBG(KERN_DEBUG "%x ",  buf[6]); 
ASPEN_DBG(KERN_DEBUG "%x ",  buf[7]); 
ASPEN_DBG(KERN_DEBUG "%x ",  buf[8]); 
ASPEN_DBG(KERN_DEBUG "%x ",  buf[9]); 
ASPEN_DBG(KERN_DEBUG "%x ",  buf[10]); 
ASPEN_DBG(KERN_DEBUG "%x ",  buf[11]); 
}

/**
 *This function
 *@function
 *@return
 *@param
 */
static void \
sata_bmdma_post_internal_cmd(struct ata_queued_cmd *qc){

  ASPEN_DBG(KERN_DEBUG"\n%s Line %d", __FUNCTION__, __LINE__);

  /*Stop the DMA Channel*/      
  sata_bmdma_stop(qc);
}

/**
 *This function DMA Pad memory?
 *@function ata_pad_alloc
 *@return
 *@param
 */
static int \
sata_port_start(struct ata_port *ap){

  ASPEN_DBG(KERN_DEBUG"\n%s Line %d", __FUNCTION__, __LINE__);
  /* No PRD to alloc */
  return ata_pad_alloc(ap, ap->dev);
}



static struct scsi_host_template sata_aspen_sht = {
  .module			= THIS_MODULE,
//  .name                 = DEV_NAME_SATA1,
  .ioctl			= ata_scsi_ioctl,
  .queuecommand	= ata_scsi_queuecmd,
  .can_queue		= ATA_DEF_QUEUE,
  .this_id			= ATA_SHT_THIS_ID,
  .sg_tablesize	   = 256, 	// min size 32+1 , 0x21000
  .cmd_per_lun	= ATA_SHT_CMD_PER_LUN,
  .emulated		= ATA_SHT_EMULATED,
  .use_clustering	= ATA_SHT_USE_CLUSTERING,
//  .proc_name		= DEV_NAME_SATA1,
  .dma_boundary	= ATA_DMA_BOUNDARY,
  .slave_configure	= ata_scsi_slave_config,
  .slave_destroy	= ata_scsi_slave_destroy,
  .bios_param		= ata_std_bios_param,
};

#if 0
static void \
sata_aspen_freeze(struct ata_port *ap){

	unsigned int reg;
	Barcelona_private_t* ptBar_priv;

	ASPEN_DBG(KERN_DEBUG"\n%s Line %d", __FUNCTION__, __LINE__);
	
	if ( ap->private_data )
	{
		ptBar_priv = ap->private_data;
 
		reg = ioread32(REG_INTCON_MASK);
		reg |= (1 << ptBar_priv->uiIrqNum);
		iowrite32(reg, REG_INTCON_MASK);
	}
}

static void \
sata_aspen_thaw(struct ata_port *ap){

	unsigned int reg;
	Barcelona_private_t* ptBar_priv;

	ASPEN_DBG(KERN_DEBUG"\n%s Line %d", __FUNCTION__, __LINE__);
		
	if ( ap->private_data )
	{
		ptBar_priv = ap->private_data;
  
		reg = ioread32(REG_INTCON_MASK);
		reg &= ~(1 << ptBar_priv->uiIrqNum);
		iowrite32(reg, REG_INTCON_MASK);
	}
}
#endif

#if defined ( ASPEN_DEBUG )
/**
 *	ata_err_string - convert err_mask to descriptive string
 *	@err_mask: error mask to convert to string
 *
 *	Convert @err_mask to descriptive string.  Errors are
 *	prioritized according to severity and only the most severe
 *	error is reported.
 *
 *	LOCKING:
 *	None.
 *
 *	RETURNS:
 *	Descriptive string for @err_mask
 */
static const char *Barata_err_string(unsigned int err_mask)
{
	if (err_mask & AC_ERR_HOST_BUS)
		return "host bus error";
	if (err_mask & AC_ERR_ATA_BUS)
		return "ATA bus error";
	if (err_mask & AC_ERR_TIMEOUT)
		return "timeout";
	if (err_mask & AC_ERR_HSM)
		return "HSM violation";
	if (err_mask & AC_ERR_SYSTEM)
		return "internal error";
	if (err_mask & AC_ERR_MEDIA)
		return "media error";
	if (err_mask & AC_ERR_INVALID)
		return "invalid argument";
	if (err_mask & AC_ERR_DEV)
		return "device error";
	return "unknown error";
}
#endif

/**
 *	ata_bmdma_error_handler - Stock error handler for BMDMA controller
 *	@ap: port to handle error for
 *
 *	Stock error handler for BMDMA controller.
 *
 *	LOCKING:
 *	Kernel thread context (may sleep)
 */
void Barata_bmdma_error_handler(struct ata_port *ap)
{
	ata_reset_fn_t hardreset=NULL;
	u32 scr1;
	Barcelona_private_t* ptBar_priv=ap->private_data;

	sata_aspen_scr_read(ap, SCR_ERROR, &scr1); 
	printk("Handler controls 0x%x\n", scr1);

	/* When hdd case, reset device */
	if ( ptBar_priv && (ptBar_priv->uiIrqNum == INT_SATA1) )
	{
		if(unlikely(lldSataSetDMAIsLocked(((dma_regs_t*)ap->ioaddr.bmdma_addr)->DMAMASK)))
		{
		  struct ata_queued_cmd *qc = __ata_qc_from_tag(ap, ap->link.active_tag);
		  if (qc && (qc->flags & ATA_QCFLAG_FAILED))
		  {
			 ASPEN_DBG("qc->err_mask: %s ", Barata_err_string(qc->err_mask));		  
			 if ( qc->err_mask & (AC_ERR_TIMEOUT|AC_ERR_ATA_BUS|AC_ERR_HOST_BUS) )
			 {
			  	if ( ptBar_priv->ResetFuntion )
			  	{
					(*ptBar_priv->ResetFuntion)(ptBar_priv->uiIrqNum);
				}
			 }
		  }
		}

		/* set hard reset function pointer */
		if (sata_scr_valid(&ap->link))
		{
			hardreset = sata_std_hardreset;
		}

	}
	else
	{
		/* When using raptor, don't make generate comreset */
	}

	/* call common error handler */
	ata_bmdma_drive_eh(ap, ata_std_prereset, ata_std_softreset, hardreset, ata_std_postreset);

}


static const struct ata_port_operations sata_aspen_ops = {
  .dev_config           	= NULL,
  .tf_load                      = sata_tf_load,
  .tf_read                      = sata_tf_read, 
  .check_status         	= sata_check_status,
  .exec_command 		= sata_exec_command,
  .dev_select           	= ata_std_dev_select,
  .bmdma_setup       	= sata_bmdma_setup,
  .bmdma_start        	= sata_bmdma_start,
  .bmdma_stop           	= sata_bmdma_stop, 
  .bmdma_status		= sata_bmdma_status,
  .qc_prep			= sata_qc_prep,
  .qc_issue			= sata_aspen_qc_issue_prot,
  .data_xfer			= sata_data_xfer,
  .error_handler		= Barata_bmdma_error_handler,
  .post_internal_cmd	= sata_bmdma_post_internal_cmd,
  .irq_clear			= sata_bmdma_irq_clear,
  .irq_on				= ata_irq_on,
  .scr_read			= sata_aspen_scr_read,
  .scr_write			= sata_aspen_scr_write,
  .port_start			= sata_port_start,
#if 0
  .freeze = sata_aspen_freeze,
  .thaw = sata_aspen_thaw,
#endif
};

static const struct ata_port_info sata_aspen_port_info = {
  .flags			= ATA_FLAG_SATA,
  .link_flags		= ATA_LFLAG_HRST_TO_RESUME,
  .pio_mask		= ATA_PIO4,
  .mwdma_mask	= ATA_MWDMA2,
  .udma_mask		= ATA_UDMA6, 
  .port_ops		= &sata_aspen_ops,
};

/**
 *This function
 *@function
 *@return
 *@param
 */
static unsigned int \
sata_aspen_qc_issue_prot(struct ata_queued_cmd *qc){
        
  ASPEN_DBG(KERN_DEBUG"\n%s Line %d", __FUNCTION__, __LINE__);
  return ata_qc_issue_prot(qc);
}

/**
 *This function
 *@function
 *@return
 *@param
 */
static int \
sata_aspen_scr_read(struct ata_port *ap, unsigned int sc_reg, u32 *val){

  switch(sc_reg){
  case SCR_STATUS:
    	*val=ioread32((int*)ap->ioaddr.scr_addr+0x0);
    break;
                        
  case SCR_ERROR:
    *val=ioread32((int*)ap->ioaddr.scr_addr+0x1);
    break;
                        
  case SCR_CONTROL:
    *val=ioread32((int*)ap->ioaddr.scr_addr+0x2);
    break;                      
                        
  default:
    ASPEN_DBG(KERN_ERR"\n%s %d Unrecognized sc_reg", __FUNCTION__, __LINE__);
    return -EINVAL;
  };
  ASPEN_DBG(KERN_DEBUG"\n%s Line %d sc_reg=%d val=%x", __FUNCTION__, __LINE__, sc_reg, *val);   

  return 0;
}

/**
 *This function
 *@function
 *@return
 *@param
 */
static int \
sata_aspen_scr_write(struct ata_port *ap, unsigned int sc_reg, u32 val){

  ASPEN_DBG(KERN_DEBUG"\n%s Line %d sc_reg=%d val=%d", __FUNCTION__, __LINE__, sc_reg,val);        
  switch(sc_reg){
  case SCR_STATUS:
    iowrite32(val, (int*)ap->ioaddr.scr_addr+0x0);
    break;
                        
  case SCR_ERROR:
    iowrite32(val, (int*)ap->ioaddr.scr_addr+0x1);
    break;
                        
  case SCR_CONTROL:
    iowrite32(val, (int*)ap->ioaddr.scr_addr+0x2);
    break;                      
                        
  default:
    ASPEN_DBG(KERN_ERR"\n%s %d Unrecognized sc_reg", __FUNCTION__, __LINE__);
    return -EINVAL;
  };
  return 0;
}

/**
 *This function
 *@function
 *@return
 *@param
 */
static int \
aspen_remove(struct platform_device *pdev){
	struct ata_host *host = dev_get_drvdata((struct device *)pdev);
	Barcelona_private_t* ptBar_priv= host->private_data;
  
  /*TBD*/
  printk(KERN_DEBUG"\n%s Line %d", __FUNCTION__, __LINE__);
  
  ata_host_detach(host);
  
	dev_set_drvdata((struct device *)pdev, NULL);
  
	if ( ptBar_priv )
  {
		free_irq(ptBar_priv->uiIrqNum, host);
  }

  return 0;
} 


/**
 *	ata_interrupt - Default ATA host interrupt handler
 *	@irq: irq line (unused)
 *	@dev_instance: pointer to our ata_host information structure
 *
 *	Default interrupt handler for PCI IDE devices.  Calls
 *	ata_host_intr() for each port that is not disabled.
 *
 *	LOCKING:
 *	Obtains host lock during operation.
 *
 *	RETURNS:
 *	IRQ_NONE or IRQ_HANDLED.
 */

irqreturn_t Barata_interrupt(int irq, void *dev_instance)
{
	irqreturn_t iret;
	
	iret = ata_interrupt(irq, dev_instance);
#if 1
	if(unlikely(iret == 0)){
	  struct ata_host *host = dev_instance;
	  struct ata_port *ap;
	  /* Libata did not handle interrupt, if we not cleared the SDMA interrupt flags,
		 it continue interrupting and hung the system */
	  ap = host->ports[0];
	  ata_chk_status(ap);
	  sata_bmdma_irq_clear(ap);
	}
#endif
	return	1;
}


/**
 *This function ; Registered with platform probe
 *@function aspen_probe
 *@return int
 *@param pdev
 */
static int \
aspen_probe(struct platform_device *pdev){
  struct device *dev = &pdev->dev;
  struct ata_host *host;
  struct ata_port *ap;
  struct ata_ioports *ioaddr;   
  Barcelona_private_t* ptBar_priv=NULL;
  const struct ata_port_info *ppi[] = { &sata_aspen_port_info, NULL };  
  int retVal;

  ASPEN_DBG(KERN_DEBUG"\n%s Line %d", __FUNCTION__, __LINE__);

  host = ata_host_alloc_pinfo(dev, ppi, 1);
  if (!host){
    ASPEN_DBG(KERN_ERR"\nENOMEM %s Line %d", __FUNCTION__, __LINE__);  	
    return -ENOMEM;  	
  }
        
  ap = host->ports[0];
  ioaddr = &ap->ioaddr;

  if(pdev->id==0){
	  ioaddr->cmd_addr=             REG_SATA0_CDR0;
	  ioaddr->altstatus_addr =      REG_SATA0_CLR0;
	  ioaddr->ctl_addr =		   REG_SATA0_CLR0;
	  ioaddr->scr_addr =            REG_SATA0_SCR0;  	
  }else if(pdev->id==1){
	  ioaddr->cmd_addr=             REG_SATA1_CDR0;
	  ioaddr->altstatus_addr =      REG_SATA1_CLR0;
	  ioaddr->ctl_addr =		   REG_SATA1_CLR0;
	  ioaddr->scr_addr =            REG_SATA1_SCR0;  	  
  }else{
  	/*Error*/
	BUG_ON(1);
  }

  ioaddr->data_addr = (int *)ioaddr->cmd_addr + ATA_REG_DATA;
  ioaddr->error_addr =(int *) ioaddr->cmd_addr + ATA_REG_ERR;
  ioaddr->feature_addr = (int *)ioaddr->cmd_addr + ATA_REG_FEATURE;
  ioaddr->nsect_addr = (int *)ioaddr->cmd_addr + ATA_REG_NSECT;
  ioaddr->lbal_addr = (int *)ioaddr->cmd_addr + ATA_REG_LBAL;
  ioaddr->lbam_addr = (int *)ioaddr->cmd_addr + ATA_REG_LBAM;
  ioaddr->lbah_addr = (int *)ioaddr->cmd_addr + ATA_REG_LBAH;
  ioaddr->device_addr = (int *)ioaddr->cmd_addr + ATA_REG_DEVICE;
  ioaddr->status_addr = (int *)ioaddr->cmd_addr + ATA_REG_STATUS;
  ioaddr->command_addr = (int *)ioaddr->cmd_addr + ATA_REG_CMD;
  /*Not a single addr. It points to the address region for dma registers*/
  ioaddr->bmdma_addr=&bmdmareg[pdev->id];


  /*Mask checked before DMA capable mem alloc by generic code*/
  dev->dma_mask=(u64*)ATA_DMA_MASK;
  dev->coherent_dma_mask=ATA_DMA_MASK;

  /* Get platfrom data */
  ptBar_priv = pdev->dev.platform_data;

  /*Set Intr & pass scsi info to libata core*/
  sata_aspen_sht.name=ptBar_priv->pcDeviceName;
  sata_aspen_sht.proc_name=ptBar_priv->pcDeviceName;  

  /* Assign private data */
  ap->private_data = ptBar_priv;

  retVal=ata_host_activate(host, ptBar_priv->uiIrqNum, Barata_interrupt, IRQF_SHARED,&sata_aspen_sht);

  dev_set_drvdata(dev, host);
  
  return retVal;
}

#define PMU_PLL_P_VALUE(x)	((x >> 20) & 0x3F)
#define PMU_PLL_M_VALUE(x)	((x >> 8) & 0x1FF)
#define PMU_PLL_S_VALUE(x)	(x & 7)

#define CWMIN_DFT 0x0C
#define CWMAX_DFT 0x17
#define CIMIN_DFT 0x28
#define CIMAX_DFT 0x47
#define ABHCLK_DFT 166667 // 166667 Khz

#define SATA_PHY_INIT_RETRY_CNT  (20)		// 5->20, by jinchen.kim
#define SATA_PHY_INIT_WAIT_CNT   (10000)


static void barc_sata_phy0_init(void)
{
	unsigned int regval;
	int i, cnt;

	unsigned int p, m, s, k, ahbclk;
	unsigned int cwMin, cwMax, ciMin, ciMax;
	
	// release HSATA 0 reset
	regval=barc_sata_reg_get(0x300908c0)|0x00400000;
	barc_sata_reg_set(0x300908c0, regval);

	// select SATA1/PCIe  1=SATA1, 0=PCIe(default)
	//regval=barc_sata_reg_get(0x300d000c)|0x00000001;
	//barc_sata_reg_set(0x300d000c, regval);

	// HSATA0 SCR2.SPD = 0001b (Limit to Gen1)
	//regval=barc_sata_reg_get(0x3001002c)&0xffffff0f;
	//regval=regval|0x00000010;
	//barc_sata_reg_set(0x3001002c, regval);

	regval = barc_sata_reg_get(0x30090808);
	p = PMU_PLL_P_VALUE(regval);
	m = PMU_PLL_M_VALUE(regval);
	s = PMU_PLL_S_VALUE(regval);
	k = barc_sata_reg_get(0x3009081C)&0xffff;

	ahbclk = (((m+k/1024) * ((24000000 >> s) / p)) >> 2) / 1000; //khz

	cwMin = CWMIN_DFT * ahbclk / ABHCLK_DFT;
	cwMax = CWMAX_DFT * ahbclk / ABHCLK_DFT;
	ciMin = CIMIN_DFT * ahbclk / ABHCLK_DFT;	
	ciMax = CIMAX_DFT * ahbclk / ABHCLK_DFT;		

	// OOBR - adjust values according to the Bus(SMX) clock 
	regval=barc_sata_reg_get(0x300100bc)|0x80000000;
	barc_sata_reg_set(0x300100bc, regval);
	regval = 0x80000000 | (cwMin << 24) | (cwMax << 16) | (ciMin << 8) | (ciMax << 0);
	barc_sata_reg_set(0x300100bc, regval);
	regval=barc_sata_reg_get(0x300100bc)&0x7fffffff;
	barc_sata_reg_set(0x300100bc, regval);

	// RXERR override (PHYCR[9] should be 1)
	regval=barc_sata_reg_get(0x30010088)&0xfffffdff;
	regval=regval|0x00000200;
	barc_sata_reg_set(0x30010088, regval);

	// release SATA 1 PHY reset
	regval=barc_sata_reg_get(0x300908c0)|0x00002000;
	barc_sata_reg_set(0x300908c0, regval);

	// may need time for resetting PHY
	for( i = 0; i < 100000; i++ );
	
	// PHYCR[14] = 1 release CMU PowerDown Exit
	regval=barc_sata_reg_get(0x30010088)|0x00004000;
	barc_sata_reg_set(0x30010088, regval);
	for( i = 0; i < 100000; i++ );

	// CMU
	barc_sata_reg_set(0x300f0000, 0x00000006);
	barc_sata_reg_set(0x300f0004, 0x00000000);
	barc_sata_reg_set(0x300f0008, 0x00000088);
	barc_sata_reg_set(0x300f000c, 0x00000000);
	barc_sata_reg_set(0x300f0010, 0x0000007b);
	barc_sata_reg_set(0x300f0014, 0x000000c9);
	barc_sata_reg_set(0x300f0018, 0x00000003);
	barc_sata_reg_set(0x300f001c, 0x00000000);
	barc_sata_reg_set(0x300f0020, 0x00000000);
	barc_sata_reg_set(0x300f0024, 0x00000000);
	barc_sata_reg_set(0x300f0028, 0x00000000);
	barc_sata_reg_set(0x300f002c, 0x00000000);
	barc_sata_reg_set(0x300f0030, 0x00000000);
	barc_sata_reg_set(0x300f0034, 0x00000000);
	barc_sata_reg_set(0x300f0038, 0x00000000);
	barc_sata_reg_set(0x300f003c, 0x00000000);
	barc_sata_reg_set(0x300f0040, 0x00000000);
	barc_sata_reg_set(0x300f0044, 0x00000000);
	barc_sata_reg_set(0x300f0048, 0x00000000);
	barc_sata_reg_set(0x300f004c, 0x00000000);
	barc_sata_reg_set(0x300f0050, 0x00000000);
	barc_sata_reg_set(0x300f0054, 0x00000000);
	barc_sata_reg_set(0x300f0058, 0x00000000);
	barc_sata_reg_set(0x300f005c, 0x00000000);
	barc_sata_reg_set(0x300f0060, 0x00000000);
	barc_sata_reg_set(0x300f0064, 0x00000000);
	barc_sata_reg_set(0x300f0068, 0x00000000);
	barc_sata_reg_set(0x300f006c, 0x00000000);
	barc_sata_reg_set(0x300f0070, 0x00000000);
	barc_sata_reg_set(0x300f0074, 0x00000000);
	barc_sata_reg_set(0x300f0078, 0x00000000);
	barc_sata_reg_set(0x300f007c, 0x00000000);
	barc_sata_reg_set(0x300f0080, 0x00000000);
	barc_sata_reg_set(0x300f0084, 0x00000000);
	barc_sata_reg_set(0x300f0088, 0x000000a0);
	barc_sata_reg_set(0x300f008c, 0x00000054);
	barc_sata_reg_set(0x300f0090, 0x00000000);
	barc_sata_reg_set(0x300f0094, 0x00000000);
	barc_sata_reg_set(0x300f0098, 0x00000000);
	barc_sata_reg_set(0x300f009c, 0x00000000);
	barc_sata_reg_set(0x300f00a0, 0x00000000);
	barc_sata_reg_set(0x300f00a4, 0x00000000);
	barc_sata_reg_set(0x300f00a8, 0x00000000);
	barc_sata_reg_set(0x300f00ac, 0x00000000);
	barc_sata_reg_set(0x300f00b0, 0x00000000);
	barc_sata_reg_set(0x300f00b4, 0x00000000);
	barc_sata_reg_set(0x300f00b8, 0x00000004);
	barc_sata_reg_set(0x300f00bc, 0x00000050);
	barc_sata_reg_set(0x300f00c0, 0x00000070);
	barc_sata_reg_set(0x300f00c4, 0x00000002);
	barc_sata_reg_set(0x300f00c8, 0x00000025);
	barc_sata_reg_set(0x300f00cc, 0x00000040);
	barc_sata_reg_set(0x300f00d0, 0x00000001);
	barc_sata_reg_set(0x300f00d4, 0x00000040);
	barc_sata_reg_set(0x300f00d8, 0x00000000);
	barc_sata_reg_set(0x300f00dc, 0x00000000);
	barc_sata_reg_set(0x300f00e0, 0x00000000);
	barc_sata_reg_set(0x300f00e4, 0x00000000);
	barc_sata_reg_set(0x300f00e8, 0x00000000);
	barc_sata_reg_set(0x300f00ec, 0x00000000);
	barc_sata_reg_set(0x300f00f0, 0x00000000);
	barc_sata_reg_set(0x300f00f4, 0x00000000);
	barc_sata_reg_set(0x300f00f8, 0x00000000);
	barc_sata_reg_set(0x300f00fc, 0x00000000);
	barc_sata_reg_set(0x300f0100, 0x00000000);
	barc_sata_reg_set(0x300f0104, 0x00000000);
	barc_sata_reg_set(0x300f0108, 0x00000000);
	barc_sata_reg_set(0x300f010c, 0x00000000);
	barc_sata_reg_set(0x300f0110, 0x00000000);
	barc_sata_reg_set(0x300f0114, 0x00000000);
	barc_sata_reg_set(0x300f0118, 0x00000000);
	barc_sata_reg_set(0x300f011c, 0x00000000);
	barc_sata_reg_set(0x300f0120, 0x00000000);
	barc_sata_reg_set(0x300f0124, 0x00000000);
	barc_sata_reg_set(0x300f0128, 0x00000000);
	barc_sata_reg_set(0x300f012c, 0x00000000);
	barc_sata_reg_set(0x300f0130, 0x00000000);
	barc_sata_reg_set(0x300f0134, 0x00000000);
	barc_sata_reg_set(0x300f0138, 0x00000000);
	barc_sata_reg_set(0x300f013c, 0x00000000);
	barc_sata_reg_set(0x300f0140, 0x00000000);
	barc_sata_reg_set(0x300f0144, 0x00000000);
	barc_sata_reg_set(0x300f0148, 0x00000000);
	barc_sata_reg_set(0x300f014c, 0x00000000);
	barc_sata_reg_set(0x300f0150, 0x00000000);
	barc_sata_reg_set(0x300f0154, 0x00000000);
	barc_sata_reg_set(0x300f0158, 0x00000000);
	barc_sata_reg_set(0x300f015c, 0x00000000);
	barc_sata_reg_set(0x300f0160, 0x00000000);
	barc_sata_reg_set(0x300f0164, 0x00000000);
	barc_sata_reg_set(0x300f0168, 0x00000000);
	barc_sata_reg_set(0x300f016c, 0x00000000);
	barc_sata_reg_set(0x300f0170, 0x00000000);
	barc_sata_reg_set(0x300f0174, 0x00000000);
	barc_sata_reg_set(0x300f0178, 0x00000000);
	barc_sata_reg_set(0x300f017c, 0x00000000);
	barc_sata_reg_set(0x300f0180, 0x00000000);
	barc_sata_reg_set(0x300f0184, 0x0000002e);
	barc_sata_reg_set(0x300f0188, 0x00000000);
	barc_sata_reg_set(0x300f018c, 0x0000005e);
	barc_sata_reg_set(0x300f0190, 0x00000000);
	barc_sata_reg_set(0x300f0194, 0x00000042);
	barc_sata_reg_set(0x300f0198, 0x000000d1);
	barc_sata_reg_set(0x300f019c, 0x00000020);
	barc_sata_reg_set(0x300f01a0, 0x00000028);
	barc_sata_reg_set(0x300f01a4, 0x00000078);
	barc_sata_reg_set(0x300f01a8, 0x0000002c);
	barc_sata_reg_set(0x300f01ac, 0x000000b9);
	barc_sata_reg_set(0x300f01b0, 0x0000005e);
	//barc_sata_reg_set(0x300f01b4, 0x00000000); //external differential reference clock input
	barc_sata_reg_set(0x300f01b4, 0x00000003); //internal refclk_core_i input
	barc_sata_reg_set(0x300f01b8, 0x00000000);
	barc_sata_reg_set(0x300f01bc, 0x00000000);
	barc_sata_reg_set(0x300f01c0, 0x00000000);
	barc_sata_reg_set(0x300f01c4, 0x00000000);
	barc_sata_reg_set(0x300f01c8, 0x00000000);

	// LANE0
	barc_sata_reg_set(0x300f0800, 0x00000002);
	barc_sata_reg_set(0x300f0804, 0x00000000);
	barc_sata_reg_set(0x300f0808, 0x00000000);
	barc_sata_reg_set(0x300f080c, 0x00000000);
	barc_sata_reg_set(0x300f0810, 0x00000000);
	barc_sata_reg_set(0x300f0814, 0x00000010);
	barc_sata_reg_set(0x300f0818, 0x00000084);
	barc_sata_reg_set(0x300f081c, 0x00000004);
	barc_sata_reg_set(0x300f0820, 0x000000e0);
	barc_sata_reg_set(0x300f0840, 0x00000023);
	barc_sata_reg_set(0x300f0844, 0x00000000);
	barc_sata_reg_set(0x300f0848, 0x00000040);
	barc_sata_reg_set(0x300f084c, 0x00000005);
	barc_sata_reg_set(0x300f0850, 0x000000d0);
	barc_sata_reg_set(0x300f0854, 0x00000017);
	barc_sata_reg_set(0x300f0858, 0x00000000);
	barc_sata_reg_set(0x300f085c, 0x00000068);
	barc_sata_reg_set(0x300f0860, 0x000000f2);
	barc_sata_reg_set(0x300f0864, 0x0000001e);
	barc_sata_reg_set(0x300f0868, 0x00000018);
	barc_sata_reg_set(0x300f086c, 0x0000000d);
	barc_sata_reg_set(0x300f0870, 0x0000000c);
	barc_sata_reg_set(0x300f0874, 0x00000000);
	barc_sata_reg_set(0x300f0878, 0x00000000);
	barc_sata_reg_set(0x300f087c, 0x00000000);
	barc_sata_reg_set(0x300f0880, 0x00000000);
	barc_sata_reg_set(0x300f0884, 0x00000000);
	barc_sata_reg_set(0x300f0888, 0x00000000);
	barc_sata_reg_set(0x300f088c, 0x00000000);
	barc_sata_reg_set(0x300f0890, 0x00000000);
	barc_sata_reg_set(0x300f0894, 0x00000000);
	barc_sata_reg_set(0x300f0898, 0x00000000);
	barc_sata_reg_set(0x300f089c, 0x00000000);
	barc_sata_reg_set(0x300f08a0, 0x00000000);
	barc_sata_reg_set(0x300f08a4, 0x00000000);
	barc_sata_reg_set(0x300f08a8, 0x00000000);
	barc_sata_reg_set(0x300f08ac, 0x00000000);
	barc_sata_reg_set(0x300f08b0, 0x00000000);
	barc_sata_reg_set(0x300f08b4, 0x00000000);
	barc_sata_reg_set(0x300f08b8, 0x00000000);
	barc_sata_reg_set(0x300f08bc, 0x00000000);
	barc_sata_reg_set(0x300f08c0, 0x00000000);
	barc_sata_reg_set(0x300f08c4, 0x00000000);
	barc_sata_reg_set(0x300f08c8, 0x00000000);
	barc_sata_reg_set(0x300f08cc, 0x00000000);
	barc_sata_reg_set(0x300f08d0, 0x00000000);
	barc_sata_reg_set(0x300f08d4, 0x00000000);
	barc_sata_reg_set(0x300f08d8, 0x00000000);
	barc_sata_reg_set(0x300f08dc, 0x00000000);
	barc_sata_reg_set(0x300f08e0, 0x00000000);
	barc_sata_reg_set(0x300f08e4, 0x00000000);
	barc_sata_reg_set(0x300f08e8, 0x00000000);
	barc_sata_reg_set(0x300f08ec, 0x00000000);
	barc_sata_reg_set(0x300f08f0, 0x00000000);
	barc_sata_reg_set(0x300f08f4, 0x00000000);
	barc_sata_reg_set(0x300f08f8, 0x00000000);
	barc_sata_reg_set(0x300f08fc, 0x00000000);
	barc_sata_reg_set(0x300f0900, 0x00000000);
	barc_sata_reg_set(0x300f0904, 0x00000000);
	barc_sata_reg_set(0x300f0908, 0x00000000);
	barc_sata_reg_set(0x300f090c, 0x00000000);
	barc_sata_reg_set(0x300f0910, 0x00000000);
	barc_sata_reg_set(0x300f0914, 0x00000000);
	barc_sata_reg_set(0x300f0918, 0x00000000);
	barc_sata_reg_set(0x300f091c, 0x00000000);
	barc_sata_reg_set(0x300f0920, 0x00000000);
	barc_sata_reg_set(0x300f0924, 0x00000000);
	barc_sata_reg_set(0x300f0928, 0x00000000);
	barc_sata_reg_set(0x300f092c, 0x00000000);
	barc_sata_reg_set(0x300f0930, 0x00000000);
	barc_sata_reg_set(0x300f0934, 0x00000000);
	barc_sata_reg_set(0x300f0938, 0x00000000);
	barc_sata_reg_set(0x300f093c, 0x00000000);
	barc_sata_reg_set(0x300f0940, 0x00000060);
	barc_sata_reg_set(0x300f0944, 0x0000000f);

	// COMLANE
	barc_sata_reg_set(0x300f2800, 0x00000000);
	barc_sata_reg_set(0x300f2804, 0x00000020);
	barc_sata_reg_set(0x300f2808, 0x00000000);
	barc_sata_reg_set(0x300f280c, 0x00000040);
	barc_sata_reg_set(0x300f2810, 0x00000024);
	barc_sata_reg_set(0x300f2814, 0x000000ae);
	barc_sata_reg_set(0x300f2818, 0x00000019);
	barc_sata_reg_set(0x300f281c, 0x00000049);
	barc_sata_reg_set(0x300f2820, 0x00000004);
	barc_sata_reg_set(0x300f2824, 0x00000083);
	barc_sata_reg_set(0x300f2828, 0x0000004b);
	barc_sata_reg_set(0x300f282c, 0x000000c5);
	barc_sata_reg_set(0x300f2830, 0x00000001);
	barc_sata_reg_set(0x300f2834, 0x00000003);
	barc_sata_reg_set(0x300f2838, 0x00000028);
	barc_sata_reg_set(0x300f283c, 0x00000098);
	barc_sata_reg_set(0x300f2840, 0x00000019);
	barc_sata_reg_set(0x300f2844, 0x00000000);
	barc_sata_reg_set(0x300f2848, 0x00000000);
	barc_sata_reg_set(0x300f284c, 0x00000080);
	barc_sata_reg_set(0x300f2850, 0x000000f0);
	barc_sata_reg_set(0x300f2854, 0x000000d0);
	barc_sata_reg_set(0x300f2858, 0x00000000);
	barc_sata_reg_set(0x300f285c, 0x00000000);
	barc_sata_reg_set(0x300f28c0, 0x00000000);
	barc_sata_reg_set(0x300f28c4, 0x00000000);
	barc_sata_reg_set(0x300f28c8, 0x00000000);
	barc_sata_reg_set(0x300f28cc, 0x00000000);
	barc_sata_reg_set(0x300f28d0, 0x00000000);
	barc_sata_reg_set(0x300f28d4, 0x00000000);
	barc_sata_reg_set(0x300f28d8, 0x00000000);
	barc_sata_reg_set(0x300f28dc, 0x00000000);
	barc_sata_reg_set(0x300f28e0, 0x00000000);
	barc_sata_reg_set(0x300f28e4, 0x000000a0);
	barc_sata_reg_set(0x300f28e8, 0x000000a0);
	barc_sata_reg_set(0x300f28ec, 0x000000a0);
	barc_sata_reg_set(0x300f28f0, 0x000000a0);
	barc_sata_reg_set(0x300f28f4, 0x000000a0);
	barc_sata_reg_set(0x300f28f8, 0x000000a0);
	barc_sata_reg_set(0x300f28fc, 0x000000a0);
	barc_sata_reg_set(0x300f2900, 0x00000054);
	barc_sata_reg_set(0x300f2904, 0x00000000);
	barc_sata_reg_set(0x300f2908, 0x00000080);
	barc_sata_reg_set(0x300f290c, 0x00000058);
	barc_sata_reg_set(0x300f2910, 0x00000000);
	barc_sata_reg_set(0x300f2914, 0x00000044);
	barc_sata_reg_set(0x300f2918, 0x0000005c);
	barc_sata_reg_set(0x300f291c, 0x00000086);
	barc_sata_reg_set(0x300f2920, 0x0000008d);
	barc_sata_reg_set(0x300f2924, 0x000000d0);
	barc_sata_reg_set(0x300f2928, 0x00000009);
	barc_sata_reg_set(0x300f292c, 0x00000090);
	barc_sata_reg_set(0x300f2930, 0x00000007);
	barc_sata_reg_set(0x300f2934, 0x00000040);
	barc_sata_reg_set(0x300f2938, 0x00000000);
	barc_sata_reg_set(0x300f293c, 0x00000000);
	barc_sata_reg_set(0x300f2940, 0x00000000);
	barc_sata_reg_set(0x300f2944, 0x00000020);
	barc_sata_reg_set(0x300f2948, 0x00000032);
	barc_sata_reg_set(0x300f294c, 0x00000000);
	barc_sata_reg_set(0x300f2950, 0x00000000);
	barc_sata_reg_set(0x300f2954, 0x00000000);
	barc_sata_reg_set(0x300f2958, 0x00000000);
	barc_sata_reg_set(0x300f295c, 0x00000000);
	barc_sata_reg_set(0x300f2960, 0x00000000);
	barc_sata_reg_set(0x300f2964, 0x00000000);
	barc_sata_reg_set(0x300f2968, 0x00000000);
	barc_sata_reg_set(0x300f296c, 0x00000000);
	barc_sata_reg_set(0x300f2970, 0x00000000);
	barc_sata_reg_set(0x300f2974, 0x00000000);
	barc_sata_reg_set(0x300f2978, 0x00000000);
	barc_sata_reg_set(0x300f297c, 0x00000000);
	barc_sata_reg_set(0x300f2980, 0x00000000);
	barc_sata_reg_set(0x300f2984, 0x00000000);
	barc_sata_reg_set(0x300f2988, 0x00000000);
	barc_sata_reg_set(0x300f298c, 0x00000000);
	barc_sata_reg_set(0x300f2990, 0x00000000);
	barc_sata_reg_set(0x300f2994, 0x00000000);
	barc_sata_reg_set(0x300f2998, 0x00000000);
	barc_sata_reg_set(0x300f299c, 0x00000000);
	barc_sata_reg_set(0x300f29a0, 0x00000000);
	barc_sata_reg_set(0x300f29a4, 0x00000000);
	barc_sata_reg_set(0x300f29a8, 0x00000000);
	barc_sata_reg_set(0x300f29ac, 0x00000000);
	barc_sata_reg_set(0x300f29b0, 0x00000000);
	barc_sata_reg_set(0x300f29b4, 0x00000000);
	barc_sata_reg_set(0x300f29b8, 0x00000000);
	barc_sata_reg_set(0x300f29bc, 0x00000000);
	barc_sata_reg_set(0x300f29c0, 0x00000000);
	barc_sata_reg_set(0x300f29c4, 0x00000000);
	barc_sata_reg_set(0x300f29c8, 0x00000000);
	barc_sata_reg_set(0x300f29cc, 0x00000000);
	barc_sata_reg_set(0x300f29d0, 0x00000000);
	barc_sata_reg_set(0x300f29d4, 0x00000000);
	barc_sata_reg_set(0x300f29d8, 0x00000000);
	barc_sata_reg_set(0x300f29dc, 0x00000000);
	barc_sata_reg_set(0x300f29e0, 0x00000000);
	barc_sata_reg_set(0x300f29e4, 0x00000000);
	barc_sata_reg_set(0x300f29e8, 0x00000000);
	barc_sata_reg_set(0x300f29ec, 0x00000000);
	barc_sata_reg_set(0x300f29f0, 0x00000000);
	barc_sata_reg_set(0x300f29f4, 0x00000000);
	barc_sata_reg_set(0x300f29f8, 0x00000000);
	barc_sata_reg_set(0x300f29fc, 0x000000d8);
	barc_sata_reg_set(0x300f2a00, 0x0000001a);
	barc_sata_reg_set(0x300f2a04, 0x000000ff);
	barc_sata_reg_set(0x300f2a08, 0x00000011);
	barc_sata_reg_set(0x300f2a0c, 0x00000000);
	barc_sata_reg_set(0x300f2a10, 0x00000000);
	barc_sata_reg_set(0x300f2a14, 0x00000000);
	barc_sata_reg_set(0x300f2a18, 0x00000000);
	barc_sata_reg_set(0x300f2a1c, 0x000000f0);
	barc_sata_reg_set(0x300f2a20, 0x000000ff);
	barc_sata_reg_set(0x300f2a24, 0x000000ff);
	barc_sata_reg_set(0x300f2a28, 0x000000ff);
	barc_sata_reg_set(0x300f2a2c, 0x000000ff);
	barc_sata_reg_set(0x300f2a30, 0x0000001c);
	barc_sata_reg_set(0x300f2a34, 0x000000c2);
	barc_sata_reg_set(0x300f2a38, 0x000000c3);
	barc_sata_reg_set(0x300f2a3c, 0x0000003f);
	barc_sata_reg_set(0x300f2a40, 0x0000000a);
	barc_sata_reg_set(0x300f2a44, 0x00000000);
	barc_sata_reg_set(0x300f2a48, 0x00000000);
	barc_sata_reg_set(0x300f2a4c, 0x00000000);
	barc_sata_reg_set(0x300f2a50, 0x00000000);
	barc_sata_reg_set(0x300f2a54, 0x00000000);
	barc_sata_reg_set(0x300f2a58, 0x000000f8);
	barc_sata_reg_set(0x300f0000, 0x00000007);


	// PHYCR[25] = 1 release CMU reset
	regval=barc_sata_reg_get(0x30010088)|0x02000000;
	barc_sata_reg_set(0x30010088, regval);

	// Wait PHYSR[0] CMU_OK (it takes about 300us)
	regval=barc_sata_reg_get(0x3001008c)&0x00000001;
	while(regval!=0x00000001)
	{
		regval=barc_sata_reg_get(0x3001008c)&0x00000001;
	}

	barc_sata_reg_set(0x300f2800, 0x00000000);
	barc_sata_reg_set(0x300f2800, 0x00000002);

	// PHYCR[27] = 1 release LANE0 reset
	regval=barc_sata_reg_get(0x30010088)|0x08000000;
	barc_sata_reg_set(0x30010088, regval);

	// Wait PHYSR[13] LN0_OK (it takes about 388us)
	regval=barc_sata_reg_get(0x3001008c)&0x00002000;
	while(regval!=0x00002000)
	{
		regval=barc_sata_reg_get(0x3001008c)&0x00002000;
	}

	cnt = SATA_PHY_INIT_RETRY_CNT;
	regval = 0;
	while(regval!=0x00000003 && cnt--)
	{
		i = SATA_PHY_INIT_WAIT_CNT;
		while(i--);
		
		regval=barc_sata_reg_get(0x30010024)&0x0000000f;

		barc_sata_reg_set(0x3001002c, 0x301);
		i = 100;
		while(i--);
		barc_sata_reg_set(0x3001002c, 0x300);
	}
}


static void barc_sata_phy1_init(void)
{
	unsigned int regval;
	int i, cnt;

	unsigned int p, m, s, k, ahbclk;
	unsigned int cwMin, cwMax, ciMin, ciMax;
	
	// release HSATA 1 reset
	regval=barc_sata_reg_get(0x300908c0)|0x08000000;
	barc_sata_reg_set(0x300908c0, regval);

	// select SATA1/PCIe  1=SATA1, 0=PCIe(default)
	regval=barc_sata_reg_get(0x300d000c)|0x00000001;
	barc_sata_reg_set(0x300d000c, regval);

	// HSATA1 SCR2.SPD = 0001b (Limit to Gen1)
	//regval=barc_sata_reg_get(0x3001802c)&0xffffff0f;
	//regval=regval|0x00000010;
	//barc_sata_reg_set(0x3001802c, regval);

	regval = barc_sata_reg_get(0x30090808);
	p = PMU_PLL_P_VALUE(regval);
	m = PMU_PLL_M_VALUE(regval);
	s = PMU_PLL_S_VALUE(regval);
	k = barc_sata_reg_get(0x3009081C)&0xffff;

	ahbclk = (((m+k/1024) * ((24000000 >> s) / p)) >> 2) / 1000;

	cwMin = CWMIN_DFT * ahbclk / ABHCLK_DFT;
	cwMax = CWMAX_DFT * ahbclk / ABHCLK_DFT;
	ciMin = CIMIN_DFT * ahbclk / ABHCLK_DFT;	
	ciMax = CIMAX_DFT * ahbclk / ABHCLK_DFT;

	// OOBR - adjust values according to the Bus(SMX) clock
	regval=barc_sata_reg_get(0x300180bc)|0x80000000;
	barc_sata_reg_set(0x300180bc, regval);
	//regval=0x89111e35;
	//regval = 0x870d182a;
	regval = 0x80000000 | (cwMin << 24) | (cwMax << 16) | (ciMin << 8) | (ciMax << 0);	
	barc_sata_reg_set(0x300180bc, regval);
	regval=barc_sata_reg_get(0x300180bc)&0x7fffffff;
	barc_sata_reg_set(0x300180bc, regval);

	// RXERR override (PHYCR[9] should be 1)
	regval=barc_sata_reg_get(0x30018088)&0xfffffdff;
	regval=regval|0x00000200;
	barc_sata_reg_set(0x30018088, regval);

	// release SATA 1 PHY reset
	regval=barc_sata_reg_get(0x300908c0)|0x00004000;
	barc_sata_reg_set(0x300908c0, regval);

	// may need time for resetting PHY
	for( i = 0; i < 100000; i++ );

	// PHYCR[14] = 1 release CMU PowerDown Exit
	regval=barc_sata_reg_get(0x30018088)|0x00004000;
	barc_sata_reg_set(0x30018088, regval);
	for( i = 0; i < 100000; i++ );

	// CMU
	barc_sata_reg_set(0x300f8000, 0x00000006);
	barc_sata_reg_set(0x300f8004, 0x00000000);
	barc_sata_reg_set(0x300f8008, 0x00000088);
	barc_sata_reg_set(0x300f800c, 0x00000000);
	barc_sata_reg_set(0x300f8010, 0x0000007b);
	barc_sata_reg_set(0x300f8014, 0x000000c9);
	barc_sata_reg_set(0x300f8018, 0x00000003);
	barc_sata_reg_set(0x300f801c, 0x00000000);
	barc_sata_reg_set(0x300f8020, 0x00000000);
	barc_sata_reg_set(0x300f8024, 0x00000000);
	barc_sata_reg_set(0x300f8028, 0x00000000);
	barc_sata_reg_set(0x300f802c, 0x00000000);
	barc_sata_reg_set(0x300f8030, 0x00000000);
	barc_sata_reg_set(0x300f8034, 0x00000000);
	barc_sata_reg_set(0x300f8038, 0x00000000);
	barc_sata_reg_set(0x300f803c, 0x00000000);
	barc_sata_reg_set(0x300f8040, 0x00000000);
	barc_sata_reg_set(0x300f8044, 0x00000000);
	barc_sata_reg_set(0x300f8048, 0x00000000);
	barc_sata_reg_set(0x300f804c, 0x00000000);
	barc_sata_reg_set(0x300f8050, 0x00000000);
	barc_sata_reg_set(0x300f8054, 0x00000000);
	barc_sata_reg_set(0x300f8058, 0x00000000);
	barc_sata_reg_set(0x300f805c, 0x00000000);
	barc_sata_reg_set(0x300f8060, 0x00000000);
	barc_sata_reg_set(0x300f8064, 0x00000000);
	barc_sata_reg_set(0x300f8068, 0x00000000);
	barc_sata_reg_set(0x300f806c, 0x00000000);
	barc_sata_reg_set(0x300f8070, 0x00000000);
	barc_sata_reg_set(0x300f8074, 0x00000000);
	barc_sata_reg_set(0x300f8078, 0x00000000);
	barc_sata_reg_set(0x300f807c, 0x00000000);
	barc_sata_reg_set(0x300f8080, 0x00000000);
	barc_sata_reg_set(0x300f8084, 0x00000000);
	barc_sata_reg_set(0x300f8088, 0x000000a0);
	barc_sata_reg_set(0x300f808c, 0x00000054);
	barc_sata_reg_set(0x300f8090, 0x00000000);
	barc_sata_reg_set(0x300f8094, 0x00000000);
	barc_sata_reg_set(0x300f8098, 0x00000000);
	barc_sata_reg_set(0x300f809c, 0x00000000);
	barc_sata_reg_set(0x300f80a0, 0x00000000);
	barc_sata_reg_set(0x300f80a4, 0x00000000);
	barc_sata_reg_set(0x300f80a8, 0x00000000);
	barc_sata_reg_set(0x300f80ac, 0x00000000);
	barc_sata_reg_set(0x300f80b0, 0x00000000);
	barc_sata_reg_set(0x300f80b4, 0x00000000);
	barc_sata_reg_set(0x300f80b8, 0x00000004);
	barc_sata_reg_set(0x300f80bc, 0x00000050);
	barc_sata_reg_set(0x300f80c0, 0x00000070);
	barc_sata_reg_set(0x300f80c4, 0x00000002);
	barc_sata_reg_set(0x300f80c8, 0x00000025);
	barc_sata_reg_set(0x300f80cc, 0x00000040);
	barc_sata_reg_set(0x300f80d0, 0x00000001);
	barc_sata_reg_set(0x300f80d4, 0x00000040);
	barc_sata_reg_set(0x300f80d8, 0x00000000);
	barc_sata_reg_set(0x300f80dc, 0x00000000);
	barc_sata_reg_set(0x300f80e0, 0x00000000);
	barc_sata_reg_set(0x300f80e4, 0x00000000);
	barc_sata_reg_set(0x300f80e8, 0x00000000);
	barc_sata_reg_set(0x300f80ec, 0x00000000);
	barc_sata_reg_set(0x300f80f0, 0x00000000);
	barc_sata_reg_set(0x300f80f4, 0x00000000);
	barc_sata_reg_set(0x300f80f8, 0x00000000);
	barc_sata_reg_set(0x300f80fc, 0x00000000);
	barc_sata_reg_set(0x300f8100, 0x00000000);
	barc_sata_reg_set(0x300f8104, 0x00000000);
	barc_sata_reg_set(0x300f8108, 0x00000000);
	barc_sata_reg_set(0x300f810c, 0x00000000);
	barc_sata_reg_set(0x300f8110, 0x00000000);
	barc_sata_reg_set(0x300f8114, 0x00000000);
	barc_sata_reg_set(0x300f8118, 0x00000000);
	barc_sata_reg_set(0x300f811c, 0x00000000);
	barc_sata_reg_set(0x300f8120, 0x00000000);
	barc_sata_reg_set(0x300f8124, 0x00000000);
	barc_sata_reg_set(0x300f8128, 0x00000000);
	barc_sata_reg_set(0x300f812c, 0x00000000);
	barc_sata_reg_set(0x300f8130, 0x00000000);
	barc_sata_reg_set(0x300f8134, 0x00000000);
	barc_sata_reg_set(0x300f8138, 0x00000000);
	barc_sata_reg_set(0x300f813c, 0x00000000);
	barc_sata_reg_set(0x300f8140, 0x00000000);
	barc_sata_reg_set(0x300f8144, 0x00000000);
	barc_sata_reg_set(0x300f8148, 0x00000000);
	barc_sata_reg_set(0x300f814c, 0x00000000);
	barc_sata_reg_set(0x300f8150, 0x00000000);
	barc_sata_reg_set(0x300f8154, 0x00000000);
	barc_sata_reg_set(0x300f8158, 0x00000000);
	barc_sata_reg_set(0x300f815c, 0x00000000);
	barc_sata_reg_set(0x300f8160, 0x00000000);
	barc_sata_reg_set(0x300f8164, 0x00000000);
	barc_sata_reg_set(0x300f8168, 0x00000000);
	barc_sata_reg_set(0x300f816c, 0x00000000);
	barc_sata_reg_set(0x300f8170, 0x00000000);
	barc_sata_reg_set(0x300f8174, 0x00000000);
	barc_sata_reg_set(0x300f8178, 0x00000000);
	barc_sata_reg_set(0x300f817c, 0x00000000);
	barc_sata_reg_set(0x300f8180, 0x00000000);
	barc_sata_reg_set(0x300f8184, 0x0000002e);
	barc_sata_reg_set(0x300f8188, 0x00000000);
	barc_sata_reg_set(0x300f818c, 0x0000005e);
	barc_sata_reg_set(0x300f8190, 0x00000000);
	barc_sata_reg_set(0x300f8194, 0x00000042);
	barc_sata_reg_set(0x300f8198, 0x000000d1);
	barc_sata_reg_set(0x300f819c, 0x00000020);
	barc_sata_reg_set(0x300f81a0, 0x00000028);
	barc_sata_reg_set(0x300f81a4, 0x00000078);
	barc_sata_reg_set(0x300f81a8, 0x0000002c);
	barc_sata_reg_set(0x300f81ac, 0x000000b9);
	barc_sata_reg_set(0x300f81b0, 0x0000005e);
	//barc_sata_reg_set(0x300f81b4, 0x00000000); //external differential reference clock input
	barc_sata_reg_set(0x300f81b4, 0x00000003); //internal refclk_core_i input
	barc_sata_reg_set(0x300f81b8, 0x00000000);
	barc_sata_reg_set(0x300f81bc, 0x00000000);
	barc_sata_reg_set(0x300f81c0, 0x00000000);
	barc_sata_reg_set(0x300f81c4, 0x00000000);
	barc_sata_reg_set(0x300f81c8, 0x00000000);

	// LANE0
	barc_sata_reg_set(0x300f8800, 0x00000002);
	barc_sata_reg_set(0x300f8804, 0x00000000);
	barc_sata_reg_set(0x300f8808, 0x00000000);
	barc_sata_reg_set(0x300f880c, 0x00000000);
	barc_sata_reg_set(0x300f8810, 0x00000000);
	barc_sata_reg_set(0x300f8814, 0x00000010);
	barc_sata_reg_set(0x300f8818, 0x00000084);
	barc_sata_reg_set(0x300f881c, 0x00000004);
	barc_sata_reg_set(0x300f8820, 0x000000e0);
	barc_sata_reg_set(0x300f8840, 0x00000023);
	barc_sata_reg_set(0x300f8844, 0x00000000);
	barc_sata_reg_set(0x300f8848, 0x00000040);
	barc_sata_reg_set(0x300f884c, 0x00000005);
	barc_sata_reg_set(0x300f8850, 0x000000d0);
	barc_sata_reg_set(0x300f8854, 0x00000017);
	barc_sata_reg_set(0x300f8858, 0x00000000);
	barc_sata_reg_set(0x300f885c, 0x00000068);
	barc_sata_reg_set(0x300f8860, 0x000000f2);
	barc_sata_reg_set(0x300f8864, 0x0000001e);
	barc_sata_reg_set(0x300f8868, 0x00000018);
	barc_sata_reg_set(0x300f886c, 0x0000000d);
	barc_sata_reg_set(0x300f8870, 0x0000000c);
	barc_sata_reg_set(0x300f8874, 0x00000000);
	barc_sata_reg_set(0x300f8878, 0x00000000);
	barc_sata_reg_set(0x300f887c, 0x00000000);
	barc_sata_reg_set(0x300f8880, 0x00000000);
	barc_sata_reg_set(0x300f8884, 0x00000000);
	barc_sata_reg_set(0x300f8888, 0x00000000);
	barc_sata_reg_set(0x300f888c, 0x00000000);
	barc_sata_reg_set(0x300f8890, 0x00000000);
	barc_sata_reg_set(0x300f8894, 0x00000000);
	barc_sata_reg_set(0x300f8898, 0x00000000);
	barc_sata_reg_set(0x300f889c, 0x00000000);
	barc_sata_reg_set(0x300f88a0, 0x00000000);
	barc_sata_reg_set(0x300f88a4, 0x00000000);
	barc_sata_reg_set(0x300f88a8, 0x00000000);
	barc_sata_reg_set(0x300f88ac, 0x00000000);
	barc_sata_reg_set(0x300f88b0, 0x00000000);
	barc_sata_reg_set(0x300f88b4, 0x00000000);
	barc_sata_reg_set(0x300f88b8, 0x00000000);
	barc_sata_reg_set(0x300f88bc, 0x00000000);
	barc_sata_reg_set(0x300f88c0, 0x00000000);
	barc_sata_reg_set(0x300f88c4, 0x00000000);
	barc_sata_reg_set(0x300f88c8, 0x00000000);
	barc_sata_reg_set(0x300f88cc, 0x00000000);
	barc_sata_reg_set(0x300f88d0, 0x00000000);
	barc_sata_reg_set(0x300f88d4, 0x00000000);
	barc_sata_reg_set(0x300f88d8, 0x00000000);
	barc_sata_reg_set(0x300f88dc, 0x00000000);
	barc_sata_reg_set(0x300f88e0, 0x00000000);
	barc_sata_reg_set(0x300f88e4, 0x00000000);
	barc_sata_reg_set(0x300f88e8, 0x00000000);
	barc_sata_reg_set(0x300f88ec, 0x00000000);
	barc_sata_reg_set(0x300f88f0, 0x00000000);
	barc_sata_reg_set(0x300f88f4, 0x00000000);
	barc_sata_reg_set(0x300f88f8, 0x00000000);
	barc_sata_reg_set(0x300f88fc, 0x00000000);
	barc_sata_reg_set(0x300f8900, 0x00000000);
	barc_sata_reg_set(0x300f8904, 0x00000000);
	barc_sata_reg_set(0x300f8908, 0x00000000);
	barc_sata_reg_set(0x300f890c, 0x00000000);
	barc_sata_reg_set(0x300f8910, 0x00000000);
	barc_sata_reg_set(0x300f8914, 0x00000000);
	barc_sata_reg_set(0x300f8918, 0x00000000);
	barc_sata_reg_set(0x300f891c, 0x00000000);
	barc_sata_reg_set(0x300f8920, 0x00000000);
	barc_sata_reg_set(0x300f8924, 0x00000000);
	barc_sata_reg_set(0x300f8928, 0x00000000);
	barc_sata_reg_set(0x300f892c, 0x00000000);
	barc_sata_reg_set(0x300f8930, 0x00000000);
	barc_sata_reg_set(0x300f8934, 0x00000000);
	barc_sata_reg_set(0x300f8938, 0x00000000);
	barc_sata_reg_set(0x300f893c, 0x00000000);
	barc_sata_reg_set(0x300f8940, 0x00000060);
	barc_sata_reg_set(0x300f8944, 0x0000000f);

	// COMLANE
	barc_sata_reg_set(0x300fa800, 0x00000000);
	barc_sata_reg_set(0x300fa804, 0x00000020);
	barc_sata_reg_set(0x300fa808, 0x00000000);
	barc_sata_reg_set(0x300fa80c, 0x00000040);
	barc_sata_reg_set(0x300fa810, 0x00000024);
	barc_sata_reg_set(0x300fa814, 0x000000ae);
	barc_sata_reg_set(0x300fa818, 0x00000019);
	barc_sata_reg_set(0x300fa81c, 0x00000049);
	barc_sata_reg_set(0x300fa820, 0x00000004);
	barc_sata_reg_set(0x300fa824, 0x00000083);
	barc_sata_reg_set(0x300fa828, 0x0000004b);
	barc_sata_reg_set(0x300fa82c, 0x000000c5);
	barc_sata_reg_set(0x300fa830, 0x00000001);
	barc_sata_reg_set(0x300fa834, 0x00000003);
	barc_sata_reg_set(0x300fa838, 0x00000028);
	barc_sata_reg_set(0x300fa83c, 0x00000098);
	barc_sata_reg_set(0x300fa840, 0x00000019);
	barc_sata_reg_set(0x300fa844, 0x00000000);
	barc_sata_reg_set(0x300fa848, 0x00000000);
	barc_sata_reg_set(0x300fa84c, 0x00000080);
	barc_sata_reg_set(0x300fa850, 0x000000f0);
	barc_sata_reg_set(0x300fa854, 0x000000d0);
	barc_sata_reg_set(0x300fa858, 0x00000000);
	barc_sata_reg_set(0x300fa85c, 0x00000000);
	barc_sata_reg_set(0x300fa8c0, 0x00000000);
	barc_sata_reg_set(0x300fa8c4, 0x00000000);
	barc_sata_reg_set(0x300fa8c8, 0x00000000);
	barc_sata_reg_set(0x300fa8cc, 0x00000000);
	barc_sata_reg_set(0x300fa8d0, 0x00000000);
	barc_sata_reg_set(0x300fa8d4, 0x00000000);
	barc_sata_reg_set(0x300fa8d8, 0x00000000);
	barc_sata_reg_set(0x300fa8dc, 0x00000000);
	barc_sata_reg_set(0x300fa8e0, 0x00000000);
	barc_sata_reg_set(0x300fa8e4, 0x000000a0);
	barc_sata_reg_set(0x300fa8e8, 0x000000a0);
	barc_sata_reg_set(0x300fa8ec, 0x000000a0);
	barc_sata_reg_set(0x300fa8f0, 0x000000a0);
	barc_sata_reg_set(0x300fa8f4, 0x000000a0);
	barc_sata_reg_set(0x300fa8f8, 0x000000a0);
	barc_sata_reg_set(0x300fa8fc, 0x000000a0);
	barc_sata_reg_set(0x300fa900, 0x00000054);
	barc_sata_reg_set(0x300fa904, 0x00000000);
	barc_sata_reg_set(0x300fa908, 0x00000080);
	barc_sata_reg_set(0x300fa90c, 0x00000058);
	barc_sata_reg_set(0x300fa910, 0x00000000);
	barc_sata_reg_set(0x300fa914, 0x00000044);
	barc_sata_reg_set(0x300fa918, 0x0000005c);
	barc_sata_reg_set(0x300fa91c, 0x00000086);
	barc_sata_reg_set(0x300fa920, 0x0000008d);
	barc_sata_reg_set(0x300fa924, 0x000000d0);
	barc_sata_reg_set(0x300fa928, 0x00000009);
	barc_sata_reg_set(0x300fa92c, 0x00000090);
	barc_sata_reg_set(0x300fa930, 0x00000007);
	barc_sata_reg_set(0x300fa934, 0x00000040);
	barc_sata_reg_set(0x300fa938, 0x00000000);
	barc_sata_reg_set(0x300fa93c, 0x00000000);
	barc_sata_reg_set(0x300fa940, 0x00000000);
	barc_sata_reg_set(0x300fa944, 0x00000020);
	barc_sata_reg_set(0x300fa948, 0x00000032);
	barc_sata_reg_set(0x300fa94c, 0x00000000);
	barc_sata_reg_set(0x300fa950, 0x00000000);
	barc_sata_reg_set(0x300fa954, 0x00000000);
	barc_sata_reg_set(0x300fa958, 0x00000000);
	barc_sata_reg_set(0x300fa95c, 0x00000000);
	barc_sata_reg_set(0x300fa960, 0x00000000);
	barc_sata_reg_set(0x300fa964, 0x00000000);
	barc_sata_reg_set(0x300fa968, 0x00000000);
	barc_sata_reg_set(0x300fa96c, 0x00000000);
	barc_sata_reg_set(0x300fa970, 0x00000000);
	barc_sata_reg_set(0x300fa974, 0x00000000);
	barc_sata_reg_set(0x300fa978, 0x00000000);
	barc_sata_reg_set(0x300fa97c, 0x00000000);
	barc_sata_reg_set(0x300fa980, 0x00000000);
	barc_sata_reg_set(0x300fa984, 0x00000000);
	barc_sata_reg_set(0x300fa988, 0x00000000);
	barc_sata_reg_set(0x300fa98c, 0x00000000);
	barc_sata_reg_set(0x300fa990, 0x00000000);
	barc_sata_reg_set(0x300fa994, 0x00000000);
	barc_sata_reg_set(0x300fa998, 0x00000000);
	barc_sata_reg_set(0x300fa99c, 0x00000000);
	barc_sata_reg_set(0x300fa9a0, 0x00000000);
	barc_sata_reg_set(0x300fa9a4, 0x00000000);
	barc_sata_reg_set(0x300fa9a8, 0x00000000);
	barc_sata_reg_set(0x300fa9ac, 0x00000000);
	barc_sata_reg_set(0x300fa9b0, 0x00000000);
	barc_sata_reg_set(0x300fa9b4, 0x00000000);
	barc_sata_reg_set(0x300fa9b8, 0x00000000);
	barc_sata_reg_set(0x300fa9bc, 0x00000000);
	barc_sata_reg_set(0x300fa9c0, 0x00000000);
	barc_sata_reg_set(0x300fa9c4, 0x00000000);
	barc_sata_reg_set(0x300fa9c8, 0x00000000);
	barc_sata_reg_set(0x300fa9cc, 0x00000000);
	barc_sata_reg_set(0x300fa9d0, 0x00000000);
	barc_sata_reg_set(0x300fa9d4, 0x00000000);
	barc_sata_reg_set(0x300fa9d8, 0x00000000);
	barc_sata_reg_set(0x300fa9dc, 0x00000000);
	barc_sata_reg_set(0x300fa9e0, 0x00000000);
	barc_sata_reg_set(0x300fa9e4, 0x00000000);
	barc_sata_reg_set(0x300fa9e8, 0x00000000);
	barc_sata_reg_set(0x300fa9ec, 0x00000000);
	barc_sata_reg_set(0x300fa9f0, 0x00000000);
	barc_sata_reg_set(0x300fa9f4, 0x00000000);
	barc_sata_reg_set(0x300fa9f8, 0x00000000);
	barc_sata_reg_set(0x300fa9fc, 0x000000d8);
	barc_sata_reg_set(0x300faa00, 0x0000001a);
	barc_sata_reg_set(0x300faa04, 0x000000ff);
	barc_sata_reg_set(0x300faa08, 0x00000011);
	barc_sata_reg_set(0x300faa0c, 0x00000000);
	barc_sata_reg_set(0x300faa10, 0x00000000);
	barc_sata_reg_set(0x300faa14, 0x00000000);
	barc_sata_reg_set(0x300faa18, 0x00000000);
	barc_sata_reg_set(0x300faa1c, 0x000000f0);
	barc_sata_reg_set(0x300faa20, 0x000000ff);
	barc_sata_reg_set(0x300faa24, 0x000000ff);
	barc_sata_reg_set(0x300faa28, 0x000000ff);
	barc_sata_reg_set(0x300faa2c, 0x000000ff);
	barc_sata_reg_set(0x300faa30, 0x0000001c);
	barc_sata_reg_set(0x300faa34, 0x000000c2);
	barc_sata_reg_set(0x300faa38, 0x000000c3);
	barc_sata_reg_set(0x300faa3c, 0x0000003f);
	barc_sata_reg_set(0x300faa40, 0x0000000a);
	barc_sata_reg_set(0x300faa44, 0x00000000);
	barc_sata_reg_set(0x300faa48, 0x00000000);
	barc_sata_reg_set(0x300faa4c, 0x00000000);
	barc_sata_reg_set(0x300faa50, 0x00000000);
	barc_sata_reg_set(0x300faa54, 0x00000000);
	barc_sata_reg_set(0x300faa58, 0x000000f8);
	barc_sata_reg_set(0x300f8000, 0x00000007);


	// PHYCR[25] = 1 release CMU reset
	regval=barc_sata_reg_get(0x30018088)|0x02000000;
	barc_sata_reg_set(0x30018088, regval);

	// Wait PHYSR[0] CMU_OK (it takes about 300us)
	regval=barc_sata_reg_get(0x3001808c)&0x00000001;
	while(regval!=0x00000001)
	{
		regval=barc_sata_reg_get(0x3001808c)&0x00000001;
	}


	barc_sata_reg_set(0x300fa800, 0x00000000);
	barc_sata_reg_set(0x300fa800, 0x00000002);


	// PHYCR[27] = 1 release LANE0 reset
	regval=barc_sata_reg_get(0x30018088)|0x08000000;
	barc_sata_reg_set(0x30018088, regval);

	// Wait PHYSR[13] LN0_OK (it takes about 388us)
	regval=barc_sata_reg_get(0x3001808c)&0x00002000;
	while(regval!=0x00002000)
	{
		regval=barc_sata_reg_get(0x3001808c)&0x00002000;
	}


	cnt = SATA_PHY_INIT_RETRY_CNT;
	regval = 0;
	while(regval!=0x00000003 && cnt--)
	{
		i = SATA_PHY_INIT_WAIT_CNT;
		while(i--);
		
		regval=barc_sata_reg_get(0x30018024)&0x0000000f;

		barc_sata_reg_set(0x3001802c, 0x301);
		i = 100;
		while(i--);
		barc_sata_reg_set(0x3001802c, 0x300);
}
}



void Bar_ReSetFunction(unsigned int uiIrqNum)
{
	u32 u32ResetVal;

	u32ResetVal = barc_sata_reg_get(0x300908c0);

	printk("Reset Device Irq....no..%d\n", uiIrqNum);		
	if ( uiIrqNum == INT_SATA0 )
	{
		u32ResetVal &= ~( 1 << 22); 		/* sata 0 sw reset	*/
		barc_sata_reg_set(0x300908c0,u32ResetVal);	
		barc_sata_phy0_init();				/* Initializing sata0 phy */
	}
	else
	{
		u32ResetVal &= ~( 1 << 27); 		/* sata 1 sw reset	*/
		barc_sata_reg_set(0x300908c0,u32ResetVal);	
		barc_sata_phy1_init();				/* Initializing sata1 phy */
	}
}


/**
 *This function Init routine
 *@function aspen_init
 *@return int 
 *@param void
 */
static int __init \
aspen_init(void){
  int retVal,i;
  Barcelona_private_t* ptBar_priv;
  unsigned int uiIntTable[NR_DEVS]={INT_SATA0,INT_SATA1};

  /* Initializing sata0/sata1 phy */
  barc_sata_phy0_init();
  barc_sata_phy1_init();
  
 /*We need to register both the host controllers*/
 for( i = START_SATA_NUM ; i < LAST_SATA_NUM ; i++)
{
	sprintf(sdp93host[i].name,"%s%d","SATA",i);

	sdp93host[i].pdevice=platform_device_alloc(sdp93host[i].name, i);

	if (!sdp93host[i].pdevice) {
		ASPEN_DBG(KERN_ERR"\nENOMEM %s Line %d", __FUNCTION__, __LINE__);  	  	
		retVal = -ENOMEM;
		goto out;
	}	

	/* Allocate memory	*/
	ptBar_priv = kmalloc(sizeof(Barcelona_private_t), GFP_KERNEL);
	if (!ptBar_priv)
	{
		ASPEN_DBG(KERN_ERR"\nENOMEM %s Line %d", __FUNCTION__, __LINE__);	
		return -ENOMEM; 	
		goto out;
	}

	/* Init memory */
	memset(ptBar_priv, 0, sizeof(Barcelona_private_t));

	/* Assign Interrupt number*/
	ptBar_priv->uiIrqNum = uiIntTable[i];
	
	/*	Assign device id*/
	ptBar_priv->uiDevno = i;
	
	/* Assign buffer address */
	ptBar_priv->pvDmaBufVA = pvSataDmaBufVA[i];
	ptBar_priv->uiDmaBufPA = uiSataDmaBufPA[i];
	
	/* Assign reset function */
	ptBar_priv->ResetFuntion = Bar_ReSetFunction;

	/* Assign device Name*/
	ptBar_priv->pcDeviceName = sdp93host[i].name;

	/* Allocate memory */
	ptBar_priv->ptasklet_FuncPrivate = kmalloc(sizeof(Barcelona_private_t), GFP_KERNEL);
	if (!ptBar_priv->ptasklet_FuncPrivate)
	{
		ASPEN_DBG(KERN_ERR"\nENOMEM %s Line %d", __FUNCTION__, __LINE__);	
		kfree(ptBar_priv);
		return -ENOMEM; 	
		goto out;
	}

	/* init data*/
	ptBar_priv->ptasklet_FuncPrivate->uiDevno = ptBar_priv->uiDevno;
	ptBar_priv->ptasklet_FuncPrivate->uiIrqNum = ptBar_priv->uiIrqNum;
	ptBar_priv->ptasklet_FuncPrivate->pvDmaBufVA = ptBar_priv->pvDmaBufVA;
	ptBar_priv->ptasklet_FuncPrivate->uiDmaBufPA = ptBar_priv->uiDmaBufPA;
	ptBar_priv->ptasklet_FuncPrivate->uiReadCount = 0;
	ptBar_priv->ptasklet_FuncPrivate->uiReadSize = 0;
	ptBar_priv->ptasklet_FuncPrivate->uiWriteCount = 0;
	ptBar_priv->ptasklet_FuncPrivate->uiWriteSize = 0;

	sdp93host[i].pdevice->dev.platform_data = ptBar_priv;

	retVal = platform_device_add(sdp93host[i].pdevice);
	if (retVal)
		goto put_dev;  

	aspen_platform_driver[i].driver.name=sdp93host[i].name;
	retVal = platform_driver_register(&aspen_platform_driver[i]);
	if(retVal==0){
		continue;
	}
  }
  
  if (retVal == 0)
    goto out;

  platform_device_del(sdp93host[i].pdevice);
 put_dev:
  ASPEN_DBG(KERN_ERR"\nput_dev %s Line %d", __FUNCTION__, __LINE__);  	  	
  platform_device_put(sdp93host[i].pdevice);         
 out:
  return retVal;
}

/**
 *This function Exit routine
 *@function aspen_exit
 *@return void 
 *@param void
 */
static void __exit aspen_exit(void){
  int i;
  Barcelona_private_t* ptBar_priv;
  
  platform_driver_unregister(&aspen_platform_driver[0]);
  platform_driver_unregister(&aspen_platform_driver[1]);

 for( i = START_SATA_NUM ; i < LAST_SATA_NUM ; i++)
  {
	if ( sdp93host[i].pdevice )
    {
	  platform_device_unregister(sdp93host[i].pdevice); 
	  if (sdp93host[i].pdevice->dev.platform_data)
	  {
	  	 ptBar_priv = sdp93host[i].pdevice->dev.platform_data;
	  	 kfree(ptBar_priv->ptasklet_FuncPrivate);
	     kfree(sdp93host[i].pdevice->dev.platform_data);
	  }
	}
  }
}


module_init(aspen_init);
module_exit(aspen_exit);
MODULE_AUTHOR("Rudrajit Sengupta");
MODULE_DESCRIPTION("low-level driver for SATA CONTROLLER in SAMSUNG SDP93 SoC");
MODULE_LICENSE("GPL");



