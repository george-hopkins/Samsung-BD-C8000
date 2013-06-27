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
 
#ifndef _PTP_DEVICE_LIST_H_ 
#define _PTP_DEVICE_LIST_H_
#include "Singleton.h"
#include <map>
#include <list>


extern "C" { 
#include "usb.h"   
#include "ptpapi.h"
    struct usb_device;
}


//========================================================================
class CPTPDeviceListUpdate : 
       public CSingleton<CPTPDeviceListUpdate>
{
    
public:
    	int Initialize(void);
    	int Finalize(void);

		int GetPTPDeviceList( uint32_t a_Size, UsbDev** a_pDevpathList, uint32_t *a_Dev_cnt);

    	void OnDeviceChange(struct usb_device* device, bool isRemoved);
    				  
    	void MakeUsbDevStr(UsbDev* ptpDevice);
  
		void PrintDeviceList(void);

		void GetPTPDeviceContext(PTPDeviceHandle ptpHandle, PTPDevContext** pDC);
    
private:
	CPTPDeviceListUpdate();
	~CPTPDeviceListUpdate();
	friend class CSingleton<CPTPDeviceListUpdate>;

    //callback to be registered with usb
    static void callback_on_device_change(struct usb_device* dev, int is_removed);
    
private :
	std::map<UsbDev*, PTPDevContext*>	 m_pPTPDevPathCtxMap;
	typedef std::map<UsbDev*, PTPDevContext*>::iterator devpath_iter;
	uint16 m_available_ids[16];
};




#endif // _PTP_DEVICE_LIST_H_

