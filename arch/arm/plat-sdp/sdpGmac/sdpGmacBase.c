/***************************************************************************
 *
 *	arch/arm/plat-sdp/sdpGmac/sdpGmac.c
 *	Samsung Elecotronics.Co
 *	Created by tukho.kim	
 *
 * ************************************************************************/
/*
 * 2009.08.03: Created by tukho.kim@samsung.com
 * 2009.11.18,tukho.kim: rtl8201E RXD clock toggle active, hidden function 0.9431
 * 2009.12.15,tukho.kim: hwSpeed init value change to STATUS_SPEED_100 0.953
 * 2010.01.23,tukho.kim: debug wait code about DMA reset in sdpGmac_reset  0.956
 */ 

#include "sdpGMAC.h"

int sdpGmac_mdioRead(struct net_device *pNetDev, int phyId, int loc)
{
	SDP_GMAC_DEV_T* pGmacDev = netdev_priv(pNetDev);
	SDP_GMAC_T * pGmacReg = pGmacDev->pGmacBase;

	u32 addr, data;

//	DPRINTK_GMAC ("called phyId: %d, loc: %02x\n", phyId, loc);
	addr = GMII_ADDR_READ (phyId, loc);

	while(pGmacReg->gmiiAddr & B_GMII_ADDR_BUSY);

	pGmacReg->gmiiAddr = addr;
	while(pGmacReg->gmiiAddr & B_GMII_ADDR_BUSY);

	data = pGmacReg->gmiiData & 0xFFFF;
	
//	DPRINTK_GMAC ("exit\n");
	return data;
}

void sdpGmac_mdioWrite(struct net_device * pNetDev, int phyId, int loc, int val)
{
	SDP_GMAC_DEV_T* pGmacDev = netdev_priv(pNetDev);
	SDP_GMAC_T * pGmacReg = pGmacDev->pGmacBase;

	u32 addr, data;

//	DPRINTK_GMAC ("called phyId: %d, loc: %02x\n", phyId, loc);
	while(pGmacReg->gmiiAddr & B_GMII_ADDR_BUSY);

	pGmacReg->gmiiData = val & 0xFFFF;

	addr = GMII_ADDR_WRITE(phyId, loc);

	pGmacReg->gmiiAddr = addr;
	while(pGmacReg->gmiiAddr & B_GMII_ADDR_BUSY);

	data = pGmacReg->gmiiData & 0xFFFF;
	
//	DPRINTK_GMAC ("exit\n");
	return;
}

// Realtek 8201E
static inline void rtl8201_phyReset(struct net_device *pNetDev)
{
	u16 phyVal;
	SDP_GMAC_DEV_T* pGmacDev = netdev_priv(pNetDev);
	int phyId = pGmacDev->phyId;

//	DPRINTK_GMAC_FLOW("phy reset\n");
// phy reset 
	phyVal = sdpGmac_mdioRead(pNetDev, phyId, MII_BMCR);
	phyVal = phyVal | (1 << 15);
	sdpGmac_mdioWrite(pNetDev, phyId, MII_BMCR, phyVal);	

	while(1){
		phyVal = sdpGmac_mdioRead(pNetDev, phyId, MII_BMCR);
		if(!(phyVal & (1 << 15))) break;
	}

	// caution!!!! if change phy, Must check this part
	// Test regiset Clock output set 
	phyVal = sdpGmac_mdioRead(pNetDev, phyId, MII_PHYADDR);
	phyVal = phyVal & ~(1 << 11);
	phyVal = phyVal & ~(1 << 13);  // 0.9431
	sdpGmac_mdioWrite(pNetDev, phyId, MII_PHYADDR, phyVal);	

	phyVal = sdpGmac_mdioRead(pNetDev, phyId, MII_PHYSID1);
	pGmacDev->phyType = phyVal << 16;	
	phyVal = sdpGmac_mdioRead(pNetDev, phyId, MII_PHYSID2);
	pGmacDev->phyType |= phyVal;	
	
//	DPRINTK_GMAC_FLOW("phy id is %08x\n", pGmacDev->phyType);
}



int sdpGmac_reset(struct net_device *pNetDev)
{
	int retVal = SDP_GMAC_OK;

	SDP_GMAC_DEV_T* pGmacDev = netdev_priv(pNetDev);
	SDP_GMAC_T * pGmacReg = pGmacDev->pGmacBase;
	SDP_GMAC_DMA_T * pDmaReg = pGmacDev->pDmaBase;
	
	u32 regVal;

/* version check */
	if((pGmacReg->version & 0xFF) != GMAC_SYNOP_VER) {
		DPRINTK_GMAC_ERROR("could not find GMAC!!!!\n");
		return -SDP_GMAC_ERR;
	}
	PRINTK_GMAC("Find SDP GMAC ver %04x\n", pGmacReg->version);	
// For Debug 
// Gpio pad control : Address MSB -> phy pin
#if defined(CONFIG_ARCH_SDP92)
 	regVal = *(volatile u32 *)0xF8090D88 | (1 << 21);
 	*(volatile u32 *)0xF8090D88 = regVal;
#elif defined(CONFIG_ARCH_SDP93)
 	*(volatile u32 *)0xF80D0008 = 1;	// FIFO stepping stone
 	regVal = *(volatile u32 *)0xF8090880;
	regVal &= 0x8FFFFFFF;
 	*(volatile u32 *)0xF8090880 = regVal | (4 << 28);
#endif

// set phy id
	pGmacDev->phyId = GMAC_PHY_ID;

#if 1 //RTL8201E clock output
	rtl8201_phyReset(pNetDev);
#endif
// For Debug end

/* sdpGmac Reset */
// DMA Reset 
//	PRINTK_GMAC("DMA reset\n");
	pDmaReg->busMode = B_SW_RESET; // '0x1'

// caution!!!! 
// if don't recive phy clock, this code is hang 
// 0.956
	udelay(5);	// need 3 ~ 5us 
	do {
		if(pDmaReg->busMode & B_SW_RESET) {
			PRINTK_GMAC("check phy clock output register 0x19(0x1E40)\n");	
			msleep(1000);
		} else {	
			PRINTK_GMAC("DMA reset is release(normal status)\n");	
			break;
		}
	} while(1);
// 0.956 end

// all interrupt disable 
	pGmacReg->intrMask = 0x20F;
	regVal = pGmacReg->interrupt; // clear status register 
	pDmaReg->intrEnable = 0;
	pDmaReg->status = 0x0001E7FF;  // clear status register 

// function define
	pGmacDev->hwSpeed = STATUS_SPEED_100;	// 0.953
	pGmacDev->hwDuplex = 1;
	pGmacDev->msg_enable = NETIF_MSG_LINK;
	pGmacDev->oldBmcr = BMCR_ANENABLE;

	return retVal;
}

int sdpGmac_getMacAddr(struct net_device *pNetDev, u8* pMacAddr)
{
	int retVal = SDP_GMAC_OK;

	SDP_GMAC_DEV_T* pGmacDev = netdev_priv(pNetDev);
	SDP_GMAC_T * pGmacReg = pGmacDev->pGmacBase;

	unsigned long flags;
	u32 macAddr[2] = {0, 0};

	DPRINTK_GMAC_FLOW ("called\n");

	spin_lock_irqsave(&pGmacDev->lock, flags);

	macAddr[0] = pGmacReg->macAddr_00_Low;
	macAddr[1] = pGmacReg->macAddr_00_High & 0xFFFF;

	spin_unlock_irqrestore(&pGmacDev->lock, flags);
	
	DPRINTK_GMAC_FLOW ("macAddrLow is 0x%08x\n", macAddr[0]);
	DPRINTK_GMAC_FLOW ("macAddrHigh is 0x%08x\n", macAddr[1]);
	memcpy(pMacAddr, macAddr, N_MAC_ADDR);

	flags = macAddr[0] + macAddr[1] ;

	if(flags == 0)	retVal = -SDP_GMAC_ERR;
	else if ((macAddr[0] == 0xFFFFFFFF) &&
		 (macAddr[1] == 0x0000FFFF)) retVal = -SDP_GMAC_ERR;

//	DPRINTK_GMAC ("exit\n");
	return retVal;

}

void sdpGmac_setMacAddr(struct net_device * pNetDev, const u8* pMacAddr)
{
	unsigned long flags;
	u32 macAddr[2] = {0, 0};
	SDP_GMAC_DEV_T* pGmacDev = netdev_priv(pNetDev);
	SDP_GMAC_T * pGmacReg = pGmacDev->pGmacBase;

	DPRINTK_GMAC_FLOW ("called\n");

	memcpy(macAddr, pMacAddr, N_MAC_ADDR);

	spin_lock_irqsave(&pGmacDev->lock, flags);

	pGmacReg->macAddr_00_Low = macAddr[0];
	pGmacReg->macAddr_00_High = macAddr[1];

	spin_unlock_irqrestore(&pGmacDev->lock, flags);
	
	DPRINTK_GMAC_FLOW ("exit\n");
}

void sdpGmac_dmaInit(struct net_device * pNetDev)
{
	unsigned long flags;
	SDP_GMAC_DEV_T* pGmacDev = netdev_priv(pNetDev);
	SDP_GMAC_DMA_T * pDmaReg = pGmacDev->pDmaBase;

	DPRINTK_GMAC_FLOW ("called\n");

	spin_lock_irqsave(&pGmacDev->lock, flags);

	pDmaReg->busMode = B_FIX_BURST_EN | B_BURST_LEN(8) | B_DESC_SKIP_LEN(2);
	DPRINTK_GMAC ("busMode set %08x\n", 
			   B_FIX_BURST_EN | B_BURST_LEN(8) | B_DESC_SKIP_LEN(2));

	pDmaReg->operationMode = B_TX_STR_FW | B_TX_THRESHOLD_192;
	DPRINTK_GMAC ("opMode set %08x\n", 
				 B_TX_STR_FW | B_TX_THRESHOLD_192);

	spin_unlock_irqrestore(&pGmacDev->lock, flags);
	
	DPRINTK_GMAC_FLOW ("exit\n");
}

void sdpGmac_gmacInit(struct net_device * pNetDev)
{
	unsigned long flags;
	SDP_GMAC_DEV_T* pGmacDev = netdev_priv(pNetDev);
	SDP_GMAC_T * pGmacReg = pGmacDev->pGmacBase;

	u32 regVal = 0;

	DPRINTK_GMAC_FLOW ("called\n");

	spin_lock_irqsave(&pGmacDev->lock, flags);

	// wd enable // jab enable 
	// frame burst enable
#ifdef CONFIG_SDP_GMAC_GIGA_BIT
	regVal = B_FRAME_BURST_EN;  	// only GMAC, not support 10/100
#else	
	regVal = B_PORT_MII;		// Port select mii
#endif
	// jumbo frame disable // rx own enable // loop back off // retry enable
	// pad crc strip disable // back off limit set 0 // deferral check disable
	pGmacReg->configuration = regVal; 
	DPRINTK_GMAC ("configuration set %08x\n", regVal);

// frame filter disable
#if 0	
//	regVal = B_RX_ALL;
	// set pass control -> GmacPassControl 0 // broadcast enable
	// src addr filter disable // multicast disable  ????  // promisc disable
	// unicast hash table filter disable
//	pGmacReg->frameFilter = regVal;	
	DPRINTK_GMAC ("frameFilter set %08x\n", regVal);
#endif 

	spin_unlock_irqrestore(&pGmacDev->lock, flags);
	
	DPRINTK_GMAC_FLOW ("exit\n");
}



int
sdpGmac_setRxQptr(SDP_GMAC_DEV_T* pGmacDev, const DMA_DESC_T* pDmaDesc, const u32 length2)
{
	int retVal = SDP_GMAC_OK;

	u32 rxNext = pGmacDev->rxNext;
	DMA_DESC_T* pRxDesc = pGmacDev->pRxDescNext;
	u32 length;
	
	unsigned long flag;


	DPRINTK_GMAC_FLOW("called \n");

	if(!sdpGmac_descEmpty(pRxDesc)) { // should empty
		DPRINTK_GMAC_FLOW("Error Not Empty \n");
		retVal =  -SDP_GMAC_ERR;
		goto __setRxQptr_out;
	}

	spin_lock_irqsave(&pGmacDev->lock, flag);

	if(sdpGmac_rxDescChained(pRxDesc)){
		// TODO
		PRINTK_GMAC("rx chained desc Not Implenedted yet\n");
		retVal =  -SDP_GMAC_ERR;
		goto __setRxQptr_out;

	} else {
		length = (pDmaDesc->length & DESC_SIZE_MASK);
		length |= (length2 & DESC_SIZE_MASK) << DESC_SIZE2_SHIFT;

		if(sdpGmac_lastRxDesc(pGmacDev, pRxDesc)) {
			length |= RX_DESC_END_RING;
		} 
// set descriptor
		pRxDesc->buffer1 = pDmaDesc->buffer1;
		pRxDesc->data1 = pDmaDesc->data1;

		pRxDesc->buffer2 = pDmaDesc->buffer2;
		pRxDesc->data2 = pDmaDesc->data2;

		pRxDesc->length = length;
		pRxDesc->status = DESC_OWN_BY_DMA; 	

		if(length & RX_DESC_END_RING) {
			DPRINTK_GMAC_FLOW ("Set Last RX Desc no: %d length : 0x%08x\n", 
					pGmacDev->rxNext , pRxDesc->length);
			pGmacDev->rxNext = 0;  
			pGmacDev->pRxDescNext = pGmacDev->pRxDesc;
		}
		else {
			pGmacDev->rxNext = rxNext + 1;
			pGmacDev->pRxDescNext = pRxDesc + 1;
		}

	}

	spin_unlock_irqrestore(&pGmacDev->lock, flag);

	retVal = rxNext;
// if set rx qptr is success, print status
	DPRINTK_GMAC_FLOW("rxNext: %02d  pRxDesc: %08x status: %08x \n"
		      "length: %08x  buffer1: %08x buffer2: %08x \n"
		      "data1: %08x  data2: %08x\n",
		      rxNext,  (u32)pRxDesc, pRxDesc->status, 
		      pRxDesc->length, pRxDesc->buffer1, pRxDesc->buffer2,
		      pRxDesc->data1, pRxDesc->data2);
__setRxQptr_out:

	DPRINTK_GMAC("exit\n");
	return retVal;
}


// return rxIndex
int
sdpGmac_getRxQptr(SDP_GMAC_DEV_T* pGmacDev, DMA_DESC_T *pDmaDesc)
{
	int retVal = SDP_GMAC_OK;	

	u32 rxBusy = pGmacDev->rxBusy;
	DMA_DESC_T *pRxDesc = pGmacDev->pRxDescBusy;

	unsigned long flag;

	DPRINTK_GMAC_FLOW("called\n");
	
	if(pRxDesc->status & DESC_OWN_BY_DMA) {
		DPRINTK_GMAC_FLOW("rx desc %d status is desc own by dma\n", rxBusy);
		retVal = -SDP_GMAC_ERR;	
		goto __getRxQptr_out;
	}
	
	if(sdpGmac_descEmpty(pRxDesc)) {
		DPRINTK_GMAC_FLOW("rx desc %d status empty\n", rxBusy);
		retVal = -SDP_GMAC_ERR;	
		goto __getRxQptr_out;
	}

	*pDmaDesc = *pRxDesc; // get Descriptor data	


	spin_lock_irqsave(&pGmacDev->lock, flag);

	if(sdpGmac_rxDescChained(pRxDesc)){
		PRINTK_GMAC("rx chained desc Not Implenedted yet\n");
		retVal =  -SDP_GMAC_ERR;
		goto __getRxQptr_out;
	}
	else {
		if(sdpGmac_lastRxDesc(pGmacDev, pRxDesc)) {
			DPRINTK_GMAC_FLOW ("Last RX Desc\n");
			pRxDesc->status = 0;

			pRxDesc->data1 = 0;
			pRxDesc->buffer1 = 0;
			pRxDesc->data2 = 0;
			pRxDesc->buffer2 = 0;

			pRxDesc->length = RX_DESC_END_RING;  // caution !!!!

			pGmacDev->rxBusy = 0;
			pGmacDev->pRxDescBusy = pGmacDev->pRxDesc;

		}
		else {
			memset(pRxDesc, 0, sizeof(DMA_DESC_T));
			pGmacDev->rxBusy = rxBusy + 1;
			pGmacDev->pRxDescBusy = pRxDesc + 1;
		}
	}

	spin_unlock_irqrestore(&pGmacDev->lock, flag);

	retVal = rxBusy;

// if set rx qptr is success, print status
	DPRINTK_GMAC_FLOW("rxBusy: %02d  pRxDesc: %08x status: %08x \n"
		      "length: %08x  buffer1: %08x buffer2: %08x \n"
		      "data1: %08x  data2: %08x\n",
		      rxBusy,  (u32)pRxDesc, pRxDesc->status, 
		      pRxDesc->length, pRxDesc->buffer1, pRxDesc->buffer2,
		      pRxDesc->data1, pRxDesc->data2);

__getRxQptr_out:
	DPRINTK_GMAC("exit\n");

	return retVal;
}

int
sdpGmac_setTxQptr(SDP_GMAC_DEV_T* pGmacDev, 
		const DMA_DESC_T* pDmaDesc, const u32 length2, const int offloadNeeded)
{
	int retVal = SDP_GMAC_OK;

	u32 txNext = pGmacDev->txNext;
	DMA_DESC_T *pTxDesc = pGmacDev->pTxDescNext;
	u32 length;
	u32 status;

	unsigned long flag;
	

	DPRINTK_GMAC_FLOW("called\n");

	if(!sdpGmac_descEmpty(pTxDesc)){
		retVal =  -SDP_GMAC_ERR;
		goto __setTxQptr_out;
	}

	spin_lock_irqsave(&pGmacDev->lock, flag);

	if(sdpGmac_txDescChained(pTxDesc)) {
		// TODO: chained 
		PRINTK_GMAC("tx chained desc Not Implenedted yet\n");
		retVal =  -SDP_GMAC_ERR;
		goto __setTxQptr_out;
	}
	else {
		length = (pDmaDesc->length & DESC_SIZE_MASK);
		length |= (length2 & DESC_SIZE_MASK) << DESC_SIZE2_SHIFT;

		status = DESC_OWN_BY_DMA | DESC_TX_1ST | DESC_TX_LAST | DESC_TX_INT_EN;	// status
	
		if (offloadNeeded) {
			PRINTK_GMAC("H/W Checksum Not Implenedted yet\n");
		}		

		if(sdpGmac_lastTxDesc(pGmacDev, pTxDesc)) {
			pGmacDev->txNext = 0;
			pGmacDev->pTxDescNext = pGmacDev->pTxDesc;
			status |= DESC_TX_END_RING;
		}
		else {
			pGmacDev->txNext = txNext + 1;
			pGmacDev->pTxDescNext = pTxDesc + 1;
		}

		pTxDesc->buffer1 = pDmaDesc->buffer1;
		pTxDesc->data1	= pDmaDesc->data1;
		pTxDesc->buffer2 = pDmaDesc->buffer2;
		pTxDesc->data2	= pDmaDesc->data2;

		pTxDesc->status = status;
		pTxDesc->length = length;	// length

		// ENH DESC
	}
	
	spin_unlock_irqrestore(&pGmacDev->lock, flag);

// if set tx qptr is success, print status
	DPRINTK_GMAC_FLOW("txNext: %02d  pTxDesc: %08x status: %08x \n"
		      "length: %08x  buffer1: %08x buffer2: %08x \n"
		      "data1: %08x  data2: %08x\n",
		      txNext,  (u32)pTxDesc, pTxDesc->status, 
		      pTxDesc->length, pTxDesc->buffer1, pTxDesc->buffer2,
		      pTxDesc->data1, pTxDesc->data2);


	retVal = txNext;	

__setTxQptr_out:
	DPRINTK_GMAC("exit\n");
	return retVal;
}


// Read 
int
sdpGmac_getTxQptr(SDP_GMAC_DEV_T* pGmacDev, DMA_DESC_T *pDmaDesc)
{
	int retVal = SDP_GMAC_OK;	

	u32 txBusy = pGmacDev->txBusy;
	DMA_DESC_T *pTxDesc = pGmacDev->pTxDescBusy;
	unsigned long flag;

	DPRINTK_GMAC_FLOW ("called\n");
	
	if(pTxDesc->status & DESC_OWN_BY_DMA) {
		DPRINTK_GMAC_FLOW("Err: DESC_OWN_BY_DMA\n");
		retVal = -SDP_GMAC_ERR;	
		goto __getTxQptr_out;
	}
	
	if(sdpGmac_descEmpty(pTxDesc)) {
		DPRINTK_GMAC_FLOW("Err: Desc Empty\n");
		retVal = -SDP_GMAC_ERR;	
		goto __getTxQptr_out;
	}

	*pDmaDesc = *pTxDesc; // get Descriptor data	

	spin_lock_irqsave(&pGmacDev->lock,flag);

	if(sdpGmac_txDescChained(pTxDesc)){
		PRINTK_GMAC("tx chained desc Not Implenedted yet\n");
		retVal =  -SDP_GMAC_ERR;
		goto __getTxQptr_out;
	}
	else {

		if(sdpGmac_lastTxDesc(pGmacDev, pTxDesc)) {
			DPRINTK_GMAC_FLOW ("Get desc last Tx desc\n");
			pGmacDev->txBusy = 0;
			pGmacDev->pTxDescBusy = pGmacDev->pTxDesc;
			pTxDesc->status = TX_DESC_END_RING;
		}
		else {
			pGmacDev->txBusy = txBusy + 1;
			pGmacDev->pTxDescBusy = pTxDesc + 1;
			pTxDesc->status = 0;
		}

		pTxDesc->length = 0;
		pTxDesc->data1 = 0;
		pTxDesc->buffer1 = 0;
		pTxDesc->data2 = 0;
		pTxDesc->buffer2 = 0;
	}

	spin_unlock_irqrestore(&pGmacDev->lock,flag);

	retVal = txBusy;

// if set rx qptr is success, print status
	DPRINTK_GMAC_FLOW("txBusy: %02d  pTxDesc: %08x status: %08x \n"
		      "length: %08x  buffer1: %08x buffer2: %08x \n"
		      "data1: %08x  data2: %08x\n",
		      txBusy,  (u32)pTxDesc, pTxDesc->status, 
		      pTxDesc->length, pTxDesc->buffer1, pTxDesc->buffer2,
		      pTxDesc->data1, pTxDesc->data2);

__getTxQptr_out:
	DPRINTK_GMAC_FLOW("exit\n");

	return retVal;
}


void sdpGmac_clearAllDesc(struct net_device *pNetDev)
{

	SDP_GMAC_DEV_T *pGmacDev = netdev_priv(pNetDev);
	DMA_DESC_T *pDmaDesc;
	u32 length2;
	int i;

// RX DESC Clear
	pDmaDesc = pGmacDev->pRxDesc;

	for(i = 0; i < NUM_RX_DESC; i ++){
		length2 = DESC_GET_SIZE2(pDmaDesc->length);
		pDmaDesc->length &= (DESC_SIZE1_MASK | RX_DESC_END_RING);

		if((pDmaDesc->length) && (pDmaDesc->data1)){
			DPRINTK_GMAC("rx desc %d clear length %x, data1 %x\n", 
				 i, pDmaDesc->length, pDmaDesc->data1);
			dma_unmap_single(pGmacDev->pDev, pDmaDesc->buffer1, 
					0, DMA_FROM_DEVICE);
			dev_kfree_skb((struct sk_buff*) pDmaDesc->data1);
			
			pDmaDesc->length &= RX_DESC_END_RING;			
		}

		if((length2) && (pDmaDesc->data2)){
			DPRINTK_GMAC("rx desc2 %d clear length2 %x, data2 %x\n", 
				 i, length2, pDmaDesc->data2);

			dma_unmap_single(pGmacDev->pDev, pDmaDesc->buffer2, 
					0, DMA_FROM_DEVICE);
			dev_kfree_skb((struct sk_buff*) pDmaDesc->data2);
		}

		pDmaDesc++;
			
	}

	pGmacDev->pRxDesc[NUM_RX_DESC-1].length = RX_DESC_END_RING;
	DPRINTK_GMAC_FLOW("last rx desc length is 0x%08x\n", pGmacDev->pRxDesc[NUM_RX_DESC-1].length);

	pGmacDev->rxNext = 0;
	pGmacDev->rxBusy = 0;
	pGmacDev->pRxDescBusy = pGmacDev->pRxDesc;
	pGmacDev->pRxDescNext = pGmacDev->pRxDesc;

// TX DESC Clear
	pDmaDesc = pGmacDev->pTxDesc;

	for(i = 0; i < NUM_RX_DESC; i ++){
		length2 = DESC_GET_SIZE2(pDmaDesc->length);
		pDmaDesc->length &= DESC_SIZE1_MASK;

		if((pDmaDesc->length) && (pDmaDesc->data1)){
			DPRINTK_GMAC("tx desc %d clear length %x, data1 %x\n", 
				 i, pDmaDesc->length, pDmaDesc->data1);

			dma_unmap_single(pGmacDev->pDev, pDmaDesc->buffer1, 
					0, DMA_TO_DEVICE);
			dev_kfree_skb((struct sk_buff*) pDmaDesc->data1);
			pDmaDesc->length = 0;

			pDmaDesc->status &= TX_DESC_END_RING;
		}

		if((length2) && (pDmaDesc->data2)){
			DPRINTK_GMAC("tx desc2 %d clear length2 %x, data2 %x\n", 
				 i, length2, pDmaDesc->data2);

			dma_unmap_single(pGmacDev->pDev, pDmaDesc->buffer2, 
					0, DMA_TO_DEVICE);
			dev_kfree_skb((struct sk_buff*) pDmaDesc->data2);
		}

		pDmaDesc++;
	}
	
	pGmacDev->txNext = 0;
	pGmacDev->txBusy = 0;
	pGmacDev->pTxDescBusy = pGmacDev->pTxDesc;
	pGmacDev->pTxDescNext = pGmacDev->pTxDesc;

	return;
}





