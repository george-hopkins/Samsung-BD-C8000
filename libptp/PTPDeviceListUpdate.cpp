/*
* Copyright (C) 2003-2005 Samsung Electronics Co., Ltd. All rights reserved.
*
* Shadow is a trademark of Samsung Electronics Co., Ltd.
*
* Software Center, Systems R&D Laboratories
* Corporate Technology Operations, Samsung Electronics Co., Ltd.
*
* The Shadow software and its documentation are confidential and proprietary
* information of Samsung Electronics Co., Ltd.  No part of the software and
* documents may be copied, reproduced, transmitted, translated, or reduced to
* any electronic medium or machine-readable form without the prior written
* consent of Samsung Electronics.
*
* Samsung Electronics makes no representations with respect to the contents,
* and assumes no responsibility for any errors that might appear in the
* software and documents. This publication and the contents hereof are subject
* to change without notice.
*/

/**
* @class		CPTPDeviceListUpdate
* @brief		USB device manager
* @author	    Sheetal(SISC) and Niraj(SISC)
* @Date	    	August, 2008
*/



#include "PTPDeviceListUpdate.h"
#include "usb.h"
#include "ptpapi.h"

#include <string.h>
#include <stdio.h>
#include <iostream>
#include <pthread.h>
using namespace std;


static pthread_mutex_t __m_mutex = PTHREAD_MUTEX_INITIALIZER;

#define MUTEX_BEGIN		pthread_mutex_lock(&__m_mutex)
#define MUTEX_END		pthread_mutex_unlock(&__m_mutex)


/**
* @brief         CPTPDeviceListUpdate - Constructor.
* @remarks       
* @param         void
* @return        none.
* @see           none
*/

CPTPDeviceListUpdate::CPTPDeviceListUpdate()
{
	for (int i=0; i <16;i++)
		m_available_ids[i] = 0;
}


/**
* @brief         CPTPDeviceListUpdate - Destructor.
* @remarks       
* @param         void
* @return        none
* @see           none
*/

CPTPDeviceListUpdate::~CPTPDeviceListUpdate()
{
	
}


/**
* @brief         This function initialises the PTP device.
* @remarks       
* @param         void
* @return        error code
*					-1 : in case of error.
*					 0 : in case of success.
* @see           none
*/

int CPTPDeviceListUpdate::Initialize(void)
{
	//set callback to receive device events
	usb_set_callback(callback_on_device_change);	
	
	return 0;    
}


/**
* @brief         This function stops the polling thread.
* @remarks       
* @param         void
* @return        error code
*					-1 : in case of error.
*					 0 : in case of success.
* @see           none
*/

int CPTPDeviceListUpdate::Finalize(void)
{
    //remove the callback
    usb_set_callback(NULL);
    
	return 0;    
}


/**
* @brief         This function fetches the current Device List in the map.
* @remarks       * @param		 unit32_t a_size:	maximum number of devices in the list
* 			     UsbDev* a_pDevpathList[]:		Device List
* 			     uint32_t *a_Dev_cnt:		number of Devices in the list
* @return        error code
*					-1 : in case of error.
*					 0 : in case of success.
* @see           none*/

int CPTPDeviceListUpdate::GetPTPDeviceList( uint32_t a_Size, UsbDev* a_pDevpathList[],uint32_t *a_Dev_cnt)
{
	int result = 0;
	uint32_t maxSize = a_Size;
	uint32_t count = 0;
	
    if((a_Size == 0) || (a_pDevpathList == 0))
		return -1;


	MUTEX_BEGIN;   

	for(devpath_iter MapItor = m_pPTPDevPathCtxMap.begin(); MapItor != m_pPTPDevPathCtxMap.end(); ++MapItor)
	{
		if(count == maxSize) 
		{
			result = -1; 
			break;
		}
			
		a_pDevpathList[count]=(*MapItor).first;	
		count++;
	}

	*a_Dev_cnt = count;
	
	MUTEX_END;

	return result;
}


/**
* @brief         callback_on_device_change.
* @remarks       
* @param         usb_device, device being connected/disconnected
*					isRemoved - 1 for removed; 0 for attached
* @return        void.
* @see           none
*/

void CPTPDeviceListUpdate::callback_on_device_change(struct usb_device* device, int isRemoved)
{
    CPTPDeviceListUpdate::Get().OnDeviceChange(device,(isRemoved == 1));
}                                                 



/**
* @brief         This function is called if there is any change in number of connected devices in libusb.
* @remarks       
* @param         usb_device* - device being attached or removed
*					isRemoved - 1 for removed; 0 for attached
* @return        void
* @see           none
*/

void CPTPDeviceListUpdate::OnDeviceChange(struct usb_device* device, bool isRemoved)
{

	// NULL check, because somtimes usbfs was crashed by on-chip usb device 
	if(device == NULL ||device->config == NULL 
		|| device->config->interface == NULL 
		|| device->config->interface->altsetting == NULL )
	{
		printf(" usb_device's address is NULL \n");
		return;
	}
	
	//A device is removed
	if (device->config->interface->altsetting->bInterfaceClass == USB_CLASS_PTP)
	{
		if(isRemoved == 1) 
		{        
			// extract the dc giving the dev & update both maps deleting corresponding dc.
			UsbDev ptpDevice;

			ptpDevice.ptp_path.bus_num=atoi(device->bus->dirname);
			ptpDevice.ptp_path.dev_num=(int)device->devnum;
			
			MUTEX_BEGIN;
			
			for(devpath_iter MapItor = m_pPTPDevPathCtxMap.begin(); MapItor != m_pPTPDevPathCtxMap.end(); ++MapItor)
			{
				
				UsbDev* ptpDevice_map = (*MapItor).first;//to check whether there is a first field

				if ((ptpDevice_map->ptp_path.dev_num == ptpDevice.ptp_path.dev_num) && (ptpDevice_map->ptp_path.bus_num == ptpDevice.ptp_path.bus_num))
				{
					m_available_ids[ptpDevice_map->DeviceId - 16] = 0;
					m_pPTPDevPathCtxMap.erase (MapItor);	
					//PTP_API_Clear_PTPStorageInfo(&(ptpDevice_map->storageinfo[0]));
					//PTP_API_Clear_StorageIDs(&(ptpDevice_map->sids));
					delete ptpDevice_map;

					break;
				}
			}
			MUTEX_END;
			
			///////////////////////////////////////////////////
		}
    
		//A new device has arrived
		else 
		{
			// make a new dc giving the dev & update both maps adding corresponding dc.
			PTPDevContext* pNewDC = new PTPDevContext;
		
			pNewDC->dev = device;
			pNewDC->bus = device->bus;
			UsbDev *ptpDevice=new UsbDev;
			ptpDevice->ptp_path.bus_num=atoi(device->bus->dirname);
			ptpDevice->ptp_path.dev_num=(int)device->devnum;

			for (int i=0; i <16;i++)
			{
				if (m_available_ids[i] == 0)
				{
					ptpDevice->DeviceId = i+16;
					m_available_ids[i] = 1;
					break;
				}
			}
		
			MUTEX_BEGIN;
			// Assign DevPath value...
 			m_pPTPDevPathCtxMap[ptpDevice]= pNewDC;
			MUTEX_END;

			MakeUsbDevStr(ptpDevice);	//updete the fields of UsbDev *ptpdevice
	
		}			
		PrintDeviceList();	// for checking list of connected devices; can be removed if not required
	}
}



/**
* @brief         This function updates the fields of UsbDev structure in the map
* @remarks       
* @param         UsbDev *ptpDevice - pointer to the UsbDev structure which is being updated

* @return        void
* @see           none
*/

void CPTPDeviceListUpdate::MakeUsbDevStr(UsbDev *ptpDevice)
{
	#define PTP_DEFAULT_MODEL_NAME		"Digital Camera"	
	#define PTP_DEFAULT_VENDOR_NAME		"PTP Device"
	#define PTP_DEFAULT_SERIAL_NAME		"123456789012345678901234567890"
	PTPDeviceInfo deviceinfo;
	PTPStorageIDs storageids;
	uint16 nResult;

	
	if ((nResult = PTP_API_INIT_COMM_MPTP (ptpDevice->ptp_path)) != PTP_RC_OK)
	{       
		//error handling		
		if (nResult==0X2ff || nResult== 0x2fd || nResult ==0x2fe)
				printf("\ncamera could not be intialised due to IO error\nPlease restart the camera");
		else 
		 		printf("\nPTP_API_INIT_COMM_MPTP failed\n");

		//erasing from map
		MUTEX_BEGIN;
		for(devpath_iter MapItor = m_pPTPDevPathCtxMap.begin(); MapItor != m_pPTPDevPathCtxMap.end(); ++MapItor)
		{
		
			UsbDev* ptpDevice_map = (*MapItor).first;//to check whether there is a first field
			if(ptpDevice_map == NULL)
					continue;

			if (ptpDevice_map->ptp_path.dev_num==ptpDevice->ptp_path.dev_num && ptpDevice_map->ptp_path.bus_num==ptpDevice->ptp_path.bus_num)
			{
				m_available_ids[ptpDevice_map->DeviceId - 16] = 0;
				m_pPTPDevPathCtxMap.erase (MapItor);
				break;
			}
		}
		MUTEX_END;
		return; 			
	}
////
	PTP_API_Init_DeviceInfo( &deviceinfo );
	nResult = PTP_API_Get_DeviceInfo_MPTP( &deviceinfo, (ptpDevice->ptp_path));
				
	if(nResult == PTP_RC_OK)
	{
		//printf( "Vendor = %s\n",deviceinfo.Manufacturer);
		if(deviceinfo.Manufacturer)
		{
			strcpy(ptpDevice->vendor,deviceinfo.Manufacturer);			
		}
		else	
		{
			strcpy (ptpDevice->vendor, PTP_DEFAULT_VENDOR_NAME);			
		}

		if(deviceinfo.Model)
		{
			strcpy(ptpDevice->model, deviceinfo.Model);
		}
		else	
		{
			//Panasonic DMC-FX12ŽÂ PTP DeviceInfo·Î  model(product)ží readœÃ NULL ž®ÅÏÇÏ¹Ç·Î ¿¹¿ÜÃ³ž®, by kks
			strcpy (ptpDevice->model, PTP_DEFAULT_MODEL_NAME);			
		}

		if(deviceinfo.SerialNumber)
		{
			strcpy(ptpDevice->SerialNumber, deviceinfo.SerialNumber);
		}
		else
		{
			strcpy (ptpDevice->SerialNumber,PTP_DEFAULT_SERIAL_NAME);
		}
		

	}
	else
	{
		printf("\n	PTP_API_Get_DeviceInfo_MPTP Failed\r\n");
		printf("\ncamera connection error \nplease restart the camera\n"); 

		// erase from map
		MUTEX_BEGIN;
		for(devpath_iter MapItor = m_pPTPDevPathCtxMap.begin(); MapItor != m_pPTPDevPathCtxMap.end(); ++MapItor)
		{
		
			UsbDev* ptpDevice_map = (*MapItor).first;
			if(ptpDevice_map == NULL)
					continue;

			if (ptpDevice_map->ptp_path.dev_num==ptpDevice->ptp_path.dev_num && ptpDevice_map->ptp_path.bus_num==ptpDevice->ptp_path.bus_num)
			{
				m_available_ids[ptpDevice_map->DeviceId - 16] = 0;
				m_pPTPDevPathCtxMap.erase (MapItor);
				break;
			}
		}
		MUTEX_END;
		return;
	}
	PTP_API_Clear_DeviceInfo(&deviceinfo);
////

	PTP_API_Init_StorageIDs(&storageids);
	
	if( (PTP_API_Get_StorageIDs_MPTP((ptpDevice->ptp_path),&storageids)) == PTP_RC_OK)
	{
		for(uint32_t i=0;i<storageids.n && i<2;i++)
		{	
			ptpDevice->storage_id[i]=storageids.Storage[i];

		}
		ptpDevice->sids = storageids;
		PTPStorageInfo 	storageinfo[2];
		
		for(uint32_t index=0;index<storageids.n && index<2;index++)
		{
			PTP_API_Init_PTPStorageInfo(&(storageinfo[index]));
			
			if(PTP_API_Get_StorageInfos_MPTP(ptpDevice->ptp_path,storageids.Storage[index],&(storageinfo[index])) == PTP_RC_OK)
			{
				ptpDevice->storageinfo[index]=storageinfo[index];				
			}
			else
			{
				printf("\n	PTP_API_Get_StorageInfos_MPTP failed for %x\r\n",storageids.Storage[index]);
			}		
		}		
	}
	else
		printf("\n	PTP_API_Get_StorageIDs_MPTP Failed\r\n");
	
////		
	return;	
}



/**
* @brief         This function prints the device list in OnDeviceChange().
* @remarks       This function is used for checking connection & disconnection of PTP devices
* @param         void
* @return        void
* @see           
*/

void CPTPDeviceListUpdate::PrintDeviceList( void )
{
	uint32_t cnt = 0;
	devpath_iter MapItor; 

	MUTEX_BEGIN;   

	printf("\n ================= DEV LIST =====================\n");

	for(MapItor = m_pPTPDevPathCtxMap.begin(); MapItor != m_pPTPDevPathCtxMap.end(); ++MapItor)
	{
		UsbDev *ptpDevice=(*MapItor).first;

		if (ptpDevice)
		{
			if(ptpDevice->vendor)
			{
				printf("#%d. Device Man. %s \t", cnt++,  ptpDevice->vendor );
			}

			if(ptpDevice->model)
			{
				printf("Device Model. %s", ptpDevice->model);
			}

			printf(" \tBusNum.%d  DevNum.%d",ptpDevice->ptp_path.bus_num,ptpDevice->ptp_path.dev_num);
			printf("\n");	
		}
	}

	MUTEX_END; 

	printf("======================================\n");
	return;
}


/**
* @brief         This function maps the ptpHandle of a ptp device to its DeviceContext
* @remarks       pDC is set to NULL if the device is not in the list
* @param         PTPDeviceHandle ptpHandle - ptpHandle of the camera  
* 				 PTPDevContext** pDC  -  Device Context of the camera
* @return        void
* @see           
*/

void CPTPDeviceListUpdate::GetPTPDeviceContext(PTPDeviceHandle ptpHandle, PTPDevContext** pDC)
{
	PTPDevContext *dc = NULL;

	MUTEX_BEGIN;
		
	for(devpath_iter MapItor = m_pPTPDevPathCtxMap.begin(); MapItor != m_pPTPDevPathCtxMap.end(); ++MapItor)
	{
		UsbDev* ptpDevice = (*MapItor).first;
	
		if ((ptpDevice->ptp_path.dev_num == ptpHandle.dev_num) && (ptpDevice->ptp_path.bus_num==ptpHandle.bus_num))
		{
			dc=(*MapItor).second;
			break;
		}	
	}

	MUTEX_END;

	*pDC=dc;

	return;
}


