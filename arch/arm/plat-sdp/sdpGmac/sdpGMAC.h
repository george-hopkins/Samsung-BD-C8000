/***************************************************************************
 *
 *	arch/arm/plat-sdp/sdpGmac/sdpGmac.h
 *	Samsung Elecotronics.Co
 *	Created by tukho.kim	
 *
 * ************************************************************************/
/*
 * 2009.08.02: Created by tukho.kim@samsung.com
 * 2009.10.22,tukho.kim: debug when rx buffer alloc is failed 0.940
 * 2009.12.01,tukho.kim: debug phy check media, 0.951
 *                       full <-> half ok, 10 -> 100 ok, 100 -> 10 need to unplug and plug cable
 */ 


#ifndef __SDP_GMAC_H

/* define debug */


#if 0
#define DPRINTK_GMAC_FLOW(fmt,args...) printk("[GMAC %s %u]" fmt,__FUNCTION__,(unsigned int)jiffies,##args)
#define DPRINTK_GMAC(fmt,args...) printk("[GMAC %s %u]" fmt,__FUNCTION__,(unsigned int)jiffies,##args)
#else
#define DPRINTK_GMAC_FLOW(fmt,args...)
#define DPRINTK_GMAC(fmt,args...) 
#endif


#define DPRINTK_GMAC_ERROR(fmt,args...) printk(KERN_ERR "[GMAC %s]" fmt,__FUNCTION__,##args)
#define PRINTK_GMAC(fmt,args...) printk("[SDP GMAC]" fmt,##args)

#define SDP_GMAC_OK 	0
#define SDP_GMAC_ERR 	1	

#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/delay.h>
#include <linux/slab.h>
#include <linux/interrupt.h>
#include <linux/errno.h>
#include <linux/ioport.h>
#include <linux/crc32.h>
#include <linux/device.h>
#include <linux/platform_device.h>
#include <linux/workqueue.h>
#include <linux/skbuff.h>
#include <linux/dma-mapping.h>

/* Ethernet header */
#include <linux/ethtool.h>
#include <linux/mii.h>
#include <linux/netdevice.h>
#include <linux/etherdevice.h>

/* Io Header */
#include <asm/uaccess.h>
#include <asm/io.h>

#ifdef CONFIG_MAC_GET_I2C
#include <asm/plat-sdp/sdp_i2c_io.h>
#endif 

#include <asm/plat-sdp/sdpGmac_reg.h>	// 0.952

//#define ETHER_NAME		"SDP_GMAC" // 0.952 move to sdpGmac_reg.h

#define N_MAC_ADDR	 	ETH_ALEN
#define DEFAULT_MAC_ADDR 	{0xFF,0xFF,0xFF,0xFF,0xFF,0xFF}

#define STATUS_SPEED_10			(0x00)
#define STATUS_SPEED_100		(0x01)

#define STATUS_DUPLEX_HALF		(0x00)
#define STATUS_DUPLEX_FULL		(0x01)

#define STATUS_UNLINK			(0x00)
#define STATUS_LINK			(0x01)

#define CFG_LINK_CHECK_TIME		(HZ >> 1) 	// every 500ms Time 


typedef struct {
	u32 dma_busMode;	
	u32 dma_operationMode;
	u32 gmac_configuration;
	u32 gmac_frameFilter;
	u32 gmac_macAddr[32];	
}SDP_GMAC_POWER_T;


typedef struct {
/* network resouce */
	struct net_device 	 	*pNetDev;
	struct net_device_stats		netStats;
/* RMII resrouce */
	struct mii_if_info		mii;

/* OS */
	/* mutex resource  */
	spinlock_t			lock;
	/* work queue */
	struct work_struct		phyConf;
	/* timer */
	struct timer_list		linkCheckTimer;

/* Operation Resource*/
	u8 				hwSpeed;
	u8				hwDuplex;
	u8 				phyId;
	u8				reserve;
	u32				phyType;
	u32				msg_enable;
	u32				oldBmcr;	// 0.951

/* DMA resource - non-cached resource */
	struct device			*pDev;
	dma_addr_t			txDescDma;	// physical address
	dma_addr_t			rxDescDma;

	DMA_DESC_T			*pTxDesc;	// logical address
	DMA_DESC_T			*pRxDesc;

	DMA_DESC_T			*pTxDescBusy;
	DMA_DESC_T			*pRxDescBusy;
	DMA_DESC_T			*pTxDescNext;
	DMA_DESC_T			*pRxDescNext;

	struct	sk_buff			*pRxDummySkb;	// 0.940
//	struct	sk_buff			*currentRxSkb;
//	struct	sk_buff			*currentTxSkb;

//	u32				numTxDesc;	
//	u32				numRxDesc;	

	int				txBusy;
	int				rxBusy;

	int				txNext;
	int				rxNext;

/* Power Management resource */
	SDP_GMAC_POWER_T		power;	

/* register resource */
	SDP_GMAC_T		 	*pGmacBase;
	SDP_GMAC_MMC_T		 	*pMmcBase;
	SDP_GMAC_TIME_STAMP_T    	*pTimeStampBase;
	SDP_GMAC_MAC_2ND_BLOCK_T 	*pMac2ndBlk;
	SDP_GMAC_DMA_T 		 	*pDmaBase;

}SDP_GMAC_DEV_T; 

extern int 
sdpGmac_reset(struct net_device *pNetDev);
extern int 
sdpGmac_getMacAddr(struct net_device *pNetDev, u8 *pAddr);
extern void 
sdpGmac_setMacAddr(struct net_device *pNetDev, const u8 *pAddr );
extern int 
sdpGmac_mdioRead(struct net_device *pNetDev, int phyId, int loc);
extern void 
sdpGmac_mdioWrite(struct net_device *pNetDev , int phyId, int loc, int val);
extern void
sdpGmac_dmaInit(struct net_device *pNetDev);	
extern void
sdpGmac_gmacInit(struct net_device *pNetDev);	
extern int
sdpGmac_setRxQptr(SDP_GMAC_DEV_T* pGmacDev, const DMA_DESC_T* pDmaDesc, const u32 length2);
extern int
sdpGmac_getRxQptr(SDP_GMAC_DEV_T* pGmacDev, DMA_DESC_T *pDmaDesc);
extern int
sdpGmac_setTxQptr(SDP_GMAC_DEV_T* pGmacDev, 
		const DMA_DESC_T* pDmaDesc, const u32 length2, const int offloadNeeded);

extern int
sdpGmac_getTxQptr(SDP_GMAC_DEV_T* pGmacDev, DMA_DESC_T *pDmaDesc);

extern void 
sdpGmac_clearAllDesc(struct net_device *pNetDev);

#define SDP_GMAC_GET_PHY_BMCR \
		sdpGmac_mdioRead(pNetDev, phyId, MII_BMCR)
#define SDP_GMAC_SET_PHY_BMCR(val) \
		sdpGmac_mdioWrite(pNetDev, phyId, MII_BMCR, val)

#define SDP_GMAC_GET_PHY_BMSR \
		sdpGmac_mdioRead(pNetDev, phyId, MII_BMSR)

#define SDP_GMAC_GET_PHY_LPA \
		sdpGmac_mdioRead(pNetDev, phyId, MII_LPA)

#define SDP_GMAC_GET_PHY_ADV \
		sdpGmac_mdioRead(pNetDev, phyId, MII_ADVERTISE)
#define SDP_GMAC_SET_PHY_ADV(val) \
		sdpGmac_mdioWrite(pNetDev, phyId, MII_ADVERTISE, val)

#endif /*  __SDP_GMAC_H */
