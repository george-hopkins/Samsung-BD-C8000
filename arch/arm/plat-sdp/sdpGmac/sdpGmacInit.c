/***************************************************************************
 *
 *	arch/arm/plat-sdp/sdpGmac/sdpGmac_init.c
 *	Samsung Elecotronics.Co
 *	Created by tukho.kim	
 *
 * ************************************************************************/
/*
 * 2009.08.02,tukho.kim: Created by tukho.kim@samsung.com
 * 2009.08.31,tukho.kim: Revision 1.00
 * 2009.09.23,tukho.kim: re-define drv version 0.90
 * 2009.09.27,tukho.kim: debug dma_free_coherent and modify probe, initDesc 0.91
 * 2009.09.27,tukho.kim: add power management function drv version 0.92
 * 2009.09.28,tukho.kim: add retry count, when read mac address by i2c bus  0.921
 * 2009.10.06,tukho.kim: debug when rx sk buffer allocation is failed 0.93 
 * 2009.10.19,tukho.kim: rx buffer size is fixed, ETH_DATA_LEN 1500 0.931
 * 2009.10.22,tukho.kim: debug when rx buffer alloc is failed 0.940
 * 2009.10.25,tukho.kim: recevice packet is error, not re-alloc buffer 0.941
 * 2009.10.27,tukho.kim: mac configuration initial value is "set watchdog disable 0.942
 * 2009.11.05,tukho.kim: debug to check tx descriptor in hardStart_xmit 0.943
 * 2009.11.18,tukho.kim: rtl8201E RXD clock toggle active, rtl8201E hidden register 0.9431
 * 2009.11.21,tukho.kim: modify netDev_rx routine 0.95
 * 2009.12.01,tukho.kim: debug phy check media, 0.951
 * 			 full <-> half ok, 10 -> 100 ok, 100 -> 10 need to unplug and plug cable
 * 2009.12.02,tukho.kim: debug module init, exit  0.952
 * 2009.12.15,tukho.kim: hwSpeed init value change to STATUS_SPEED_100 0.953
 * 2009.12.15,tukho.kim: when alloc rx buffer,  align rx->data pointer by 16 0.954
 * 2009.12.28,tukho.kim: remove to check magic code when read mac address by i2c 0.9541
 * 2010.01.13,tukho.kim: debug sdpGmac_rxDescRecovery function 0.955
 * 2010.01.23,tukho.kim: debug wait code about DMA reset in sdpGmac_reset  0.956
 */ 

//#include "sdpGmac_env.h"
#include "sdpGMAC.h"
#include<linux/syscalls.h>
#include<linux/fcntl.h>
#include<asm/uaccess.h>

#define GMAC_DRV_VERSION	"0.956"	

// 0.954
static inline void 
sdpGmac_skbAlign(struct sk_buff *pSkb) {	
	u32 align = (u32)pSkb->data & 3;

	switch(align){
		case 1:
		case 3:
			skb_reserve(pSkb, align);
			break;
		case 2:
			break;
		default:
			skb_reserve(pSkb, 2);
	}
} // 0.954 end

static void sdpGmac_netDev_tx(struct net_device *pNetDev)
{
	SDP_GMAC_DEV_T	*pGmacDev = netdev_priv(pNetDev);

	struct net_device_stats *pNetStats = &pGmacDev->netStats;
	DMA_DESC_T	txDesc;

	u32 checkStatus;
	int descIndex;
	
	DPRINTK_GMAC_FLOW("called\n");

#if 1
	do{
		descIndex = sdpGmac_getTxQptr(pGmacDev, &txDesc);

		if(descIndex < 0) break;	// break condition
		if(!txDesc.data1) continue;	// not receive
	
		DPRINTK_GMAC_FLOW("Tx Desc %d for skb 0x%08x whose status is %08x\n",
					descIndex, txDesc.data1, txDesc.status);

		//TODO : IPC_OFFLOAD  ??? H/W Checksum	
		//
		//
		//
		
		dma_unmap_single(pGmacDev->pDev, txDesc.buffer1, 
				txDesc.length, DMA_FROM_DEVICE);
	
		dev_kfree_skb_irq((struct sk_buff*)txDesc.data1);		

		// ENH_MODE
		checkStatus = (txDesc.status & 0xFFFF) & DESC_ERROR;

		if(!checkStatus) {
			pNetStats->tx_bytes += txDesc.length & DESC_SIZE_MASK;
			pNetStats->tx_packets++;
		}
		else {	// ERROR
			pNetStats->tx_errors++;

			if(txDesc.status & (DESC_TX_LATE_COLL | DESC_TX_EXC_COLL)) 
				pNetStats->tx_aborted_errors++ ;
			if(txDesc.status & (DESC_TX_LOST_CARR | DESC_TX_NO_CARR)) 
				pNetStats->tx_carrier_errors++;
		}

		pNetStats->collisions += DESC_TX_COLLISION(txDesc.status);

	}while(1);

	netif_wake_queue(pNetDev);

#endif
	DPRINTK_GMAC_FLOW("exit\n");
	return;
}



static int sdpGmac_netDev_rx(struct net_device *pNetDev)
{
	SDP_GMAC_DEV_T	*pGmacDev = netdev_priv(pNetDev);

	struct net_device_stats *pNetStats = &pGmacDev->netStats;
	struct sk_buff * pSkb;
	DMA_DESC_T	rxDesc;

	u32 checkStatus;
	u32 len;
	int descIndex;
	dma_addr_t rxDmaAddr;
	int nrChkDesc = 0;	// packet 

	DPRINTK_GMAC_FLOW("called\n");

	do{
		descIndex = sdpGmac_getRxQptr(pGmacDev, &rxDesc);

		if(descIndex < 0) {
			break;	// break condition
		}

		nrChkDesc++;
//		if(rxDesc.data1 == (u32)NULL) {	// 0.93, remove warning msg
		if(rxDesc.data1 == (u32)pGmacDev->pRxDummySkb) {	// 0.940
			DPRINTK_GMAC_ERROR("rx skb is none\n");
//			continue;	// not receive
			goto __alloc_rx_skb;	// 0.93
		}
	

		DPRINTK_GMAC("Rx Desc %d for skb 0x%08x whose status is %08x\n",
				descIndex, rxDesc.data1, rxDesc.status);

		dma_unmap_single(pGmacDev->pDev, rxDesc.buffer1, 0, DMA_FROM_DEVICE);
	
		pSkb = (struct sk_buff*)rxDesc.data1;		
// 0.95
		checkStatus = rxDesc.status & (DESC_ERROR | DESC_RX_LEN_ERR);

// rx byte
		if(!checkStatus &&
		   (rxDesc.status & DESC_RX_LAST) && 
		   (rxDesc.status & DESC_RX_1ST) ) 
		{
			len = DESC_GET_FRAME_LEN(rxDesc.status) - 4; // without crc byte

			skb_put(pSkb,len);

			//TODO : IPC_OFFLOAD  ??? H/W Checksum	
				
			// Network buffer post process
			pSkb->dev = pNetDev;
			pSkb->protocol = eth_type_trans(pSkb, pNetDev);
			netif_rx(pSkb);		// post buffer to the network code 

			pNetDev->last_rx = jiffies;
			pNetStats->rx_packets++;
			pNetStats->rx_bytes += len;
			
		}
		else {	// ERROR
			pNetStats->rx_errors++;

			if(rxDesc.status & DESC_RX_OVERFLOW) pNetStats->rx_over_errors++;	// 0.95
			if(rxDesc.status & DESC_RX_COLLISION) pNetStats->collisions++;
			if(rxDesc.status & DESC_RX_WATCHDOG) pNetStats->rx_frame_errors++;	// 0.95
			if(rxDesc.status & DESC_RX_CRC) pNetStats->rx_crc_errors++ ;
//			if(rxDesc.status & DESC_RX_DRIBLING) pNetStats->rx_frame_errors++ ;	// 0.95
			if(rxDesc.status & DESC_RX_LEN_ERR) pNetStats->rx_length_errors++;	// 0.95

			memset(pSkb->data, 0, ETH_DATA_LEN + ETHER_PACKET_EXTRA);	// buffer init

			goto __set_rx_qptr;	// 0.941
		}
//0.95 end

__alloc_rx_skb:
//		pSkb = dev_alloc_skb(pNetDev->mtu + ETHER_PACKET_EXTRA);
		pSkb = dev_alloc_skb(ETH_DATA_LEN + ETHER_PACKET_EXTRA + 4);	// 0.931 // 0.954

		if (pSkb == NULL){
			DPRINTK_GMAC_ERROR("could not allocate skb memory size %d, loop %d\n", 
						(ETH_DATA_LEN + ETHER_PACKET_EXTRA), nrChkDesc);	// 0.931

			pNetStats->rx_dropped++;
			// TODO : check this status
			//return nrChkDesc;
			pSkb = pGmacDev->pRxDummySkb;	// 0.940
		}
		else {
			sdpGmac_skbAlign(pSkb); 	// 0.954
		}


__set_rx_qptr:		// 0.941
		rxDmaAddr = dma_map_single(pGmacDev->pDev, 
					   pSkb->data, 
					   skb_tailroom(pSkb),
					   DMA_FROM_DEVICE);

		CONVERT_DMA_QPTR(rxDesc, skb_tailroom(pSkb), rxDmaAddr, pSkb, 0, 0);

		if(sdpGmac_setRxQptr(pGmacDev, &rxDesc, 0) < 0){
			if(pSkb != pGmacDev->pRxDummySkb) {	// 0.940
				dev_kfree_skb_irq(pSkb);	
			}
			DPRINTK_GMAC_ERROR("Error set rx qptr\n");
			break;
		}
	}while(1);

	DPRINTK_GMAC_FLOW("exit\n");
	return nrChkDesc;
}

static inline void 
sdpGmac_rxDescRecovery(struct net_device *pNetDev)
{
	SDP_GMAC_DEV_T *pGmacDev = netdev_priv(pNetDev);
	SDP_GMAC_T *pGmacReg = pGmacDev->pGmacBase;	
	SDP_GMAC_DMA_T *pDmaReg = pGmacDev->pDmaBase;	
	DMA_DESC_T *pRxDesc = pGmacDev->pRxDesc;
	
	int idx = 0;

	pDmaReg->operationMode &= ~B_RX_EN;
	pGmacReg->configuration &= ~B_RX_ENABLE;	
	
	for (idx = 0; idx < NUM_RX_DESC; idx++) {
		if(!(pRxDesc->status & DESC_OWN_BY_DMA)) {
			pGmacDev->rxNext = idx;
			pGmacDev->pRxDescNext = pRxDesc;
			pGmacDev->rxBusy = idx;
			pGmacDev->pRxDescBusy = pRxDesc;

			sdpGmac_netDev_rx(pNetDev);  // check buffer 
			
			break;
		}
		pRxDesc++;
	}

	idx = (pDmaReg->curHostRxDesc - pDmaReg->rxDescListAddr) / sizeof(DMA_DESC_T);

	DPRINTK_GMAC_FLOW("Recovery rx desc\n");
	DPRINTK_GMAC_FLOW("current rx desc %d \n", idx);

	pRxDesc = pGmacDev->pRxDesc;		// 0.955
	pGmacDev->rxNext = idx;
	pGmacDev->pRxDescNext = pRxDesc + idx;
	pGmacDev->rxBusy = idx;
	pGmacDev->pRxDescBusy = pRxDesc + idx;

	pGmacReg->configuration |= B_RX_ENABLE;	
	pDmaReg->operationMode |= B_RX_EN;

	pGmacDev->netStats.rx_dropped++;  	// rx packet is dropped 1 packet 
}

static irqreturn_t sdpGmac_intrHandler(int irq, void * devId)
{
	struct net_device *pNetDev = (struct net_device*)devId;
	SDP_GMAC_DEV_T *pGmacDev = netdev_priv(pNetDev);
//	SDP_GMAC_T *pGmacReg = pGmacDev->pGmacBase;	
	SDP_GMAC_MMC_T *pMmcReg = pGmacDev->pMmcBase;	
	SDP_GMAC_DMA_T *pDmaReg = pGmacDev->pDmaBase;	

	u32 intrStatus;
	int nrChkDesc = 0;

	DPRINTK_GMAC_FLOW("called\n");

	if(unlikely(pNetDev == NULL)) {
		DPRINTK_GMAC_ERROR("Not register Network Device, please check System\n");
		return -SDP_GMAC_ERR;
	}

	if(unlikely(pGmacDev == NULL)) {
		DPRINTK_GMAC_ERROR("Not register SDP-GMAC, please check System\n");
		return -SDP_GMAC_ERR;
	}

	intrStatus = pDmaReg->status;

	if(intrStatus == 0) return IRQ_NONE;


	// DMA Self Clear bit is check
	
	if(intrStatus & B_TIME_STAMP_TRIGGER) {
		DPRINTK_GMAC("INTR: time stamp trigger\n");
	}

	if(intrStatus & B_PMT_INTR) {
		// TODO: make pmt resume function 
		DPRINTK_GMAC("INTR: PMT interrupt\n");
	}

	if(intrStatus & B_MMC_INTR) {
		// TODO: make reading mmc intr rx and tx register 
		// this register is hidden in Datasheet
		DPRINTK_GMAC_ERROR("INTR: MMC rx: 0x%08x\n", pMmcReg->intrRx);
		DPRINTK_GMAC_ERROR("INTR: MMC tx: 0x%08x\n", pMmcReg->intrTx);
		DPRINTK_GMAC_ERROR("INTR: MMC ipc offload: 0x%08x\n", pMmcReg->mmcIpcIntrRx);
	}

	if(intrStatus & B_LINE_INTF_INTR) {
		DPRINTK_GMAC("INTR: line interface interrupt\n");
	}


// Normal interrupt
	if(intrStatus & B_RX_INTR) {
		nrChkDesc = sdpGmac_netDev_rx(pNetDev);	
	}

// sample source don't support Early Rx, Tx
	if(intrStatus & B_EARLY_RX) {
	
	}

	if(intrStatus & B_TX_BUF_UNAVAIL) {
	
	}

	if(intrStatus & B_TX_INTR) {
		sdpGmac_netDev_tx(pNetDev);	
	}


// ABNORMAL Interrupt
	if(intrStatus & B_ABNORM_INTR_SUM) {

		if(intrStatus & B_FATAL_BUS_ERROR) {
			DPRINTK_GMAC_ERROR("Fatal Bus Error: \n");

			if(intrStatus & B_ERR_DESC) 
				DPRINTK_GMAC_ERROR("\tdesc access error\n");
			else
				DPRINTK_GMAC_ERROR("\tdata buffer access error\n");

			if(intrStatus & B_ERR_READ_TR) 
				DPRINTK_GMAC_ERROR("\tread access error\n");
			else
				DPRINTK_GMAC_ERROR("\twrite access error\n");

			if(intrStatus & B_ERR_TXDMA) 
				DPRINTK_GMAC_ERROR("\ttx dma error\n");
			else
				DPRINTK_GMAC_ERROR("\trx dmaerror\n");

			//TODO : DMA re-initialize 

		}

		if(intrStatus & B_EARLY_TX) {
			DPRINTK_GMAC_ERROR("Early Tx Error\n");
			// TODO : 
		}

		if(intrStatus & B_RX_WDOG_TIMEOUT) {
			DPRINTK_GMAC_ERROR("Rx WatchDog timeout Error\n");
			// TODO : 
		}
	
		if(intrStatus & B_RX_STOP_PROC) {
			DPRINTK_GMAC_FLOW("Rx process stop\n");
			// TODO : 
		}


		if(intrStatus & B_RX_BUF_UNAVAIL) {
			DPRINTK_GMAC_FLOW("Rx buffer unavailable Error %d\n", nrChkDesc);
			sdpGmac_rxDescRecovery(pNetDev);	
		} 
		
		if(intrStatus & B_RX_OVERFLOW) {
			DPRINTK_GMAC_ERROR("Rx overflow Error\n");
			// TODO : 
		}

		if(intrStatus & B_TX_UNDERFLOW) {
			DPRINTK_GMAC_ERROR("Tx underflow Error\n");
			// TODO : tx descriptor fix
		}



		if(intrStatus & B_TX_JAB_TIMEOUT) {
			DPRINTK_GMAC_ERROR("Tx jabber timeout Error\n");
			// TODO :
		}


		if(intrStatus & B_TX_STOP_PROC) {
			DPRINTK_GMAC_FLOW("Tx process stop\n");
			// TODO : 
		}

	}

	pDmaReg->status = intrStatus;	// Clear interrupt pending register 

	DPRINTK_GMAC_FLOW("exit\n");

	return IRQ_HANDLED;
}

static void sdpGmac_phyCheckMedia(struct net_device *pNetDev, int init)
{
        SDP_GMAC_DEV_T 	*pGmacDev = netdev_priv(pNetDev);
	SDP_GMAC_T 	*pGmacReg = pGmacDev->pGmacBase;
	SDP_GMAC_DMA_T 	*pDmaReg = pGmacDev->pDmaBase;

        int 		idx, phyId = pGmacDev->phyId;
        unsigned int 	phyVal;
	u32		regVal;
	unsigned long 	flags = 0;

	DPRINTK_GMAC_FLOW("called\n");

// 0.951 
	if(!mii_check_media(&pGmacDev->mii, netif_msg_link(pGmacDev), init) && 
	   !mii_link_ok(&pGmacDev->mii)) return;	// check link and duplex 

	spin_lock_irqsave(&pGmacDev->lock, flags);		// 0.951
		phyVal = SDP_GMAC_GET_PHY_BMCR;
	spin_unlock_irqrestore(&pGmacDev->lock, flags);		// 0.951

	if(phyVal & BMCR_ANRESTART) return;	

	phyVal &= (BMCR_FULLDPLX | BMCR_SPEED100); 

	if(phyVal != pGmacDev->oldBmcr) {
		DPRINTK_GMAC_FLOW("phy 0x%04x, old 0x%04x\n", phyVal, pGmacDev->oldBmcr);
		pGmacDev->oldBmcr = phyVal;	
	}
	else return;

	/* duplex state has changed */
	spin_lock_irqsave(&pGmacDev->lock, flags);		// 0.951
		regVal = pGmacReg->configuration; 

	pDmaReg->operationMode &= ~(B_TX_EN | B_RX_EN) ;
	pGmacReg->configuration &= ~(B_TX_ENABLE | B_RX_ENABLE);	
	
	if (phyVal & BMCR_FULLDPLX) {		// 0.951
                        regVal |= B_DUPLEX_FULL;
			pGmacDev->hwDuplex = STATUS_DUPLEX_FULL;
                } else { // HALF Duplex
                        regVal &= ~B_DUPLEX_FULL;
			pGmacDev->hwDuplex = STATUS_DUPLEX_HALF;
                }

		// TODO : check 10MBps , 100MBps phy status 
	if (phyVal & BMCR_SPEED100) {
                        regVal |= B_SPEED_100M;
		pGmacDev->hwSpeed = STATUS_SPEED_100;
                } else {
                        regVal &= ~B_SPEED_100M;
		pGmacDev->hwSpeed = STATUS_SPEED_10;
	}
	// desc pointer set by DMA desc pointer
	// RX Desc
	idx = (pDmaReg->curHostRxDesc - pDmaReg->rxDescListAddr) / sizeof(DMA_DESC_T);
	if(idx < NUM_RX_DESC) {
		pGmacDev->rxNext = idx;
		pGmacDev->pRxDescNext = pGmacDev->pRxDesc + idx;
		pGmacDev->rxBusy = idx;
		pGmacDev->pRxDescBusy = pGmacDev->pRxDesc + idx;
	}
	// TX Desc
	idx = (pDmaReg->curHostTxDesc - pDmaReg->txDescListAddr) / sizeof(DMA_DESC_T);
	if(idx < NUM_TX_DESC) {
		pGmacDev->txNext = idx;
		pGmacDev->pTxDescNext = pGmacDev->pTxDesc + idx;
		pGmacDev->txBusy = idx;
		pGmacDev->pTxDescBusy = pGmacDev->pTxDesc + idx;
                }

		pGmacReg->configuration = regVal; 
	pDmaReg->operationMode |= B_TX_EN | B_RX_EN ;
	
	spin_unlock_irqrestore(&pGmacDev->lock, flags);		// 0.951
	
	PRINTK_GMAC( "%s: Set %dMBps, %s-duplex \n", 
			pNetDev->name, 
			((phyVal & BMCR_SPEED100)? 100 : 10), 
			((phyVal & BMCR_FULLDPLX) ? "full" : "half") ); 
// 0.951 end

	DPRINTK_GMAC_FLOW("exit\n");
}

static void sdpGmac_linkCheckTimer(unsigned long arg)
{
	struct net_device *pNetDev = (struct net_device*)arg;
	SDP_GMAC_DEV_T *pGmacDev = netdev_priv(pNetDev);
	
	sdpGmac_phyCheckMedia(pNetDev, 0);

	pGmacDev->linkCheckTimer.expires = CFG_LINK_CHECK_TIME + jiffies;
	add_timer(&pGmacDev->linkCheckTimer);
// for test 
	if(!is_valid_ether_addr(pNetDev->dev_addr)) {
		DPRINTK_GMAC_ERROR("%s debug: no valid ethernet hw addr\n",pNetDev->name);
		return;
	}
}


static void sdpGmac_phyConf(struct work_struct *pWork)
{
	SDP_GMAC_DEV_T *pGmacDev = 
		container_of(pWork, SDP_GMAC_DEV_T, phyConf);
	struct net_device* pNetDev = pGmacDev->pNetDev;
	unsigned long flags;
	u8 phyId = pGmacDev->phyId;
	int phyCaps; 	// phy Capability
	int adCaps;	// ad Caps

	DPRINTK_GMAC_FLOW("called\n");

	spin_lock_irqsave(&pGmacDev->lock, flags);	

	if(pGmacDev->mii.force_media){
		// TODO	
		PRINTK_GMAC("%s: Not Implemeted yet\n", pNetDev->name);
		goto __phyConf_out;
	}

	phyCaps = SDP_GMAC_GET_PHY_BMSR;

        if (!(phyCaps & BMSR_ANEGCAPABLE)) {
                PRINTK_GMAC("%s: Auto negotiation NOT supported\n", pNetDev->name);
		PRINTK_GMAC("%s: Not Implemeted yet\n",pNetDev->name);
		goto __phyConf_out;
        }

       /* CSMA capable w/ both pauses */
       adCaps = ADVERTISE_CSMA | ADVERTISE_PAUSE_CAP | ADVERTISE_PAUSE_ASYM;

        if (phyCaps & BMSR_100BASE4)
                adCaps |= ADVERTISE_100BASE4;
        if (phyCaps & BMSR_100FULL)
                adCaps |= ADVERTISE_100FULL;
        if (phyCaps & BMSR_100HALF)
                adCaps |= ADVERTISE_100HALF;
        if (phyCaps & BMSR_10FULL)
                adCaps |= ADVERTISE_10FULL;
        if (phyCaps & BMSR_10HALF)
                adCaps |= ADVERTISE_10HALF;

        /* Disable capabilities not selected by our user */
        if (pGmacDev->hwSpeed == STATUS_SPEED_10) {		// 0.953
		PRINTK_GMAC("hwSpeed is %d\n", pGmacDev->hwSpeed);
                adCaps &= ~(ADVERTISE_100BASE4|ADVERTISE_100FULL|ADVERTISE_100HALF);
	}

         if (!pGmacDev->hwDuplex)
                adCaps &= ~(ADVERTISE_100FULL|ADVERTISE_10FULL);

        /* Update our Auto-Neg Advertisement Register */
        SDP_GMAC_SET_PHY_ADV(adCaps);
        pGmacDev->mii.advertising = adCaps;

	udelay(10);

	DPRINTK_GMAC_FLOW("%s: Set value ADV %04x\n", pNetDev->name, adCaps);
	adCaps = SDP_GMAC_GET_PHY_ADV;
	DPRINTK_GMAC_FLOW("%s: Get value ADV %04x\n", pNetDev->name, adCaps);

// TODO: if do BMCR_ANRESTART, can't active RTL 8201E 
//	SDP_GMAC_SET_PHY_BMCR(BMCR_ANENABLE | BMCR_ANRESTART); 
	SDP_GMAC_SET_PHY_BMCR(BMCR_ANENABLE);

	sdpGmac_phyCheckMedia(pNetDev, 1);

	spin_unlock_irqrestore(&pGmacDev->lock, flags);	

__phyConf_out:
	DPRINTK_GMAC_FLOW("exit\n");
	return;
}


static struct net_device_stats *
sdpGmac_netDev_getStats(struct net_device *pNetDev)
{
	struct net_device_stats *pRet;
	SDP_GMAC_DEV_T * pGmacDev = netdev_priv(pNetDev);

	DPRINTK_GMAC_FLOW("called\n");

	pRet = &pGmacDev->netStats;

	DPRINTK_GMAC_FLOW("exit\n");
	return pRet;
}


static int 
sdpGmac_netDev_hardStartXmit(struct sk_buff *pSkb, struct net_device *pNetDev)
{
	int retVal = SDP_GMAC_OK;

	SDP_GMAC_DEV_T *pGmacDev = netdev_priv(pNetDev);
	SDP_GMAC_DMA_T *pDmaReg = pGmacDev->pDmaBase;

	int offloadNeeded = 0;
	dma_addr_t txDmaAddr;
	DMA_DESC_T txDesc;
	
	int retry = 1;

	DPRINTK_GMAC_FLOW ("called\n");

	if(unlikely(pSkb == NULL)) {
		DPRINTK_GMAC_ERROR("xmit skb is NULL!!!\n");
		retVal = -SDP_GMAC_ERR;
		goto __xmit_out;
	}
	
	netif_stop_queue(pNetDev);

	if(pSkb->ip_summed == CHECKSUM_PARTIAL){
		// TODO: 
		DPRINTK_GMAC_ERROR("H/W Checksum?? Not Implemente yet\n");	
		offloadNeeded = 1;
	}

	txDmaAddr = dma_map_single(pGmacDev->pDev, pSkb->data, 
					 pSkb->len, DMA_TO_DEVICE);

			//        length,  buffer1, data1, buffer2, data2
	CONVERT_DMA_QPTR(txDesc, pSkb->len, txDmaAddr, pSkb, 0, 0 );

	while(sdpGmac_setTxQptr(pGmacDev, &txDesc, 0, offloadNeeded) < 0) {
		if(retry--){
			udelay(150);		// 1 packet 1515 byte -> 121.20 us 
			continue;
		}

		DPRINTK_GMAC_ERROR("No more Free Tx Descriptor\n");		
		pGmacDev->netStats.tx_errors++;
		pGmacDev->netStats.tx_dropped++;
		dev_kfree_skb(pSkb);		
		break;		// 0.943
	}

	/* start tx */
	pDmaReg->txPollDemand = 0;
	pNetDev->trans_start = jiffies;

__xmit_out:
	netif_wake_queue(pNetDev);

	DPRINTK_GMAC_FLOW ("exit\n");
	return retVal;
}
static int 
sdpGmac_netDev_open (struct net_device *pNetDev)
{
	int retVal = SDP_GMAC_OK;
	SDP_GMAC_DEV_T *pGmacDev = netdev_priv(pNetDev);		
	SDP_GMAC_T *pGmacReg = pGmacDev->pGmacBase;		
	SDP_GMAC_MMC_T *pMmcReg = pGmacDev->pMmcBase;		
	SDP_GMAC_DMA_T *pDmaReg = pGmacDev->pDmaBase;		
	
	struct sk_buff *pSkb;
	dma_addr_t  	rxDmaAddr;
	DMA_DESC_T	rxInitDesc;
	int	idx;

	DPRINTK_GMAC_FLOW("%s: open called\n", pNetDev->name);

	if(!is_valid_ether_addr(pNetDev->dev_addr)) {
		DPRINTK_GMAC_ERROR("[SDP GMAC]%s: no valid ethernet hw addr\n",pNetDev->name);
		return -EINVAL;
	}

	// desc pointer set by DMA desc pointer
	// RX Desc
	idx = (pDmaReg->curHostRxDesc - pDmaReg->rxDescListAddr) / sizeof(DMA_DESC_T);
	if(idx < NUM_RX_DESC) {
		pGmacDev->rxNext = idx;
		pGmacDev->pRxDescNext = pGmacDev->pRxDesc + idx;
		pGmacDev->rxBusy = idx;
		pGmacDev->pRxDescBusy = pGmacDev->pRxDesc + idx;
		DPRINTK_GMAC_FLOW("Rx Desc set %d\n", idx);
	}
	// TX Desc
	idx = (pDmaReg->curHostTxDesc - pDmaReg->txDescListAddr) / sizeof(DMA_DESC_T);
	if(idx < NUM_TX_DESC) {
		pGmacDev->txNext = idx;
		pGmacDev->pTxDescNext = pGmacDev->pTxDesc + idx;
		pGmacDev->txBusy = idx;
		pGmacDev->pTxDescBusy = pGmacDev->pTxDesc + idx;
		DPRINTK_GMAC_FLOW("Tx Desc set %d\n", idx);
	}

	//skb buffer initialize ????	check
	do{
//		pSkb = dev_alloc_skb(pNetDev->mtu + ETHER_HEADER + ETHER_CRC);
		pSkb = dev_alloc_skb(ETH_DATA_LEN + ETHER_HEADER + ETHER_CRC + 4);	// 0.931 // 0.954

		if(pSkb == NULL){
                        DPRINTK_GMAC_ERROR("could not allocate sk buffer \n");
                        break;
		}

		sdpGmac_skbAlign(pSkb); 	// 0.954

		rxDmaAddr = dma_map_single(pGmacDev->pDev, pSkb->data, 
					 skb_tailroom(pSkb), DMA_FROM_DEVICE);
		
		CONVERT_DMA_QPTR(rxInitDesc, skb_tailroom(pSkb), rxDmaAddr, pSkb, 0, 0);

		if(sdpGmac_setRxQptr(pGmacDev, &rxInitDesc, 0) < 0){
			dev_kfree_skb(pSkb);	
			break;
		}
	}while(1);

// check line status
	// call sdpGmac_phyConf	
	// speed, Duplex mode set
	sdpGmac_phyConf(&pGmacDev->phyConf);

	// Init link status timer 
	init_timer(&pGmacDev->linkCheckTimer);
	pGmacDev->linkCheckTimer.function = sdpGmac_linkCheckTimer;	
	pGmacDev->linkCheckTimer.data = (u32) pNetDev;	
	pGmacDev->linkCheckTimer.expires = (unsigned long)(CFG_LINK_CHECK_TIME + jiffies);	
	add_timer(&pGmacDev->linkCheckTimer);

//	pGmacReg->configuration |= B_JUMBO_FRAME_EN ;
	pGmacReg->configuration |= B_WATCHDOG_DISABLE ; // 0.942

	//  MMC interrupt disable all
	//  TODO: check this block what use 
	pMmcReg->intrMaskRx = 0x00FFFFFF;  // disable
	pMmcReg->intrMaskTx = 0x01FFFFFF;  // disable 
	pMmcReg->mmcIpcIntrMaskRx = 0x3FFF3FFF;  // disable 

	//  interrupt enable all
	pDmaReg->intrEnable = B_INTR_ENABLE_ALL;

	// tx, rx enable
	pGmacReg->configuration |= B_TX_ENABLE | B_RX_ENABLE;	
	pDmaReg->operationMode |= B_TX_EN | B_RX_EN ;

	netif_start_queue(pNetDev);	

	DPRINTK_GMAC_FLOW("%s: exit\n", pNetDev->name);

	return retVal;
}


static int 
sdpGmac_netDev_close (struct net_device *pNetDev)
{
	int retVal = SDP_GMAC_OK;
	SDP_GMAC_DEV_T *pGmacDev = netdev_priv(pNetDev);
	SDP_GMAC_T *pGmacReg = pGmacDev->pGmacBase;
	SDP_GMAC_DMA_T *pDmaReg = pGmacDev->pDmaBase;

	DPRINTK_GMAC_FLOW("%s: close called\n", pNetDev->name);

	netif_stop_queue(pNetDev);
	netif_carrier_off(pNetDev);


	// rx, tx disable
	pDmaReg->operationMode &= ~(B_TX_EN | B_RX_EN) ;
	pGmacReg->configuration &= ~(B_TX_ENABLE | B_RX_ENABLE);	

	// all interrupt disable
	pDmaReg->intrEnable = 0;	
	pDmaReg->status = pDmaReg->status;	// Clear interrupt pending register 

	// phy control

	// skb control
	sdpGmac_clearAllDesc(pNetDev);	

	// timer control
	del_timer(&pGmacDev->linkCheckTimer);

	DPRINTK_GMAC_FLOW("%s: close exit\n", pNetDev->name);
	return retVal;
}

static void
sdpGmac_netDev_setMulticastList (struct net_device *pNetDev)
{
	SDP_GMAC_DEV_T *pGmacDev = netdev_priv(pNetDev);
	SDP_GMAC_T *pGmacReg = pGmacDev->pGmacBase;

	u32 frameFilter;
	unsigned long flag;

	DPRINTK_GMAC_FLOW("called\n");

	spin_lock_irqsave(&pGmacDev->lock, flag);
	frameFilter = pGmacReg->frameFilter;
	spin_unlock_irqrestore(&pGmacDev->lock, flag);

	frameFilter &= ~( B_PROMISCUOUS_EN | 
			  B_PASS_ALL_MULTICAST | 
			  B_HASH_PERFECT_FILTER | 
			  B_HASH_MULTICAST );

	if (pNetDev->flags & IFF_PROMISC) {
		PRINTK_GMAC ("%s: PROMISCUOUS MODE\n", pNetDev->name);
		frameFilter |= B_PROMISCUOUS_EN;
	}

	else if(pNetDev->flags & IFF_ALLMULTI || (pNetDev->mc_count > 15)) {
		DPRINTK_GMAC_FLOW("%s: PASS ALL MULTICAST \n", pNetDev->name);
		frameFilter |= B_PASS_ALL_MULTICAST;
	}

	else if(pNetDev->mc_count){	

		int i;
		struct dev_mc_list *cur_addr;
		volatile u32 *mcRegHigh = &pGmacReg->macAddr_01_High;
		volatile u32 *mcRegLow = &pGmacReg->macAddr_01_Low;
		u32 *mcValHigh;
		u32 *mcValLow;

		DPRINTK_GMAC_FLOW ("%s: HASH MULTICAST %d \n", 
					pNetDev->name, pNetDev->mc_count);

		cur_addr = pNetDev->mc_list;

		// clear mc list
		i = 15;
		while (i--) {
			if(*mcRegHigh == 0) break;

			spin_lock_irqsave(&pGmacDev->lock, flag);
			*mcRegHigh = 0;
			*mcRegLow = 0;
			spin_unlock_irqrestore(&pGmacDev->lock, flag);

			mcRegLow += 2;
			mcRegHigh += 2;
		}

		// set 
		mcRegHigh = &pGmacReg->macAddr_01_High;
		mcRegLow = &pGmacReg->macAddr_01_Low;

		for(i = 0; i < pNetDev->mc_count; i++, cur_addr = cur_addr->next) {

			if(!cur_addr) break;

			DPRINTK_GMAC_FLOW ("%s: cur_addr len is %d \n", pNetDev->name, cur_addr->dmi_addrlen);
			DPRINTK_GMAC_FLOW ("%s: cur_addr is %d.%d.%d.%d.%d.%d \n", pNetDev->name, 
						cur_addr->dmi_addr[0], cur_addr->dmi_addr[1],
						cur_addr->dmi_addr[2], cur_addr->dmi_addr[3],
						cur_addr->dmi_addr[4], cur_addr->dmi_addr[5] );

			if(!(*cur_addr->dmi_addr & 1)) continue;

			mcValLow = (u32*)(cur_addr->dmi_addr);
			mcValHigh = mcValLow + 1;	// u32 pointer 
			
			spin_lock_irqsave(&pGmacDev->lock, flag);
			*mcRegLow = *mcValLow;
			*mcRegHigh = 0x80000000 | (*mcValHigh & 0xFFFF);
			spin_unlock_irqrestore(&pGmacDev->lock, flag);

			mcRegLow += 2;
			mcRegHigh += 2;
		}	

	}
	else {	// clear
		int i;	
		volatile u32 *mcRegHigh = &pGmacReg->macAddr_01_High;
		volatile u32 *mcRegLow = &pGmacReg->macAddr_01_Low;

		// clear mc list
		i = 15;
		while (i--) {
			if(*mcRegHigh == 0) break;

			spin_lock_irqsave(&pGmacDev->lock, flag);
			*mcRegHigh = 0;
			*mcRegLow = 0;
			spin_unlock_irqrestore(&pGmacDev->lock, flag);

			mcRegLow += 2;
			mcRegHigh += 2;
		}

	}


	spin_lock_irqsave(&pGmacDev->lock, flag);
	pGmacReg->frameFilter = frameFilter;
	spin_unlock_irqrestore(&pGmacDev->lock, flag);

	DPRINTK_GMAC_FLOW("exit\n");
	return;
}



/* OK */
static int 
sdpGmac_netDev_setMacAddr (struct net_device *pNetDev, void *pEthAddr)
{
	int retVal = SDP_GMAC_OK;

	u8 *pAddr = (u8*)pEthAddr;

	DPRINTK_GMAC_FLOW("called\n");

	pAddr += 2;	// Etheret Mac is 6

	sdpGmac_setMacAddr(pNetDev, (const u8*)pAddr);
	sdpGmac_getMacAddr(pNetDev, (u8*)pAddr);

	PRINTK_GMAC ("%s: Ethernet address %02x:%02x:%02x:%02x:%02x:%02x\n",
		     pNetDev->name, *pAddr, *(pAddr+1), *(pAddr+2), 
		     		*(pAddr+3), *(pAddr+4), *(pAddr+5));	

	memcpy(pNetDev->dev_addr, pAddr, N_MAC_ADDR);

	DPRINTK_GMAC_FLOW("exit\n");
	return retVal;
}

static int
sdpGmac_netDev_ioctl (struct net_device *pNetDev, struct ifreq *pRq, int cmd)
{
	int retVal = SDP_GMAC_OK;
	SDP_GMAC_DEV_T *pGmacDev = netdev_priv(pNetDev);

	unsigned long flags;

	DPRINTK_GMAC("called\n");

	if(!netif_running(pNetDev))
		return -EINVAL;

	spin_lock_irqsave(&pGmacDev->lock, flags);

	retVal = generic_mii_ioctl(&pGmacDev->mii, if_mii(pRq), cmd, NULL);

	spin_unlock_irqrestore(&pGmacDev->lock, flags);

	DPRINTK_GMAC("exit\n");
	
	return retVal;
}

// TODO: 
#if 0
static int
sdpGmac_netDev_chgMtu(struct net_device *pNetDev, int newMtu)
{
	int retVal = SDP_GMAC_OK;

	DPRINTK_GMAC("called\n");

//  TODO

	DPRINTK_GMAC("exit\n");
	return retVal;
}


static void
sdpGmac_netDev_txTimeOut (struct net_device *pNetDev)
{
	DPRINTK_GMAC("called\n");

//  TODO

	DPRINTK_GMAC("exit\n");
	return;
}
#endif


/*
 * Ethernet Tool Support 
 */
static int  
sdpGmac_ethtool_getsettings (struct net_device *pNetDev, struct ethtool_cmd *pCmd)
{
	int retVal;

	SDP_GMAC_DEV_T *pGmacDev = netdev_priv(pNetDev);		
	unsigned long flags;

	DPRINTK_GMAC("called\n");

	pCmd->maxtxpkt = 1;	// ????
	pCmd->maxrxpkt = 1;	// ????

	spin_lock_irqsave(&pGmacDev->lock, flags);
	retVal = mii_ethtool_gset(&pGmacDev->mii, pCmd);
	spin_unlock_irqrestore(&pGmacDev->lock, flags);

	DPRINTK_GMAC("exit\n");
	return retVal;
}

static int 
sdpGmac_ethtool_setsettings (struct net_device *pNetDev, struct ethtool_cmd *pCmd)
{
	int retVal = SDP_GMAC_OK;

	SDP_GMAC_DEV_T *pGmacDev = netdev_priv(pNetDev);		
	unsigned long flags;

	DPRINTK_GMAC_FLOW("called\n");

	spin_lock_irqsave(&pGmacDev->lock, flags);
	retVal = mii_ethtool_sset(&pGmacDev->mii, pCmd);
	spin_unlock_irqrestore(&pGmacDev->lock, flags);

	DPRINTK_GMAC_FLOW("exit\n");
	return retVal;
}

static void 
sdpGmac_ethtool_getdrvinfo (struct net_device *pNetDev, struct ethtool_drvinfo *pDrvInfo)
{

	DPRINTK_GMAC("called\n");

        strncpy(pDrvInfo->driver, ETHER_NAME, sizeof(pDrvInfo->driver));
//      strncpy(info->version, version, sizeof(info->version));
        strncpy(pDrvInfo->bus_info, pNetDev->dev.parent->bus_id, sizeof(pDrvInfo->bus_info));

	DPRINTK_GMAC("exit\n");
	return;
}

static u32
sdpGmac_ethtool_getmsglevel (struct net_device *pNetDev)
{
	u32 retVal = SDP_GMAC_OK;

	SDP_GMAC_DEV_T *pGmacDev = netdev_priv(pNetDev);		
	DPRINTK_GMAC("called\n");

	retVal = pGmacDev->msg_enable;

	DPRINTK_GMAC("exit\n");
	return retVal;
}

static void
sdpGmac_ethtool_setmsglevel (struct net_device *pNetDev, u32 level)
{
	SDP_GMAC_DEV_T *pGmacDev = netdev_priv(pNetDev);		
	DPRINTK_GMAC("called\n");

	pGmacDev->msg_enable = level;

	DPRINTK_GMAC("exit\n");

	return;
}



// phy reset 
static int
sdpGmac_ethtool_nwayreset (struct net_device *pNetDev)
{
	int retVal = SDP_GMAC_OK;

	SDP_GMAC_DEV_T *pGmacDev = netdev_priv(pNetDev);
	unsigned long flags;
	u16 phyVal;
	u8 phyId = pGmacDev->phyId;

	DPRINTK_GMAC("called\n");

	spin_lock_irqsave(&pGmacDev->lock, flags);
	retVal = mii_nway_restart(&pGmacDev->mii);

	/* SDP GMAC need source clock */
// TODO phy Id check 
	phyVal = sdpGmac_mdioRead(pNetDev, phyId, MII_PHYADDR);
	phyVal = phyVal & ~(1 << 11);
	sdpGmac_mdioWrite(pNetDev, phyId, MII_PHYADDR, phyVal);	

	spin_unlock_irqrestore(&pGmacDev->lock, flags);

	DPRINTK_GMAC("exit\n");
	return retVal;
}


/* number of registers GMAC + MII */
static int 
sdpGmac_ethtool_getregslen (struct net_device *pNetDev)
{
	int retVal = SDP_GMAC_OK;

	DPRINTK_GMAC("called\n");

	retVal = sizeof(SDP_GMAC_T);
	retVal += sizeof(SDP_GMAC_MMC_T);
	retVal += sizeof(SDP_GMAC_TIME_STAMP_T);
	retVal += sizeof(SDP_GMAC_MAC_2ND_BLOCK_T);
	retVal += sizeof(SDP_GMAC_DMA_T);
	retVal += (32 << 2);  // MII address

	DPRINTK_GMAC("exit\n");

	return retVal;
}

/* get all registers value GMAC + MII */
static void
sdpGmac_ethtool_getregs (struct net_device *pNetDev, struct ethtool_regs *pRegs, void *pBuf)
{

	SDP_GMAC_DEV_T *pGmacDev = netdev_priv(pNetDev);
	unsigned long flags;
	volatile u32 *pRegAddr;
	u32 *pData = (u32*) pBuf;
	u8 phyId = pGmacDev->phyId;
	u16 phyVal;
	int i, j = 0;

	DPRINTK_GMAC_FLOW("called\n");

	pRegAddr = (volatile u32*)pGmacDev->pGmacBase;
	for(i = 0; i < (sizeof(SDP_GMAC_T) >> 2); i++)
		pData[j++] = *pRegAddr++;

	pRegAddr = (volatile u32*)pGmacDev->pMmcBase;
	for(i = 0; i < (sizeof(SDP_GMAC_MMC_T) >> 2); i++)
		pData[j++] = *pRegAddr++;

	pRegAddr = (volatile u32*)pGmacDev->pTimeStampBase;
	for(i = 0; i < (sizeof(SDP_GMAC_TIME_STAMP_T) >> 2); i++)
		pData[j++] = *pRegAddr++;

	pRegAddr = (volatile u32*)pGmacDev->pDmaBase;
	for(i = 0; i < (sizeof(SDP_GMAC_DMA_T) >> 2); i++)
		pData[j++] = *pRegAddr++;

	for(i = 0; i < 32; i++){
		spin_lock_irqsave(&pGmacDev->lock, flags);
		phyVal = sdpGmac_mdioRead(pNetDev, phyId, i);
		pData[j++] = phyVal & 0xFFFF;
		spin_unlock_irqrestore(&pGmacDev->lock, flags);
	}

	DPRINTK_GMAC_FLOW("exit\n");

	return;
}

static const struct ethtool_ops sdpGmac_ethtool_ops = {
        .get_settings	= sdpGmac_ethtool_getsettings,
        .set_settings	= sdpGmac_ethtool_setsettings,
        .get_drvinfo	= sdpGmac_ethtool_getdrvinfo,
        .get_msglevel	= sdpGmac_ethtool_getmsglevel,
        .set_msglevel	= sdpGmac_ethtool_setmsglevel,
        .nway_reset	= sdpGmac_ethtool_nwayreset,
        .get_link  	= ethtool_op_get_link,
        .get_regs_len	= sdpGmac_ethtool_getregslen,
        .get_regs	= sdpGmac_ethtool_getregs,
};

// 0.92
static int sdpGmac_drv_suspend(struct platform_device *pDev, pm_message_t state)
{
	struct net_device *pNetDev = platform_get_drvdata(pDev);
	unsigned long flags;

	SDP_GMAC_DEV_T *pGmacDev ;
	SDP_GMAC_T *pGmacReg ;		
	SDP_GMAC_DMA_T *pDmaReg ;
	SDP_GMAC_POWER_T    *pPower ;

	if(pNetDev){
		pGmacDev = netdev_priv(pNetDev);
		pGmacReg = pGmacDev->pGmacBase;
		pDmaReg = pGmacDev->pDmaBase;
		pPower = &pGmacDev->power;

		spin_lock_irqsave(&pGmacDev->lock, flags);

		pPower->dma_busMode = pDmaReg->busMode;	
		pPower->dma_operationMode = pDmaReg->operationMode;	
		pPower->gmac_configuration = pGmacReg->configuration;
		pPower->gmac_frameFilter = pGmacReg->frameFilter;

		spin_unlock_irqrestore(&pGmacDev->lock, flags);

// backup mac address 
		memcpy((void *)pPower->gmac_macAddr, 
				(void *)&pGmacReg->macAddr_00_High,
				 	sizeof(pPower->gmac_macAddr));

		if(netif_running(pNetDev)){

			netif_device_detach(pNetDev);

			spin_lock_irqsave(&pGmacDev->lock, flags);

			pDmaReg->operationMode &= ~(B_TX_EN | B_RX_EN) ;
			pGmacReg->configuration &= ~(B_TX_ENABLE | B_RX_ENABLE);	

			// all interrupt disable
			pDmaReg->intrEnable = 0;	
			pDmaReg->status = pDmaReg->status;// Clear interrupt pending register 

			spin_unlock_irqrestore(&pGmacDev->lock, flags);

			del_timer(&pGmacDev->linkCheckTimer);
// long time
			// skb control
			// TODO -> Tx buffer initial, Rx Buffer empty
			sdpGmac_clearAllDesc(pNetDev);	
// long time
			// timer control
		}
	}

	return 0;
}

// 0.92
static void sdpGmac_resume(SDP_GMAC_DEV_T *pGmacDev)
{

	SDP_GMAC_T 	 *pGmacReg = pGmacDev->pGmacBase;		
 	SDP_GMAC_DMA_T 	 *pDmaReg  = pGmacDev->pDmaBase;
	SDP_GMAC_MMC_T 	 *pMmcReg  = pGmacDev->pMmcBase;
	SDP_GMAC_POWER_T *pPower   = &pGmacDev->power;

	int idx;

// resotore mac address 
	memcpy( (void *)&pGmacReg->macAddr_00_High, 
		(void *)pPower->gmac_macAddr,
		sizeof(pPower->gmac_macAddr));

#if 1
	sdpGmac_txInitDesc(pGmacDev->pTxDesc);
	sdpGmac_rxInitDesc(pGmacDev->pRxDesc);
	pDmaReg->txDescListAddr = pGmacDev->txDescDma; // set register 
	pDmaReg->rxDescListAddr = pGmacDev->rxDescDma; // set register 
#endif
	// RX Desc
	idx = (pDmaReg->curHostRxDesc - pDmaReg->rxDescListAddr) / sizeof(DMA_DESC_T);

	if(idx >= NUM_RX_DESC) idx = 0;
			
	pGmacDev->rxNext = idx;
	pGmacDev->rxBusy = idx;
	pGmacDev->pRxDescNext = pGmacDev->pRxDesc + idx;
	pGmacDev->pRxDescBusy = pGmacDev->pRxDesc + idx;

	// TX Desc
	idx = (pDmaReg->curHostTxDesc - pDmaReg->txDescListAddr) / sizeof(DMA_DESC_T);

	if(idx >= NUM_RX_DESC) idx = 0;

	pGmacDev->txNext = idx;
	pGmacDev->txBusy = idx;
	pGmacDev->pTxDescNext = pGmacDev->pTxDesc + idx;
	pGmacDev->pTxDescBusy = pGmacDev->pTxDesc + idx;

	//  MMC interrupt disable all
	pMmcReg = pGmacDev->pMmcBase;
	pMmcReg->intrMaskRx = 0x00FFFFFF;  // disable
	pMmcReg->intrMaskTx = 0x01FFFFFF;  // disable 
	pMmcReg->mmcIpcIntrMaskRx = 0x3FFF3FFF;  // disable 

	pDmaReg->busMode = pPower->dma_busMode;	
	pDmaReg->operationMode = pPower->dma_operationMode & ~(B_TX_EN | B_RX_EN);	
	pGmacReg->configuration = pPower->gmac_configuration & ~(B_TX_ENABLE | B_RX_ENABLE);
	pGmacReg->frameFilter = pPower->gmac_frameFilter;

	return;
}

// 0.92
static int sdpGmac_drv_resume(struct platform_device *pDev)
{
	struct net_device *pNetDev = platform_get_drvdata(pDev);
	SDP_GMAC_DEV_T *pGmacDev;
	

	if(pNetDev){
		printk("[%s]%s resume\n", __FUNCTION__, pNetDev->name);

		pGmacDev = netdev_priv(pNetDev);

		sdpGmac_reset(pNetDev);   	// long time
		sdpGmac_resume(pGmacDev);
	
		if(netif_running(pNetDev)){
			SDP_GMAC_T     *pGmacReg = pGmacDev->pGmacBase;
 			SDP_GMAC_DMA_T *pDmaReg  = pGmacDev->pDmaBase;
			
			struct sk_buff  *pSkb;
			dma_addr_t  	rxDmaAddr;
			DMA_DESC_T	rxInitDesc;

			//skb buffer initialize ????	check
			do{
//				pSkb = dev_alloc_skb(pNetDev->mtu + ETHER_HEADER + ETHER_CRC);
				pSkb = dev_alloc_skb(ETH_DATA_LEN + ETHER_HEADER + ETHER_CRC + 4);   // 0.931 // 0.954

				if(pSkb == NULL){
                        		DPRINTK_GMAC_ERROR("could not allocate sk buffer \n");
                        		break;
				}
				sdpGmac_skbAlign(pSkb); // 0.954

				rxDmaAddr = dma_map_single(pGmacDev->pDev, pSkb->data, skb_tailroom(pSkb), DMA_FROM_DEVICE);
		
				CONVERT_DMA_QPTR(rxInitDesc, skb_tailroom(pSkb), rxDmaAddr, pSkb, 0, 0);

				if(sdpGmac_setRxQptr(pGmacDev, &rxInitDesc, 0) < 0){
					dev_kfree_skb(pSkb);	
					break;
				}
			}while(1);

			// Init link status timer 
			init_timer(&pGmacDev->linkCheckTimer);
			add_timer(&pGmacDev->linkCheckTimer);

			//  interrupt enable all
			pDmaReg->intrEnable = B_INTR_ENABLE_ALL;

			// tx, rx enable
			netif_device_attach(pNetDev);
			pDmaReg->operationMode |= (B_TX_EN | B_RX_EN);	
			pGmacReg->configuration |= (B_TX_ENABLE | B_RX_ENABLE);
		}

	}

	return 0;
}


#ifdef CONFIG_MAC_GET_I2C
#include <asm/plat-sdp/sdp_i2c_io.h>
#define SDPGMAC_I2C_MAGIC_LEN	4
#define SDPGMAC_I2C_NEW_MAGIC_LEN	12
#define SDPGMAC_I2C_NEW_MAC_LEN	8
//#define SDPGMAC_I2C_MAGIC_FACTOR	0xDEAFBEEF
#define SDPGMAC_I2C_MAGIC_USER		0xFAFEF0F0		
const char SDPGMAC_I2C_NEW_MAGIC_USER[12] = { 0x09, 0x74, 0x40, 0x00, 0x00, 0x00, 0x00, 0x84, 0x74, 0x40, 0x00, 0xB2 };
#define SDPGMAC_READ_I2C_LEN		SDPGMAC_I2C_NEW_MAGIC_LEN + SDPGMAC_I2C_NEW_MAC_LEN
#define SDPGMAC_READ_I2C_RETRY		5		// 0.921



static inline void sdpGmac_getMacFromI2c(struct net_device *pNetDev)
{
	int i = 0;
	int is_magic_ok = 0;
	u8 i2cSubAddr[CONFIG_MAC_I2C_SUBADDR_NR];
	u8 readData[SDPGMAC_READ_I2C_LEN];
	u8 *pMacAddr = (readData + SDPGMAC_I2C_NEW_MAGIC_LEN);
	u32 *pFactory = (u32*)readData;

	struct sdp_i2c_packet_t getMacPacket =
			{
				.slaveAddr = CONFIG_MAC_I2C_SLV_ADDR & 0xFF,
				.subAddrSize = CONFIG_MAC_I2C_SUBADDR_NR,
				.udelay = 1000,
				.speedKhz = 400,
				.dataSize = SDPGMAC_READ_I2C_LEN,
				.pSubAddr = i2cSubAddr,
				.pDataBuffer = readData,
				.reserve[0] = 0,		// 0.94
				.reserve[1] = 0,		// 0.94
				.reserve[2] = 0,		// 0.94
				.reserve[3] = 0,		// 0.94
			};

// initial data to 0xFF
	for(i = 0 ; i < SDPGMAC_READ_I2C_LEN; i++)
		readData[i] = 0xFF;
// align
	switch(CONFIG_MAC_I2C_SUBADDR_NR){
		case(1):
			i2cSubAddr[0] = CONFIG_MAC_I2C_SUBADDR & 0xFF;
			break;
		case(2):
			i2cSubAddr[0] = (CONFIG_MAC_I2C_SUBADDR >> 8) & 0xFF;
			i2cSubAddr[1] = CONFIG_MAC_I2C_SUBADDR & 0xFF;
			break;
		case(3):
			i2cSubAddr[0] = (CONFIG_MAC_I2C_SUBADDR >> 16) & 0xFF;
			i2cSubAddr[1] = (CONFIG_MAC_I2C_SUBADDR >> 8) & 0xFF;
			i2cSubAddr[2] = CONFIG_MAC_I2C_SUBADDR & 0xFF;
			break;
		case(4):
			i2cSubAddr[0] = (CONFIG_MAC_I2C_SUBADDR >> 24) & 0xFF;
			i2cSubAddr[1] = (CONFIG_MAC_I2C_SUBADDR >> 16) & 0xFF;
			i2cSubAddr[2] = (CONFIG_MAC_I2C_SUBADDR >> 8) & 0xFF;
			i2cSubAddr[3] = CONFIG_MAC_I2C_SUBADDR & 0xFF;
			break;
		default:
			memcpy(pNetDev->dev_addr, pMacAddr, N_MAC_ADDR);	
			return;	
			break;
	}

	i = SDPGMAC_READ_I2C_RETRY;
	
	do {
		if(sdp_i2c_request(CONFIG_MAC_I2C_PORT, 	
				I2C_CMD_COMBINED_READ, 
				&getMacPacket) >= 0) break;	// 0.921
	}while(i--);
//#    if 0 	// 0.9541
#    if 1 	// 0.9541

	is_magic_ok = 1;

	for(i = 0; i < SDPGMAC_I2C_NEW_MAGIC_LEN ; i++){
		if(SDPGMAC_I2C_NEW_MAGIC_USER[i] != ((char*)pFactory)[i]){
			is_magic_ok = 0;
			break;
		}
	}

	if(is_magic_ok){
		printk("Magic Code is OK \n\n");
		memcpy(pNetDev->dev_addr, pMacAddr, N_MAC_ADDR);

		printk("pNedtDev->dev_addr : ");
		for(i = 0; i < 6 ; i++){
			printk(" %2x,", ((char*)pNetDev->dev_addr)[i]);
		}
		printk("\n");

	}
	else {
		printk("Magic Code is Not OK \n\n");
		memset(pNetDev->dev_addr, 0xFF, N_MAC_ADDR);
	}
#    else		
	memcpy(pNetDev->dev_addr, pMacAddr, N_MAC_ADDR);
#    endif 	// 0.9541
		
	return;
}
#elif defined(CONFIG_MAC_GET_FLASH)
static inline void sdpGmac_getMacFromFlash(struct net_device *pNetDev)
{

	//int flash_fd=0;
	u8 *pDev_addr = pNetDev->dev_addr;
	struct file* flash_fd=NULL;
	int ret = 0;
	char magicCode[4]={0,};
	u8 tempMac[6] = {0,};
	mm_segment_t oldfs = get_fs();
	set_fs(KERNEL_DS);

	//flash_fd = filp_open("/dev/tbml12", O_RDONLY, 0);
	printk("[SDP GMAC] Get Mac Address from %s\n",CONFIG_MAC_FLASH_PARTITION);
	flash_fd = filp_open(CONFIG_MAC_FLASH_PARTITION, O_RDONLY, 0);

	if(flash_fd < 0){
		goto out;
	}

	ret = flash_fd->f_op->read(flash_fd, magicCode,4, &flash_fd->f_pos);
	
	if(ret != 4){
		goto close_out;
	}
	

	// If Mac Araea Read (Magic Code Check)
	if( (magicCode[0] == 'B') && (magicCode[1] == 'M') && (magicCode[2] == 'V')){
		//ret = sys_read(flash_fd, tempMac,6);
		ret = flash_fd->f_op->read(flash_fd, tempMac,6, &flash_fd->f_pos);
		
		if(ret != 6){
			goto close_out;
		}
		
		printk("[SDP GMAC] Magic Key OK\n");
		printk("[SDP GMAC] Mac : %2x:%2x:%2x:%2x:%2x:%2x \n", tempMac[0],tempMac[1],tempMac[2],tempMac[3],tempMac[4],tempMac[5]);
		pDev_addr[0] = (u8)tempMac[0];
		pDev_addr[1] = (u8)tempMac[1];
		pDev_addr[2] = (u8)tempMac[2];
		pDev_addr[3] = (u8)tempMac[3];
		pDev_addr[4] = (u8)tempMac[4];
		pDev_addr[5] = (u8)tempMac[5];
	}else{
		printk("[SDP GMAC] Magic Key NO-OK : No Mac Address, Use Default Mac Address \n");

		pDev_addr[0] = (u8)0x11;
		pDev_addr[1] = (u8)0x22;
		pDev_addr[2] = (u8)0x33;
		pDev_addr[3] = (u8)0x44;
		pDev_addr[4] = (u8)0x55;
		pDev_addr[5] = (u8)0x66;
	}

close_out:
	filp_close(flash_fd,NULL);
out:
	set_fs(oldfs);
	return;
}
#else
static inline void sdpGmac_setMacByUser(struct net_device *pNetDev)
{
	u32 front_4Byte = CONFIG_MAC_FRONT_4BYTE;
	u32 end_2Byte = CONFIG_MAC_END_2BYTE;
	u8 *pDev_addr = pNetDev->dev_addr;

	pDev_addr[0] = (u8)((front_4Byte >> 24) & 0xFF);
	pDev_addr[1] = (u8)((front_4Byte >> 16) & 0xFF);
	pDev_addr[2] = (u8)((front_4Byte >> 8) & 0xFF);
	pDev_addr[3] = (u8)((front_4Byte) & 0xFF);
	pDev_addr[4] = (u8)((end_2Byte >> 8) & 0xFF);
	pDev_addr[5] = (u8)(end_2Byte & 0xFF);

}
#endif 

static int __devinit sdpGmac_netDev_initDesc(SDP_GMAC_DEV_T* pGmacDev, u32 descMode)
{
	int retVal = SDP_GMAC_OK;
	SDP_GMAC_DMA_T* pDmaReg = pGmacDev->pDmaBase;

	DPRINTK_GMAC_FLOW ("called\n");

	if (descMode == DESC_MODE_RING) {
// Tx Desc initialize 
		pGmacDev->txNext = 0;
		pGmacDev->txBusy = 0;
		pGmacDev->pTxDescNext = pGmacDev->pTxDesc;
		pGmacDev->pTxDescBusy = pGmacDev->pTxDesc;
		pDmaReg->txDescListAddr = pGmacDev->txDescDma; // set register 

		sdpGmac_txInitDesc(pGmacDev->pTxDesc);
		DPRINTK_GMAC_FLOW("last TxDesc.status is 0x%08x\n", 
				pGmacDev->pTxDesc[NUM_TX_DESC-1].status);
	
// Rx Desc initialize
		pGmacDev->rxNext = 0;
		pGmacDev->rxBusy = 0;
		pGmacDev->pRxDescNext = pGmacDev->pRxDesc;
		pGmacDev->pRxDescBusy = pGmacDev->pRxDesc;
		pDmaReg->rxDescListAddr = pGmacDev->rxDescDma; // set register 

		sdpGmac_rxInitDesc(pGmacDev->pRxDesc);
		DPRINTK_GMAC_FLOW("last RxDesc.status is 0x%08x\n", 
				pGmacDev->pRxDesc[NUM_RX_DESC-1].length);
	}
	else {
		DPRINTK_GMAC_ERROR("Not support CHAIN MODE yet\n");
		retVal = -SDP_GMAC_ERR;
	}

	DPRINTK_GMAC_FLOW("Tx Desc is 0x%08x\n",  (int)pGmacDev->pTxDesc);
	DPRINTK_GMAC_FLOW("Rx Desc is 0x%08x\n",  (int)pGmacDev->pRxDesc);

	DPRINTK_GMAC_FLOW ("exit\n");
	return retVal;	
}



static int __devinit sdpGmac_probe(struct net_device *pNetDev)
{
	int retVal = 0;
//	const u8 defaultMacAddr[N_MAC_ADDR] = DEFAULT_MAC_ADDR;
	SDP_GMAC_DEV_T *pGmacDev = netdev_priv(pNetDev);
	
	DPRINTK_GMAC ("called\n");

	spin_lock_init(&pGmacDev->lock);

	/* Device reset MAC addr is cleared*/	
	if(sdpGmac_reset(pNetDev) < 0){
		DPRINTK_GMAC_ERROR("reset Error\n");
		retVal = -ENODEV;		// 0.952
		goto  __probe_out_err;		// 0.952
	}

	ether_setup(pNetDev);

	pNetDev->get_stats = sdpGmac_netDev_getStats;
	pNetDev->ethtool_ops = &sdpGmac_ethtool_ops;
	pNetDev->hard_start_xmit = sdpGmac_netDev_hardStartXmit;
	pNetDev->watchdog_timeo = 5 * HZ;
	pNetDev->open = sdpGmac_netDev_open;
	pNetDev->stop = sdpGmac_netDev_close;
	pNetDev->set_multicast_list = sdpGmac_netDev_setMulticastList;
	pNetDev->set_mac_address = sdpGmac_netDev_setMacAddr;
//	pNetDev->tx_timeout = sdpGmac_netDev_txTimeOut; // TODO
//	pNetDev->change_mtu = sdpGmac_netDev_chgMtu;	// TODO
	pNetDev->do_ioctl = sdpGmac_netDev_ioctl;

// Phy configuration
	INIT_WORK(&pGmacDev->phyConf, sdpGmac_phyConf);
	pGmacDev->mii.phy_id_mask = 0x1F;
	pGmacDev->mii.reg_num_mask = 0x1F;
	pGmacDev->mii.force_media = 0;  // ???
	pGmacDev->mii.full_duplex = 0; 	// ???
	pGmacDev->mii.dev = pNetDev;
	pGmacDev->mii.mdio_read = sdpGmac_mdioRead;
	pGmacDev->mii.mdio_write = sdpGmac_mdioWrite;
	pGmacDev->mii.phy_id = pGmacDev->phyId;	 // phy address 
	pGmacDev->pRxDummySkb = dev_alloc_skb(ETH_DATA_LEN + ETHER_PACKET_EXTRA + 4);   // 0.931  // 0.954

	if(pGmacDev->pRxDummySkb == NULL) {
		retVal = -ENOMEM;
		goto __probe_out_err;
	}

	sdpGmac_skbAlign(pGmacDev->pRxDummySkb); // 0.954

// net stats init
	pGmacDev->netStats.rx_errors 	    = 0;
	pGmacDev->netStats.collisions       = 0;
	pGmacDev->netStats.rx_crc_errors    = 0;
	pGmacDev->netStats.rx_frame_errors  = 0;
	pGmacDev->netStats.rx_length_errors = 0;

// TODO check RING MODE and CHAIN MODE
	pGmacDev->pTxDesc = dma_alloc_coherent(pGmacDev->pDev, TX_DESC_SIZE, 
						&pGmacDev->txDescDma, GFP_ATOMIC);
	if(pGmacDev->pTxDesc == NULL) {
		retVal = -ENOMEM;		// 0.952
		goto __probe_out_err1;		// 0.952
	}

	pGmacDev->pRxDesc = dma_alloc_coherent(pGmacDev->pDev, RX_DESC_SIZE, 
						&pGmacDev->rxDescDma, GFP_ATOMIC);

	if(pGmacDev->pRxDesc == NULL) {
		retVal = -ENOMEM;
		goto __probe_out_err2;
	}

	retVal = sdpGmac_netDev_initDesc(pGmacDev, DESC_MODE_RING);

	if(retVal < 0) {
		retVal = -EPERM;		// 0.952
		goto __probe_out_err3;		// 0.952
	}
	

// register initalize
	sdpGmac_dmaInit(pNetDev);	
	sdpGmac_gmacInit(pNetDev);	
			
// request interrupt resource 
	retVal = request_irq(pNetDev->irq, sdpGmac_intrHandler, 0, pNetDev->name, pNetDev);
	if(retVal < 0) goto __probe_out_err3;	// 0.952

	if(register_netdev(pNetDev) < 0) {
		DPRINTK_GMAC_ERROR ("register netdev error\n");
		retVal = -EPERM;
		goto __probe_out_err4;
	}
	
#if defined(CONFIG_MAC_GET_I2C)
	sdpGmac_getMacFromI2c(pNetDev);		// get mac address 
#elif defined(CONFIG_MAC_GET_FLASH)
	sdpGmac_getMacFromFlash(pNetDev);
#elif defined(CONFIG_MAC_SET_BY_USER)
	sdpGmac_setMacByUser(pNetDev);
#endif 

	if(!is_valid_ether_addr(pNetDev->dev_addr)){
		PRINTK_GMAC("%s: Invalid ethernet MAC address. Please "
				"set using ifconfig\n", pNetDev->name);
	}
	else {
		PRINTK_GMAC ("%s: Ethernet address %02x:%02x:%02x:%02x:%02x:%02x\n",
		     pNetDev->name, *pNetDev->dev_addr, *(pNetDev->dev_addr+1),
		     *(pNetDev->dev_addr+2), *(pNetDev->dev_addr+3),
		     *(pNetDev->dev_addr+4), *(pNetDev->dev_addr+5));	

		sdpGmac_setMacAddr(pNetDev, (const u8*)pNetDev->dev_addr);
	}

	PRINTK_GMAC ("%s: IRQ is %d\n", pNetDev->name, pNetDev->irq);
	
	if(pGmacDev->phyType == 0) {
		DPRINTK_GMAC_ERROR("could not find PHY. Please check b'd\n");
		retVal = -ENODEV;
		goto __probe_out_err5;
	}

		PRINTK_GMAC("Find PHY. PHY ID: %08x\n", pGmacDev->phyType);

	DPRINTK_GMAC ("exit\n");
	return 0;

__probe_out_err5:				// 0.952
	unregister_netdev(pNetDev);
__probe_out_err4:				// 0.952
	free_irq(pNetDev->irq, pNetDev);
__probe_out_err3:				// 0.952
	dma_free_coherent(pGmacDev->pDev, RX_DESC_SIZE, 
				pGmacDev->pRxDesc, pGmacDev->rxDescDma);
__probe_out_err2:				// 0.952
	dma_free_coherent(pGmacDev->pDev, TX_DESC_SIZE, 
				pGmacDev->pTxDesc, pGmacDev->txDescDma);
__probe_out_err1:				// 0.952
	dev_kfree_skb(pGmacDev->pRxDummySkb);	
__probe_out_err:
	return retVal;
}


static inline int sdpGmac_setMemBase(int id, struct resource *pRes, SDP_GMAC_DEV_T* pGmacDev)
{
	int	retVal = 0;
	void  	*remapAddr;	
	size_t 	size = (pRes->end) - (pRes->start);


// TODO: request_mem_region 

	if (id < N_REG_BASE)
		remapAddr = (void*)ioremap_nocache(pRes->start, size);
	else {
		DPRINTK_GMAC_ERROR("id is wrong \n");	
		return retVal -1;
	}
	

	switch(id){
		case (0):
		   pGmacDev->pGmacBase = (SDP_GMAC_T*)remapAddr;
		   PRINTK_GMAC ("GMAC remap is 0x%08x\n",(int) remapAddr);
		   break;

		case (1):
		   pGmacDev->pMmcBase = (SDP_GMAC_MMC_T*)remapAddr;
		   DPRINTK_GMAC ("Gmac manage count remap is 0x%08x\n",(int) remapAddr);
		   break;

		case (2):
		   pGmacDev->pTimeStampBase = (SDP_GMAC_TIME_STAMP_T*)remapAddr;
		   DPRINTK_GMAC ("time stamp remap is 0x%08x\n",(int) remapAddr);
		   break;

		case (3):
		   pGmacDev->pMac2ndBlk = (SDP_GMAC_MAC_2ND_BLOCK_T*)remapAddr;
		   DPRINTK_GMAC ("mac 2nd remap is 0x%08x\n",(int) remapAddr);
		   break;

		case (4):
		   pGmacDev->pDmaBase = (SDP_GMAC_DMA_T*)remapAddr;
		   PRINTK_GMAC ("DMA remap is 0x%08x\n",(int) remapAddr);
		   break;

		default:
		   break;
	}

//	DPRINTK_GMAC ("exit\n");

	return retVal;
}

/* Linux probe and remove */
static int __devinit sdpGmac_drv_probe (struct platform_device *pDev)
{
	struct resource *pRes = pDev->resource;	
	struct net_device *pNetDev;
	SDP_GMAC_DEV_T *pGmacDev;
	int retVal = 0;
	int i = 0;

	DPRINTK_GMAC_FLOW ("called\n");

	if(pRes == NULL) goto __probe_drv_out;

	printk(KERN_INFO"sdp GMAC 10/100M network driver ver %s\n", GMAC_DRV_VERSION);
// net device 
	pNetDev = alloc_etherdev(sizeof(SDP_GMAC_DEV_T));

        if (!pNetDev) {
                DPRINTK_GMAC_ERROR("%s: could not allocate device.\n", ETHER_NAME);
                retVal = -ENOMEM;
                goto __probe_drv_out;
        }
	SET_NETDEV_DEV(pNetDev, &pDev->dev);

	pNetDev->dma = (unsigned char) -1;
	pGmacDev = netdev_priv(pNetDev);
	pGmacDev->pNetDev = pNetDev;
// need to request dma memory 
	pGmacDev->pDev = &pDev->dev;		
	
	pGmacDev->pGmacBase = NULL;		// 0.952
	pGmacDev->pMmcBase = NULL;		// 0.952
	pGmacDev->pTimeStampBase = NULL;	// 0.952
	pGmacDev->pMac2ndBlk = NULL;		// 0.952
	pGmacDev->pDmaBase = NULL;		// 0.952

// GMAC resource initialize 
	for (i = 0; i < N_REG_BASE; i++) {
		pRes = platform_get_resource(pDev, IORESOURCE_MEM, i);
		if(!pRes){	// 0.952
                	DPRINTK_GMAC_ERROR("%s: could not find device %d resource.\n", 
						ETHER_NAME, i);
			retVal = -ENODEV;
			goto __probe_drv_err;
		}
		sdpGmac_setMemBase(i, pRes, pGmacDev);
	}

	pNetDev->irq = platform_get_irq(pDev, 0);
	DPRINTK_GMAC_FLOW("IRQ is %d\n", pNetDev->irq);

// save resource about net driver 
	platform_set_drvdata(pDev, pNetDev);

// DMA mask
	pDev->dev.coherent_dma_mask = 0xFFFFFFFFUL;
	retVal = sdpGmac_probe(pNetDev);

__probe_drv_err:
	if (retVal < 0) { 
		if(pGmacDev->pGmacBase) iounmap(pGmacDev->pGmacBase);
		if(pGmacDev->pMmcBase) iounmap(pGmacDev->pMmcBase);	//0.952
		if(pGmacDev->pTimeStampBase) iounmap(pGmacDev->pTimeStampBase);
		if(pGmacDev->pMac2ndBlk) iounmap(pGmacDev->pMac2ndBlk);
		if(pGmacDev->pDmaBase) iounmap(pGmacDev->pDmaBase);
		free_netdev(pNetDev);
	}

__probe_drv_out:
	DPRINTK_GMAC_FLOW ("exit\n");
	return retVal;
}


static int __devexit sdpGmac_drv_remove (struct platform_device *pDev)
{
	struct net_device *pNetDev;
	SDP_GMAC_DEV_T *pGmacDev;
	SDP_GMAC_T *pGmacReg;
	SDP_GMAC_DMA_T *pDmaReg;

	DPRINTK_GMAC_FLOW ("called\n");

	if(!pDev) return 0;
	pNetDev = platform_get_drvdata(pDev);

	if(!pNetDev) return 0;
	pGmacDev = netdev_priv(pNetDev);

	unregister_netdev(pNetDev);

	pGmacReg = pGmacDev->pGmacBase;
	pDmaReg = pGmacDev->pDmaBase;

	netif_stop_queue(pNetDev);
	netif_carrier_off(pNetDev);

	// rx, tx disable
	pDmaReg->operationMode &= ~(B_TX_EN | B_RX_EN) ;
	pGmacReg->configuration &= ~(B_TX_ENABLE | B_RX_ENABLE);	

	// all interrupt disable
	pDmaReg->intrEnable = 0;	
	pDmaReg->status = pDmaReg->status;	// Clear interrupt pending register 

	// phy control

	// skb control
	sdpGmac_clearAllDesc(pNetDev);	

	// timer control
	del_timer(&pGmacDev->linkCheckTimer);


 	if(pGmacDev->pRxDummySkb)
	dev_kfree_skb(pGmacDev->pRxDummySkb);	//0.940

	dma_free_coherent(pGmacDev->pDev, TX_DESC_SIZE, 
				pGmacDev->pTxDesc, pGmacDev->txDescDma);

	dma_free_coherent(pGmacDev->pDev, RX_DESC_SIZE, 
				pGmacDev->pRxDesc, pGmacDev->rxDescDma);

	free_irq(pNetDev->irq, pNetDev);
	iounmap(pGmacDev->pGmacBase);
	iounmap(pGmacDev->pTimeStampBase);
	iounmap(pGmacDev->pMac2ndBlk);
	iounmap(pGmacDev->pDmaBase);

	free_netdev(pNetDev);	
	platform_set_drvdata(pDev, NULL);

	DPRINTK_GMAC_FLOW ("exit\n");

	return 0;
}

#if 0	// remove platform device 0.952
// 0.952 move base address to sdpGmac_reg.h 

/* Platform device register */
static struct resource sdpGmac_resource[] = {
        [0] = {
                .start  = PA_SDP_GMAC_BASE,
                .end    = PA_SDP_GMAC_BASE + sizeof(SDP_GMAC_T),
                .flags  = IORESOURCE_MEM,
        },

        [1] = {
                .start  = PA_SDP_GMAC_MMC_BASE,
                .end    = PA_SDP_GMAC_MMC_BASE + sizeof(SDP_GMAC_MMC_T), 
                .flags  = IORESOURCE_MEM,
        },

        [2] = {
                .start  = PA_SDP_GMAC_TIME_STAMP_BASE,
                .end    = PA_SDP_GMAC_TIME_STAMP_BASE + sizeof(SDP_GMAC_TIME_STAMP_T), 
                .flags  = IORESOURCE_MEM,
        },

        [3] = {
                .start  = PA_SDP_GMAC_MAC_2ND_BLOCK_BASE,
                .end    = PA_SDP_GMAC_MAC_2ND_BLOCK_BASE 
			  	+ sizeof(SDP_GMAC_MAC_2ND_BLOCK_T),  // 128KByte
                .flags  = IORESOURCE_MEM,
        },

        [4] = {
                .start  = PA_SDP_GMAC_DMA_BASE,
                .end    = PA_SDP_GMAC_DMA_BASE + sizeof(SDP_GMAC_DMA_T),  // 128KByte
                .flags  = IORESOURCE_MEM,
        },

        [5] = {
                .start  = IRQ_SDP_GMAC,
                .end    = IRQ_SDP_GMAC,
                .flags  = IORESOURCE_IRQ,
        },
};

static struct platform_device sdpGmac_devs = {
	.name 		= ETHER_NAME,
	.id		= 0,
	.num_resources 	= ARRAY_SIZE(sdpGmac_resource),
	.resource	= sdpGmac_resource,
};

static struct platform_device* pSdpGmac_devs_list[] =
{
	&sdpGmac_devs,
};
#endif

static struct platform_device *gpSdpGmacPlatDev;

static struct platform_driver sdpGmac_device_driver = {
	.probe		= sdpGmac_drv_probe,
	.remove		= __devexit_p(sdpGmac_drv_remove),
	.driver		= {
		.name	= ETHER_NAME,
		.owner  = THIS_MODULE,
	},
	.suspend	= sdpGmac_drv_suspend,
	.resume		= sdpGmac_drv_resume,
};


/* Module init and exit */
static int __init sdpGmac_init(void)
{
	int retVal = 0;

	DPRINTK_GMAC_FLOW ("called\n");

//	platform_add_devices(pSdpGmac_devs_list, ARRAY_SIZE(pSdpGmac_devs_list)); // 0.952 reomove

	gpSdpGmacPlatDev = platform_device_alloc(ETHER_NAME, -1);

	if(!gpSdpGmacPlatDev)
		return -ENOMEM;

	retVal = platform_device_add(gpSdpGmacPlatDev);
	if(retVal) goto __put_dev;

	retVal = platform_driver_register(&sdpGmac_device_driver);
	if(retVal == 0) goto __out;

	platform_device_del(gpSdpGmacPlatDev);
__put_dev:
	platform_device_put(gpSdpGmacPlatDev);
__out:
	DPRINTK_GMAC_FLOW ("exit\n");
	return retVal;
}


static void __exit sdpGmac_exit(void)
{
	DPRINTK_GMAC_FLOW ("called\n");

	platform_driver_unregister(&sdpGmac_device_driver);
	platform_device_unregister(gpSdpGmacPlatDev);

	DPRINTK_GMAC_FLOW ("exit\n");
}


module_init(sdpGmac_init);
module_exit(sdpGmac_exit);

MODULE_AUTHOR("VD Division, Samsung Electronics Co.");
MODULE_LICENSE("GPL");




#if 0 // 0.95 org, netDev_rx 
		checkStatus = rxDesc.status & (DESC_ERROR | DESC_RX_1ST | DESC_RX_LAST);

		if(checkStatus == (DESC_RX_1ST | DESC_RX_LAST)) {
			len = DESC_GET_FRAME_LEN(rxDesc.status) - 4; // without crc byte

			skb_put(pSkb,len);

			//TODO : IPC_OFFLOAD  ??? H/W Checksum	
				
			// Network buffer post process
			pSkb->dev = pNetDev;
			pSkb->protocol = eth_type_trans(pSkb, pNetDev);
			netif_rx(pSkb);		// post buffer to the network code 

			pNetDev->last_rx = jiffies;
			pNetStats->rx_packets++;
			pNetStats->rx_bytes += len;
		}
		else {	// ERROR
			pNetStats->rx_errors++;

			if(rxDesc.status & DESC_RX_COLLISION) pNetStats->collisions++;
			if(rxDesc.status & DESC_RX_CRC) pNetStats->rx_crc_errors++ ;
			if(rxDesc.status & DESC_RX_DRIBLING) pNetStats->rx_frame_errors++ ;
			if(rxDesc.status & DESC_RX_LEN_ERR) pNetStats->rx_length_errors++;

			//dev_kfree_skb_irq(pSkb);	// discard packet 
			goto __set_rx_qptr;	// 0.941
		}
#endif 
