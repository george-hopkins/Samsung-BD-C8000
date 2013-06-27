/**
*  @mainpage PTP project
*  @section Description
* - º» °úÁ¦´Â WiseLink¿¡¼­ Sony¿Í Canon°ú °°ÀÌ PTP¸¦ Áö¿øÇÏ´Â µðÁöÅÐ Ä«¸Þ¶ó¿¡¼­ 
	jpeg & thumbnail imageµéÀ» °¡Á®¿À±â À§ÇØ PTP API¸¦ ±¸ÇöÇÑ´Ù.
*  @section Information
*  - ÀÛ ¼º ÀÚ : ÀÌ½ÂÈÆ Ã¥ÀÓ(VD/SW1)
*  - ÀÛ¼ºÀÏÀÚ : 2006/04/14
*  @section MODIFYINFO
* - Author	: SANG-U, SHIM (VD)
* - DATE	: Nov 2006
*/

/** 
* @File Description
* @file name		ptpapi.c
* @brief			implementation of apis for PTP
  * @author			ÀÌ½ÂÈÆ Ã¥ÀÓ(VD/SW1)
* @DATE			2006/04/14
* @MODIFYINFO	      Modified for Linux 2.6
*					Sheetal,SISC and Niraj,SISC - Modified for supporting multiple PTP devices at the same time.
* @Author               Sang U Shim (VD)
* @Date		      Nov 2006
*/
/***********************************************************************/
/* Header Files */
/***********************************************************************/

#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <sys/times.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <pthread.h>
#include <errno.h>
#include <unistd.h>
#include <stdio.h>
#include <stdarg.h>
#include <pthread.h>
#include "ptpapi.h"

#include "PTPDeviceListUpdate.h"

// global variables
static pthread_mutex_t __mutex_avh = PTHREAD_MUTEX_INITIALIZER; /*Function entry critical section*/

///////////////////////////////////////////////
//Global dc : Using in PTP API
PTPDevContext gdc;
///////////////////////////////////////////////
static PTPDevContext *getDc(PTPDeviceHandle  ptpHandle);
///////////////////////////////////////////////
//added by sheetal -start
//#define MAX_PTP_DEV 5

#define ENABLE_MPTP_DEBUG_PRINT 0
#define ENABLE_MPTP_INFO_PRINT 1

//added by sheetal - end

#define ENABLE_MPTP_PRINT 0 //VISHAL
//#define	ENDIAN_PATCH_BY_SISC	1 /* 20090724  SISC by Ajeet*/
///////////////////////////////////////////////

#if defined(ENDIAN_PATCH_BY_SISC)
static inline uint16_t
dtoh16ap (PTPParams *params, unsigned char *a)
{
	uint16_t var=0;
	memcpy(&var, a, 2);
	return ((params->byteorder==PTP_DL_LE) ? var : swap16(var));
}
#endif
/*******************************************************************************
MACRO CONSTANT DEFINITIONS 
*******************************************************************************/  
#if 1
#define PTP_BEGIN		pthread_mutex_lock(&__mutex_avh)
#define PTP_END			pthread_mutex_unlock(&__mutex_avh)
#else
#define PTP_BEGIN
#define PTP_END

#endif

#define PTP_PRINT		printf
//only to test svn 


static PTPDevContext *getDc(PTPDeviceHandle  ptpHandle);

typedef enum PTP_DEBUG_LEVEL_
{
	PTP_DBG_LVL0,
		PTP_DBG_LVL1,
		PTP_DBG_LVL2,
}PTP_DEBUG_LEVEL;

PTP_DEBUG_LEVEL	PTP_Debug_Level = PTP_DBG_LVL0;



///////////////////////////////////////////////
//added by sheetal -start
void MPTP_Info_Print(const char *fmt, ...)
{
	va_list	    ap;
	
	if (ENABLE_MPTP_INFO_PRINT)
	{
		va_start(ap, fmt);
		vfprintf (stdout, fmt, ap);
		va_end(ap);
	}
}

void MPTP_Debug_Print(const char *fmt, ...)
{
	va_list	    ap;
	
	if (ENABLE_MPTP_DEBUG_PRINT)
	{
		va_start(ap, fmt);
		vfprintf (stdout, fmt, ap);
		va_end(ap);
	}
}
//added by sheetal - end

void MPTP_Print(const char *fmt, ...) //VISHAL
{
	va_list	    ap;
	
	if (ENABLE_MPTP_PRINT)
	{
		va_start(ap, fmt);
		vfprintf (stdout, fmt, ap);
		va_end(ap);
	}
}
///////////////////////////////////////////////


/**
* @brief 
         ³»¿ëÀ» ³ÖÀ¸¼¼¿ä.
* @remarks 
         ³»¿ëÀ» ³ÖÀ¸¼¼¿ä.
* @param
         di ³»¿ëÀ» ³ÖÀ¸¼¼¿ä.
* @return
          ¾øÀ½.
* @see
         ³»¿ëÀ» ³ÖÀ¸¼¼¿ä.
*/
void PTP_Debug_Print(const char *fmt, ...)
{
	va_list	    ap;
	
	if (PTP_Debug_Level >= PTP_DBG_LVL1)
	{
		va_start(ap, fmt);
		vfprintf (stdout, fmt, ap);
		va_end(ap);
	}
}

unsigned long Tick(void)
{
	struct tms buf;
	
	return (unsigned long)(times(&buf) * 10);
	
}


/**
* @brief 
         ³»¿ëÀ» ³ÖÀ¸¼¼¿ä.
* @remarks 
         ³»¿ëÀ» ³ÖÀ¸¼¼¿ä.
* @param
         di ³»¿ëÀ» ³ÖÀ¸¼¼¿ä.
* @return
          ¾øÀ½.
* @see
         ³»¿ëÀ» ³ÖÀ¸¼¼¿ä.
*/
void PTP_API_Init_DeviceInfo(PTPDeviceInfo *di)
{
	di->VendorExtensionDesc = NULL;
	di->OperationsSupported = NULL;
	di->EventsSupported = NULL;
	di->DevicePropertiesSupported = NULL;
	di->CaptureFormats = NULL;
	di->ImageFormats = NULL;
	di->Manufacturer = NULL;
	di->Model = NULL;
	di->DeviceVersion = NULL;
	di->SerialNumber = NULL;
}

/**
* @brief 
         ³»¿ëÀ» ³ÖÀ¸¼¼¿ä.
* @remarks 
         ³»¿ëÀ» ³ÖÀ¸¼¼¿ä.
* @param
         oh ³»¿ëÀ» ³ÖÀ¸¼¼¿ä.
* @return
          ¾øÀ½.
* @see
         ³»¿ëÀ» ³ÖÀ¸¼¼¿ä.
*/
void PTP_API_Init_ObjectHandles(PTPObjectHandles *oh)
{
	oh->n = 0;
	oh->Handler = NULL;
}

/**
* @brief 
         ³»¿ëÀ» ³ÖÀ¸¼¼¿ä.
* @remarks 
         ³»¿ëÀ» ³ÖÀ¸¼¼¿ä.
* @param
         storageids ³»¿ëÀ» ³ÖÀ¸¼¼¿ä.
* @return
          ¾øÀ½.
* @see
         ³»¿ëÀ» ³ÖÀ¸¼¼¿ä.
*/
void PTP_API_Init_StorageIDs(PTPStorageIDs *storageids)
{
	storageids->n = 0;
	storageids->Storage = NULL;
}

/**
* @brief 
         ³»¿ëÀ» ³ÖÀ¸¼¼¿ä.
* @remarks 
         ³»¿ëÀ» ³ÖÀ¸¼¼¿ä.
* @param
         oi ³»¿ëÀ» ³ÖÀ¸¼¼¿ä.
* @return
          ¾øÀ½.
* @see
         ³»¿ëÀ» ³ÖÀ¸¼¼¿ä.
*/
void PTP_API_Init_ObjectInfo(PTPObjectInfo *oi)
{
	oi->Filename = NULL;
	oi->Keywords = NULL;
}

/**
* @brief 
         ³»¿ëÀ» ³ÖÀ¸¼¼¿ä.
* @remarks 
         ³»¿ëÀ» ³ÖÀ¸¼¼¿ä.
* @param
         fileinfo ³»¿ëÀ» ³ÖÀ¸¼¼¿ä.
* @return
          ¾øÀ½.
* @see
Ã¯Â¿Â½Ã¯Â¿Â½Ã¯Â¿Â½Ã¯Â¿Â½Ã¯Â¿Â½ Ã¯Â¿Â½Ã¯Â¿Â½Ã¯Â¿Â½Ã¯Â¿Â½Ã¯Â¿Â½Ã¯Â¿Â½Ã¯Â¿Â½.
*/
void PTP_API_Init_PTPFileListInfo(PTPFileListInfo **fileinfo)
{
	*fileinfo = NULL;
}

/**
* @brief 
         ³»¿ëÀ» ³ÖÀ¸¼¼¿ä.
* @remarks 
         ³»¿ëÀ» ³ÖÀ¸¼¼¿ä.
* @param
         storageInfo ³»¿ëÀ» ³ÖÀ¸¼¼¿ä.
* @return
          ¾øÀ½.
* @see
         ³»¿ëÀ» ³ÖÀ¸¼¼¿ä.
*/
void PTP_API_Init_PTPStorageInfo(PTPStorageInfo *storageInfo)
{
	storageInfo->StorageDescription = NULL;
	storageInfo->VolumeLabel = NULL;
}

/**
* @brief 
         ³»¿ëÀ» ³ÖÀ¸¼¼¿ä.
* @remarks 
         ³»¿ëÀ» ³ÖÀ¸¼¼¿ä.
* @param
         dc ³»¿ëÀ» ³ÖÀ¸¼¼¿ä.
* @return
          ¾øÀ½.
* @see
         ³»¿ëÀ» ³ÖÀ¸¼¼¿ä.
*/
void PTP_Init_Transaction(PTPDevContext *dc)
{
	PTP_API_Init_DeviceInfo (&(dc->params.deviceinfo));
	PTP_API_Init_ObjectHandles (&(dc->params.handles));
	dc->params.objectinfo = NULL;
}

/**
* @brief 
         ³»¿ëÀ» ³ÖÀ¸¼¼¿ä.
* @remarks 
         ³»¿ëÀ» ³ÖÀ¸¼¼¿ä.
* @param
         storageids ³»¿ëÀ» ³ÖÀ¸¼¼¿ä.
* @return
          ¾øÀ½.
* @see
         ³»¿ëÀ» ³ÖÀ¸¼¼¿ä.
*/
void PTP_API_Clear_StorageIDs(PTPStorageIDs *storageids)
{
	storageids->n = 0;
	if (storageids->Storage != NULL)
	{
		free (storageids->Storage);
		storageids->Storage = NULL;
	}
}

/**
* @brief 
         ³»¿ëÀ» ³ÖÀ¸¼¼¿ä.
* @remarks 
         ³»¿ëÀ» ³ÖÀ¸¼¼¿ä.
* @param
         di ³»¿ëÀ» ³ÖÀ¸¼¼¿ä.
* @return
          ¾øÀ½.
* @see
         ³»¿ëÀ» ³ÖÀ¸¼¼¿ä.
*/
void PTP_API_Clear_DeviceInfo(PTPDeviceInfo *di)
{
	if ( di->VendorExtensionDesc != NULL)
	{
		free (di->VendorExtensionDesc);
		di->VendorExtensionDesc = NULL;
	}
	
	if ( di->OperationsSupported != NULL)
	{
		free (di->OperationsSupported);
		di->OperationsSupported = NULL;
	}
	
	if ( di->EventsSupported != NULL)
	{
		free (di->EventsSupported);
		di->EventsSupported = NULL;
	}
	
	if ( di->DevicePropertiesSupported != NULL)
	{
		free (di->DevicePropertiesSupported);
		di->DevicePropertiesSupported = NULL;
	}
	
	if ( di->CaptureFormats != NULL)
	{
		free (di->CaptureFormats);
		di->CaptureFormats = NULL;
	}
	
	if ( di->ImageFormats != NULL)
	{
		free (di->ImageFormats);
		di->ImageFormats = NULL;
	}
	
	if ( di->Manufacturer != NULL)
	{
		free (di->Manufacturer);
		di->Manufacturer = NULL;
	}
	
	if ( di->Model != NULL)
	{
		free (di->Model);
		di->Model = NULL;
	}
	
	if ( di->DeviceVersion != NULL)
	{
		free (di->DeviceVersion);
		di->DeviceVersion = NULL;
	}
	
	if ( di->SerialNumber != NULL)
	{
		free (di->SerialNumber);
		di->SerialNumber = NULL;
	}
}

/**
* @brief 
         ³»¿ëÀ» ³ÖÀ¸¼¼¿ä.
* @remarks 
         ³»¿ëÀ» ³ÖÀ¸¼¼¿ä.
* @param
         oh ³»¿ëÀ» ³ÖÀ¸¼¼¿ä.
* @return
          ¾øÀ½.
* @see
         ³»¿ëÀ» ³ÖÀ¸¼¼¿ä.
*/
void PTP_API_Clear_ObjectHandles(PTPObjectHandles *oh)
{

	if  (oh->n > 0)
	{
		free (oh->Handler);
		oh->Handler = NULL;
	}
	
	oh->n = 0;
}

/**
* @brief 
         ³»¿ëÀ» ³ÖÀ¸¼¼¿ä.
* @remarks 
         ³»¿ëÀ» ³ÖÀ¸¼¼¿ä.
* @param
         oi ³»¿ëÀ» ³ÖÀ¸¼¼¿ä.
* @return
          ¾øÀ½.
* @see
         ³»¿ëÀ» ³ÖÀ¸¼¼¿ä.
*/
void PTP_API_Clear_ObjectInfo(PTPObjectInfo *oi)
{
	if (oi == NULL)
	{
		return;
	}
	
	if ( oi->Filename != NULL)
	{
		free (oi->Filename);
		oi->Filename = NULL;
	}
	
	if ( oi->Keywords != NULL)
	{
		free (oi->Keywords);
		oi->Keywords = NULL;
	}
}

/**
* @brief 
         ³»¿ëÀ» ³ÖÀ¸¼¼¿ä.
* @remarks 
         ³»¿ëÀ» ³ÖÀ¸¼¼¿ä.
* @param
         fileinfo ³»¿ëÀ» ³ÖÀ¸¼¼¿ä.
* @return
          ¾øÀ½.
* @see
         ³»¿ëÀ» ³ÖÀ¸¼¼¿ä.
*/
void PTP_API_Clear_PTPFileListInfoInfo(PTPFileListInfo **fileinfo)
{
	if (*fileinfo != NULL)
	{
		free (*fileinfo);
		*fileinfo = NULL;
	}
}

/**
* @brief 
         ³»¿ëÀ» ³ÖÀ¸¼¼¿ä.
* @remarks 
         ³»¿ëÀ» ³ÖÀ¸¼¼¿ä.
* @param
         storageInfo ³»¿ëÀ» ³ÖÀ¸¼¼¿ä.
* @return
Ã¯Â¿Â½Ã¯Â¿Â½Ã¯Â¿Â½.
* @see
         ³»¿ëÀ» ³ÖÀ¸¼¼¿ä.
*/
void PTP_API_Clear_PTPStorageInfo(PTPStorageInfo *storageInfo)
{
	if (storageInfo->StorageDescription != NULL)
	{
		free (storageInfo->StorageDescription);
		storageInfo->StorageDescription = NULL;
	}
	
	if (storageInfo->VolumeLabel != NULL)
	{
		free (storageInfo->VolumeLabel);
		storageInfo->VolumeLabel = NULL;
	}
}

/**
* @brief 
         ³»¿ëÀ» ³ÖÀ¸¼¼¿ä.
* @remarks 
         ³»¿ëÀ» ³ÖÀ¸¼¼¿ä.
* @param
         dc ³»¿ëÀ» ³ÖÀ¸¼¼¿ä.
* @return
          ¾øÀ½.
* @see
         ³»¿ëÀ» ³ÖÀ¸¼¼¿ä.
*/
void PTP_Terminate_Transaction(PTPDevContext *dc)
{
	if (dc->params.data != NULL)
	{
		//free (dc->params.data);
		dc->params.data = NULL;
	}
	
	PTP_API_Clear_DeviceInfo(&dc->params.deviceinfo);
	PTP_API_Clear_ObjectHandles(&dc->params.handles);
	PTP_API_Clear_ObjectInfo(dc->params.objectinfo);
}


/**
*@brief		This function Initialises the PTPDevice list
*@remarks	Initialise and register device handler for PTP with thread monitoring for 
*	 		new PTP device connection or disconnection
*@param		void
*@return	uint16
PTP_RC_OK : success, else : failure
*/
uint16 PTP_API_Init_DeviceList (void)
{
	uint16 result = PTP_RC_Undefined;

	// Initialise and register device handler for PTP with thread monitoring for 
	// new PTP device connection or disconnection
	//
	if (CPTPDeviceListUpdate::Get().Initialize() == 0)
		return PTP_RC_OK;

	return result;

}

/**
*@brief		This function opens the given device and updates the DC
*@ remarks	Endpoints are updated and ports are intialized
*@param		PTPDevContext *dc:	Device context of the given device
*@return		uint16
PTP_RC_OK : success, else : failure
*/
uint16 PTP_Open_Device_MPTP(PTPDevContext *dc)
{
	
	uint16 result = PTP_RC_Undefined;
	Bool bRet = false;
	
	PTP_Debug_Print("PTP_Open_Device : start\n");
	if(dc==NULL)
	{
		PTP_Debug_Print("[ERROR] PTP_Open_Device_MPTP : PTPDevContext is null.\n");
		return PTPAPI_ERROR;//added by Niraj   
	}
	bRet = usb_find_device_MPTP(dc);
	  
	if (bRet == false)
	{
		PTP_Debug_Print("[ERROR] PTP_Open_Device : Can't find devices\n");
		MPTP_Print("[ERROR] PTP_Open_Device : Can't find devices\n");
		return result;
	}
	
	result = usb_find_endpoints(dc);
	if (result != PTP_RC_OK)
	{
		PTP_Debug_Print("[ERROR] PTP_Open_Device : Can't find endpoint (0x%4x)\n", result);
		MPTP_Print("[ERROR] PTP_Open_Device : Can't find endpoint (0x%4x)\n", result);
		return result;
	}
  
	result = usb_initialize_port(dc);
	if (result != PTP_RC_OK)
	{
		PTP_Debug_Print("[ERROR] PTP_Open_Device : Can't open usb port (0x%4x)\n", result);
		MPTP_Print("[ERROR] PTP_Open_Device : Can't open usb port (0x%4x)\n", result);
		return result;
	}
	
	PTP_Debug_Print("PTP_Open_Device : end (ret : %x)\n", result);
  
	return PTP_RC_OK;
}

/**
* @brief 
         ³»¿ëÀ» ³ÖÀ¸¼¼¿ä.
* @remarks 
         ³»¿ëÀ» ³ÖÀ¸¼¼¿ä.
* @param
         dc ³»¿ëÀ» ³ÖÀ¸¼¼¿ä.
* @return
          ³»¿ëÀ» ³ÖÀ¸¼¼¿ä.
* @see
         ³»¿ëÀ» ³ÖÀ¸¼¼¿ä.
*/
 uint16 PTP_Open_Device(PTPDevContext *dc)
 {
#if 0
 	uint16 result = PTP_RC_Undefined;
 	Bool bRet = false;
 	
 	PTP_Debug_Print("PTP_Open_Device : start\n");
 	
 	bRet = usb_find_device(dc);
 	if (bRet == false) 
 	{
 		PTP_Debug_Print("[ERROR] PTP_Open_Device : Can't find devices\n");
 		return result;
 	}
 	
 	result = usb_find_endpoints(dc);
 	if (result != PTP_RC_OK) 
 	{
 		PTP_Debug_Print("[ERROR] PTP_Open_Device : Can't find endpoint (0x%4x)\n", result);
 		return result;
 	}
 	
 	result = usb_initialize_port(dc);
 	if (result != PTP_RC_OK) 
 	{
 		PTP_Debug_Print("[ERROR] PTP_Open_Device : Can't open usb port (0x%4x)\n", result);
 		return result;
 	}
 	
 	PTP_Debug_Print("PTP_Open_Device : end (ret : %x)\n", result);
 	
 	return PTP_RC_OK;
#endif
		
	dc = NULL;//intialising unused variable
	return PTPAPI_ERROR;
 }


/**
* @brief 
         ³»¿ëÀ» ³ÖÀ¸¼¼¿ä.
* @remarks 
         ³»¿ëÀ» ³ÖÀ¸¼¼¿ä.
* @param
         dc ³»¿ëÀ» ³ÖÀ¸¼¼¿ä.
* @return
          ³»¿ëÀ» ³ÖÀ¸¼¼¿ä.
* @see
         ³»¿ëÀ» ³ÖÀ¸¼¼¿ä.
*/
uint16 PTP_Close_Device(PTPDevContext *dc)
{
	uint16	result = PTP_RC_OK;
	int16	nRet = 0;
	
	PTP_Debug_Print ("PTP_Close_Device : start\n");
	
	result = usb_clear_stall(dc);	// defined in libusb
	
	if (dc->dev != NULL)
	{
		usb_release_interface(dc->ptp_usb.handle, \
			dc->dev->config->interface->altsetting->bInterfaceNumber);
	}
	else
	{
		PTP_Debug_Print("[ERROR] PTP_Close_Device : PTPDevContext dev is null.\n");
	}
	
	nRet = usb_close(dc->ptp_usb.handle);	
	
	PTP_Debug_Print ("PTP_Close_Device : end(nRet:0x%x)\n", nRet);
	
	return result;
}

/**
* @brief 
         ³»¿ëÀ» ³ÖÀ¸¼¼¿ä.
* @remarks 
         ³»¿ëÀ» ³ÖÀ¸¼¼¿ä.
* @param
         dc ³»¿ëÀ» ³ÖÀ¸¼¼¿ä.
* @param
         status ³»¿ëÀ» ³ÖÀ¸¼¼¿ä.
* @return
          ³»¿ëÀ» ³ÖÀ¸¼¼¿ä.
* @see
         ³»¿ëÀ» ³ÖÀ¸¼¼¿ä.
*/
uint16 get_usb_endpoint_status(PTPDevContext *dc, uint16 *status)
{
	uint16	result = PTP_RC_OK;
	int16	nRet = 0;
	
	if (dc == NULL)
	{
		PTP_Debug_Print("[ERROR] get_usb_endpoint_status: PTPDevContext dc is null.\n"); 
		return PTP_RC_Undefined;
	}
	
	nRet = usb_control_msg(dc->ptp_usb.handle, USB_DP_DTH|USB_RECIP_ENDPOINT, USB_REQ_GET_STATUS, \
		USB_FEATURE_HALT, dc->ptp_usb.inep, (char *)status, 2, PTP_TIMEOUT);
#if defined(ENDIAN_PATCH_BY_SISC)
	*status = dtoh16ap (&dc->params, (unsigned char *)status);
#endif
	if (nRet < 0)
	{
		PTP_Debug_Print("[ERROR] get_usb_endpoint_status: usb_control_msg return error(%x)\n", nRet); 
		result = PTP_RC_Undefined;
	}
	//else
	{
		if (*status)
		{
			PTP_Debug_Print("get_usb_endpoint_status: usb_control_msg - IN endpoint status(%x)\n", *status); 
			nRet = clear_usb_stall_feature(dc, dc->ptp_usb.inep);
			PTP_Debug_Print("get_usb_endpoint_status: usb_control_msg - IN endpoint Request Clear Feature(%x)\n", nRet);
		}
	}
	
	*status = 0;
	nRet = usb_control_msg(dc->ptp_usb.handle, USB_DP_DTH|USB_RECIP_ENDPOINT, USB_REQ_GET_STATUS,	\
		USB_FEATURE_HALT, dc->ptp_usb.outep, (char *)status, 2, PTP_TIMEOUT);
#if defined(ENDIAN_PATCH_BY_SISC)
	*status = dtoh16ap (&dc->params, (unsigned char *)status);
#endif
	if (nRet < 0)
	{
		PTP_Debug_Print("[ERROR] get_usb_endpoint_status: usb_control_msg return error(%x)\n", nRet); 
		result = PTP_RC_Undefined;
	}
	//else
	{
		if (*status)
		{
			PTP_Debug_Print("get_usb_endpoint_status: usb_control_msg - OUT endpoint status(%x)\n", *status); 
			nRet = clear_usb_stall_feature(dc, dc->ptp_usb.outep);
			PTP_Debug_Print("get_usb_endpoint_status: usb_control_msg - OUT endpoint Request Clear Feature(%x)\n", nRet);
		}
	}
	
	*status = 0;
	nRet = usb_control_msg(dc->ptp_usb.handle, USB_DP_DTH|USB_RECIP_ENDPOINT, USB_REQ_GET_STATUS,	\
		USB_FEATURE_HALT, dc->ptp_usb.intep, (char *)status, 2, PTP_TIMEOUT);
#if defined(ENDIAN_PATCH_BY_SISC)
	*status = dtoh16ap (&dc->params, (unsigned char *)status);
#endif
	if (nRet < 0)
	{
		PTP_Debug_Print("[ERROR] get_usb_endpoint_status: usb_control_msg return error(%x)\n", nRet); 
		result = PTP_RC_Undefined;
	}
	//else
	{
		if (*status)
		{
			PTP_Debug_Print("get_usb_endpoint_status: usb_control_msg - INTERRUPT endpoint status(%x)\n", *status); 
			nRet = clear_usb_stall_feature(dc, dc->ptp_usb.intep);
			PTP_Debug_Print("get_usb_endpoint_status: usb_control_msg - INTERRUPT endpoint Request Clear Feature(%x)\n", nRet);
		}
	}
	
	
	return result;
}

/**
* @brief 
         ³»¿ëÀ» ³ÖÀ¸¼¼¿ä.
* @remarks 
         ³»¿ëÀ» ³ÖÀ¸¼¼¿ä.
* @param
         dc ³»¿ëÀ» ³ÖÀ¸¼¼¿ä.
* @param
         ep ³»¿ëÀ» ³ÖÀ¸¼¼¿ä.
* @return
          ³»¿ëÀ» ³ÖÀ¸¼¼¿ä.
* @see
         ³»¿ëÀ» ³ÖÀ¸¼¼¿ä.
*/
uint16 clear_usb_stall_feature(PTPDevContext *dc, int ep)
{
	uint16	result = PTP_RC_OK;
	int16	nRet = 0;
	
	nRet = usb_control_msg(dc->ptp_usb.handle, USB_RECIP_ENDPOINT, USB_REQ_CLEAR_FEATURE,	\
		USB_FEATURE_HALT, ep, NULL, 0, PTP_TIMEOUT);
	if (nRet < 0)
	{
		result = PTP_RC_Undefined;
	}
	
	return result;
}

/**
* @brief 
         ³»¿ëÀ» ³ÖÀ¸¼¼¿ä.
* @remarks 
         ³»¿ëÀ» ³ÖÀ¸¼¼¿ä.
* @param
         dc ³»¿ëÀ» ³ÖÀ¸¼¼¿ä.
* @param
         device_status ³»¿ëÀ» ³ÖÀ¸¼¼¿ä.
* @return
          ³»¿ëÀ» ³ÖÀ¸¼¼¿ä.
* @see
         ³»¿ëÀ» ³ÖÀ¸¼¼¿ä.
*/
uint16 get_usb_device_status(PTPDevContext *dc, uint16 *device_status)
{
	uint16	result = PTP_RC_OK;
	int16	nRet = 0;

#if defined(ENDIAN_PATCH_BY_SISC)
	unsigned char status[8] = {0,};
	nRet = usb_control_msg(dc->ptp_usb.handle, USB_DP_DTH|USB_TYPE_CLASS|USB_RECIP_INTERFACE, \
		USB_REQ_GET_DEVICE_STATUS, 0, 0, (char *)status, 8, PTP_TIMEOUT);
	*device_status = dtoh16ap (&dc->params, &status[2]);	

	if (nRet < 0)
	{
		result = PTP_RC_Undefined;
	}
	
	PTP_Debug_Print(" get_usb_device_status ===>> 0x%0x\n", *device_status);
#else
	uint16	status[4] = {0,};
	nRet = usb_control_msg(dc->ptp_usb.handle, USB_DP_DTH|USB_TYPE_CLASS|USB_RECIP_INTERFACE, \
		USB_REQ_GET_DEVICE_STATUS, 0, 0, (char *)device_status/*status*/, 4, PTP_TIMEOUT);

	if (nRet < 0)
	{
		result = PTP_RC_Undefined;
	}
	
	PTP_Debug_Print(" get_usb_device_status ===>> 0x%0x\n", *device_status);

	*device_status = status[3];
#endif	
	return result;
}

/**
* @brief 
         ³»¿ëÀ» ³ÖÀ¸¼¼¿ä.
* @remarks 
         ³»¿ëÀ» ³ÖÀ¸¼¼¿ä.
* @param
         dc ³»¿ëÀ» ³ÖÀ¸¼¼¿ä.
* @return
          ³»¿ëÀ» ³ÖÀ¸¼¼¿ä.
* @see
         ³»¿ëÀ» ³ÖÀ¸¼¼¿ä.
*/
uint16 set_usb_device_reset(PTPDevContext *dc)
{
	uint16	result = PTP_RC_OK;
	int16	nRet = 0;
	
	PTP_Debug_Print ("=====>>>set_usb_device_reset use a handle(%d)\n", dc->ptp_usb.handle);
	
	nRet = usb_control_msg(dc->ptp_usb.handle, USB_TYPE_CLASS|USB_RECIP_INTERFACE, \
		USB_REQ_DEVICE_RESET, 0, 0, NULL, 0, PTP_TIMEOUT);
	if (nRet < 0)
	{
		PTP_Debug_Print ("[Error] : set_usb_device_reset : usb_control_msg return (%x)\n", nRet);
		result = PTP_RC_Undefined;
	}
	
	return result;
	
}


/**
*@brief		This function finds  the given device and rechecks the device connnectivity
*@ remarks	bus number, device number and product id are matched
*@param		PTPDevContext *dc:	Device context of the given ptp device
*@return		Bool 
true : device present, false : device absent
*/
Bool  usb_find_device_MPTP (PTPDevContext *dc)
{
	Bool bReturn = false;
	
	char busNum[10],devNum[10];
	
	if (dc == NULL)
	{
		PTP_Debug_Print("[ERROR] usb_find_device : PTPDevContext is null.\n");
		return false;
	}
// 	uint16 ProdID = dc->dev->descriptor.idProduct;
	strncpy(busNum,dc->bus->dirname,10);
	strncpy(devNum,dc->dev->filename,10);
	
	usb_init();
	usb_find_busses();
// 	usb_find_devices();
	dc->bus = usb_get_busses();
	if (dc->bus == NULL)
	{
		PTP_Debug_Print("[ERROR] usb_find_device : dc->bus is null.\n");
		return false;
	}
	
	
	for (;dc->bus; dc->bus = dc->bus->next)
	{
		for (dc->dev = dc->bus->devices; dc->dev; dc->dev = dc->dev->next)
		{
			if ((dc->dev->config->interface->altsetting->bInterfaceClass == USB_CLASS_PTP)&& (dc->dev->descriptor.bDeviceClass!=USB_CLASS_HUB))
			{
				if ((strcmp(busNum,dc->bus->dirname)==0) && (strcmp(devNum,dc->dev->filename)==0) /*&& (dc->dev->descriptor.idProduct == ProdID)*/)
				{
					
					bReturn = true;
					return bReturn;
					
					
				}
			}
		}
	}
	
	return bReturn;
	
}

/**
* @brief 
         ³»¿ëÀ» ³ÖÀ¸¼¼¿ä.
* @remarks 
         ³»¿ëÀ» ³ÖÀ¸¼¼¿ä.
* @param
         dc ³»¿ëÀ» ³ÖÀ¸¼¼¿ä.
* @return
          ³»¿ëÀ» ³ÖÀ¸¼¼¿ä.
* @see
         ³»¿ëÀ» ³ÖÀ¸¼¼¿ä.
*/
 Bool  usb_find_device (PTPDevContext *dc)
 //struct usb_device* usb_find_device (int busn, int devn,PTPDevContext *dc)
 {
#if 0
 	Bool bReturn = false;
 	
 	if (dc == NULL)
 	{
 		PTP_Debug_Print("[ERROR] usb_find_device : PTPDevContext is null.\n");
 		return false;
 	}
 	
 	usb_init();
 	usb_find_busses();
 	usb_find_devices();
 	dc->bus = usb_get_busses();
 	if (dc->bus == NULL)
 	{
 		PTP_Debug_Print("[ERROR] usb_find_device : dc->bus is null.\n");
 		return false;
 	}
 	
 	for (;dc->bus; dc->bus = dc->bus->next)
 	{
 		for (dc->dev = dc->bus->devices; dc->dev; dc->dev = dc->dev->next)
 		{
 			if ((dc->dev->config->interface->altsetting->bInterfaceClass == USB_CLASS_PTP))
 			{
 				if (dc->dev->descriptor.bDeviceClass!=USB_CLASS_HUB)
 				{
 					
 					bReturn = true;
 					return bReturn;
 					
 				}
 			}
 		}
 	}
 	
 	return bReturn;
#endif
	return false;
 }


/**
* @brief 
         ³»¿ëÀ» ³ÖÀ¸¼¼¿ä.
* @remarks 
         ³»¿ëÀ» ³ÖÀ¸¼¼¿ä.
* @param
         dc ³»¿ëÀ» ³ÖÀ¸¼¼¿ä.
* @return
          ³»¿ëÀ» ³ÖÀ¸¼¼¿ä.
* @see
         ³»¿ëÀ» ³ÖÀ¸¼¼¿ä.
*/
uint16 usb_find_endpoints(PTPDevContext *dc)
{
	uint16 nIndex = 0;
	uint16 nNumEp = 0;
	uint16 result = PTP_RC_OK;
	struct usb_endpoint_descriptor *ep = NULL;
	
	if (dc == NULL)
	{
		PTP_Debug_Print("[ERROR] usb_find_endpoints : PTPDevContext is null.\n");
		return PTP_RC_Undefined;
	}
	
	if (dc->dev == NULL)
	{
		PTP_Debug_Print("[ERROR] usb_find_endpoints : usb device is null.\n");
		return PTP_RC_Undefined;
	}
	
	ep = dc->dev->config->interface->altsetting->endpoint;
	if (ep == NULL)
	{
		PTP_Debug_Print("[ERROR] usb_find_endpoints : endpoint descriptor is null.\n");
		return PTP_RC_Undefined;
	}
	
	nNumEp = dc->dev->config->interface->altsetting->bNumEndpoints;
	if (nNumEp < 3 || nNumEp > 3)
	{
		PTP_Debug_Print("[ERROR] usb_find_endpoints : endpoint number (%x) is not matched.\n", nNumEp);
		return PTP_RC_Undefined;
	}
	
	for (nIndex = 0; nIndex < nNumEp; nIndex++)
	{
		switch (ep[nIndex].bmAttributes)
		{
		case USB_ENDPOINT_TYPE_INTERRUPT:
			if ((ep[nIndex].bEndpointAddress & USB_ENDPOINT_DIR_MASK)== USB_ENDPOINT_DIR_MASK)
			{
				dc->ptp_usb.intep = ep[nIndex].bEndpointAddress;
				PTP_Debug_Print("usb_find_endpoints : find a interrupt in endpoint at 0x%02x\n", dc->ptp_usb.intep);
			}
			break;
		case USB_ENDPOINT_TYPE_BULK:
			if ((ep[nIndex].bEndpointAddress & USB_ENDPOINT_DIR_MASK)== USB_ENDPOINT_DIR_MASK)
			{
				dc->ptp_usb.inep = ep[nIndex].bEndpointAddress;
				PTP_Debug_Print("usb_find_endpoints : find a Bulk(IN) in endpoint at 0x%02x\n", dc->ptp_usb.inep);
			}
			else if ((ep[nIndex].bEndpointAddress&USB_ENDPOINT_DIR_MASK)==0)
			{
				dc->ptp_usb.outep = ep[nIndex].bEndpointAddress;
				PTP_Debug_Print("usb_find_endpoints : find a Bulk(OUT) in endpoint at 0x%02x\n", dc->ptp_usb.outep);
			}
			break;
		default:
			PTP_Debug_Print("usb_find_endpoints : undefind endpoint type (0x%02x)\n", ep[nIndex].bmAttributes);
		}
	}
	
	
	return result;
	
}

/**
* @brief 
         ³»¿ëÀ» ³ÖÀ¸¼¼¿ä.
* @remarks 
         ³»¿ëÀ» ³ÖÀ¸¼¼¿ä.
* @param
         args ³»¿ëÀ» ³ÖÀ¸¼¼¿ä.
* @param
         data ³»¿ëÀ» ³ÖÀ¸¼¼¿ä.
* @param
         format ³»¿ëÀ» ³ÖÀ¸¼¼¿ä.
* @return
          ¾øÀ½.
* @see
         ³»¿ëÀ» ³ÖÀ¸¼¼¿ä.
*/
void PTP_print_debug (void *data, const char *format, va_list args)
{
	data = NULL; 					//intialising unused variable
	PTP_PRINT (format, args);
}

/**
* @brief 
         ³»¿ëÀ» ³ÖÀ¸¼¼¿ä.
* @remarks 
         ³»¿ëÀ» ³ÖÀ¸¼¼¿ä.
* @param
         args ³»¿ëÀ» ³ÖÀ¸¼¼¿ä.
* @param
         data ³»¿ëÀ» ³ÖÀ¸¼¼¿ä.
* @param
         format ³»¿ëÀ» ³ÖÀ¸¼¼¿ä.
* @return
          ¾øÀ½.
* @see
         ³»¿ëÀ» ³ÖÀ¸¼¼¿ä.
*/
void PTP_print_error (void *data, const char *format, va_list args)
{
	data = NULL; 					//intialising unused variable
	PTP_PRINT (format, args);
}

/**
* @brief 
         ³»¿ëÀ» ³ÖÀ¸¼¼¿ä.
* @remarks 
         ³»¿ëÀ» ³ÖÀ¸¼¼¿ä.
* @param
         bytes ³»¿ëÀ» ³ÖÀ¸¼¼¿ä.
* @param
         data ³»¿ëÀ» ³ÖÀ¸¼¼¿ä.
* @param
         readbytes ³»¿ëÀ» ³ÖÀ¸¼¼¿ä.
* @param
         size ³»¿ëÀ» ³ÖÀ¸¼¼¿ä.
* @return
          ³»¿ëÀ» ³ÖÀ¸¼¼¿ä.
* @see
         ³»¿ëÀ» ³ÖÀ¸¼¼¿ä.
*/
static short ptp_read_func (unsigned char *bytes, unsigned int size, void *data, unsigned int *readbytes)
{
	int result=-1;
	PTP_USB *ptp_usb=(PTP_USB *)data;
	int toread=0;
	signed long int rbytes=size;
	
	 
	readbytes = NULL;					//intialising unused variable  
	
	do {
		bytes += toread;
		if (rbytes > PTP_USB_URB) 
		{
			toread = PTP_USB_URB;
		}
		else
		{
			toread = rbytes;
		}
		
		result = USB_BULK_READ(ptp_usb->handle, ptp_usb->inep,(char *)bytes, toread, PTP_TIMEOUT);
		
		if (result==0)
		{
			result=USB_BULK_READ(ptp_usb->handle, ptp_usb->inep,(char *)bytes, toread, PTP_TIMEOUT);
		}
		
		if (result < 0)
		{
			break;
		}
		
		rbytes-=PTP_USB_URB;
	} while (rbytes > 0);
	
	if (result >= 0) 
	{
		return (PTP_RC_OK);
	}
	else 
	{
		return PTP_ERROR_IO;
	}
}

/**
* @brief 
         ³»¿ëÀ» ³ÖÀ¸¼¼¿ä.
* @remarks 
         ³»¿ëÀ» ³ÖÀ¸¼¼¿ä.
* @param
         bytes ³»¿ëÀ» ³ÖÀ¸¼¼¿ä.
* @param
         data ³»¿ëÀ» ³ÖÀ¸¼¼¿ä.
* @param
         size ³»¿ëÀ» ³ÖÀ¸¼¼¿ä.
* @return
          ³»¿ëÀ» ³ÖÀ¸¼¼¿ä.
* @see
         ³»¿ëÀ» ³ÖÀ¸¼¼¿ä.
*/
static short ptp_write_func (unsigned char *bytes, unsigned int size, void *data)
{
	int result;
	PTP_USB *ptp_usb=(PTP_USB *)data;
	
	result=USB_BULK_WRITE(ptp_usb->handle,ptp_usb->outep,(char *)bytes,size,PTP_TIMEOUT);
	if (result >= 0)
		return (PTP_RC_OK);
	else 
	{
		  
		printf ("\n\tERROR IN SENDING COMMAND TO PTP CAMERA\n");
		return PTP_ERROR_IO;
	}
}

/**
* @brief 
         ³»¿ëÀ» ³ÖÀ¸¼¼¿ä.
* @remarks 
         ³»¿ëÀ» ³ÖÀ¸¼¼¿ä.
* @param
         bytes ³»¿ëÀ» ³ÖÀ¸¼¼¿ä.
* @param
         data ³»¿ëÀ» ³ÖÀ¸¼¼¿ä.
* @param
         rlen ³»¿ëÀ» ³ÖÀ¸¼¼¿ä.
* @param
         size ³»¿ëÀ» ³ÖÀ¸¼¼¿ä.
* @return
          ³»¿ëÀ» ³ÖÀ¸¼¼¿ä.
* @see
         ³»¿ëÀ» ³ÖÀ¸¼¼¿ä.
*/
static short ptp_check_int (unsigned char *bytes, unsigned int size, void *data, unsigned int *rlen)
{
	int result;
	
	rlen = NULL;//intialising unused variable
	PTP_USB *ptp_usb=(PTP_USB *)data;
	
	result = USB_BULK_READ(ptp_usb->handle, ptp_usb->intep,(char *)bytes,size,PTP_TIMEOUT);
	if (result == 0)
	{
		result = USB_BULK_READ(ptp_usb->handle, ptp_usb->intep,(char *) bytes, size, PTP_TIMEOUT);
	}
	
	if (result >= 0) 
	{
		return PTP_RC_OK;
	} 
	else 
	{
		return PTP_ERROR_IO;
	}
}

/**
* @brief 
         ³»¿ëÀ» ³ÖÀ¸¼¼¿ä.
* @remarks 
         ³»¿ëÀ» ³ÖÀ¸¼¼¿ä.
* @param
         dc ³»¿ëÀ» ³ÖÀ¸¼¼¿ä.
* @return
          ³»¿ëÀ» ³ÖÀ¸¼¼¿ä.
* @see
         ³»¿ëÀ» ³ÖÀ¸¼¼¿ä.
*/
uint16 usb_initialize_port (PTPDevContext *dc)
{
	uint16	result = PTP_RC_OK;
	int16	nRet = 0;
	
	if (dc == NULL)
	{
		PTP_Debug_Print("[ERROR] usb_initialize_port : PTPDevContext is null\n");
		MPTP_Print("[ERROR] usb_initialize_port : PTPDevContext is null\n");
		return PTP_RC_Undefined;
	}
	
	dc->params.write_func = ptp_write_func;
	dc->params.read_func = ptp_read_func;
	dc->params.check_int_func = ptp_check_int;
	dc->params.check_int_fast_func = ptp_check_int;
	dc->params.error_func = PTP_print_error;
	dc->params.debug_func = PTP_print_debug;
	dc->params.sendreq_func = ptp_usb_sendreq;
	dc->params.senddata_func = ptp_usb_senddata;
	dc->params.getresp_func = ptp_usb_getresp;
	dc->params.getdata_func = ptp_usb_getdata;
	dc->params.data = (void *)(&(dc->ptp_usb));
	dc->params.transaction_id = 0;
	//dc->params.byteorder = PTP_DL_BE;
	//Test to check byteorder by Ajeet Yadav
	result = 0xBB11;	
	dc->params.byteorder = (*(unsigned char *)(& result) == '\x11')?
							PTP_DL_LE : PTP_DL_BE;
	result = PTP_RC_OK;
	
	if (dc->dev < 0)
	{
		PTP_Debug_Print("[ERROR] usb_initialize_port : dc->dev is null\n");
		MPTP_Print("[ERROR] usb_initialize_port : dc->dev is null\n");
		return PTP_RC_Undefined;
	}
	
	if ((dc->ptp_usb.handle=usb_open(dc->dev)))
	{
		if ( ! dc->ptp_usb.handle ) 
		{
			PTP_Debug_Print("[ERROR] usb_initialize_port : usb_open (%x)\n", dc->ptp_usb.handle);
			MPTP_Print("[ERROR] usb_initialize_port : usb_open (%x)\n", dc->ptp_usb.handle);
			result = PTP_RC_Undefined;
		}
		else
		{
			nRet = usb_claim_interface(dc->ptp_usb.handle, \
				dc->dev->config->interface->altsetting->bInterfaceNumber);
			if (nRet < 0)
			{
				PTP_Debug_Print("[ERROR] usb_initialize_port : usb_claim_interface (%x)\n", nRet);
				PTP_Debug_Print("[ERROR] usb_initialize_port : usb_claim_interface (%x)\n", dc->dev->config->interface->altsetting->bInterfaceNumber);
				MPTP_Print("[ERROR] usb_initialize_port : usb_claim_interface (%x)\n", nRet);
				result = PTP_RC_Undefined;
				PTP_Close_Device (dc);
			}
		}
	}
	
	return result;
	
}

/**
* @brief 
         ³»¿ëÀ» ³ÖÀ¸¼¼¿ä.
* @remarks 
         ³»¿ëÀ» ³ÖÀ¸¼¼¿ä.
* @param
         dc ³»¿ëÀ» ³ÖÀ¸¼¼¿ä.
* @return
          ³»¿ëÀ» ³ÖÀ¸¼¼¿ä.
* @see
         ³»¿ëÀ» ³ÖÀ¸¼¼¿ä.
*/
uint16 usb_clear_stall(PTPDevContext *dc)
{
	uint16	result = PTP_RC_OK;
	int16	nRet = 0;
	int16	status = 0;
	
	nRet = usb_control_msg(dc->ptp_usb.handle, USB_DP_DTH|USB_RECIP_ENDPOINT, \
		USB_REQ_GET_STATUS, USB_FEATURE_HALT, dc->ptp_usb.inep, (char *)&status, 2, PTP_TIMEOUT);
#if defined(ENDIAN_PATCH_BY_SISC)
	status = dtoh16ap (&dc->params, (unsigned char *)&status);
#endif
	if ( nRet < 0)
	{
		PTP_Debug_Print("[ERROR] usb_clear_stall: usb_control_msg(in ep) return error(%x)\n", nRet); 
		result = PTP_RC_Undefined;
	}
	else
	{
		if (status)
		{
			PTP_Debug_Print("usb_clear_stall: usb_control_msg - IN endpoint status(%x)\n", status); 
			result = clear_usb_stall_feature(dc, dc->ptp_usb.inep);
			PTP_Debug_Print("usb_clear_stall: usb_control_msg - IN endpoint Request Clear Feature(%x)\n", result);
		}
	}
	
	nRet  = usb_control_msg(dc->ptp_usb.handle, USB_DP_DTH|USB_RECIP_ENDPOINT, \
		USB_REQ_GET_STATUS, USB_FEATURE_HALT, dc->ptp_usb.outep, (char *)&status, 2, PTP_TIMEOUT);
#if defined(ENDIAN_PATCH_BY_SISC)
	status = dtoh16ap (&dc->params, (unsigned char *)&status);
#endif
	if (nRet < 0)
	{
		PTP_Debug_Print("[ERROR] usb_clear_stall: usb_control_msg(out ep) return error(%x)\n", nRet); 
		result = PTP_RC_Undefined;
	}
	else
	{
		if (status)
		{
			PTP_Debug_Print("usb_clear_stall: usb_control_msg - OUT endpoint status(%x)\n", status); 
			result = clear_usb_stall_feature(dc, dc->ptp_usb.outep);
			PTP_Debug_Print("usb_clear_stall: usb_control_msg - OUT endpoint Request Clear Feature(%x)\n", result);
		}
	}
	
	nRet = usb_control_msg(dc->ptp_usb.handle, USB_DP_DTH|USB_RECIP_ENDPOINT, \
		USB_REQ_GET_STATUS, USB_FEATURE_HALT, dc->ptp_usb.intep, (char *)&status, 2, PTP_TIMEOUT);
#if defined(ENDIAN_PATCH_BY_SISC)
	status = dtoh16ap (&dc->params, (unsigned char *)&status);
#endif
	if (nRet < 0)
	{
		PTP_Debug_Print("[ERROR] usb_clear_stall: usb_control_msg(interrup ep) return error(%x)\n", result); 
		result = PTP_RC_Undefined;
	}
	else
	{
		if (status)
		{
			PTP_Debug_Print("usb_clear_stall: usb_control_msg - INTERRUPT endpoint status(%x)\n", status); 
			result = clear_usb_stall_feature(dc, dc->ptp_usb.intep);
			PTP_Debug_Print("usb_clear_stall: usb_control_msg - INTERRUPT endpoint Request Clear Feature(%x)\n", result);
		}
	}
	
	return result;
}

/**
* @brief 
         ³»¿ëÀ» ³ÖÀ¸¼¼¿ä.
* @remarks 
         ³»¿ëÀ» ³ÖÀ¸¼¼¿ä.
* @param
         dest ³»¿ëÀ» ³ÖÀ¸¼¼¿ä.
* @param
         src ³»¿ëÀ» ³ÖÀ¸¼¼¿ä.
* @return
          ³»¿ëÀ» ³ÖÀ¸¼¼¿ä.
* @see
         ³»¿ëÀ» ³ÖÀ¸¼¼¿ä.
*/
uint16 ptp_deviceinfo_copy(PTPDeviceInfo *dest, PTPDeviceInfo *src)
{
	int16	result = PTP_RC_OK;
	
	dest->StaqndardVersion = src->StaqndardVersion;
	dest->VendorExtensionID = src->VendorExtensionID;
	dest->VendorExtensionVersion = src->VendorExtensionVersion;
	
	if (src->VendorExtensionDesc != NULL)
	{
		dest->VendorExtensionDesc = (char*)calloc(strlen(src->VendorExtensionDesc) + 1, sizeof (char));	//typecasting g++ vishal
		memcpy (dest->VendorExtensionDesc, src->VendorExtensionDesc, strlen(src->VendorExtensionDesc));
	}
	
	dest->FunctionalMode = src->FunctionalMode;
	
	dest->OperationsSupported_len = src->OperationsSupported_len;
	if (src->OperationsSupported_len > 0)
	{
		dest->OperationsSupported = (uint16_t*)calloc (src->OperationsSupported_len, sizeof (char));//typecasting g++ vishal
		memcpy (dest->OperationsSupported, src->OperationsSupported, src->OperationsSupported_len);
	}
	
	dest->EventsSupported_len = src->EventsSupported_len;
	if (src->EventsSupported_len > 0)
	{
		dest->EventsSupported = (uint16_t*)calloc (src->EventsSupported_len, sizeof (char)) ;//typecasting g++ vishal
		memcpy (dest->EventsSupported, src->EventsSupported, src->EventsSupported_len);
	}
	
	dest->DevicePropertiesSupported_len = src->DevicePropertiesSupported_len;
	if (src->DevicePropertiesSupported_len > 0)
	{
		dest->DevicePropertiesSupported = (uint16_t*)calloc (src->DevicePropertiesSupported_len, sizeof(char));//typecasting g++ vishal
		memcpy (dest->DevicePropertiesSupported, src->DevicePropertiesSupported, src->DevicePropertiesSupported_len);
	}
	
	dest->CaptureFormats_len = src->CaptureFormats_len;
	if (src->CaptureFormats_len > 0)
	{
		dest->CaptureFormats = (uint16_t*)calloc (src->CaptureFormats_len, sizeof (char));//typecasting g++ vishal
		memcpy (dest->CaptureFormats, src->CaptureFormats, src->CaptureFormats_len);
	}
	
	dest->ImageFormats_len = src->ImageFormats_len;
	if (src->ImageFormats_len > 0)
	{
		src->ImageFormats = (uint16_t*)calloc (dest->ImageFormats_len, sizeof (char));//typecasting g++ vishal
		dest->ImageFormats = dest->ImageFormats;
	}
	
	if (src->Manufacturer != NULL)
	{
		dest-> Manufacturer = (char*)calloc (strlen(src->Manufacturer) + 1, sizeof(char));//typecasting g++ vishal
		strcpy (dest->Manufacturer, src->Manufacturer);
	}
	
	if (src->Model != NULL)
	{
		dest->Model = (char*)calloc (strlen (src->Model) + 1, sizeof (char));//typecasting g++ vishal
		strcpy (dest->Model, src->Model);
	}
	
	if (src->DeviceVersion != NULL)
	{
		dest->DeviceVersion = (char*)calloc (strlen (src->DeviceVersion) + 1, sizeof (char));//typecasting g++ vishal
		strcpy(dest->DeviceVersion, src->DeviceVersion);
	}
	
	if (src->SerialNumber != NULL)
	{
		dest-> SerialNumber = (char*)calloc (strlen(src-> SerialNumber) + 1, sizeof(char));//typecasting g++ vishal
		strcpy(dest->SerialNumber, src->SerialNumber);
	}
	
	return result;
}


/**
* @brief 
         ³»¿ëÀ» ³ÖÀ¸¼¼¿ä.
* @remarks 
         ³»¿ëÀ» ³ÖÀ¸¼¼¿ä.
* @param
         dest ³»¿ëÀ» ³ÖÀ¸¼¼¿ä.
* @param
         src ³»¿ëÀ» ³ÖÀ¸¼¼¿ä.
* @return
          ³»¿ëÀ» ³ÖÀ¸¼¼¿ä.
* @see
         ³»¿ëÀ» ³ÖÀ¸¼¼¿ä.
*/
uint16 ptp_objectinfo_copy(PTPObjectInfo *dest, PTPObjectInfo *src)
{
	int16	result = PTP_RC_OK;
	
	dest->StorageID = src->StorageID;
	dest->ObjectFormat = src->ObjectFormat;
	dest->ProtectionStatus = src->ProtectionStatus;
	dest->ObjectCompressedSize = src->ObjectCompressedSize;
	dest->ThumbFormat = src->ThumbFormat;
	dest->ThumbCompressedSize = src->ThumbCompressedSize;
	dest->ThumbPixWidth = src->ThumbPixWidth;
	dest->ThumbPixHeight = src->ThumbPixHeight;
	dest->ImagePixWidth = src->ImagePixWidth;
	dest->ImagePixHeight = src->ImagePixHeight;
	dest->ImageBitDepth = src->ImageBitDepth;
	dest->ParentObject = src->ParentObject;
	dest->AssociationType = src->AssociationType;
	dest->AssociationDesc = src->AssociationDesc;
	dest->SequenceNumber = src->SequenceNumber;
	dest->CaptureDate = src->CaptureDate;
	dest->ModificationDate = src->ModificationDate;
	
	if (src->Filename != NULL)
	{
		if (strlen (src->Filename) > 0)
		{
			dest->Filename = (char*)calloc (strlen(src->Filename) + 1, sizeof (char));//typecasting g++ vishal
			strcpy (dest->Filename, src->Filename);
		}
	}
	
	if (src->Keywords != NULL)
	{
		if (strlen(src->Keywords) > 0)
		{
			dest->Keywords = (char*)calloc (strlen(src->Keywords) + 1, sizeof (char));//typecasting g++ vishal
			strcpy (dest->Keywords, src->Keywords);
		}
	}
	
	return result;
	
}

/**
* @brief 
         ³»¿ëÀ» ³ÖÀ¸¼¼¿ä.
* @remarks 
         ³»¿ëÀ» ³ÖÀ¸¼¼¿ä.
* @param
         dest ³»¿ëÀ» ³ÖÀ¸¼¼¿ä.
* @param
         src ³»¿ëÀ» ³ÖÀ¸¼¼¿ä.
* @return
          ³»¿ëÀ» ³ÖÀ¸¼¼¿ä.
* @see
         ³»¿ëÀ» ³ÖÀ¸¼¼¿ä.
*/
uint16 ptp_handles_copy(PTPObjectHandles *dest , PTPObjectHandles *src)
{
	int16	result = PTP_RC_OK;
	uint32	nIndex = 0;
	
	dest->n = src->n;
	dest->Handler = (uint32_t*)calloc (sizeof (uint32), src->n);//typecasting g++ vishal
	
	for (nIndex = 0; nIndex < src->n; nIndex++)
	{
		dest->Handler[nIndex] = src->Handler[nIndex];
	}
	
	return result;
}


/**
* @brief 
         ³»¿ëÀ» ³ÖÀ¸¼¼¿ä.
* @remarks 
         ³»¿ëÀ» ³ÖÀ¸¼¼¿ä.
* @param
          ¾øÀ½.
* @return
          ³»¿ëÀ» ³ÖÀ¸¼¼¿ä.
* @see
         ³»¿ëÀ» ³ÖÀ¸¼¼¿ä.
*/

 Bool PTP_API_Check_Devices(void)
 {
#if 0
 	struct usb_bus *usb_bus;
 	struct usb_device *usb_dev;
 	Bool bReturn = false;
 	
 	PTP_BEGIN;
 	
 	usb_init();
 	usb_find_busses();
 	usb_find_devices();
 	usb_bus = usb_get_busses();
 	
 	for (; usb_bus; usb_bus = usb_bus->next)
 	{
 		for (usb_dev = usb_bus->devices; usb_dev; usb_dev = usb_dev->next)
 		{
 			if ((usb_dev->config->interface->altsetting->bInterfaceClass == USB_CLASS_PTP))
 			{
 				if (usb_dev->descriptor.bDeviceClass!=USB_CLASS_HUB)
 				{
 					bReturn = true;
 				}
 			}
 		}
 	}
 	
 	PTP_END;
 	
 	return bReturn;
#endif
	return false;
 	
 }






/**
* @brief	      checks whether new devices are connected to the system
* @remarks		counts the number of ptp devices connected to the system
* @param		uint16 *ptp_dev_cnt 			: the num of PTP Devices  
* @return		Bool
True:PTP device present  False:PTP Device absent

*/ 
Bool PTP_API_Check_Devices_MPTP (uint16 *ptp_dev_cnt)
{
	struct usb_bus *usb_bus;
	struct usb_device *usb_dev;
	Bool bReturn = false;
		
	PTP_BEGIN;
	
	MPTP_Debug_Print("Sheetal - PTP_API_Check_Devices : start\n");
	usb_init();
	usb_find_busses();
	usb_find_devices();
	usb_bus = usb_get_busses();
	
	MPTP_Debug_Print("Sheetal - PTP_API_Check_Devices : initialised usb buses\n");
	
	// allocate array for dcs of all devices..for the moment max ptp devices
	for (; usb_bus; usb_bus = usb_bus->next)
	{
		for (usb_dev = usb_bus->devices; usb_dev; usb_dev = usb_dev->next)
		{
			
			if ((usb_dev->config->interface->altsetting->bInterfaceClass == USB_CLASS_PTP))
			{
				if (usb_dev->descriptor.bDeviceClass!=USB_CLASS_HUB)
				{
					(*ptp_dev_cnt)++;
					bReturn = true;
				}
			}
		}
	}
	
	
	PTP_END;
	
	//MPTP_Info_Print("\nSheetal PTP_API_CHECK_Devices : Total USB Devices found \t%d\n",count);
	//MPTP_Info_Print("Sheetal PTP_API_CHECK_Devices : PTP Devices found \t%d\n",*ptp_dev_cnt);
	//MPTP_Debug_Print("Sheetal PTP_API_CHECK_Devices : end\n");
	
	return bReturn;
}


////////////////////////////////////////////////////////////////////////////////////////////
// Newly updated     : ¸®´ª½º 2.6¿¡¼­ PTP µ¿ÀÛÀ» Áö¿øÇÏ±â À§ÇØ »õ·Î ±¸¼ºÇÔ 
//                            Below apis are newly updated for supporting the platform in linux2.6
// Date 	 : Nov 2006
// Author : Sang U Shim
////////////////////////////////////////////////////////////////////////////////////////////
/**
* @brief	      Check whether the file is existing or not
* @remarks		none
* @param		const char *fileName
* @return		Bool
true : success, false : failure
*/ 
Bool t_FileExist(const char* fileName)
{
	FILE *file = NULL;
	Bool ret = false;
	if ((file = fopen(fileName, "r")))
	{
		ret = true;
		fclose(file);
	}
	else
		ret = false;
	
	return ret;
}

/**
* @brief	      Check whether the string is existing in the source string or not
* @remarks		none
* @param		const char *pStr
* @param		const char *pFindStr
* @param		int bContinue    
* @return		Bool
true : success, false : failure
*/ 
Bool t_FindString(char *pStr, const char *pFindStr, int bContinue)
{
	int strIndex = 0;
	static int findIndex = 0;
	static char* pOldStr = 0x0;
	
	if(!bContinue || pStr != pOldStr)
	{
		findIndex = 0;		
		pOldStr = (char*)pStr;
	}
	
	int len = strlen(pStr);
	
	int findLen = strlen(pFindStr);
	for(strIndex = 0; strIndex < len; strIndex++)
	{
		if(*(pStr+strIndex) == *(pFindStr+findIndex) )
		{
			findIndex++;
			
		}
		else
		{
			findIndex = 0;
		}
		if(findIndex == findLen)
		{	
			return true;
		}
	}
	
	return false;	
}

/**
* @brief	      Check whether the ptp device is existing or not
* @remarks		none
* @param		none
* @return		Bool
true : success, false : failure
*/ 
 Bool PTP_API_CHKDEV(void)
 {
#if 0
 #define USB_PROC_BUS_USB_DEVICES_FILE "/proc/bus/usb/devices"
 #define PTP_ATTACHED_STRING1 "Cls=06"
 #define PTP_ATTACHED_STRING2 "still"
 #define PTP_MANUFACTURER "Manufacturer="
 #define PTP_PRODUCT "Product="
 	
 	FILE * fp;
 	char pDataLine[1024];
 	int byteCount = 0;
 	
 	int finish = false;
 	unsigned long count;
 	char *pDest = NULL;
 	char product[50] = {0, };
 	
 	
 	if (!t_FileExist(USB_PROC_BUS_USB_DEVICES_FILE))
 	{
 		system("mount -t usbfs none /proc/bus/usb/");
 	}
 	
 	if(!(fp = fopen(USB_PROC_BUS_USB_DEVICES_FILE, "r")))
 	{
 		printf("[ FILE ]file open error : %s\n", USB_PROC_BUS_USB_DEVICES_FILE);
 		return  false;
 	}	
 	
 	
 	while (1) {
 		count = 500;
 		
 		memset (pDataLine, 0, sizeof (pDataLine));
 		byteCount = fread(pDataLine, sizeof(char), 1024, fp);
 		if (byteCount == 0)
 		{
 			finish = true;
 		}
 		else
 		{
 			if ( t_FindString(pDataLine,PTP_ATTACHED_STRING1, true))
 			{	
 				fclose(fp);	
 				if(!(fp = fopen(USB_PROC_BUS_USB_DEVICES_FILE, "r")))
 				{
 					printf("[ FILE ]file open error : %s\n", USB_PROC_BUS_USB_DEVICES_FILE);
 					return  false;
 				}		
 				while(fread(pDataLine, sizeof(char), 1024, fp))	
 				{	
 					pDest = strstr (pDataLine,PTP_PRODUCT);
 					
 					
 					if (pDest != NULL)
 					{
 						pDest = strchr (pDest, '=');
 						if (pDest != NULL)
 						{
 							++pDest;
 							uint16 nIndex = 0;
 							do 
 							{
 								product[nIndex] = *pDest;
 								++nIndex;
 								++pDest;
 							} while (*pDest != '\n' && *pDest != '\0');
 							//printf("\n ProdID=%s \n",product);
 							fclose(fp);
 							return true;
 						}
 					}
 					
 					else	
 						printf("\n pDest = NULL\n"); //vishal
 				}
 				
 				fclose(fp);
 				return true;
 			}
 		}
 		
 		if(finish) break;
 	}
 	
 	fclose(fp);
 	return false;
#endif
	return false;
 }
 


/**
* @brief	      Check whether the ptp device is existing or not
* @remarks		none
* @param		PTPDeviceHandle ptpHandle:    Handle for the given ptp device
* @return		Bool
true : success, false : failure
*/ 

Bool PTP_API_CHKDEV_MPTP(PTPDeviceHandle  ptpHandle)	
{
	
#define USB_PROC_BUS_USB_DEVICES_FILE "/proc/bus/usb/devices"
#define PTP_ATTACHED_STRING1 "Cls=06"
#define PTP_ATTACHED_STRING2 "still"
#define PTP_MANUFACTURER "Manufacturer="
#define PTP_PRODUCT "Product="

	int busNum;/*=(int)dc->bus->location;*/
	int devNum;
	FILE * fp;
	char pDataLine[1024];
	int byteCount = 0;
	
	int finish = false;
	
	char *pDest = NULL;
	char *pDest1=NULL;
	//char product[50] = {0, };
	int len;

	PTPDevContext *dc;
	dc=getDc(ptpHandle);

	if(dc==NULL)
	{
		PTP_Debug_Print("[ERROR] PTP_API_CHKDEV_MPTP : PTPDevContext is null.\n");
		return false;//added by Niraj   
	}
	if (!t_FileExist(USB_PROC_BUS_USB_DEVICES_FILE))
	{
		system("mount -t usbfs none /proc/bus/usb/");
	}
	
	if(!(fp = fopen(USB_PROC_BUS_USB_DEVICES_FILE, "r")))
	{
		printf("[ FILE ]file open error : %s\n", USB_PROC_BUS_USB_DEVICES_FILE);
		return  false;
	}	
	
	char dest1[25];
	memset(dest1,0,25);
	busNum=atoi((dc->bus)->dirname);
	
	if(busNum<9)
		sprintf(dest1,"Bus=0%d",busNum);
	else	
		sprintf(dest1,"Bus=%d",busNum);
	
	
	char dest2[25];
	memset(dest2,0,25);
	//devNum=(int)dc->dev->devnum;
	devNum=atoi((dc->dev)->filename);
	
	sprintf(dest2,"Dev#=%3d",devNum);
	
	while (1) {
		
		
		memset (pDataLine, 0, sizeof (pDataLine));
		byteCount = fread(pDataLine, sizeof(char), 1024, fp);

		if (byteCount == 0)
		{
			finish = true;
			
		}
		else
		{
			if ( t_FindString(pDataLine,PTP_ATTACHED_STRING1, true))
			{	
				
				fclose(fp);	
				if(!(fp = fopen(USB_PROC_BUS_USB_DEVICES_FILE, "r")))
				{
					printf("[ FILE ]file open error : %s\n", USB_PROC_BUS_USB_DEVICES_FILE);
					return  false;
				}	
				
				
				while(fread(pDataLine, sizeof(char),1024 , fp))
				{	
					
					
					pDest = strstr (pDataLine,dest1);
					
					if (pDest != NULL)
					{
						
						if((len=strlen(pDest))<50) //actual value is 45
						{
							
							fseek(fp,-(len+4),SEEK_CUR);
							continue;
						}
						
						pDest1 = strstr (pDest, dest2);
						if(pDest1!=NULL)
						{
							
							fclose(fp);
							return true;
						}
						else
						{
							pDest=pDest+6;//increment the pointer to point after string dest1
							pDest1 = strstr(pDest,dest1);
							if(pDest1!=NULL)
							{
								len=strlen(pDest1);
								fseek(fp,-(len+4),SEEK_CUR);
							}	
							continue;
						}

					}			
				}
			
				fclose(fp);
				return false;
			}
		}
		
		if(finish) break;
	}
	
	fclose(fp);
	return false;
}






/**
* @brief	      open session for ptp
* @remarks		none
* @param		none
* @return		uint16
PTP_RC_OK : success, else : failure
*/ 
 uint16 PTP_API_OPENSESSION(void)
 {
#if 0
 	uint16 result;	
 	if (PTP_API_CHKDEV() != true)
 		return PTPAPI_ERROR; //just representing status error
 	
 	result = ptp_opensession(&(gdc.params), PTP_START_INDEX);
 	if (result != PTP_RC_OK) 
 	{               
 		PTP_Debug_Print("[ERROR] PTP_API_OPENSESSION : Can't open PTP session (%x)\n", result);
 	}
 	else
 	{
 		PTP_Debug_Print("PTP_API_OPENSESSION is ok\n");		
 	}	
 	return result;
#endif
	return PTPAPI_ERROR;
 }

/**
* @brief	      open session for the given ptp
* @remarks		none
* @param		PTPDeviceHandle ptpHandle:    Handle for the given ptp device
* @return		uint16
PTP_RC_OK : success, else : failure
*/ 
uint16 PTP_API_OPENSESSION_MPTP(PTPDeviceHandle  ptpHandle)
{	
	uint16 result;	

	PTPDevContext *dc;
	dc=getDc(ptpHandle);
	if(dc==NULL)
	{
		PTP_Debug_Print("[ERROR] PTP_API_OPENSESSION_MPTP : PTPDevContext is null.\n");
		return PTPAPI_ERROR;//added by Niraj   
	}
	if (PTP_API_CHKDEV_MPTP(ptpHandle) != true)
	{
		  
		return PTPAPI_ERROR; //just representing status error
	}
	
	result = ptp_opensession(&(dc->params), PTP_START_INDEX);
	if (result != PTP_RC_OK) 
	{        
		  
	    //printf("[ERROR] PTP_API_OPENSESSION : Can't open PTP session (%x)\n", result);
		PTP_Debug_Print("[ERROR] PTP_API_OPENSESSION : Can't open PTP session (%x)\n", result);
	}
	else
	{
		PTP_Debug_Print("PTP_API_OPENSESSION is ok\n");		
	}	

/// added for panasonic - to chk
	if (result == PTP_RC_SessionAlreadyOpened)
	{
		result = ptp_closesession(&(dc->params)); 	
		if (result != PTP_RC_OK) 
		{               
			PTP_Debug_Print("[ERROR] PTP_API_CLOSESESSION : Can't close PTP session (%x)\n", result);
		}
		else
		{
			PTP_Debug_Print("PTP_API_CLOSESESSION is ok\n");		

			result = ptp_opensession(&(dc->params), PTP_START_INDEX);
			if (result != PTP_RC_OK) 
			{        
				//printf("[ERROR] PTP_API_OPENSESSION : Can't open PTP session (%x)\n", result);
				PTP_Debug_Print("[ERROR] PTP_API_OPENSESSION : Can't open PTP session (%x)\n", result);
			}
			else
			{
				PTP_Debug_Print("PTP_API_OPENSESSION is ok\n");		
			}	
		}	
	}
	return result;
}

/**
* @brief	      close session for ptp
* @remarks		none
* @param		none
* @return		uint16
PTP_RC_OK : success, else : failure
*/ 
 uint16 PTP_API_CLOSESESSION(void)
 {
#if 0
 	uint16 result;	
 	if (PTP_API_CHKDEV() != true)
 		return PTPAPI_ERROR; //just representing status error
 	
 	result = ptp_closesession(&(gdc.params)); 	
 	if (result != PTP_RC_OK) 
 	{               
 		PTP_Debug_Print("[ERROR] PTP_API_CLOSESESSION : Can't close PTP session (%x)\n", result);
 	}
 	else
 	{
 		PTP_Debug_Print("PTP_API_CLOSESESSION is ok\n");		
 	}	
 	return result;
#endif
	return PTPAPI_ERROR;
 }


/**
* @brief	      close session for the given ptp device
* @remarks		none
* @param		PTPDeviceHandle ptpHandle:    Handle for the given ptp device
* @return		uint16
PTP_RC_OK : success, else : failure
*/ 
uint16 PTP_API_CLOSESESSION_MPTP(PTPDeviceHandle  ptpHandle)
{	
	uint16 result;	

	PTPDevContext *dc;
	dc=getDc(ptpHandle);

	if(dc==NULL)
	{
		PTP_Debug_Print("[ERROR] PTP_API_CLOSESESSION_MPTP : PTPDevContext is null.\n");
		return PTPAPI_ERROR;//added by Niraj   
	}
	if (PTP_API_CHKDEV_MPTP(ptpHandle) != true){
		return PTPAPI_ERROR; //just representing status error
	}
		result = ptp_closesession(&(dc->params)); 	
	if (result != PTP_RC_OK) 
	{               
		PTP_Debug_Print("[ERROR] PTP_API_CLOSESESSION : Can't close PTP session (%x)\n", result);
	}
	else
	{
		PTP_Debug_Print("PTP_API_CLOSESESSION is ok\n");		
	}	
	return result;
}

/**
* @brief	      open device for ptp
* @remarks		none
  * @param		Bool isgetdeviceinfo - 1 ÀÌ¸é device info¸¦ °¡Á®¿À°í, ±×·¸Áö ¾ÊÀ¸¸é °¡Á®¿ÀÁö ¾Ê´Â´Ù. 
* @return		uint16
PTP_RC_OK : success, else : failure
*/ 
 uint16 PTP_API_OPENDEVICE(Bool isgetdeviceinfo)
 {
#if 0
 	uint16 result;	
 	int i = 0;
 	if (PTP_API_CHKDEV() != true)
 		return PTPAPI_ERROR; //just representing status error
 	
 	PTP_Init_Transaction (&gdc);
 	result = PTP_Open_Device(&gdc);
 	
 	if (result != PTP_RC_OK)
 	{       
 		PTP_Debug_Print("[Error] PTP_API_OPENDEVICE : PTP_API_OPENDEVICE return (%x)\n", result);
 		return result;
 	}
 	
 	if (isgetdeviceinfo)
 	{
 		result = ptp_getdeviceinfo (&(gdc.params), &(gdc.params.deviceinfo));
 		if( result != PTP_RC_OK) 
 		{
 			PTP_Debug_Print("[Error] ptp_getdeviceinfo : ptp_getdeviceinfo return (%x)\n", result);
 			PTP_Close_Device(&gdc);
 			return result;
 		}			
 		else
 		{
 			PTP_Debug_Print("ptp_getdeviceinfo is OK\n");
 			PTP_Debug_Print("The Model is : %s\n\n", gdc.params.deviceinfo.Model);
 			PTP_Debug_Print("The supported operations are \n");
 			for (i = 0; i < gdc.params.deviceinfo.OperationsSupported_len; i++)
 			{				
 				PTP_Debug_Print ("%x\n", gdc.params.deviceinfo.OperationsSupported[i]);
 			}
 		}
 	}
 	
 	return result;	
#endif
	isgetdeviceinfo=0;
	return PTPAPI_ERROR;
 }



/**
* @brief	      opens the given ptp device
* @remarks		none
* @param		Bool isgetdeviceinfo	:flag whether device info is to fetched or not
* @param		PTPDevContext *dc	:Device context of the given device
* @return		uint16
PTP_RC_OK : success, else : failure
*/ 
uint16 PTP_API_OPENDEVICE_MPTP(Bool isgetdeviceinfo, PTPDeviceHandle  ptpHandle)
{
	
	uint16 result;	
	uint32 i = 0;

	PTPDevContext *dc;
	dc=getDc(ptpHandle);

	if(dc==NULL)
	{
		PTP_Debug_Print("[ERROR] PTP_API_OPENDEVICE_MPTP : PTPDevContext is null.\n");
		return PTPAPI_ERROR;//added by Niraj   
	}
	//if (PTP_API_CHKDEV_MPTP(ptpHandle) != true)//vishal
	result=PTP_API_CHKDEV_MPTP(ptpHandle);
	if (result != true)
	{	
		MPTP_Print("\nInside PTP_API_OPENDEVICE_MPTP : PTP_API_CHKDEV_MPTP FAIL result=%x\n",result);
		return PTPAPI_ERROR; //just representing status error
	}
	MPTP_Print("\nInside PTP_API_OPENDEVICE_MPTP : PTP_API_CHKDEV_MPTP success\n");
	PTP_Init_Transaction (dc);
	MPTP_Print("\nInside PTP_API_OPENDEVICE_MPTP : Before PTP_Open_Device_MPTP\n");
	result = PTP_Open_Device_MPTP(dc);
	
	if (result != PTP_RC_OK)
	{       
		PTP_Debug_Print("[Error] PTP_API_OPENDEVICE : PTP_API_OPENDEVICE return (%x)\n", result);
		MPTP_Print("[Error] PTP_API_OPENDEVICE : PTP_API_OPENDEVICE return (%x)\n", result);
		return result;
	}
	
	if (isgetdeviceinfo)
	{
		result = ptp_getdeviceinfo (&(dc->params), &(dc->params.deviceinfo));
		if( result != PTP_RC_OK) 
		{
			PTP_Debug_Print("[Error] ptp_getdeviceinfo : ptp_getdeviceinfo return (%x)\n", result);
		//	printf("\n[Error] ptp_getdeviceinfo : ptp_getdeviceinfo return (%x)\n", result);
			PTP_Close_Device(dc);
			return result;
		}			
		else
		{
			PTP_Debug_Print("ptp_getdeviceinfo is OK\n");
			PTP_Debug_Print("The Model is : %s\n\n", dc->params.deviceinfo.Model);
			PTP_Debug_Print("The supported operations are \n");
			for (i = 0; i < dc->params.deviceinfo.OperationsSupported_len; i++)
			{				
				PTP_Debug_Print ("%x\n", dc->params.deviceinfo.OperationsSupported[i]);
			}
		}
	}
	
	return result;	
}


/**
* @brief	      close device for ptp
* @remarks		none
* @param		none
* @return		uint16
PTP_RC_OK : success, else : failure
*/ 
 uint16 PTP_API_CLOSEDEVICE(void)
 {
#if 0
 	uint16 result;
 	if (PTP_API_CHKDEV() != true)
 		return PTPAPI_ERROR; //just representing status error
 	
 	result = PTP_Close_Device(&gdc);
 	if (result != PTP_RC_OK)
 	{       
 		PTP_Debug_Print("[Error] PTP_API_CLOSEDEVICE : PTP_API_CLOSEDEVICE return (%x)\n", result);
 	}
 	return result;
#endif
	return PTPAPI_ERROR;
 }

/**
* @brief	      close device for the given ptp device
* @remarks		none
* @param		PTPDeviceHandle ptpHandle:    Handle for the given ptp device
* @return		uint16
PTP_RC_OK : success, else : failure
*/ 
uint16 PTP_API_CLOSEDEVICE_MPTP(PTPDeviceHandle  ptpHandle)
{
	
	uint16 result;
	
	PTPDevContext *dc;
	dc=getDc(ptpHandle);

	if(dc==NULL)
	{
		PTP_Debug_Print("[ERROR] PTP_API_CLOSEDEVICE_MPTP : PTPDevContext is null.\n");
		return PTPAPI_ERROR;//added by Niraj   
	}
	if (PTP_API_CHKDEV_MPTP(ptpHandle) != true)
		return PTPAPI_ERROR; //just representing status error
	
	result = PTP_Close_Device(dc);
	if (result != PTP_RC_OK)
	{       
		PTP_Debug_Print("[Error] PTP_API_CLOSEDEVICE : PTP_API_CLOSEDEVICE return (%x)\n", result);
	}
	return result;
}

/**
* @brief	      Initialization of communication with ptp device
* @remarks		this function must be called onetime when the device is attached
* @param		none
* @return		uint16
PTP_RC_OK : success, else : failure
*/ 
 uint16 PTP_API_INIT_COMM(void)
 {
 #if 0
 	uint16 result;
 	
 	if (PTP_API_CHKDEV() != true)
 		return PTPAPI_ERROR; //just representing status error
 	
 	result = PTP_API_OPENDEVICE(DO_GET_DEVICE_INFO);
 	if (result == PTP_RC_OK)
 	{
 		PTP_Debug_Print("PTP_API_OPENDEVICE is ok\n");		
 		result = PTP_API_OPENSESSION();	
 	}
 	else
 	{
 		PTP_Debug_Print("[ERROR] PTP_API_INIT_COMM is not ok\n");
 	}
 	
 	return result;
 #endif
 	
 	return PTPAPI_ERROR;
 }

/**
* @brief	      Initialization of communication with the given ptp device
* @remarks		this function must be called onetime when the device is attached
* @param		PTPDeviceHandle ptpHandle:    Handle for the given ptp device
* @return		uint16
PTP_RC_OK : success, else : failure
*/
uint16 PTP_API_INIT_COMM_MPTP(PTPDeviceHandle  ptpHandle)
{
	
	uint16 result;
	PTPDevContext *dc;
	dc=getDc(ptpHandle);

	if(dc==NULL)
	{
		PTP_Debug_Print("[ERROR] PTP_API_INIT_COMM_MPTP : PTPDevContext is null.\n");
// printf("[ERROR] PTP_API_INIT_COMM_MPTP : PTPDevContext is null.\n ptpHandle=%s",ptpHandle);
		return PTPAPI_ERROR;//added by Niraj   
	}
	MPTP_Print("\nBefore PTP_API_CHKDEV_MPTP\n");
	if (PTP_API_CHKDEV_MPTP(ptpHandle) != true)
	{
//printf("\n PTP_API_CHKDEV_MPTP failed");
		return PTPAPI_ERROR; //just representing status error
	}
	
	//MPTP_Print("\nBefore PTP_API_OPENDEVICE_MPTP\n");
	result = PTP_API_OPENDEVICE_MPTP(DO_GET_DEVICE_INFO,ptpHandle);
	if (result == PTP_RC_OK)
	{
		PTP_Debug_Print("PTP_API_OPENDEVICE is ok\n");	
		MPTP_Print("\nPTP_API_OPENDEVICE_MPTP is ok\n");
		result = PTP_API_OPENSESSION_MPTP(ptpHandle);	
		if(result == PTP_RC_OK)//vishal
			MPTP_Print("\nPTP_API_OPENSESSION_MPTPis ok\n");
		else
		{
		  
//			printf("\nPTP_API_OPENSESSION_MPTPis NOT ok\n");
		}
	}
	else
	{	
		  
//		printf("\nPTP_API_OPENDEVICE_MPTP is not ok\n");
		PTP_Debug_Print("[ERROR] PTP_API_INIT_COMM is not ok\n");
	}
	
	return result;
}

/**
* @brief	      finishing process of communication with the ptp device
* @remarks		none
* @param		none
* @return		uint16
PTP_RC_OK : success, else : failure
*/ 
 uint16 PTP_API_FINISH_COMM(void)
 {
#if 0
 	uint16 result;
 	result = PTP_API_CLOSESESSION();
 	if (result == PTP_RC_OK)
 		result = PTP_API_CLOSEDEVICE();
 	return result;
#endif
	return PTPAPI_ERROR;
 }

/**
* @brief	      finishing process of communication with the given ptp device
* @remarks		none
* @param		PTPDeviceHandle ptpHandle:    Handle for the given ptp device
* @return		uint16
PTP_RC_OK : success, else : failure
*/
uint16 PTP_API_FINISH_COMM_MPTP(PTPDeviceHandle  ptpHandle)
{
	uint16 result;

	result = PTP_API_CLOSESESSION_MPTP(ptpHandle);

	if (result == PTP_RC_OK)
	{
		MPTP_Print("\nPTP_API_CLOSESESSION_MPTP is OK\n");

		result = PTP_API_CLOSEDEVICE_MPTP(ptpHandle);
		if(result == PTP_RC_OK)
			MPTP_Print("\nPTP_API_CLOSEDEVICE_MPTP is OK\n");
		else
			MPTP_Print("\nPTP_API_CLOSEDEVICE_MPTP is not OK\n");
	}
	else
		MPTP_Print("\nPTP_API_CLOSESESSION_MPTP is not OK\n");

	return result;
}

/**
* @brief	      check the handle whether it's valid or not
* @remarks		none
* @param		none
* @return		Bool
true : success, false : failure
*/ 
 Bool PTP_API_Is_Handle_Vaild (uint32 ObjectHandle)
 {
#if 0
 /*
 *
	  * 2007-11-14 (KKS) : PTP ¸®½ºÆ® ÃßÃâ¼º´É °³¼±À» À§ÇØ »èÁ¦ÇÔ
	  * PTP device¿¡ ÆÄÀÏÀÌ ¸¹À»¼ö·Ï handleÀÇ À¯È¿¼º °Ë»ç·Î ÀÎÇØ µ¿ÀÛ½Ã°£ÀÌ ´À·ÁÁö´Â ¹®Á¦°¡ ¹ß»ýÇÔ
	  * ÇÚµéÀÇ À¯È¿¼º ÆÇ´ÜÀº API È£Ãâ½Ã ptp commandÀÇ return value Ã¼Å©·Î Ã³¸®ÇØ¾ßÇÔ
 	*/
 	PTPObjectHandles objecthandles;
 	Bool Is_objecthandle_found = false;
 	int i = 0;
 	
 	PTP_BEGIN;
 	
 	//HANDLE CHECK (All handles in device)
 	if (ptp_getobjecthandles (&gdc.params, 0xffffffff,
 		/*uint32_t objectformatcode -> optional*/0, /*uint32_t associationOH -> optional*/0,
 		&objecthandles) != PTP_RC_OK)
 	{
 		PTP_END;
 		return false;
 	}
 	
 	//Find a handle matched with the input handle
 	for (i = 0; i < objecthandles.n; i++)
 	{
 		PTP_Debug_Print("handle [%d] : %x ", i, objecthandles.Handler[i]);
 		if (ObjectHandle == objecthandles.Handler[i])
 		{
 			PTP_Debug_Print("\nHandle is found\n");
 			Is_objecthandle_found = true;
 			break;
 		}
 	}
 	
 	if (Is_objecthandle_found == false)
 	{
 		PTP_Debug_Print("\nHandle is not found\n");
 	}
 	
 	PTP_END;
 	
 	return Is_objecthandle_found;
#endif
	ObjectHandle = 0;
	return false;
 	
 }

/**
* @brief	      check the handle whether it's valid or not for the given ptp device
* @remarks		none
* @param		PTPDeviceHandle ptpHandle:    Handle for the given ptp device
* @param		uint32 ObjectHandle :handle of the object in the ptp device
* @return		Bool
true : success, false : failure
*/ 
Bool PTP_API_Is_Handle_Vaild_MPTP (uint32 ObjectHandle, PTPDeviceHandle  ptpHandle)
{

	
	/*
	  *
	  * 2007-11-14 (KKS) : PTP ¸®½ºÆ® ÃßÃâ¼º´É °³¼±À» À§ÇØ »èÁ¦ÇÔ
	  * PTP device¿¡ ÆÄÀÏÀÌ ¸¹À»¼ö·Ï handleÀÇ À¯È¿¼º °Ë»ç·Î ÀÎÇØ µ¿ÀÛ½Ã°£ÀÌ ´À·ÁÁö´Â ¹®Á¦°¡ ¹ß»ýÇÔ
	  * ÇÚµéÀÇ À¯È¿¼º ÆÇ´ÜÀº API È£Ãâ½Ã ptp commandÀÇ return value Ã¼Å©·Î Ã³¸®ÇØ¾ßÇÔ
	  */
	PTPObjectHandles objecthandles;
	Bool Is_objecthandle_found = false;
	uint32 i = 0;

	PTPDevContext *dc;
	dc=getDc(ptpHandle);
	if(dc==NULL)
	{
		PTP_Debug_Print("[ERROR] PTP_API_Is_Handle_Valild_MPTP : PTPDevContext is null.\n");
		return false;//added by Niraj   
	}
	
	PTP_BEGIN;	
		
	//HANDLE CHECK (All handles in device)
	//if (ptp_getobjecthandles (&gdc.params, 0xffffffff,
	if (ptp_getobjecthandles (&(dc->params), 0xffffffff,
		/*uint32_t objectformatcode -> optional*/ 0, /*uint32_t associationOH -> optional*/0,
		&objecthandles) != PTP_RC_OK)
	{
		PTP_END;
		return false;
	}
		
	//Find a handle matched with the input handle
	
	for (i = 0; i < objecthandles.n; i++)
	{
		PTP_Debug_Print("handle [%d] : %x ", i, objecthandles.Handler[i]);
		if (ObjectHandle == objecthandles.Handler[i])
		{
			PTP_Debug_Print("\nHandle is found\n");								
			Is_objecthandle_found = true;
			break;
		}
	}
		
	if (Is_objecthandle_found == false)
	{		
		PTP_Debug_Print("\nHandle is not found\n");		
	}	
	
	PTP_END;	
		
	return Is_objecthandle_found;
	
}

/**
* @brief	      check the existence of ptp device
* @remarks		none
* @param		PTPDeviceInfo *deviceinfo 
* @return		Bool
true : success, false : failure
*/ 
 Bool Is_PTP_device(PTPDeviceInfo *deviceinfo)
 {
#if 0
 	uint16  result = PTP_RC_OK;
 	Bool ret = false;
 	
 	PTP_Init_Transaction (&gdc);
 	
 	if (PTP_API_CHKDEV() != true)
 		return false; //just representing status error
 	
 	PTP_BEGIN;
 	
 	result = ptp_getdeviceinfo (&(gdc.params), &(gdc.params.deviceinfo));
 	if( result != PTP_RC_OK)
 	{
 		PTP_Debug_Print("[Error] Is_PTP_device : ptp_getdeviceinfo return (%x)\n", result);
 		ret = false;
 	}
 	else
 	{
 		PTP_Debug_Print("Is_PTP_device is OK\n");
 		ret = true;
 	}
 	
 	PTP_END;
 	
 	return ret;
#endif
	
	deviceinfo = NULL;   //intialising unused variable
	return false;
 	
 }

/**
* @brief	      check the existence of the given ptp device
* @remarks		none
* @param		PTPDeviceHandle ptpHandle:    Handle for the given ptp device
* @return		Bool
true : success, false : failure
*/ 
Bool Is_PTP_device_MPTP(PTPDeviceHandle  ptpHandle)
{
	
	uint16  result = PTP_RC_OK;
	Bool ret = false;
	
	PTPDevContext *dc;
	dc=getDc(ptpHandle);
	if(dc==NULL)
	{
		PTP_Debug_Print("[ERROR] Is_PTP_device_MPTP : PTPDevContext is null.\n");
		return false;//added by Niraj   
	}
	PTP_Init_Transaction (dc);
	
	if (PTP_API_CHKDEV_MPTP(ptpHandle) != true)
		return false; //just representing status error
	
	PTP_BEGIN;
	
	result = ptp_getdeviceinfo (&(dc->params), &(dc->params.deviceinfo));
	if( result != PTP_RC_OK)
	{
		PTP_Debug_Print("[Error] Is_PTP_device : ptp_getdeviceinfo return (%x)\n", result);
		ret = false;
	}
	else
	{
		PTP_Debug_Print("Is_PTP_device is OK\n");		
		ret = true;
	}
	
	PTP_END;
	
	return ret;
	
}

/**
* @brief	      Get file list from the handle
* @remarks		file list is including folders and jpg files
* @param		uint32 storageID			: storage id
* @param		uint32 ParentObjectHandle 	: handle 
* @param		PTPFileListInfo **fileinfo 	: store of filelist   
* @param		uint32 *nCount 			: the num of files(including folders)   
* @return		uint16
PTP_RC_OK : success, else : failure
*/ 
 uint16 PTP_API_Get_FilesList (uint32 storageID, uint32 ParentObjectHandle, PTPFileListInfo **fileinfo, uint32 *nCount)
 {
#if 0
 	uint32	n = 0;
 	uint32	nIndex = 0;
 	uint16	result = PTP_RC_OK;
 	
 	int		nRet = 0;
 	char		foldername[MAX_FILENAME] = {'0'};
 	char		filename[MAX_FILENAME] = {'0'};
 	
 	struct tm* tmPtr = 0;
 	struct tm lt;
 	
 	PTPObjectInfo		oi;
 	PTPObjectHandles	oh1, oh2;
 	
 	int i = 0;
 	uint32 numdir = 0;	
 	uint32 numjpg = 0;
 	
 	unsigned long stTick = 0;
 	unsigned long edTick = 0;
 	
 	if (PTP_API_CHKDEV() != true)
 		return PTPAPI_ERROR; //just representing status error
 	
 	PTP_Init_Transaction (&gdc);
 	
 	PTP_API_Init_ObjectHandles(&oh1);
 	PTP_API_Init_ObjectHandles(&oh2);
 	
 	PTP_Debug_Print("PTP_API_Get_FilesList : start(H:%x)\n", ParentObjectHandle);
 	
 	PTP_BEGIN;		
 	
 	stTick = Tick() ;
 	
 	//Find All list from the ParentObjectHandle
 	result = ptp_getobjecthandles (&(gdc.params),storageID, 0x00000000, ParentObjectHandle, &(gdc.params.handles));
 	if (result != PTP_RC_OK)
 	{
 		PTP_END;
 		return result;
 	}
 	
 	PTP_Debug_Print("The # of handles : %d\n", gdc.params.handles.n);
 	PTP_Debug_Print("Handler:        size: \tname:\n");
 	for (i = 0; i < gdc.params.handles.n; i++) {
 		result = ptp_getobjectinfo(&(gdc.params),gdc.params.handles.Handler[i], &oi);
 		if (result != PTP_RC_OK)
 		{
 			PTP_END;
 			return result;
 		}
 		
 		if (oi.ObjectFormat == PTP_OFC_Association)
 		{
 			numdir++;
 			continue;
 		}
 		if (oi.ObjectFormat == PTP_OFC_EXIF_JPEG)
 			numjpg ++;
 		PTP_Debug_Print("0x%08x: % 9i\t%s\n",gdc.params.handles.Handler[i],
 			oi.ObjectCompressedSize, oi.Filename);
 	}
 	
 	PTP_Debug_Print("the num of dir : %d \n", numdir);
 	PTP_Debug_Print("the num of jpg : %d \n", numjpg);
 	
	// SONY DSC T20ÀÇ °æ¿ì, PTP_OFC_Association cmd·Î Æú´õ¿¡ ´ëÇÑ  handle list ¿äÃ»½Ã device°¡ STALLµÊ
	// ÆÄÀÏ,Æú´õ ¸®½ºÆ® ±¸¼º½Ã NULL °ªÀ» Ã¼Å©ÇØ¼­ handle list ¿äÃ»
 	if(numdir != 0)
 	{
 		result = ptp_getobjecthandles (&(gdc.params), storageID, PTP_OFC_Association, ParentObjectHandle, &(oh1));
 		if( result != PTP_RC_OK) 
 		{
 			PTP_Debug_Print("[Error] PTP_API_Get_FilesList : ptp_getobjecthandles(folder) return (%x)\n", result);
 			PTP_END;
 			return result;
 		}	
 	}
 	
 	if(numjpg != 0)
 	{
 		result = ptp_getobjecthandles (&(gdc.params), storageID, PTP_OFC_EXIF_JPEG, ParentObjectHandle, &(oh2));
 		if( result != PTP_RC_OK) 
 		{
 			PTP_Debug_Print("[Error] PTP_API_Get_FilesList : ptp_getobjecthandles(file) return (%x)\n", result);
 			PTP_END;		
 			return result;		
 		}	
 	}
 	
 	
 	*nCount = numdir + numjpg;/*oh1.n + oh2.n;*/
 	if (*nCount <= 0)
 	{
 		PTP_Debug_Print("[Error] PTP_API_Get_FilesList : oh1.n + oh2.n is less than 0\n");
 		PTP_END;		
 		return PTPAPI_ERROR; // just representing wrong count error		
 	}
 	
 	*fileinfo = (PTPFileListInfo *)calloc (sizeof(PTPFileListInfo), *nCount);
 	if (*fileinfo == NULL)
 	{
 		PTP_Debug_Print("[Error] PTP_API_Get_FilesList : memory alloc error(PTPFileListInfo)\n");
 		PTP_END;		
 		return PTPAPI_ERROR; // just representing memory allocation error		
 	}
 	
 	
 	n = 0;
 	for (nIndex = 0; nIndex <numdir /*oh1.n*/; nIndex++) 
 	{
 		PTP_API_Init_ObjectInfo(&oi);
 		result = ptp_getobjectinfo(&(gdc.params), oh1.Handler[nIndex], &oi);
 		if ( result != PTP_RC_OK )
 		{
 			PTP_Debug_Print("[Error] PTP_API_Get_FilesList : ptp_getobjectinfo return(%x)\n", result);
 			continue;
 		}
 		
 		if (oi.ObjectFormat != PTP_OFC_Association) continue;
 		
 		// 2007.08.06,  Kyungsik
// ¹®Á¦Á¡ : µ¿¿µ»ó(*.MOV) ÆÄÀÏÀ» folder·Î ºÐ·ùÇÏ¿© ObjectFormatÀ» folder·Î return 
// ´ëÃ¥ : ObjectFormat(folder,0x3001)ÀÎ °æ¿ì, objectinfoÀÇ  AssociationType(folder,0x0001) ¿©ºÎ¸¦  Ã¼Å©ÇÑ´Ù.
//		ÀÌ °æ¿ì AssociationTypeÀ» Áö¿øÇÏÁö ¾Ê´Â Àåºñ°¡ Á¸ÀçÇÒ ¼ö ÀÖÀ¸¹Ç·Î, AssociationType(undef, 0x0000)µµ È®ÀÎÇØ¾ß ÇÑ´Ù.
// Àåºñ¸í :  Panasonic FX12 
 		if (oi.AssociationType > PTP_AT_GenericFolder)
 		{
 			PTP_Debug_Print("[Error] This file is not folder \n");
 			continue;
 		}
 		
 		strcpy ((*fileinfo)[n].filename, oi.Filename);
 		(*fileinfo)[n].filetype = DIR_TYPE;
 		(*fileinfo)[n].storageId = oi.StorageID;
 		(*fileinfo)[n].ParentObject = oi.ParentObject;
 		(*fileinfo)[n].handle = oh1.Handler[nIndex];
 		
 		PTP_API_Clear_ObjectInfo (&oi);
 		++n;
 	}
 	
 	
 	for (nIndex = 0; nIndex < numjpg/*oh2.n*/; nIndex++)
 	{
 		PTP_API_Init_ObjectInfo(&oi);
 		result = ptp_getobjectinfo(&(gdc.params), oh2.Handler[nIndex],&oi);
 		if ( result != PTP_RC_OK )
 		{
 			PTP_Debug_Print("[Error] PTP_API_Get_FilesList : ptp_getobjectinfo return(%x)\n", result);
 			continue;
 		}
 		
 		if (oi.ObjectFormat != PTP_OFC_EXIF_JPEG) continue;
 		
 		tmPtr = NULL;
 		tmPtr = localtime_r ( (const time_t*)&oi.CaptureDate, &lt);
 		if (tmPtr != NULL)
 		{
 			(*fileinfo)[n].year = tmPtr->tm_year + 1900;
 			(*fileinfo)[n].month = tmPtr->tm_mon + 1;
 			(*fileinfo)[n].day = tmPtr->tm_mday ;
 			(*fileinfo)[n].hour = tmPtr->tm_hour;
 			(*fileinfo)[n].min = tmPtr->tm_min;
 			(*fileinfo)[n].sec = tmPtr->tm_sec;
 		}
 		
 		strcpy ((*fileinfo)[n].filename, oi.Filename);
 		(*fileinfo)[n].filetype = FILE_TYPE;
 		(*fileinfo)[n].storageId = oi.StorageID;
 		(*fileinfo)[n].ParentObject = oi.ParentObject;
 		(*fileinfo)[n].handle = oh2.Handler[nIndex];
 		
 		PTP_API_Clear_ObjectInfo (&oi);
 		++n;
 	}
 	
 	// 2007.08.06,  ssu
 	// For the error case
 	// If any file or folder's association type is not proper, at that time we should count again the real number of files.
 	if (*nCount != n)
 		*nCount = n;
 	
 	edTick = Tick() ;
 	PTP_Debug_Print ("PTP_API_Get_FilesList : oh1.n = %d, oh2.n = %d, storageId= %x, parent handle = %x, speed = %d(ms)\n", 
 		oh1.n, oh2.n, storageID, ParentObjectHandle, (edTick -stTick ));
 	
 	PTP_API_Clear_ObjectHandles(&oh1);
 	PTP_API_Clear_ObjectHandles(&oh2);
 	
 	PTP_END;
 	
 	PTP_Debug_Print("PTP_API_Get_FilesList : end (%x)\n\n", result);
 	
 	return result;	
#endif

	storageID = 0; //intialising unused variable
	ParentObjectHandle = 0;
	fileinfo = NULL;
	*nCount = 0;

	return PTPAPI_ERROR;
 }


/**
* @brief	      Get file list from the handle
* @remarks		file list is including folders and jpg files
* @param		uint32 storageID			: storage id
* @param		uint32 ParentObjectHandle 	: handle 
* @param		PTPFileListInfo **fileinfo 	: store of filelist   
* @param		uint32 *nCount 			: the num of files(including folders)   
* @param		PTPDevContext *dc		:Device context of the given device
* @return		uint16
PTP_RC_OK : success, else : failure
*/
uint16 PTP_API_Get_FilesList_MPTP (uint32 storageID, uint32 ParentObjectHandle, PTPFileListInfo **fileinfo, uint32 *nCount,PTPDeviceHandle  ptpHandle)
{
	
	uint32	n = 0;
	uint32	nIndex = 0;
	uint16	result = PTP_RC_OK;
	
	//	int		nRet = 0;
	//	char		foldername[MAX_FILENAME] = {'0'};
	//	char		filename[MAX_FILENAME] = {'0'};
	
	struct tm* tmPtr = 0;
	struct tm lt;
	
	PTPObjectInfo		oi;
	PTPObjectHandles	oh1, oh2;
	
	uint32 i = 0;
	uint32 numdir = 0;	
	uint32 numjpg = 0;
	
	unsigned long stTick = 0;
	unsigned long edTick = 0;

	PTPDevContext *dc;
	dc=getDc(ptpHandle);
	
	if(dc==NULL)
	{
		PTP_Debug_Print("[ERROR] PTP_API_Get_FilesList_MPTP : PTPDevContext is null.\n");
		return PTPAPI_ERROR;//added by Niraj   
	}
	if (PTP_API_CHKDEV_MPTP(ptpHandle) != true)
		return PTPAPI_ERROR; //just representing status error
	
	PTP_Init_Transaction (dc);
	
	PTP_API_Init_ObjectHandles(&oh1);
	PTP_API_Init_ObjectHandles(&oh2);
	
	PTP_Debug_Print("PTP_API_Get_FilesList : start(H:%x)\n", ParentObjectHandle);
	
	PTP_BEGIN;		
	
	stTick = Tick() ;
	
	//Find All list from the ParentObjectHandle
	result = ptp_getobjecthandles (&(dc->params),storageID, 0x00000000, ParentObjectHandle, &(dc->params.handles));
	if (result != PTP_RC_OK)
	{
		PTP_END;
		return result;
	}
	
	PTP_Debug_Print("The # of handles : %d\n", dc->params.handles.n);
	PTP_Debug_Print("Handler:        size: \tname:\n");
	for (i = 0; i < dc->params.handles.n; i++) {
		result = ptp_getobjectinfo(&(dc->params),dc->params.handles.Handler[i], &oi);
		if (result != PTP_RC_OK)
		{
			PTP_END;
			return result;
		}
		
		if (oi.ObjectFormat == PTP_OFC_Association)
		{
			numdir++;
			continue;
		}
		if (oi.ObjectFormat == PTP_OFC_EXIF_JPEG)
			numjpg ++;
		PTP_Debug_Print("0x%08x: % 9i\t%s\n",dc->params.handles.Handler[i],
			oi.ObjectCompressedSize, oi.Filename);
	}
	
	PTP_Debug_Print("the num of dir : %d \n", numdir);
	PTP_Debug_Print("the num of jpg : %d \n", numjpg);
	
	
	// SONY DSC T20Ã¯Â¿Â½Ã¯Â¿Â½ Ã¯Â¿Â½Ã¯Â¿Â½Ã¯Â¿Â½Ã¯Â¿Â½, PTP_OFC_Association cmdÃ¯Â¿Â½Ã¯Â¿Â½ Ã¯Â¿Â½Ã¯Â¿Â½Ã¯Â¿Â½Ã¯Â¿Â½Ã¯Â¿Â½Ã¯Â¿Â½ Ã¯Â¿Â½Ã¯Â¿Â½Ã¯Â¿Â½Ã¯Â¿Â½  handle list Ã¯Â¿Â½Ã¯Â¿Â½ÃƒÂ»Ã¯Â¿Â½Ã¯Â¿Â½ deviceÃ¯Â¿Â½Ã¯Â¿Â½ STALLÃ¯Â¿Â½Ã¯Â¿Â½
	// Ã¯Â¿Â½Ã¯Â¿Â½Ã¯Â¿Â½Ã¯Â¿Â½,Ã¯Â¿Â½Ã¯Â¿Â½Ã¯Â¿Â½Ã¯Â¿Â½ Ã¯Â¿Â½Ã¯Â¿Â½Ã¯Â¿Â½Ã¯Â¿Â½Ã†Â® Ã¯Â¿Â½Ã¯Â¿Â½Ã¯Â¿Â½Ã¯Â¿Â½Ã¯Â¿Â½Ã¯Â¿Â½ NULL Ã¯Â¿Â½Ã¯Â¿Â½Ã¯Â¿Â½ ÃƒÂ¼Ã…Â©Ã¯Â¿Â½Ã˜Â¼Ã¯Â¿Â½ handle list Ã¯Â¿Â½Ã¯Â¿Â½ÃƒÂ»
	if(numdir != 0)
	{
		result = ptp_getobjecthandles (&(dc->params), storageID, PTP_OFC_Association, ParentObjectHandle, &(oh1));
		if( result != PTP_RC_OK) 
		{
			PTP_Debug_Print("[Error] PTP_API_Get_FilesList : ptp_getobjecthandles(folder) return (%x)\n", result);
			PTP_END;
			return result;
		}	
	}
	
	if(numjpg != 0)
	{
		result = ptp_getobjecthandles (&(dc->params), storageID, PTP_OFC_EXIF_JPEG, ParentObjectHandle, &(oh2));
		if( result != PTP_RC_OK) 
		{
			PTP_Debug_Print("[Error] PTP_API_Get_FilesList : ptp_getobjecthandles(file) return (%x)\n", result);
			PTP_END;		
			return result;		
		}	
	}
	
	
	*nCount = numdir + numjpg;/*oh1.n + oh2.n;*/
	if (*nCount <= 0)
	{
		PTP_Debug_Print("[Error] PTP_API_Get_FilesList : oh1.n + oh2.n is less than 0\n");
		PTP_END;		
		return PTPAPI_ERROR; // just representing wrong count error		
	}
	
	*fileinfo = (PTPFileListInfo *)calloc (sizeof(PTPFileListInfo), *nCount);
	if (*fileinfo == NULL)
	{
		PTP_Debug_Print("[Error] PTP_API_Get_FilesList : memory alloc error(PTPFileListInfo)\n");
		PTP_END;		
		return PTPAPI_ERROR; // just representing memory allocation error		
	}
	
	
	n = 0;
	for (nIndex = 0; nIndex <numdir /*oh1.n*/; nIndex++) 
	{
		PTP_API_Init_ObjectInfo(&oi);
		result = ptp_getobjectinfo(&(dc->params), oh1.Handler[nIndex], &oi);
		if ( result != PTP_RC_OK )
		{
			PTP_Debug_Print("[Error] PTP_API_Get_FilesList : ptp_getobjectinfo return(%x)\n", result);
			continue;
		}
		
		if (oi.ObjectFormat != PTP_OFC_Association) continue;
		
		// 2007.08.06,  Kyungsik
		// Ã¯Â¿Â½Ã¯Â¿Â½Ã¯Â¿Â½Ã¯Â¿Â½ : Ã¯Â¿Â½Ã¯Â¿Â½Ã¯Â¿Â½Ã¯Â¿Â½Ã¯Â¿Â½Ã¯Â¿Â½(*.MOV) Ã¯Â¿Â½Ã¯Â¿Â½Ã¯Â¿Â½Ã¯Â¿Â½Ã¯Â¿Â½ folderÃ¯Â¿Â½Ã¯Â¿Â½ Ã¯Â¿Â½ÃÂ·Ã¯Â¿Â½Ã¯Â¿Â½ÃÂ¿Ã¯Â¿Â½ ObjectFormatÃ¯Â¿Â½ folderÃ¯Â¿Â½Ã¯Â¿Â½ return 
		// Ã¯Â¿Â½Ã¯Â¿Â½ÃƒÂ¥ : ObjectFormat(folder,0x3001)Ã¯Â¿Â½Ã¯Â¿Â½ Ã¯Â¿Â½Ã¯Â¿Â½Ã¯Â¿Â½Ã¯Â¿Â½, objectinfoÃ¯Â¿Â½Ã¯Â¿Â½  AssociationType(folder,0x0001) Ã¯Â¿Â½Ã¯Â¿Â½Ã¯Â¿Â½ÃŽÂ¸Ã¯Â¿Â½  ÃƒÂ¼Ã…Â©Ã¯Â¿Â½Ã‘Â´Ã¯Â¿Â½.
		//		Ã¯Â¿Â½Ã¯Â¿Â½ Ã¯Â¿Â½Ã¯Â¿Â½Ã¯Â¿Â½Ã¯Â¿Â½ AssociationTypeÃ¯Â¿Â½ Ã¯Â¿Â½Ã¯Â¿Â½Ã¯Â¿Â½Ã¯Â¿Â½Ã¯Â¿Â½Ã¯Â¿Â½Ã¯Â¿Â½Ã¯Â¿Â½ Ã¯Â¿Â½ÃŠÂ´Ã¯Â¿Â½ Ã¯Â¿Â½Ã¯Â¿Â½Ã¯Â¿Â½Ã¯Â¿Â½Ã¯Â¿Â½Ã¯Â¿Â½ Ã¯Â¿Â½Ã¯Â¿Â½Ã¯Â¿Â½Ã¯Â¿Â½Ã¯Â¿Â½ Ã¯Â¿Â½Ã¯Â¿Â½ Ã¯Â¿Â½Ã¯Â¿Â½Ã¯Â¿Â½Ã¯Â¿Â½Ã‡Â·Ã¯Â¿Â½, AssociationType(undef, 0x0000)Ã¯Â¿Â½Ã¯Â¿Â½ ÃˆÂ®Ã¯Â¿Â½Ã¯Â¿Â½Ã¯Â¿Â½Ã˜Â¾Ã¯Â¿Â½ Ã¯Â¿Â½Ã‘Â´Ã¯Â¿Â½.
		// Ã¯Â¿Â½Ã¯Â¿Â½Ã¯Â¿Â½Ã¯Â¿Â½Ã¯Â¿Â½Ã¯Â¿Â½ :  Panasonic FX12 
		if (oi.AssociationType > PTP_AT_GenericFolder)
		{
			PTP_Debug_Print("[Error] This file is not folder \n");
			continue;
		}
		
		strcpy ((*fileinfo)[n].filename, oi.Filename);
		(*fileinfo)[n].filetype = DIR_TYPE;
		(*fileinfo)[n].storageId = oi.StorageID;
		(*fileinfo)[n].ParentObject = oi.ParentObject;
		(*fileinfo)[n].handle = oh1.Handler[nIndex];
		
		PTP_API_Clear_ObjectInfo (&oi);
		++n;
	}
	
	
	for (nIndex = 0; nIndex < numjpg/*oh2.n*/; nIndex++)
	{
		PTP_API_Init_ObjectInfo(&oi);
		result = ptp_getobjectinfo(&(dc->params), oh2.Handler[nIndex],&oi);
		if ( result != PTP_RC_OK )
		{
			PTP_Debug_Print("[Error] PTP_API_Get_FilesList : ptp_getobjectinfo return(%x)\n", result);
			continue;
		}
		
		if (oi.ObjectFormat != PTP_OFC_EXIF_JPEG) continue;
		
		tmPtr = NULL;
		tmPtr = localtime_r ( (const time_t*)&oi.CaptureDate, &lt);
		if (tmPtr != NULL)
		{
			(*fileinfo)[n].year = tmPtr->tm_year + 1900;
			(*fileinfo)[n].month = tmPtr->tm_mon + 1;
			(*fileinfo)[n].day = tmPtr->tm_mday ;
			(*fileinfo)[n].hour = tmPtr->tm_hour;
			(*fileinfo)[n].min = tmPtr->tm_min;
			(*fileinfo)[n].sec = tmPtr->tm_sec;
		}
		
		strcpy ((*fileinfo)[n].filename, oi.Filename);
		(*fileinfo)[n].filetype = FILE_TYPE;
		(*fileinfo)[n].storageId = oi.StorageID;
		(*fileinfo)[n].ParentObject = oi.ParentObject;
		(*fileinfo)[n].handle = oh2.Handler[nIndex];
		//printf("\nfilename= %s\n",oi.Filename);//vishal
		PTP_API_Clear_ObjectInfo (&oi);
		++n;
	}
	
	// 2007.08.06,  ssu
	// For the error case
	// If any file or folder's association type is not proper, at that time we should count again the real number of files.
	if (*nCount != n)
		*nCount = n;
	
	edTick = Tick() ;
	PTP_Debug_Print ("PTP_API_Get_FilesList : oh1.n = %d, oh2.n = %d, storageId= %x, parent handle = %x, speed = %d(ms)\n", 
		oh1.n, oh2.n, storageID, ParentObjectHandle, (edTick -stTick ));
	
	PTP_API_Clear_ObjectHandles(&oh1);
	PTP_API_Clear_ObjectHandles(&oh2);
	
	PTP_END;
	
	PTP_Debug_Print("PTP_API_Get_FilesList : end (%x)\n\n", result);
	
	return result;	
}



/**
* @brief	      Get JPG file list from the handle (same with PTP_API_Get_FileList)
* @remarks		file list is including folders and jpg files
* @param		uint32 storageID			: storage id
* @param		uint32 ParentObjectHandle 	: handle 
* @param		PTPFileListInfo **fileinfo 	: store of filelist   
* @param		uint32 *nCount 			: the num of files(including folders)   
* @return		uint16
PTP_RC_OK : success, else : failure
*/ 
 uint16 PTP_API_Get_JpegFilesList (uint32 storageID, uint32 ParentObjectHandle, PTPFileListInfo **fileinfo, uint32 *nCount)
 {
#if 0
 	uint32	n = 0;
 	uint32	nIndex = 0;
 	uint16	result = PTP_RC_OK;
 	
 	int	nRet = 0;
 	char	foldername[MAX_FILENAME] = {'0'};
 	char	filename[MAX_FILENAME] = {'0'};
 	
 	struct tm* tmPtr = 0;
 	struct tm lt;
 	
 	PTPObjectInfo		oi;
 	PTPObjectHandles	oh1, oh2;
 	
 	int i = 0;
 	uint32 numjpg = 0;
 	uint32 numdir = 0;
 	
 	if (PTP_API_CHKDEV() != true)
 		return PTPAPI_ERROR; //just representing status error
 	
 	PTP_Init_Transaction (&gdc);
 	
 	PTP_API_Init_ObjectHandles(&oh1);
 	PTP_API_Init_ObjectHandles(&oh2);
 	
 	PTP_Debug_Print("PTP_API_Get_JpegFilesList : start(H:%x)\n", ParentObjectHandle);	
 	
 	PTP_BEGIN;
 	
 	//Find All list from the ParentObjectHandle
 	result = ptp_getobjecthandles (&(gdc.params),storageID, 0x00000000, ParentObjectHandle, &(gdc.params.handles));
 	if (result != PTP_RC_OK)
 	{
 		PTP_END;		
 		return result;
 	}
 	
 	PTP_Debug_Print("The # of handles : %d\n", gdc.params.handles.n);
 	PTP_Debug_Print("Handler:        size: \tname:\n");
 	for (i = 0; i < gdc.params.handles.n; i++) {
 		result = ptp_getobjectinfo(&(gdc.params),gdc.params.handles.Handler[i], &oi);
 		if (result != PTP_RC_OK)
 		{
 			PTP_END;			
 			return result;
 		}
 		
 		if (oi.ObjectFormat == PTP_OFC_Association)
 		{
 			numdir++;
 			continue;
 		}
 		if (oi.ObjectFormat == PTP_OFC_EXIF_JPEG)
 			numjpg ++;
 		PTP_Debug_Print("0x%08x: % 9i\t%s\n",gdc.params.handles.Handler[i],
 			oi.ObjectCompressedSize, oi.Filename);
 	}
 	
 	PTP_Debug_Print("the num of dir : %d \n", numdir);
 	PTP_Debug_Print("the num of jpg : %d \n", numjpg);
 	
 	
	// SONY DSC T20ÀÇ °æ¿ì, PTP_OFC_Association cmd·Î Æú´õ¿¡ ´ëÇÑ  handle list ¿äÃ»½Ã device°¡ STALLµÊ
	// ÆÄÀÏ,Æú´õ ¸®½ºÆ® ±¸¼º½Ã NULL °ªÀ» Ã¼Å©ÇØ¼­ handle list ¿äÃ»
 	if(numdir != 0)
 	{
 		result = ptp_getobjecthandles (&(gdc.params), storageID, PTP_OFC_Association, ParentObjectHandle, &(oh1));
 		if( result != PTP_RC_OK) 
 		{
 			PTP_Debug_Print("[Error] PTP_API_Get_JpegFilesList : ptp_getobjecthandles(folder) return (%x)\n", result);
 			PTP_END;		
 			return result;
 		}
 	}
 	
 	if(numjpg != 0)
 	{
 		result = ptp_getobjecthandles (&(gdc.params), storageID, PTP_OFC_EXIF_JPEG, ParentObjectHandle, &(oh2));
 		if( result != PTP_RC_OK) 
 		{
 			PTP_Debug_Print("[Error] PTP_API_Get_JpegFilesList : ptp_getobjecthandles(file) return (%x)\n", result);
 			PTP_END;
 			return result;
 		}	
 	}
 	
 	*nCount = numdir + numjpg; /*oh1.n + oh2.n;*/
 	if (*nCount <= 0)
 	{
 		PTP_Debug_Print("[Error] PTP_API_Get_JpegFilesList : oh1.n + oh2.n is less than 0\n");
 		PTP_END;
 		return PTPAPI_ERROR; // just representing wrong count error		
 	}
 	
 	PTP_Debug_Print ("PTP_API_Get_JpegFilesList : oh1.n = %d, oh2.n = %d, storageId= %x, parent handle = %x\n", oh1.n, oh2.n, storageID, ParentObjectHandle);
 	
 	*fileinfo = (PTPFileListInfo *)calloc (sizeof(PTPFileListInfo), *nCount);
 	if (*fileinfo == NULL)
 	{
 		PTP_Debug_Print("[Error] PTP_API_Get_JpegFilesList : memory alloc error(PTPFileListInfo)\n");
 		PTP_END;
 		return PTPAPI_ERROR; // just representing memory allocation error				
 	}
 	
 	n = 0;
 	for (nIndex = 0; nIndex < numdir/*oh1.n*/; nIndex++) 
 	{
 		PTP_API_Init_ObjectInfo(&oi);
 		result = ptp_getobjectinfo(&(gdc.params), oh1.Handler[nIndex], &oi);
 		if ( result != PTP_RC_OK )
 		{
 			PTP_Debug_Print("[Error] PTP_API_Get_JpegFilesList : ptp_getobjectinfo return(%x)\n", result);
 			continue;
 		}
 		
 		if (oi.ObjectFormat != PTP_OFC_Association) continue;
 		
 		
 		// 2007.08.06,  Kyungsik
// ¹®Á¦Á¡ : µ¿¿µ»ó(*.MOV) ÆÄÀÏÀ» folder·Î ºÐ·ùÇÏ¿© ObjectFormatÀ» folder·Î return 
// ´ëÃ¥ : ObjectFormat(folder,0x3001)ÀÎ °æ¿ì, objectinfoÀÇ  AssociationType(folder,0x0001) ¿©ºÎ¸¦  Ã¼Å©ÇÑ´Ù.
//		ÀÌ °æ¿ì AssociationTypeÀ» Áö¿øÇÏÁö ¾Ê´Â Àåºñ°¡ Á¸ÀçÇÒ ¼ö ÀÖÀ¸¹Ç·Î, AssociationType(undef, 0x0000)µµ È®ÀÎÇØ¾ß ÇÑ´Ù.
// Àåºñ¸í :  Panasonic FX12 
 		if (oi.AssociationType > PTP_AT_GenericFolder)
 		{
 			PTP_Debug_Print("[Error] This file is not folder \n");
 			continue;
 		}
 		
 		strcpy ((*fileinfo)[n].filename, oi.Filename);
 		(*fileinfo)[n].filetype = DIR_TYPE;
 		(*fileinfo)[n].storageId = oi.StorageID;
 		(*fileinfo)[n].ParentObject = oi.ParentObject;
 		(*fileinfo)[n].handle = oh1.Handler[nIndex];
 		
 		PTP_API_Clear_ObjectInfo (&oi);
 		++n;
 	}
 	
 	
 	for (nIndex = 0; nIndex < numjpg/*oh2.n*/; nIndex++)
 	{
 		PTP_API_Init_ObjectInfo(&oi);
 		result = ptp_getobjectinfo(&(gdc.params), oh2.Handler[nIndex],&oi);
 		if ( result != PTP_RC_OK )
 		{
 			PTP_Debug_Print("[Error] PTP_API_Get_JpegFilesList : ptp_getobjectinfo return(%x)\n", result);
 			continue;
 		}
 		
 		if (oi.ObjectFormat != PTP_OFC_EXIF_JPEG) continue;
 		
 		tmPtr = NULL;
 		tmPtr = localtime_r ( (const time_t*)&oi.CaptureDate, &lt);
 		if (tmPtr != NULL)
 		{
 			(*fileinfo)[n].year = tmPtr->tm_year + 1900;
 			(*fileinfo)[n].month = tmPtr->tm_mon + 1;
 			(*fileinfo)[n].day = tmPtr->tm_mday ;
 			(*fileinfo)[n].hour = tmPtr->tm_hour;
 			(*fileinfo)[n].min = tmPtr->tm_min;
 			(*fileinfo)[n].sec = tmPtr->tm_sec;
 		}
 		
 		strcpy ((*fileinfo)[n].filename, oi.Filename);
 		(*fileinfo)[n].filetype = FILE_TYPE;
 		(*fileinfo)[n].storageId = oi.StorageID;
 		(*fileinfo)[n].ParentObject = oi.ParentObject;
 		(*fileinfo)[n].handle = oh2.Handler[nIndex];
 		
 		PTP_API_Clear_ObjectInfo (&oi);
 		++n;
 	}
 	
 	// 2007.08.06,  ssu
 	// For the error case
 	// If any file or folder's association type is not proper, at that time we should count again the real number of files.
 	if (*nCount != n)
 		*nCount = n;
 	
 	PTP_API_Clear_ObjectHandles(&oh1);
 	PTP_API_Clear_ObjectHandles(&oh2);
 	
 	PTP_END;
 	
 	PTP_Debug_Print("PTP_API_Get_JpegFilesList : end (%x)\n\n", result);
 	
 	return result;	
#endif

	//intialising to unused variable
	storageID = 0;
	ParentObjectHandle = 0;
	fileinfo = NULL;
	*nCount = 0;

	return PTPAPI_ERROR;
 }

/**
* @brief	      Get JPG file list from the handle (same with PTP_API_Get_FileList)
* @remarks		file list is including folders and jpg files
* @param		uint32 storageID			: storage id
* @param		uint32 ParentObjectHandle 	: handle 
* @param		PTPFileListInfo **fileinfo 	: store of filelist   
* @param		uint32 *nCount 			: the num of files(including folders)
* @param	      PTPDeviceHandle ptpHandle:	Handle for the given ptp device
* @return		uint16
PTP_RC_OK : success, else : failure
*/ 
uint16 PTP_API_Get_JpegFilesList_MPTP (uint32 storageID, uint32 ParentObjectHandle, PTPFileListInfo **fileinfo, uint32 *nCount,PTPDeviceHandle  ptpHandle)
{
	
	uint32	n = 0;
	uint32	nIndex = 0;
	uint16	result = PTP_RC_OK;
	
	//int	nRet = 0;
	//char	foldername[MAX_FILENAME] = {'0'};
	//char	filename[MAX_FILENAME] = {'0'};
	
	struct tm* tmPtr = 0;
	struct tm lt;
	
	PTPObjectInfo		oi;
	PTPObjectHandles	oh1, oh2;
	
	uint32 i = 0;
	uint32 numjpg = 0;
	uint32 numdir = 0;


	PTPDevContext *dc;
	dc=getDc(ptpHandle);
	if(dc==NULL)
	{
		PTP_Debug_Print("[ERROR] PTP_API_Get_JpegFilesList_MPTP : PTPDevContext is null.\n");
		return PTPAPI_ERROR;//added by Niraj   
	}
	if (PTP_API_CHKDEV_MPTP(ptpHandle) != true)
		return PTPAPI_ERROR; //just representing status error
	
	PTP_Init_Transaction (dc);
	
	PTP_API_Init_ObjectHandles(&oh1);
	PTP_API_Init_ObjectHandles(&oh2);
	
	PTP_Debug_Print("PTP_API_Get_JpegFilesList : start(H:%x)\n", ParentObjectHandle);	
	
	PTP_BEGIN;
	
	//Find All list from the ParentObjectHandle
	result = ptp_getobjecthandles (&(dc->params),storageID, 0x00000000, ParentObjectHandle, &(dc->params.handles));
	
	if (result != PTP_RC_OK)
	{
		PTP_END;		
		return result;
	}
	
	PTP_Debug_Print("The # of handles : %d\n", dc->params.handles.n);
	PTP_Debug_Print("Handler:        size: \tname:\n");
	for (i = 0; i < dc->params.handles.n; i++) {
		result = ptp_getobjectinfo(&(dc->params),dc->params.handles.Handler[i], &oi);
		if (result != PTP_RC_OK)
		{
			PTP_END;			
			return result;
		}
		
		if (oi.ObjectFormat == PTP_OFC_Association)
		{
			numdir++;
			continue;
		}
		if (oi.ObjectFormat == PTP_OFC_EXIF_JPEG)
			numjpg ++;
		PTP_Debug_Print("0x%08x: % 9i\t%s\n",dc->params.handles.Handler[i],
			oi.ObjectCompressedSize, oi.Filename);
	}
	
	PTP_Debug_Print("the num of dir : %d \n", numdir);
	PTP_Debug_Print("the num of jpg : %d \n", numjpg);

	
	// SONY DSC T20Ã¯Â¿Â½Ã¯Â¿Â½ Ã¯Â¿Â½Ã¯Â¿Â½Ã¯Â¿Â½Ã¯Â¿Â½, PTP_OFC_Association cmdÃ¯Â¿Â½Ã¯Â¿Â½ Ã¯Â¿Â½Ã¯Â¿Â½Ã¯Â¿Â½Ã¯Â¿Â½Ã¯Â¿Â½Ã¯Â¿Â½ Ã¯Â¿Â½Ã¯Â¿Â½Ã¯Â¿Â½Ã¯Â¿Â½  handle list Ã¯Â¿Â½Ã¯Â¿Â½ÃƒÂ»Ã¯Â¿Â½Ã¯Â¿Â½ deviceÃ¯Â¿Â½Ã¯Â¿Â½ STALLÃ¯Â¿Â½Ã¯Â¿Â½
	// Ã¯Â¿Â½Ã¯Â¿Â½Ã¯Â¿Â½Ã¯Â¿Â½,Ã¯Â¿Â½Ã¯Â¿Â½Ã¯Â¿Â½Ã¯Â¿Â½ Ã¯Â¿Â½Ã¯Â¿Â½Ã¯Â¿Â½Ã¯Â¿Â½Ã†Â® Ã¯Â¿Â½Ã¯Â¿Â½Ã¯Â¿Â½Ã¯Â¿Â½Ã¯Â¿Â½Ã¯Â¿Â½ NULL Ã¯Â¿Â½Ã¯Â¿Â½Ã¯Â¿Â½ ÃƒÂ¼Ã…Â©Ã¯Â¿Â½Ã˜Â¼Ã¯Â¿Â½ handle list Ã¯Â¿Â½Ã¯Â¿Â½ÃƒÂ»
	if(numdir != 0)
	{
		result = ptp_getobjecthandles (&(dc->params), storageID, PTP_OFC_Association, ParentObjectHandle, &(oh1));
		if( result != PTP_RC_OK) 
		{
			PTP_Debug_Print("[Error] PTP_API_Get_JpegFilesList : ptp_getobjecthandles(folder) return (%x)\n", result);
			PTP_END;		
			return result;
		}
	}
	
	if(numjpg != 0)
	{
		result = ptp_getobjecthandles (&(dc->params), storageID, PTP_OFC_EXIF_JPEG, ParentObjectHandle, &(oh2));
		if( result != PTP_RC_OK) 
		{
			PTP_Debug_Print("[Error] PTP_API_Get_JpegFilesList : ptp_getobjecthandles(file) return (%x)\n", result);
			PTP_END;
			return result;
		}	
	}
	
	*nCount = numdir + numjpg; /*oh1.n + oh2.n;*/
	if (*nCount <= 0)
	{
		PTP_Debug_Print("[Error] PTP_API_Get_JpegFilesList : oh1.n + oh2.n is less than 0\n");
		PTP_END;
		return PTPAPI_ERROR; // just representing wrong count error		
	}
	
	PTP_Debug_Print ("PTP_API_Get_JpegFilesList : oh1.n = %d, oh2.n = %d, storageId= %x, parent handle = %x\n", oh1.n, oh2.n, storageID, ParentObjectHandle);
	
	*fileinfo = (PTPFileListInfo *)calloc (sizeof(PTPFileListInfo), *nCount);
	if (*fileinfo == NULL)
	{
		PTP_Debug_Print("[Error] PTP_API_Get_JpegFilesList : memory alloc error(PTPFileListInfo)\n");
		PTP_END;
		return PTPAPI_ERROR; // just representing memory allocation error				
	}
	
	n = 0;
	for (nIndex = 0; nIndex < numdir/*oh1.n*/; nIndex++) 
	{
		PTP_API_Init_ObjectInfo(&oi);
		result = ptp_getobjectinfo(&(dc->params), oh1.Handler[nIndex], &oi);
		if ( result != PTP_RC_OK )
		{
			PTP_Debug_Print("[Error] PTP_API_Get_JpegFilesList : ptp_getobjectinfo return(%x)\n", result);
			continue;
		}
		
		if (oi.ObjectFormat != PTP_OFC_Association) continue;
		
		
		// 2007.08.06,  Kyungsik
		// Ã¯Â¿Â½Ã¯Â¿Â½Ã¯Â¿Â½Ã¯Â¿Â½ : Ã¯Â¿Â½Ã¯Â¿Â½Ã¯Â¿Â½Ã¯Â¿Â½Ã¯Â¿Â½Ã¯Â¿Â½(*.MOV) Ã¯Â¿Â½Ã¯Â¿Â½Ã¯Â¿Â½Ã¯Â¿Â½Ã¯Â¿Â½ folderÃ¯Â¿Â½Ã¯Â¿Â½ Ã¯Â¿Â½ÃÂ·Ã¯Â¿Â½Ã¯Â¿Â½ÃÂ¿Ã¯Â¿Â½ ObjectFormatÃ¯Â¿Â½ folderÃ¯Â¿Â½Ã¯Â¿Â½ return 
		// Ã¯Â¿Â½Ã¯Â¿Â½ÃƒÂ¥ : ObjectFormat(folder,0x3001)Ã¯Â¿Â½Ã¯Â¿Â½ Ã¯Â¿Â½Ã¯Â¿Â½Ã¯Â¿Â½Ã¯Â¿Â½, objectinfoÃ¯Â¿Â½Ã¯Â¿Â½  AssociationType(folder,0x0001) Ã¯Â¿Â½Ã¯Â¿Â½Ã¯Â¿Â½ÃŽÂ¸Ã¯Â¿Â½  ÃƒÂ¼Ã…Â©Ã¯Â¿Â½Ã‘Â´Ã¯Â¿Â½.
		//		Ã¯Â¿Â½Ã¯Â¿Â½ Ã¯Â¿Â½Ã¯Â¿Â½Ã¯Â¿Â½Ã¯Â¿Â½ AssociationTypeÃ¯Â¿Â½ Ã¯Â¿Â½Ã¯Â¿Â½Ã¯Â¿Â½Ã¯Â¿Â½Ã¯Â¿Â½Ã¯Â¿Â½Ã¯Â¿Â½Ã¯Â¿Â½ Ã¯Â¿Â½ÃŠÂ´Ã¯Â¿Â½ Ã¯Â¿Â½Ã¯Â¿Â½Ã¯Â¿Â½Ã¯Â¿Â½Ã¯Â¿Â½Ã¯Â¿Â½ Ã¯Â¿Â½Ã¯Â¿Â½Ã¯Â¿Â½Ã¯Â¿Â½Ã¯Â¿Â½ Ã¯Â¿Â½Ã¯Â¿Â½ Ã¯Â¿Â½Ã¯Â¿Â½Ã¯Â¿Â½Ã¯Â¿Â½Ã‡Â·Ã¯Â¿Â½, AssociationType(undef, 0x0000)Ã¯Â¿Â½Ã¯Â¿Â½ ÃˆÂ®Ã¯Â¿Â½Ã¯Â¿Â½Ã¯Â¿Â½Ã˜Â¾Ã¯Â¿Â½ Ã¯Â¿Â½Ã‘Â´Ã¯Â¿Â½.
		// Ã¯Â¿Â½Ã¯Â¿Â½Ã¯Â¿Â½Ã¯Â¿Â½Ã¯Â¿Â½Ã¯Â¿Â½ :  Panasonic FX12 
		if (oi.AssociationType > PTP_AT_GenericFolder)
		{
			PTP_Debug_Print("[Error] This file is not folder \n");
			continue;
		}
		
		strcpy ((*fileinfo)[n].filename, oi.Filename);
		(*fileinfo)[n].filetype = DIR_TYPE;
		(*fileinfo)[n].storageId = oi.StorageID;
		(*fileinfo)[n].ParentObject = oi.ParentObject;
		(*fileinfo)[n].handle = oh1.Handler[nIndex];
		
		PTP_API_Clear_ObjectInfo (&oi);
		++n;
	}
	
	
	for (nIndex = 0; nIndex < numjpg/*oh2.n*/; nIndex++)
	{
		PTP_API_Init_ObjectInfo(&oi);
		result = ptp_getobjectinfo(&(dc->params), oh2.Handler[nIndex],&oi);
		if ( result != PTP_RC_OK )
		{
			PTP_Debug_Print("[Error] PTP_API_Get_JpegFilesList : ptp_getobjectinfo return(%x)\n", result);
			continue;
		}
		
		if (oi.ObjectFormat != PTP_OFC_EXIF_JPEG) continue;
		
		tmPtr = NULL;
		tmPtr = localtime_r ( (const time_t*)&oi.CaptureDate, &lt);
		if (tmPtr != NULL)
		{
			(*fileinfo)[n].year = tmPtr->tm_year + 1900;
			(*fileinfo)[n].month = tmPtr->tm_mon + 1;
			(*fileinfo)[n].day = tmPtr->tm_mday ;
			(*fileinfo)[n].hour = tmPtr->tm_hour;
			(*fileinfo)[n].min = tmPtr->tm_min;
			(*fileinfo)[n].sec = tmPtr->tm_sec;
		}
		
		strcpy ((*fileinfo)[n].filename, oi.Filename);
		(*fileinfo)[n].filetype = FILE_TYPE;
		(*fileinfo)[n].storageId = oi.StorageID;
		(*fileinfo)[n].ParentObject = oi.ParentObject;
		(*fileinfo)[n].handle = oh2.Handler[nIndex];
		

		PTP_API_Clear_ObjectInfo (&oi);
		++n;
	}
	
	// 2007.08.06,  ssu
	// For the error case
	// If any file or folder's association type is not proper, at that time we should count again the real number of files.
	if (*nCount != n)
		*nCount = n;
	
	PTP_API_Clear_ObjectHandles(&oh1);
	PTP_API_Clear_ObjectHandles(&oh2);
	
	PTP_END;
	
	PTP_Debug_Print("PTP_API_Get_JpegFilesList : end (%x)\n\n", result);
	
	return result;	
}


/**
* @brief	      Get  jpeg file list from the PTP device (same with PTP_API_Get_FileList)
* @remarks		file list is including folders and jpg files, or jpg files only
* @param		uint32 storageID			: (in) storage id
* @param		PTPObjectHandles *oh1	 	: (out) pointer indicating object handles
* @return		uint16
PTP_RC_OK : success, else : failure
*/
 uint16 PTP_API_Get_JpegObjectHandles (uint32 storageID, PTPObjectHandles *oh1 )
 {
#if 0
 	uint16  result = PTP_RC_OK;
 	int     nRet = 0, i;
 	
 	if (PTP_API_CHKDEV() != true)
 		return PTPAPI_ERROR; //just representing status error
 	
 	PTP_Debug_Print("PTP_API_Get_JPEGObjectHandles : start\n");
 	
 	PTP_BEGIN;
 	
 	//Find all jpeg list from the PTP device
	// ÀÏºÎ PTP ÀåºñÀÇ °æ¿ì(panasonic FX12), jpeg handle ÀÌ¿ÜÀÇ handle À» return ÇÏ¹Ç·Î GetObjectinfo¸¦ 
	// object typeÀ» Ã¼Å©ÇØ¼­ filtering ÇØ¾ßÇÑ´Ù. (by kks)
 	result = ptp_getobjecthandles (&(gdc.params),storageID, PTP_OFC_EXIF_JPEG, PTP_HANDLER_ROOT, oh1);
 	if (result != PTP_RC_OK)
 	{
 		PTP_END;
 		PTP_Debug_Print("PTP_API_Get_JPEGObjectHandles : fail (0x%x)\n",result);
 		return result;
 	}
 	
 	PTP_Debug_Print("PTP_API_Get_JPEGObjectHandles : handles num %d\n", oh1->n);
 	for (i = 0; i < oh1->n; i++)
 	{
 		PTP_Debug_Print("handle [%d] : %x \n", i,oh1->Handler[i]);
 	}
 	
 	PTP_END;
 	
 	PTP_Debug_Print("PTP_API_Get_JPEGObjectHandles : succes (0x%x)\n\n", result);
 	
 	return result;
#endif
	storageID=0;
	oh1=NULL;
	return PTPAPI_ERROR;
 }

/**
* @brief	      Get  list of handles of jpeg objects from the given ptp device
* @remarks		handle is only of jpeg objects and not of folders
* @param		uint32 storageID			: (in) storage id
* @param		PTPObjectHandles *oh1	 	: (out) pointer indicating object handles
* @param	      PTPDeviceHandle ptpHandle:	Handle for the given ptp device
* @return		uint16
PTP_RC_OK : success, else : failure
*/
uint16 PTP_API_Get_JpegObjectHandles_MPTP (uint32 storageID, PTPObjectHandles *oh1, PTPDeviceHandle  ptpHandle)
{
	
	uint16	result = PTP_RC_OK;
	//int	nRet = 0, i;
	uint32 i=0;

	PTPDevContext *dc;
	dc=getDc(ptpHandle);
	if(dc==NULL)
	{
		PTP_Debug_Print("[ERROR] PTP_API_JpegObjectHandles_MPTP : PTPDevContext is null.\n");
		return PTPAPI_ERROR;//added by Niraj   
	}
	if (PTP_API_CHKDEV_MPTP(ptpHandle) != true)
		return PTPAPI_ERROR; //just representing status error
	
	PTP_Debug_Print("PTP_API_Get_JPEGObjectHandles : start\n");	
	
	PTP_BEGIN;
	
 	//Find all jpeg list from the PTP device
	// ÀÏºÎ PTP ÀåºñÀÇ °æ¿ì(panasonic FX12), jpeg handle ÀÌ¿ÜÀÇ handle À» return ÇÏ¹Ç·Î GetObjectinfo¸¦ 
	// object typeÀ» Ã¼Å©ÇØ¼­ filtering ÇØ¾ßÇÑ´Ù. (by kks)	
	result = ptp_getobjecthandles (&(dc->params),storageID, PTP_OFC_EXIF_JPEG, PTP_HANDLER_ROOT, oh1);
	if (result != PTP_RC_OK)
	{
		PTP_END;
		PTP_Debug_Print("PTP_API_Get_JPEGObjectHandles : fail (0x%x)\n",result);
		return result;
	}
	
	PTP_Debug_Print("PTP_API_Get_JPEGObjectHandles : handles num %d\n", oh1->n);
	for (i = 0; i < oh1->n; i++)
	{
		PTP_Debug_Print("handle [%d] : %x \n", i,oh1->Handler[i]);
	}
	
	PTP_END;
	
	PTP_Debug_Print("PTP_API_Get_JPEGObjectHandles : succes (0x%x)\n\n", result);
	
	return result;	
}



/**
* @brief	      Get MP3 file list from the handle
* @remarks		file list is including folders and MP3 files
* @param		uint32 storageID			: storage id
* @param		uint32 ParentObjectHandle 	: handle 
* @param		PTPFileListInfo **fileinfo 	: store of filelist   
* @param		uint32 *nCount 			: the num of files(including folders)   
* @return		uint16
PTP_RC_OK : success, else : failure
*/ 
 uint16 PTP_API_Get_MP3FilesList (uint32 storageID, uint32 ParentObjectHandle, PTPFileListInfo **fileinfo, uint32 *nCount)
 {
#if 0
 	uint32	n = 0;  
 	uint32  nIndex = 0;
 	uint16  result = PTP_RC_OK;
 	
 	int     nRet = 0;
 	char    foldername[MAX_FILENAME] = {'0'};
 	char    filename[MAX_FILENAME] = {'0'};
 	
 	struct tm* tmPtr = 0;
 	struct tm lt;
 	
 	PTPObjectInfo           oi;
 	PTPObjectHandles        oh1, oh2;
 	
 	int i = 0;
 	uint32 nummp3 = 0;
 	uint32 numdir = 0;
 	
 	
 	if (PTP_API_CHKDEV() != true)
 		return PTPAPI_ERROR; //just representing status error
 	
 	PTP_Init_Transaction (&gdc);
 	
 	PTP_API_Init_ObjectHandles(&oh1);
 	PTP_API_Init_ObjectHandles(&oh2);
 	
 	PTP_Debug_Print("PTP_API_Get_MP3FilesList : start(H:%x)\n", ParentObjectHandle);
 	
 	PTP_BEGIN;
 	
 	//Find All list from the ParentObjectHandle
 	result = ptp_getobjecthandles (&(gdc.params),storageID, 0x00000000, ParentObjectHandle, &(gdc.params.handles));
 	if (result != PTP_RC_OK)
 	{
 		PTP_END;
 		return result;
 	}
 	
 	PTP_Debug_Print("The # of handles : %d\n", gdc.params.handles.n);
 	PTP_Debug_Print("Handler:        size: \tname:\n");
 	for (i = 0; i < gdc.params.handles.n; i++) {
 		result = ptp_getobjectinfo(&(gdc.params),gdc.params.handles.Handler[i], &oi);
 		if (result != PTP_RC_OK)
 		{
 			PTP_END;
 			return result;
 		}
 		
 		if (oi.ObjectFormat == PTP_OFC_Association)
 		{
 			numdir++;
 			continue;
 		}
 		
 		if (oi.ObjectFormat == PTP_OFC_MP3)
 			nummp3 ++;
 		PTP_Debug_Print("0x%08x: % 9i\t%s\n",gdc.params.handles.Handler[i],
 			oi.ObjectCompressedSize, oi.Filename);
 	}
 	
 	PTP_Debug_Print("the num of dir : %d \n", numdir);
 	PTP_Debug_Print("the num of mp3 : %d \n", nummp3);
 	
	// SONY DSC T20ÀÇ °æ¿ì, PTP_OFC_Association cmd·Î Æú´õ¿¡ ´ëÇÑ  handle list ¿äÃ»½Ã device°¡ STALLµÊ
	// ÆÄÀÏ,Æú´õ ¸®½ºÆ® ±¸¼º½Ã NULL °ªÀ» Ã¼Å©ÇØ¼­ handle list ¿äÃ»
 	if(numdir != 0)
 	{
 		result = ptp_getobjecthandles (&(gdc.params), storageID, PTP_OFC_Association, ParentObjectHandle, &(oh1));
 		if( result != PTP_RC_OK)
 		{
 			PTP_Debug_Print("[Error] PTP_API_Get_MP3FilesList : ptp_getobjecthandles(folder) return (%x)\n", result);
 			PTP_END;
 			return result;
 		}
 	}
 	
 	if(nummp3 != 0)
 	{
 		result = ptp_getobjecthandles (&(gdc.params), storageID, PTP_OFC_MP3, ParentObjectHandle, &(oh2));
 		if( result != PTP_RC_OK)
 		{
 			PTP_Debug_Print("[Error] PTP_API_Get_MP3FilesList : ptp_getobjecthandles(file) return (%x)\n", result);
 			PTP_END;
 			return result;
 		}
 	}
 	
 	*nCount = numdir + nummp3; /*oh1.n + oh2.n*/
 	if (*nCount <= 0)
 	{
 		PTP_Debug_Print("[Error] PTP_API_Get_MP3FilesList : oh1.n + oh2.n is less than 0\n");
 		PTP_END;
 		return PTPAPI_ERROR; // just representing wrong count error                             
 	}
 	
 	PTP_Debug_Print ("PTP_API_Get_MP3FilesList : oh1.n = %d, oh2.n = %d, storageId= %x, parent handle = %x\n", oh1.n, oh2.n, storageID, ParentObjectHandle);
 	
 	*fileinfo = (PTPFileListInfo *)calloc (sizeof(PTPFileListInfo), *nCount);
 	if (*fileinfo == NULL)
 	{
 		PTP_Debug_Print("[Error] PTP_API_Get_MP3FilesList : memory alloc error(PTPFileListInfo)\n");
 		PTP_END;
 		return PTPAPI_ERROR; // just representing memory allocation error                               
 	}
 	   n = 0;
 	   for (nIndex = 0; nIndex < numdir/*oh1.n*/; nIndex++)
 	   {
 		   PTP_API_Init_ObjectInfo(&oi);
 		   result = ptp_getobjectinfo(&(gdc.params), oh1.Handler[nIndex], &oi);
 		   if ( result != PTP_RC_OK )
 		   {
 			   PTP_Debug_Print("[Error] PTP_API_Get_MP3FilesList : ptp_getobjectinfo return(%x)\n", result);
 			   continue;
 		   }
 		   
 		   if (oi.ObjectFormat != PTP_OFC_Association) continue;
 		   
 		   // 2007.08.06,  Kyungsik
// ¹®Á¦Á¡ : µ¿¿µ»ó(*.MOV) ÆÄÀÏÀ» folder·Î ºÐ·ùÇÏ¿© ObjectFormatÀ» folder·Î return 
// ´ëÃ¥ : ObjectFormat(folder,0x3001)ÀÎ °æ¿ì, objectinfoÀÇ  AssociationType(folder,0x0001) ¿©ºÎ¸¦  Ã¼Å©ÇÑ´Ù.
//		ÀÌ °æ¿ì AssociationTypeÀ» Áö¿øÇÏÁö ¾Ê´Â Àåºñ°¡ Á¸ÀçÇÒ ¼ö ÀÖÀ¸¹Ç·Î, AssociationType(undef, 0x0000)µµ È®ÀÎÇØ¾ß ÇÑ´Ù.
// Àåºñ¸í :  Panasonic FX12 
 		   if (oi.AssociationType > PTP_AT_GenericFolder)
 		   {
 			   PTP_Debug_Print("[Error] This file is not folder \n");
 			   continue;
 		   }
 		   
 		   strcpy ((*fileinfo)[n].filename, oi.Filename);
 		   (*fileinfo)[n].filetype = DIR_TYPE;
 		   (*fileinfo)[n].storageId = oi.StorageID;
 		   (*fileinfo)[n].ParentObject = oi.ParentObject;
 		   (*fileinfo)[n].handle = oh1.Handler[nIndex];
 		   
 		   PTP_API_Clear_ObjectInfo (&oi);
 		   ++n;
 	   }
 	   
 	   for (nIndex = 0; nIndex < nummp3/*oh2.n*/; nIndex++)
 	   {
 		   PTP_API_Init_ObjectInfo(&oi);
 		   result = ptp_getobjectinfo(&(gdc.params), oh2.Handler[nIndex],&oi);
 		   if ( result != PTP_RC_OK )
 		   {
 			   PTP_Debug_Print("[Error] PTP_API_Get_MP3FilesList : ptp_getobjectinfo return(%x)\n", result);
 			   continue;
 		   }
 		   
 		   if (oi.ObjectFormat != PTP_OFC_MP3) continue;
 		   
 		   tmPtr = NULL;
 		   tmPtr = localtime_r ( (const time_t*)&oi.CaptureDate, &lt);
 		   if (tmPtr != NULL)
 		   {
 			   (*fileinfo)[n].year = tmPtr->tm_year + 1900;
 			   (*fileinfo)[n].month = tmPtr->tm_mon + 1;
 			   (*fileinfo)[n].day = tmPtr->tm_mday ;
 			   (*fileinfo)[n].hour = tmPtr->tm_hour;
 			   (*fileinfo)[n].min = tmPtr->tm_min;
 			   (*fileinfo)[n].sec = tmPtr->tm_sec;
 		   }
 		   
 		   strcpy ((*fileinfo)[n].filename, oi.Filename);
 		   (*fileinfo)[n].filetype = FILE_TYPE;
 		   (*fileinfo)[n].storageId = oi.StorageID;
 		   (*fileinfo)[n].ParentObject = oi.ParentObject;
 		   (*fileinfo)[n].handle = oh2.Handler[nIndex];
 		   
 		   PTP_API_Clear_ObjectInfo (&oi);
 		   ++n;
 	   }
 	   
 	   // 2007.08.06,  ssu
 	   // For the error case
 	   // If any file or folder's association type is not proper, at that time we should count again the real number of files.
 	   if (*nCount != n)
 		   *nCount = n;
 	   
 	   PTP_API_Clear_ObjectHandles(&oh1);
 	   PTP_API_Clear_ObjectHandles(&oh2);
 	   
 	   PTP_END;
 	   
 	   PTP_Debug_Print("PTP_API_Get_MP3FilesList : end (%x)\n\n", result);
 	   
 	   return result;
   #endif

	//intialising unused variable
	storageID = 0;	
	ParentObjectHandle = 0;
	fileinfo = NULL;
	*nCount = 0;

	return PTPAPI_ERROR;
 }


/**
* @brief	      Get MP3 file list from the given ptp device
* @remarks		file list is including folders and MP3 files
* @param		uint32 storageID			: storage id
* @param		uint32 ParentObjectHandle 	: handle 
* @param		PTPFileListInfo **fileinfo 	: store of filelist   
* @param		uint32 *nCount 			: the num of files(including folders)   
* @param	      PTPDeviceHandle ptpHandle:	Handle for the given ptp device
* @return		uint16
PTP_RC_OK : success, else : failure
*/ 
uint16 PTP_API_Get_MP3FilesList_MPTP (uint32 storageID, uint32 ParentObjectHandle, PTPFileListInfo **fileinfo, uint32 *nCount, PTPDeviceHandle  ptpHandle)

{
	
	uint32	n = 0;
	uint32	nIndex = 0;
	uint16	result = PTP_RC_OK;
	
	//int	nRet = 0;
	//char	foldername[MAX_FILENAME] = {'0'};
	//char	filename[MAX_FILENAME] = {'0'};
	
	struct tm* tmPtr = 0;
	struct tm lt;
	
	PTPObjectInfo		oi;
	PTPObjectHandles	oh1, oh2;
	
	uint32 i = 0;
	uint32 nummp3 = 0;
	uint32 numdir = 0;

	PTPDevContext *dc;
	dc=getDc(ptpHandle);
	if(dc==NULL)
	{
		PTP_Debug_Print("[ERROR] PTP_API_Get_MPTFilesList_MPTP : PTPDevContext is null.\n");
		return PTPAPI_ERROR;//added by Niraj   
	}
	if (PTP_API_CHKDEV_MPTP(ptpHandle) != true)
		return PTPAPI_ERROR; //just representing status error
	
	PTP_Init_Transaction (dc);
	
	PTP_API_Init_ObjectHandles(&oh1);
	PTP_API_Init_ObjectHandles(&oh2);
	
	PTP_Debug_Print("PTP_API_Get_MP3FilesList : start(H:%x)\n", ParentObjectHandle);
	
	PTP_BEGIN;
	
	//Find All list from the ParentObjectHandle
	result = ptp_getobjecthandles (&(dc->params),storageID, 0x00000000, ParentObjectHandle, &(dc->params.handles));
	if (result != PTP_RC_OK)
	{
		PTP_END;
		return result;
	}
	
	PTP_Debug_Print("The # of handles : %d\n", dc->params.handles.n);
	PTP_Debug_Print("Handler:        size: \tname:\n");
	for (i = 0; i < dc->params.handles.n; i++) {
		result = ptp_getobjectinfo(&(dc->params),dc->params.handles.Handler[i], &oi);
		if (result != PTP_RC_OK)
		{
			PTP_END;
			return result;
		}
		
		if (oi.ObjectFormat == PTP_OFC_Association)
		{
			numdir++;
			continue;
		}
		if (oi.ObjectFormat == PTP_OFC_MP3)
			nummp3 ++;
		PTP_Debug_Print("0x%08x: % 9i\t%s\n",dc->params.handles.Handler[i],
			oi.ObjectCompressedSize, oi.Filename);
	}
	
	PTP_Debug_Print("the num of dir : %d \n", numdir);
	PTP_Debug_Print("the num of mp3 : %d \n", nummp3);	
	
	// SONY DSC T20Ã¯Â¿Â½Ã¯Â¿Â½ Ã¯Â¿Â½Ã¯Â¿Â½Ã¯Â¿Â½Ã¯Â¿Â½, PTP_OFC_Association cmdÃ¯Â¿Â½Ã¯Â¿Â½ Ã¯Â¿Â½Ã¯Â¿Â½Ã¯Â¿Â½Ã¯Â¿Â½Ã¯Â¿Â½Ã¯Â¿Â½ Ã¯Â¿Â½Ã¯Â¿Â½Ã¯Â¿Â½Ã¯Â¿Â½  handle list Ã¯Â¿Â½Ã¯Â¿Â½ÃƒÂ»Ã¯Â¿Â½Ã¯Â¿Â½ deviceÃ¯Â¿Â½Ã¯Â¿Â½ STALLÃ¯Â¿Â½Ã¯Â¿Â½
	// Ã¯Â¿Â½Ã¯Â¿Â½Ã¯Â¿Â½Ã¯Â¿Â½,Ã¯Â¿Â½Ã¯Â¿Â½Ã¯Â¿Â½Ã¯Â¿Â½ Ã¯Â¿Â½Ã¯Â¿Â½Ã¯Â¿Â½Ã¯Â¿Â½Ã†Â® Ã¯Â¿Â½Ã¯Â¿Â½Ã¯Â¿Â½Ã¯Â¿Â½Ã¯Â¿Â½Ã¯Â¿Â½ NULL Ã¯Â¿Â½Ã¯Â¿Â½Ã¯Â¿Â½ ÃƒÂ¼Ã…Â©Ã¯Â¿Â½Ã˜Â¼Ã¯Â¿Â½ handle list Ã¯Â¿Â½Ã¯Â¿Â½ÃƒÂ»
	if(numdir != 0)
	{
		result = ptp_getobjecthandles (&(dc->params), storageID, PTP_OFC_Association, ParentObjectHandle, &(oh1));
		if( result != PTP_RC_OK) 
		{
			PTP_Debug_Print("[Error] PTP_API_Get_MP3FilesList : ptp_getobjecthandles(folder) return (%x)\n", result);
			PTP_END;
			return result;
		}	
	}
	
	if(nummp3 != 0)
	{
		result = ptp_getobjecthandles (&(dc->params), storageID, PTP_OFC_MP3, ParentObjectHandle, &(oh2));
		if( result != PTP_RC_OK) 
		{
			PTP_Debug_Print("[Error] PTP_API_Get_MP3FilesList : ptp_getobjecthandles(file) return (%x)\n", result);
			PTP_END;
			return result;
		}	
	}
	
	*nCount = numdir + nummp3; /*oh1.n + oh2.n*/
	if (*nCount <= 0)
	{
		PTP_Debug_Print("[Error] PTP_API_Get_MP3FilesList : oh1.n + oh2.n is less than 0\n");
		PTP_END;
		return PTPAPI_ERROR; // just representing wrong count error				
	}
	
	PTP_Debug_Print ("PTP_API_Get_MP3FilesList : oh1.n = %d, oh2.n = %d, storageId= %x, parent handle = %x\n", oh1.n, oh2.n, storageID, ParentObjectHandle);
	
	*fileinfo = (PTPFileListInfo *)calloc (sizeof(PTPFileListInfo), *nCount);
	if (*fileinfo == NULL)
	{
		PTP_Debug_Print("[Error] PTP_API_Get_MP3FilesList : memory alloc error(PTPFileListInfo)\n");
		PTP_END;
		return PTPAPI_ERROR; // just representing memory allocation error				
	}
	
	n = 0;
	for (nIndex = 0; nIndex < numdir/*oh1.n*/; nIndex++) 
	{
		PTP_API_Init_ObjectInfo(&oi);
		result = ptp_getobjectinfo(&(dc->params), oh1.Handler[nIndex], &oi);
		if ( result != PTP_RC_OK )
		{
			PTP_Debug_Print("[Error] PTP_API_Get_MP3FilesList : ptp_getobjectinfo return(%x)\n", result);
			continue;
		}
		
		if (oi.ObjectFormat != PTP_OFC_Association) continue;
		
		// 2007.08.06,  Kyungsik
		// Ã¯Â¿Â½Ã¯Â¿Â½Ã¯Â¿Â½Ã¯Â¿Â½ : Ã¯Â¿Â½Ã¯Â¿Â½Ã¯Â¿Â½Ã¯Â¿Â½Ã¯Â¿Â½Ã¯Â¿Â½(*.MOV) Ã¯Â¿Â½Ã¯Â¿Â½Ã¯Â¿Â½Ã¯Â¿Â½Ã¯Â¿Â½ folderÃ¯Â¿Â½Ã¯Â¿Â½ Ã¯Â¿Â½ÃÂ·Ã¯Â¿Â½Ã¯Â¿Â½ÃÂ¿Ã¯Â¿Â½ ObjectFormatÃ¯Â¿Â½ folderÃ¯Â¿Â½Ã¯Â¿Â½ return 
		// Ã¯Â¿Â½Ã¯Â¿Â½ÃƒÂ¥ : ObjectFormat(folder,0x3001)Ã¯Â¿Â½Ã¯Â¿Â½ Ã¯Â¿Â½Ã¯Â¿Â½Ã¯Â¿Â½Ã¯Â¿Â½, objectinfoÃ¯Â¿Â½Ã¯Â¿Â½  AssociationType(folder,0x0001) Ã¯Â¿Â½Ã¯Â¿Â½Ã¯Â¿Â½ÃŽÂ¸Ã¯Â¿Â½  ÃƒÂ¼Ã…Â©Ã¯Â¿Â½Ã‘Â´Ã¯Â¿Â½.
		//		Ã¯Â¿Â½Ã¯Â¿Â½ Ã¯Â¿Â½Ã¯Â¿Â½Ã¯Â¿Â½Ã¯Â¿Â½ AssociationTypeÃ¯Â¿Â½ Ã¯Â¿Â½Ã¯Â¿Â½Ã¯Â¿Â½Ã¯Â¿Â½Ã¯Â¿Â½Ã¯Â¿Â½Ã¯Â¿Â½Ã¯Â¿Â½ Ã¯Â¿Â½ÃŠÂ´Ã¯Â¿Â½ Ã¯Â¿Â½Ã¯Â¿Â½Ã¯Â¿Â½Ã¯Â¿Â½Ã¯Â¿Â½Ã¯Â¿Â½ Ã¯Â¿Â½Ã¯Â¿Â½Ã¯Â¿Â½Ã¯Â¿Â½Ã¯Â¿Â½ Ã¯Â¿Â½Ã¯Â¿Â½ Ã¯Â¿Â½Ã¯Â¿Â½Ã¯Â¿Â½Ã¯Â¿Â½Ã‡Â·Ã¯Â¿Â½, AssociationType(undef, 0x0000)Ã¯Â¿Â½Ã¯Â¿Â½ ÃˆÂ®Ã¯Â¿Â½Ã¯Â¿Â½Ã¯Â¿Â½Ã˜Â¾Ã¯Â¿Â½ Ã¯Â¿Â½Ã‘Â´Ã¯Â¿Â½.
		// Ã¯Â¿Â½Ã¯Â¿Â½Ã¯Â¿Â½Ã¯Â¿Â½Ã¯Â¿Â½Ã¯Â¿Â½ :  Panasonic FX12 
		if (oi.AssociationType > PTP_AT_GenericFolder)
		{
			PTP_Debug_Print("[Error] This file is not folder \n");
			continue;
		}
		
		strcpy ((*fileinfo)[n].filename, oi.Filename);
		(*fileinfo)[n].filetype = DIR_TYPE;
		(*fileinfo)[n].storageId = oi.StorageID;
		(*fileinfo)[n].ParentObject = oi.ParentObject;
		(*fileinfo)[n].handle = oh1.Handler[nIndex];
		
		PTP_API_Clear_ObjectInfo (&oi);
		++n;
	}
	
	for (nIndex = 0; nIndex < nummp3/*oh2.n*/; nIndex++)
	{
		PTP_API_Init_ObjectInfo(&oi);
		result = ptp_getobjectinfo(&(dc->params), oh2.Handler[nIndex],&oi);
		if ( result != PTP_RC_OK )
		{
			PTP_Debug_Print("[Error] PTP_API_Get_MP3FilesList : ptp_getobjectinfo return(%x)\n", result);
			continue;
		}
		
		if (oi.ObjectFormat != PTP_OFC_MP3) continue;
		
		tmPtr = NULL;
		tmPtr = localtime_r ( (const time_t*)&oi.CaptureDate, &lt);
		if (tmPtr != NULL)
		{
			(*fileinfo)[n].year = tmPtr->tm_year + 1900;
			(*fileinfo)[n].month = tmPtr->tm_mon + 1;
			(*fileinfo)[n].day = tmPtr->tm_mday ;
			(*fileinfo)[n].hour = tmPtr->tm_hour;
			(*fileinfo)[n].min = tmPtr->tm_min;
			(*fileinfo)[n].sec = tmPtr->tm_sec;
		}
		
		strcpy ((*fileinfo)[n].filename, oi.Filename);
		(*fileinfo)[n].filetype = FILE_TYPE;
		(*fileinfo)[n].storageId = oi.StorageID;
		(*fileinfo)[n].ParentObject = oi.ParentObject;
		(*fileinfo)[n].handle = oh2.Handler[nIndex];
		
		PTP_API_Clear_ObjectInfo (&oi);
		++n;
	}
	
	// 2007.08.06,  ssu
	// For the error case
	// If any file or folder's association type is not proper, at that time we should count again the real number of files.
	if (*nCount != n)
		*nCount = n;
	
	PTP_API_Clear_ObjectHandles(&oh1);
	PTP_API_Clear_ObjectHandles(&oh2);
	
	PTP_END;
	
	PTP_Debug_Print("PTP_API_Get_MP3FilesList : end (%x)\n\n", result);
	
	return result;	
}

/**
* @brief	      Get parent handle 
* @remarks		none
* @param		uint32 ObjectHandle		: handle
* @param		uint32 *ParentHandle	 	: pointer indicating parent handle 
* @return		uint16
PTP_RC_OK : success, else : failure
*/ 
 uint16 PTP_API_Get_ParentHandle (uint32 ObjectHandle, uint32 *ParentHandle)
 {
#if 0
 	PTPObjectInfo   oi;
 	uint16  result = PTP_RC_OK;
 	
 	if (PTP_API_CHKDEV() != true)
 		return PTPAPI_ERROR; //just representing status error
 #ifndef OPTIMIZED_PTP
 	if (!PTP_API_Is_Handle_Vaild(ObjectHandle))
 		return PTPAPI_ERROR; //just representing handle error           
 #endif
 	PTP_Init_Transaction (&gdc);
 	
 	PTP_API_Init_ObjectInfo(&oi);
 	PTP_Debug_Print("PTP_API_Get_ParentHandle : start\n");
 	
 	PTP_BEGIN;
 	
 	result = ptp_getobjectinfo(&(gdc.params), ObjectHandle, &oi );
 	if ( result != PTP_RC_OK )
 	{
 		PTP_Debug_Print("[Error] PTP_API_Get_ParentHandle : ptp_getobjectinfo return(%x)\n", result);
 		PTP_END;
 		return result;
 	}
 	
 	*ParentHandle = oi.ParentObject;
 	PTP_Debug_Print (" PTP_API_Get_ParentHandle: handle = %x,  parent handle = %x\n", ObjectHandle, *ParentHandle);
 	PTP_API_Clear_ObjectInfo(&oi);
 	
 	PTP_END;
 	
 	PTP_Debug_Print("PTP_API_Get_ParentHandle : end(%x)\n\n", result);
 	
 	return result;
#endif

	// intialising unused arguments
	ObjectHandle = 0;
	*ParentHandle = 0;

	return PTPAPI_ERROR;
 	
 }

/**
* @brief	      Get parent handle of given object of the given ptp device
* @remarks		none
* @param		uint32 ObjectHandle		: handle
* @param		uint32 *ParentHandle	 	: pointer indicating parent handle 
* @param	      PTPDeviceHandle ptpHandle:	Handle for the given ptp device
* @return		uint16
PTP_RC_OK : success, else : failure
*/ 
uint16 PTP_API_Get_ParentHandle_MPTP (uint32 ObjectHandle, uint32 *ParentHandle, PTPDeviceHandle  ptpHandle)
{
	
	PTPObjectInfo 	oi;
	uint16	result = PTP_RC_OK;

	PTPDevContext *dc;
	dc=getDc(ptpHandle);
	if(dc==NULL)
	{
		PTP_Debug_Print("[ERROR] PTP_API_Get_ParentHandle_MPTP : PTPDevContext is null.\n");
		return PTPAPI_ERROR;//added by Niraj   
	}
	if (PTP_API_CHKDEV_MPTP(ptpHandle) != true)
		return PTPAPI_ERROR; //just representing status error
#ifndef OPTIMIZED_PTP
	if (!PTP_API_Is_Handle_Vaild_MPTP(ObjectHandle, ptpHandle))
		return PTPAPI_ERROR; //just representing handle error		
#endif
	PTP_Init_Transaction (dc);
	
	PTP_API_Init_ObjectInfo(&oi);
	PTP_Debug_Print("PTP_API_Get_ParentHandle : start\n");
	
	PTP_BEGIN;	
	
	result = ptp_getobjectinfo(&(dc->params), ObjectHandle, &oi );
	if ( result != PTP_RC_OK )
	{
		PTP_Debug_Print("[Error] PTP_API_Get_ParentHandle : ptp_getobjectinfo return(%x)\n", result);
		PTP_END;
		return result;
	}
	
	*ParentHandle = oi.ParentObject;
	PTP_Debug_Print (" PTP_API_Get_ParentHandle: handle = %x,  parent handle = %x\n", ObjectHandle, *ParentHandle);
	PTP_API_Clear_ObjectInfo(&oi);
	
	PTP_END;
	
	PTP_Debug_Print("PTP_API_Get_ParentHandle : end(%x)\n\n", result);
	
	return result;	
}

/**
* @brief	      Get Directory name from a handle
* @remarks		none
* @param		uint32 ObjectHandle		: handle
* @param		char *ParentDir		 	: pointer indicating parent directory
* @return		uint16
PTP_RC_OK : success, else : failure
*/ 
 uint16 PTP_API_Get_DirName (uint32 ObjectHandle, char *ParentDir)
 {
#if 0
 	PTPObjectInfo   oi;
 	uint16  result = PTP_RC_OK;
 	
 	if (PTP_API_CHKDEV() != true)
 		return PTPAPI_ERROR; //just representing status error
 #ifndef OPTIMIZED_PTP
 	if (!PTP_API_Is_Handle_Vaild(ObjectHandle))
 		return PTPAPI_ERROR; //just representing handle error           
 #endif
 	PTP_Init_Transaction (&gdc);
 	
 	PTP_API_Init_ObjectInfo(&oi);
 	PTP_Debug_Print("PTP_API_Get_DirName : start(H:%x)\n", ObjectHandle);
 	
 	PTP_BEGIN;
 	
 	result = ptp_getobjectinfo(&(gdc.params), ObjectHandle, &oi );
 	if ( result != PTP_RC_OK )
 	{
 		PTP_Debug_Print("[Error] PTP_API_Get_ParentHandle : ptp_getobjectinfo return(%x)\n", result);
 		PTP_END;
 		return result;
 	}
 	
 	strcpy(ParentDir, oi.Filename);
 	
 	PTP_Debug_Print (" PTP_API_Get_ParentHandle : handle = %x,  parent name = %s\n", ObjectHandle, ParentDir);
 	PTP_API_Clear_ObjectInfo(&oi);
 	
 	PTP_END;
 	
 	PTP_Debug_Print("PTP_API_Get_DirName : end(%x)\n\n", result);
 	
 	return result;
#endif

	// intialising unused arguments
	ObjectHandle = 0;
	*ParentDir = '\0';
	
	return PTPAPI_ERROR;
 	
 }

/**
* @brief	      Get Directory name from a handle for the given ptp device
* @remarks		none
* @param		uint32 ObjectHandle		: handle
* @param		char *ParentDir		 	: pointer indicating parent directory
* @param	      PTPDeviceHandle ptpHandle:	Handle for the given ptp device
* @return		uint16
PTP_RC_OK : success, else : failure
*/ 
uint16 PTP_API_Get_DirName_MPTP (uint32 ObjectHandle, char *ParentDir, PTPDeviceHandle  ptpHandle)
{
	
	PTPObjectInfo 	oi;
	uint16	result = PTP_RC_OK;

	PTPDevContext *dc;
	dc=getDc(ptpHandle);
	if(dc==NULL)
	{
		PTP_Debug_Print("[ERROR] PTP_API_Get_DirName_MPTP : PTPDevContext is null.\n");
		return PTPAPI_ERROR;//added by Niraj   
	}
	if (PTP_API_CHKDEV_MPTP(ptpHandle)  != true)
		return PTPAPI_ERROR; //just representing status error
#ifndef OPTIMIZED_PTP
	if (!PTP_API_Is_Handle_Vaild_MPTP(ObjectHandle, ptpHandle))
		return PTPAPI_ERROR; //just representing handle error		
#endif
	PTP_Init_Transaction (dc);
	
	PTP_API_Init_ObjectInfo(&oi);
	PTP_Debug_Print("PTP_API_Get_DirName : start(H:%x)\n", ObjectHandle);
	
	PTP_BEGIN;
	
	result = ptp_getobjectinfo(&(dc->params), ObjectHandle, &oi );
	if ( result != PTP_RC_OK )
	{
		PTP_Debug_Print("[Error] PTP_API_Get_ParentHandle : ptp_getobjectinfo return(%x)\n", result);
		PTP_END;
		return result;
	}
	
	strcpy(ParentDir, oi.Filename);
	
	PTP_Debug_Print (" PTP_API_Get_ParentHandle : handle = %x,  parent name = %s\n", ObjectHandle, ParentDir);
	PTP_API_Clear_ObjectInfo(&oi);
	
	PTP_END;
	
	PTP_Debug_Print("PTP_API_Get_DirName : end(%x)\n\n", result);
	
	return result;	
}

/**
* @brief	      Get object information from a handle
* @remarks		none
* @param		uint32 ObjectHandle		: handle
* @param		PTPObjectInfo *oi		 	: pointer indicating object information
* @return		uint16
PTP_RC_OK : success, else : failure
*/ 
 uint16 PTP_API_Get_ObjectInfo (uint32 ObjectHandle, PTPObjectInfo *oi)
 {
#if 0	
 	uint16	result = PTP_RC_OK;
 	
 	if (PTP_API_CHKDEV() != true)
 		return PTPAPI_ERROR; //just representing status error
 #ifndef OPTIMIZED_PTP
 	if (!PTP_API_Is_Handle_Vaild(ObjectHandle))
 		return PTPAPI_ERROR; //just representing handle error
 #endif
 	PTP_Init_Transaction (&gdc);
 	
 	PTP_Debug_Print("PTP_API_Get_ObjectInfo : start (H:%x)\n", ObjectHandle);
 	
 	PTP_BEGIN;
 	
 	result = ptp_getobjectinfo(&(gdc.params), ObjectHandle, oi );
 	if ( result != PTP_RC_OK )
 	{
 		PTP_Debug_Print("[Error] PTP_API_Get_ObjectInfo : ptp_getobjectinfo return(%x)\n", result);
 		PTP_END;
 		return result;
 	}
 	
 	if ((oi->ObjectFormat != PTP_OFC_Association) && (oi->ObjectFormat != PTP_OFC_EXIF_JPEG)) 
 	{
 		PTP_Debug_Print("[Error] PTP_API_Get_ObjectInfo : invalid object format (%x)\n", oi->ObjectFormat); 
 		PTP_END;
 		return PTPAPI_ERROR; //just representing format error
 	}
 	
 	PTP_Debug_Print("0x%08x: % 9i\t%16s\t0x%08x\t0x%04x\t0x%08x\n", ObjectHandle,	\
 		oi->ObjectCompressedSize, oi->Filename,			\
 		oi->StorageID, oi->ObjectFormat, oi->ParentObject);
 	
 	PTP_END;
 	
 	PTP_Debug_Print("PTP_API_Get_ObjectInfo : end(%x)\n\n", result);
 	
 	return result;	
#endif

	// intialising unused arguments
	ObjectHandle = 0;
	oi = NULL;

	return PTPAPI_ERROR;
 	
 }

/**
* @brief	      Get object information from a handle for the given ptp device
* @remarks		none
* @param		uint32 ObjectHandle		: handle
* @param		PTPObjectInfo *oi		 	: pointer indicating object information
* @param	      PTPDeviceHandle ptpHandle:	Handle for the given ptp device
* @return		uint16
PTP_RC_OK : success, else : failure
*/ 
uint16 PTP_API_Get_ObjectInfo_MPTP (uint32 ObjectHandle, PTPObjectInfo *oi, PTPDeviceHandle  ptpHandle)
{	
	uint16	result = PTP_RC_OK;
	PTPDevContext *dc;

	dc = getDc(ptpHandle);
	if(dc == NULL)
	{
		PTP_Debug_Print("[ERROR] PTP_API_Get_ObjectInfo_MPTP : PTPDevContext is null.\n");
		return PTPAPI_ERROR;
	}

	if (PTP_API_CHKDEV_MPTP(ptpHandle) != true)
		return PTPAPI_ERROR; //just representing status error

#ifndef OPTIMIZED_PTP
	if (!PTP_API_Is_Handle_Vaild_MPTP(ObjectHandle, ptpHandle)) ///TBD: to be reflected 
		return PTPAPI_ERROR; //just representing handle error
#endif

	PTP_Init_Transaction (dc);
	
	PTP_Debug_Print("PTP_API_Get_ObjectInfo : start (H:%x)\n", ObjectHandle);
	
	PTP_BEGIN;
	
	result = ptp_getobjectinfo(&(dc->params), ObjectHandle, oi );
	if ( result != PTP_RC_OK )
	{
		PTP_Debug_Print("[Error] PTP_API_Get_ObjectInfo : ptp_getobjectinfo return(%x)\n", result);
		PTP_END;
		return result;
	}
	
	if ((oi->ObjectFormat != PTP_OFC_Association) && (oi->ObjectFormat != PTP_OFC_EXIF_JPEG)) 
	{
		PTP_Debug_Print("[Error] PTP_API_Get_ObjectInfo : invalid object format (%x)\n", oi->ObjectFormat); 
		PTP_END;
		return PTPAPI_ERROR; //just representing format error
	}
	
	PTP_Debug_Print("0x%08x: % 9i\t%16s\t0x%08x\t0x%04x\t0x%08x\n", ObjectHandle,	\
		oi->ObjectCompressedSize, oi->Filename,			\
		oi->StorageID, oi->ObjectFormat, oi->ParentObject);
	
	PTP_END;
	
	PTP_Debug_Print("PTP_API_Get_ObjectInfo : end(%x)\n\n", result);
	
	return result;	
}

/**
* @brief	      Get numbers of objects below than current level (handle)
* @remarks		extingushed by object format
* @param		uint32 storageID			: stroage id
* @param		uint32 ParentObjectHandle	: handle
* @param		uint32 ObjectFormat		: ObjectFormat  
* @param		uint32 *nNum				: number of objects
* @return		uint16
PTP_RC_OK : success, else : failure
*/ 
 uint16 PTP_API_Get_NumOfObjects(uint32 storageID, uint32 ParentObjectHandle, uint32 ObjectFormat, uint32 *nNum)
 {
#if 0
 	PTPObjectInfo           oi;
 	
 	uint16  result = PTP_RC_OK;
 	uint32  nIndex = 0;
 	uint32  nRecordIndex = 0;
 	
 	if (PTP_API_CHKDEV() != true)
 		return PTPAPI_ERROR; //just representing status error
 	
 	PTP_Init_Transaction (&gdc);
 	
 	*nNum = 0;
 	
 	PTP_Debug_Print("PTP_API_Get_NumOfObjects : start(H:%d)\n", ParentObjectHandle);
 	
 	PTP_BEGIN;
 	
 	result = ptp_GetNumObjects (&(gdc.params), storageID, ObjectFormat, ParentObjectHandle, (uint32_t*)nNum);
 	if( result != PTP_RC_OK)
 	{
 		PTP_Debug_Print("[Error] PTP_API_Get_NumOfObjects : ptp_GetNumObjects return (0x%04x)\n", result);
 		PTP_END;
 		return result;
 	}
 	PTP_Debug_Print("PTP_API_Get_NumOfObjects : ptp_GetNumObjects return(%x)\n", *nNum);
 	
 	PTP_END;
 	
 	PTP_Debug_Print("PTP_API_Get_NumOfObjects : end(%x)\n\n", result);
 	
 	return result;
#endif

	// intialising unused arguments
	storageID = 0;
	ParentObjectHandle = 0;
	ObjectFormat = 0;
	*nNum = 0;

	return PTPAPI_ERROR;
 	
 }

/**
* @brief	      Get numbers of objects below than current level (handle) for the given ptp device
* @remarks		objects of the given format will be selected
* @param		uint32 storageID			: stroage id
* @param		uint32 ParentObjectHandle	: handle
* @param		uint32 ObjectFormat		: ObjectFormat  
* @param		uint32 *nNum				: number of objects
* @param	      PTPDeviceHandle ptpHandle:	Handle for the given ptp device
* @return		uint16
PTP_RC_OK : success, else : failure
*/
uint16 PTP_API_Get_NumOfObjects_MPTP (uint32 storageID, uint32 ParentObjectHandle, uint32 ObjectFormat, uint32 *nNum, PTPDeviceHandle  ptpHandle)
{
	uint16	result = PTP_RC_OK;

	PTPDevContext *dc;
	dc=getDc(ptpHandle);
	if(dc==NULL)
	{
		PTP_Debug_Print("[ERROR] PTP_API_Get_NumOfObjects_MPTP : PTPDevContext is null.\n");
		return PTPAPI_ERROR;//added by Niraj   
	}
	if (PTP_API_CHKDEV_MPTP(ptpHandle) != true)
		return PTPAPI_ERROR; //just representing status error
	
	PTP_Init_Transaction (dc);
	
	*nNum = 0;
	
	PTP_Debug_Print("PTP_API_Get_NumOfObjects : start(H:%d)\n", ParentObjectHandle);
	
	PTP_BEGIN;
	
	result = ptp_GetNumObjects (&(dc->params), storageID, ObjectFormat, ParentObjectHandle, (uint32_t*)nNum);
	
	if( result != PTP_RC_OK) 
	{
		PTP_Debug_Print("[Error] PTP_API_Get_NumOfObjects : ptp_GetNumObjects return (0x%04x)\n", result);
		PTP_END;
		return result;
	}
	PTP_Debug_Print("PTP_API_Get_NumOfObjects : ptp_GetNumObjects return(%x)\n", *nNum);
	
	PTP_END;
	
	PTP_Debug_Print("PTP_API_Get_NumOfObjects : end(%x)\n\n", result);
	
	return result;	
}

/**
* @brief	      Get numbers of objects below than current level (handle)
* @remarks		extingushed by format (folder, jpg)
* @param		uint32 storageID			: stroage id
* @param		uint32 ParentObjectHandle	: handle
* @param		uint32 *nNum				: number of objects
* @return		uint16
PTP_RC_OK : success, else : failure
*/ 
 uint16 PTP_API_Get_NumOfList(uint32 storageID, uint32 ParentObjectHandle, uint32 *nNum)
 {
#if 0
 	PTPObjectInfo           oi;
 	
 	uint16  result = PTP_RC_OK;
 	uint32  nIndex = 0;
 	uint32  nNum1 =0, nNum2 =0;
 	uint32  nRecordIndex = 0;
 	
 	if (PTP_API_CHKDEV() != true)
 		return PTPAPI_ERROR; //just representing status error
 	
 	PTP_Init_Transaction (&gdc);
 	
 	*nNum = 0;
 	
 	PTP_Debug_Print("PTP_API_Get_NumOfList : start(H:%x)\n", ParentObjectHandle);
 	
 	PTP_BEGIN;
 	
 	
 	// 0xFFFFFFFF : root
 	result = ptp_GetNumObjects (&(gdc.params), storageID, PTP_OFC_Association, ParentObjectHandle,(uint32_t * )&nNum1);
 	if( result != PTP_RC_OK)
 	{
 		PTP_Debug_Print("[Error] PTP_API_Get_NumOfList : ptp_GetNumObjects(dir) return (0x%04x)\n", result);
 		PTP_END;
 		return result;
 	}
 	PTP_Debug_Print("PTP_API_Get_NumOfList : ptp_GetNumObjects return(dir:%x)\n", nNum1);
 	
 	result = ptp_GetNumObjects (&(gdc.params), storageID, PTP_OFC_EXIF_JPEG, ParentObjectHandle, (uint32_t * )&nNum2);
 	if( result != PTP_RC_OK)
 	{
 		PTP_Debug_Print("[Error] PTP_API_Get_NumOfList : ptp_GetNumObjects(jpeg) return (0x%04x)\n", result);
 		PTP_END;
 		return result;
 	}
 	PTP_Debug_Print("PTP_API_Get_NumOfList : ptp_GetNumObjects return(file:%x)\n", nNum2);
 	*nNum = nNum1 + nNum2;
 	
 	PTP_END;
 	
 	PTP_Debug_Print("PTP_API_Get_NumOfList : end(%x), sum(%x)\n\n", result, *nNum);
 	
 	return result;
#endif

	// intialising unused arguments
	storageID = 0;
	ParentObjectHandle = 0;
	*nNum = 0;

	return PTPAPI_ERROR; 	
 }

/**
* @brief	      Get numbers of objects below than current level (handle) for the given ptp device
* @remarks		all the objects irrespective of format will be selected
* @param		uint32 storageID			: stroage id
* @param		uint32 ParentObjectHandle	: handle
* @param		uint32 *nNum				: number of objects
* @param	      PTPDeviceHandle ptpHandle:	Handle for the given ptp device
* @return		uint16
PTP_RC_OK : success, else : failure
*/ 
uint16 PTP_API_Get_NumOfList_MPTP(uint32 storageID, uint32 ParentObjectHandle, uint32 *nNum, PTPDeviceHandle  ptpHandle)
{
	uint16	result = PTP_RC_OK;

	uint32_t	nNum1 =0, nNum2 =0;

	PTPDevContext *dc;

	dc=getDc(ptpHandle);
	if(dc==NULL)
	{
		PTP_Debug_Print("[ERROR] PTP_API_Get_NumOfList_MPTP : PTPDevContext is null.\n");
		return PTPAPI_ERROR;//added by Niraj   
	}
	if (PTP_API_CHKDEV_MPTP(ptpHandle) != true)
		return PTPAPI_ERROR; //just representing status error
	
	PTP_Init_Transaction (dc);
	
	*nNum = 0;
	
	PTP_Debug_Print("PTP_API_Get_NumOfList : start(H:%x)\n", ParentObjectHandle);
	
	PTP_BEGIN;
	
	
	// 0xFFFFFFFF : root
	result = ptp_GetNumObjects (&(dc->params), storageID, PTP_OFC_Association, ParentObjectHandle,(uint32_t * )&nNum1);
	
	if( result != PTP_RC_OK) 
	{
		PTP_Debug_Print("[Error] PTP_API_Get_NumOfList : ptp_GetNumObjects(dir) return (0x%04x)\n", result);
		PTP_END;
		return result;
	}	
	PTP_Debug_Print("PTP_API_Get_NumOfList : ptp_GetNumObjects return(dir:%x)\n", nNum1);
	
	result = ptp_GetNumObjects (&(dc->params), storageID, PTP_OFC_EXIF_JPEG, ParentObjectHandle, (uint32_t * )&nNum2);
	
	if( result != PTP_RC_OK) 
	{
		PTP_Debug_Print("[Error] PTP_API_Get_NumOfList : ptp_GetNumObjects(jpeg) return (0x%04x)\n", result);
		PTP_END;		
		return result;
	}	
	PTP_Debug_Print("PTP_API_Get_NumOfList : ptp_GetNumObjects return(file:%x)\n", nNum2);
	*nNum = nNum1 + nNum2;
	
	PTP_END;
	
	PTP_Debug_Print("PTP_API_Get_NumOfList : end(%x), sum(%x)\n\n", result, *nNum);
	
	return result;	
}

/**
* @brief	      Get thumbnail image
* @remarks		none
* @param		uint32 handle				: handle
* @param		char **Image				: the store of an image
* @param		uint32 *uint32 *ImageSize	: Compressed size of an image
* @return		uint16
PTP_RC_OK : success, else : failure
*/ 
 uint16 PTP_API_Get_Thumbnail(uint32 handle, char **Image, uint32 *ImageSize)
 {
#if 0
 	PTPObjectInfo           oi;
 	unsigned long stTick = 0;
 	unsigned long edTick = 0;
 	
 	uint16  result = PTP_RC_OK;
 	
 	if (PTP_API_CHKDEV() != true)
 		return PTPAPI_ERROR; //just representing status error
 #ifndef OPTIMIZED_PTP
 	if (!PTP_API_Is_Handle_Vaild(handle))
 		return PTPAPI_ERROR; //just representing handle error           
 #endif
 	PTP_Init_Transaction (&gdc);
 	
 	PTP_API_Init_ObjectInfo (&oi);
 	
 	PTP_Debug_Print("PTP_API_Get_Thumbnail : start(H:%x)\n", handle);
 	
 	PTP_BEGIN;
 	
 	result = ptp_getobjectinfo(&(gdc.params), handle, &oi);
 	if ( result != PTP_RC_OK )
 	{
 		PTP_Debug_Print("[Error] PTP_API_Get_thumbnail : ptp_getobjectinfo return(0x%04x)\n", result);
 		PTP_END;
 		return result;
 	}
 	
 	if (oi.ObjectFormat != PTP_OFC_EXIF_JPEG)
 	{
 		PTP_Debug_Print("[Error] PTP_API_Get_thumbnail : handle(%d) is not jpeg image.\n", handle);
 		PTP_END;
 		return result;
 	}
 	
 	
 	if (PTP_Debug_Level >=  PTP_DBG_LVL2)
 	{
 		stTick = Tick() ;
 	}
 	
 	result = ptp_getthumb(&(gdc.params), handle, Image);
 	
 	if (PTP_Debug_Level >= PTP_DBG_LVL2)
 	{
 		edTick = Tick() ;
 		
 		PTP_Debug_Print ("0x%08x: %9i\t%16s\t0x%08x\t0x%04x\t0x%08x speed:%d (ms)\n", handle,   \
 			oi.ObjectCompressedSize, oi.Filename,               oi.StorageID, oi.ObjectFormat, oi.ParentObject, edTick - stTick);
 	}
 	else
 	{
 		PTP_Debug_Print ("0x%08x: %9i\t%16s\t0x%08x\t0x%04x\t0x%08x\n", handle,         \
 			oi.ObjectCompressedSize, oi.Filename,                                                                           \
 			oi.StorageID, oi.ObjectFormat, oi.ParentObject);
 	}
 	
 	if (result != PTP_RC_OK)
 	{
 		PTP_Debug_Print ("[Error] PTP_API_Get_thumbnail : ptp_getthumb return(0x%04x)\n", result);
 		/* 
 		if (result == PTP_ERROR_IO) 
 		{
 		usb_clear_stall(dc);
 		} 
 		*/
 		*ImageSize = -1;
 	}
 	else
 	{
 		*ImageSize = oi.ThumbCompressedSize;
 	}
 	
 	PTP_API_Clear_ObjectInfo(&oi);
 	
 	PTP_END;
 	
 	PTP_Debug_Print("PTP_API_Get_Thumbnail : end(%x)\n\n", result);
 	
 	return result;
#endif

	// intialising unused arguments
	handle = 0;
	Image = NULL;
	*ImageSize = 0;

	return PTPAPI_ERROR;
 	
 }

/**
* @brief	      Get thumbnail image from the given ptp device
* @remarks		none
* @param		uint32 handle				: handle
* @param		char **Image				: the store of an image
* @param		uint32 *uint32 *ImageSize	: Compressed size of an image
* @param	      PTPDeviceHandle ptpHandle:	Handle for the given ptp device
* @return		uint16
PTP_RC_OK : success, else : failure
*/ 
uint16 PTP_API_Get_Thumbnail_MPTP(uint32 handle, char **Image, uint32 *ImageSize,PTPDeviceHandle  ptpHandle)
{
	
	PTPObjectInfo		oi;
	unsigned long stTick = 0;
	unsigned long edTick = 0;
	
	uint16	result = PTP_RC_OK;

	PTPDevContext *dc;
	dc=getDc(ptpHandle);
	if(dc==NULL)
	{
		PTP_Debug_Print("[ERROR] PTP_API_Get_Thumbnail_MPTP : PTPDevContext is null.\n");
		return PTPAPI_ERROR;//added by Niraj   
	}
	if (PTP_API_CHKDEV_MPTP(ptpHandle) != true)
		return PTPAPI_ERROR; //just representing status error
#ifndef OPTIMIZED_PTP
	if (!PTP_API_Is_Handle_Vaild_MPTP(handle, ptpHandle))
		return PTPAPI_ERROR; //just representing handle error		
#endif
	PTP_Init_Transaction (dc);
	
	PTP_API_Init_ObjectInfo (&oi);
	
	PTP_Debug_Print("PTP_API_Get_Thumbnail : start(H:%x)\n", handle);
	
	PTP_BEGIN;
	
	result = ptp_getobjectinfo(&(dc->params), handle, &oi);
	if ( result != PTP_RC_OK )
	{
		PTP_Debug_Print("[Error] PTP_API_Get_thumbnail : ptp_getobjectinfo return(0x%04x)\n", result);
		PTP_END;
		return result;
	}
	
	if (oi.ObjectFormat != PTP_OFC_EXIF_JPEG) 
	{
		PTP_Debug_Print("[Error] PTP_API_Get_thumbnail : handle(%d) is not jpeg image.\n", handle);
		PTP_END;
		return result;
	}         
	
	
	if (PTP_Debug_Level >=  PTP_DBG_LVL2)
	{
		stTick = Tick() ;
	}
	
	result = ptp_getthumb(&(dc->params), handle, Image);
	
	if (PTP_Debug_Level >= PTP_DBG_LVL2)
	{
		edTick = Tick() ;
		
		PTP_Debug_Print ("0x%08x: %9i\t%16s\t0x%08x\t0x%04x\t0x%08x speed:%d (ms)\n", handle,	\
			oi.ObjectCompressedSize, oi.Filename, 										\
			oi.StorageID, oi.ObjectFormat, oi.ParentObject, edTick - stTick);
	}
	else
	{
		PTP_Debug_Print ("0x%08x: %9i\t%16s\t0x%08x\t0x%04x\t0x%08x\n", handle,		\
			oi.ObjectCompressedSize, oi.Filename, 										\
			oi.StorageID, oi.ObjectFormat, oi.ParentObject);
	}
	
	if (result != PTP_RC_OK) 
	{
		PTP_Debug_Print ("[Error] PTP_API_Get_thumbnail : ptp_getthumb return(0x%04x)\n", result);
		/* 
		if (result == PTP_ERROR_IO) 
		{
		usb_clear_stall(dc);
		} 
		*/
		//*ImageSize = -1;
		*ImageSize = 0;//Niraj	
	} 
	else 
	{
		*ImageSize = oi.ThumbCompressedSize;
	}
	
	PTP_API_Clear_ObjectInfo(&oi);
	
	PTP_END;
	
	PTP_Debug_Print("PTP_API_Get_Thumbnail : end(%x)\n\n", result);
	
	return result;
}

void PTP_API_PartialReadStart(void)
{
	PTP_BEGIN;
	return;
}

void PTP_API_PartialReadStop(void)
{
	PTP_END;
	return;
}

/**
* @brief  
: Partial Read operation of PTP device, reads up to length bytes from StartOffset into
the buffer starting at buf
* @remarks 
* @param 
StartOffset : read starting point
ReturnCurOffset : return value of current file offset after read operation
* @return	On success, the number of bytes read  are  returned,  
0 : indicates  end  of  file, -1 : failure,            

  * @see	Using other method. This will do the Partial read for all cameras. 
  This API is useful if only single camera is available for testing & that camera supports partial read.
*/
 uint16 PTP_API_PartialRead(uint32 handle, char** buf, uint32 length, uint32 StartOffset, uint32 *ReturnCurOffset)
 {
#if 0
 	uint16 ret=0, i = 0;
 	PTPObjectInfo objinfo;  
 	
 	PTP_Debug_Print(" PTP_API_PartialRead Entering\n");
 	
 	if(length < 512)
 	{
 		*ReturnCurOffset = 0;
 		PTP_Debug_Print("\nERROR in PTP_API_PartialRead; length< 512\n");
 		return  PTP_PARTREAD_ERR;
 	}
 	if(StartOffset>0)
 	{
 		PTP_Debug_Print("~ ING\n");
 		ret = ptp_getpartialobjectNS( &(gdc.params), handle, StartOffset, length, buf);
 		/* india org, bug fix*/
 		*ReturnCurOffset = gdc.params.getObjRetSize;	
 		PTP_Debug_Print("Partial read Main:StartOffset = %d \n", StartOffset);
 		PTP_Debug_Print("Partial read Main:ReturnCurOffset = %d \n", (gdc.params.getObjRetSize));
 		PTP_Debug_Print("Partial read Main:required size = %d, readSize = %d \n \n",length, (gdc.params.getObjRetSize));
 		return ret;
 	}
 	
 	if (PTP_API_CHKDEV() != true)
 		return PTP_PARTREAD_ERR;
 	
 	PTP_API_Init_ObjectInfo(&objinfo);	  
 	for(i = 0; i < RETRY_CNT; i++)
 	{                
 		ret = ptp_getobjectinfo(&(gdc.params), handle, &objinfo);
 		if (ret ==PTP_RC_OK)
 		{
 			PTP_Debug_Print("SUCCESS in getting the object info (MAX retry conut : %d, current count: %d)\r\n",RETRY_CNT, i+1);
 			PTP_Debug_Print("objinfo.ObjectCompressedSize = %d\n", objinfo.ObjectCompressedSize);
 			
 			break;
 		}
 		else
 		{
 			PTP_Debug_Print("RETRY in getting the object info (MAX retry conut : %d, current count: %d)\r\n",RETRY_CNT, i+1);
 			continue;			
 		}
 	}
 	if(ret != PTP_RC_OK)
 	{			
 		PTP_Debug_Print("ERROR: Could not GetObjectInfo\r\n");
 		return ret;				
 	}	
 	
 	if(StartOffset >= objinfo.ObjectCompressedSize)
 	{
 		PTP_Debug_Print("\n Invalid StartOffset  --- Greater than the OBJECT SIZE\r\n");
 		PTP_API_Clear_ObjectInfo(&objinfo);
 		return PTP_PARTREAD_ERR;	
 	}
 	
 	if(length > (objinfo.ObjectCompressedSize -StartOffset))
 	{
 		length = objinfo.ObjectCompressedSize -StartOffset;
 	}
 	
 	PTP_Debug_Print("\nbefore AlGetPartialObject_NS\n");
 	PTP_Debug_Print("~ ING\n");
 	ret = ptp_getpartialobjectNS( &(gdc.params), handle, StartOffset, length, buf);	
 	/* india org, bug fix*/
 	*ReturnCurOffset= (gdc.params.getObjRetSize);
 	PTP_Debug_Print("Partial read Main:StartOffset = %d \n", StartOffset);
 	PTP_Debug_Print("Partial read Main:ReturnCurOffset = %d \n", (gdc.params.getObjRetSize));
 	PTP_Debug_Print("Partial read Main:required size = %d, readSize = %d \n \n",length, (gdc.params.getObjRetSize));
 	
 	PTP_API_Clear_ObjectInfo(&objinfo);	
 	
 	return ret;
#endif

	// intialising unused arguments
	handle = 0;
	buf = NULL;
	length = 0;
	StartOffset = 0;
	*ReturnCurOffset = 0;

	 return  PTP_PARTREAD_ERR;
 }

/**
* @brief		Partial Read operation for the given PTP device,
* @remarks	reads up to length bytes from StartOffset into
the buffer starting at buf
* @param		StartOffset : read starting point
* @param      	ReturnCurOffset : return value of current file offset after read operation
* @param		uint32 handle:  handle of the particular object
* @param	 	char** buf: buffer for fetching the partial data
* @param		uint32 length: number of bytes of data to fetched
* @param	       PTPDeviceHandle ptpHandle:    Handle for the given ptp device
* @return		uint16
1 :indicates  end  of  file,  2 : failure,  0:continue            
*/
uint16 PTP_API_PartialRead_MPTP(uint32 handle, char** buf, uint32 length, uint32 StartOffset, uint32 *ReturnCurOffset, PTPDeviceHandle  ptpHandle)
{
	uint16 ret=0, i = 0;
	PTPObjectInfo objinfo;  

	PTPDevContext *dc;
	dc=getDc(ptpHandle);
	if(dc==NULL)
	{
		PTP_Debug_Print("[ERROR] PTP_API_PartialRead_MPTP : PTPDevContext is null.\n");
		return  PTP_PARTREAD_ERR;//added by Niraj   
	}
	PTP_Debug_Print(" PTP_API_PartialRead Entering\n");
	
	if(length < 512)
	{
		*ReturnCurOffset = 0;
		PTP_Debug_Print("\nERROR in PTP_API_PartialRead; length< 512\n");
		return  PTP_PARTREAD_ERR;
	}
	if(StartOffset>0)
	{
		PTP_Debug_Print("~ ING\n");
		ret = ptp_getpartialobjectNS( &(dc->params), handle, StartOffset, length, buf);//modified by Niraj
		/* india org, bug fix*/
		*ReturnCurOffset = dc->params.getObjRetSize;	//modified by Niraj
		PTP_Debug_Print("Partial read Main:StartOffset = %d \n", StartOffset);
		PTP_Debug_Print("Partial read Main:ReturnCurOffset = %d \n", (dc->params.getObjRetSize));//modified by Niraj
		PTP_Debug_Print("Partial read Main:required size = %d, readSize = %d \n \n",length, (dc->params.getObjRetSize));//modified by Niraj
		return ret;
	}
	
	if (PTP_API_CHKDEV_MPTP(ptpHandle) != true)
		return PTP_PARTREAD_ERR;
	
	PTP_API_Init_ObjectInfo(&objinfo);	  
	for(i = 0; i < RETRY_CNT; i++)
	{                
		ret = ptp_getobjectinfo(&(dc->params), handle, &objinfo);
		if (ret ==PTP_RC_OK)
		{
			PTP_Debug_Print("SUCCESS in getting the object info (MAX retry conut : %d, current count: %d)\r\n",RETRY_CNT, i+1);
			PTP_Debug_Print("objinfo.ObjectCompressedSize = %d\n", objinfo.ObjectCompressedSize);
			
			break;
		}
		else
		{
			PTP_Debug_Print("RETRY in getting the object info (MAX retry conut : %d, current count: %d)\r\n",RETRY_CNT, i+1);
			continue;			
		}
	}
	if(ret != PTP_RC_OK)
	{			
		PTP_Debug_Print("ERROR: Could not GetObjectInfo\r\n");
		return ret;				
	}	
	
	if(StartOffset >= objinfo.ObjectCompressedSize)
	{
		PTP_Debug_Print("\n Invalid StartOffset  --- Greater than the OBJECT SIZE\r\n");
		PTP_API_Clear_ObjectInfo(&objinfo);
		return PTP_PARTREAD_ERR;	
	}
	
	if(length > (objinfo.ObjectCompressedSize -StartOffset))
	{
		length = objinfo.ObjectCompressedSize -StartOffset;
	}
	
	PTP_Debug_Print("\nbefore AlGetPartialObject_NS\n");
	PTP_Debug_Print("~ ING\n");
	ret = ptp_getpartialobjectNS( &(dc->params), handle, StartOffset, length, buf);	//modified by Niraj
	/* india org, bug fix*/
	*ReturnCurOffset= (dc->params.getObjRetSize);//modified by Niraj
	PTP_Debug_Print("Partial read Main:StartOffset = %d \n", StartOffset);
	PTP_Debug_Print("Partial read Main:ReturnCurOffset = %d \n", (dc->params.getObjRetSize));//modified by Niraj
	PTP_Debug_Print("Partial read Main:required size = %d, readSize = %d \n \n",length, (dc->params.getObjRetSize));//modified by Niraj
	
	PTP_API_Clear_ObjectInfo(&objinfo);	
	
	return ret;
}


/**
* @brief	      Get jpeg image
* @remarks		none
* @param		uint32 handle				: handle
* @param		char **Image				: the store of an image
* @param		uint32 *uint32 *ImageSize	: Compressed size of an image
* @return		uint16
PTP_RC_OK : success, else : failure
*/ 
 uint16 PTP_API_Get_JpegImage(uint32 handle, char **Image, uint32 *ImageSize)
 {
#if 0
 	PTPObjectInfo           oi;
 	unsigned long stTick = 0;
 	unsigned long edTick = 0;
 	
 	uint16  result = PTP_RC_OK;
 	
 	if (PTP_API_CHKDEV() != true)
 		return PTPAPI_ERROR; //just representing status error
 #ifndef OPTIMIZED_PTP
 	if (!PTP_API_Is_Handle_Vaild(handle))
 		return PTPAPI_ERROR; //just representing handle error           
 #endif
 	PTP_Init_Transaction (&gdc);
 	
 	PTP_API_Init_ObjectInfo (&oi);
 	
 	PTP_Debug_Print("PTP_API_Get_JpegImage : start(H:%x)\n", handle);
 	
 	PTP_BEGIN;
 	
 	result = ptp_getobjectinfo(&(gdc.params), handle, &oi);
 	if ( result != PTP_RC_OK )
 	{
 		PTP_Debug_Print("[Error] PTP_API_Get_JpegImage : ptp_getobjectinfo return(0x%04x)\n", result);
 		PTP_END;
 		return result;
 	}
 	
 	if (oi.ObjectFormat != PTP_OFC_EXIF_JPEG)
 	{
 		PTP_Debug_Print("[Error] PTP_API_Get_JpegImage : handle(%d) is not jpeg image.\n", handle);
 		PTP_END;
 		return PTPAPI_ERROR; // just representing handle error
 	}
 	
 	if (PTP_Debug_Level >=  PTP_DBG_LVL2)
 	{
 		stTick = Tick() ;
 	}
 	
 	result = ptp_getobject(&(gdc.params),handle, Image);
 	
 	if (PTP_Debug_Level >= PTP_DBG_LVL2)
 	{
 		edTick = Tick() ;
 		PTP_Debug_Print ("0x%08x: %9i\t%16s\t0x%08x\t0x%04x\t0x%08x speed:%d(ms)\n", handle,    \
 			oi.ObjectCompressedSize, oi.Filename,                                                                           \
 			oi.StorageID, oi.ObjectFormat, oi.ParentObject, edTick -stTick  );
 	}
 	else
 	{
 		PTP_Debug_Print ("0x%08x: %9i\t%16s\t0x%08x\t0x%04x\t0x%08x\n", handle,         \
 			oi.ObjectCompressedSize, oi.Filename,                                                                           \
 			oi.StorageID, oi.ObjectFormat, oi.ParentObject);
 	}
 	
 	if (result != PTP_RC_OK)
 	{
 		PTP_Debug_Print ("[Error] PTP_API_Get_JpegImage : ptp_getobject return(0x%04x)\n", result);
 		*ImageSize = -1;
 	}
 	else
 	{
 		*ImageSize = oi.ObjectCompressedSize;
 	}
 	
 	PTP_API_Clear_ObjectInfo(&oi);
 	
 	PTP_END;
 	
 	PTP_Debug_Print("PTP_API_Get_JpegImage : end(%x)\n\n", result);
 	
 	return result;
#endif

	// intialising unused arguments
	handle = 0;
	Image = NULL;
	*ImageSize = 0;

	return PTPAPI_ERROR;
 	
 }

/**
* @brief	      Get jpeg image from the given device
* @remarks		none
* @param		uint32 handle				: handle
* @param		char **Image				: the store of an image
* @param		uint32 *uint32 *ImageSize	: Compressed size of an image
* @param	      PTPDeviceHandle ptpHandle:	Handle for the given ptp device
* @return		uint16
PTP_RC_OK : success, else : failure
*/ 
uint16 PTP_API_Get_JpegImage_MPTP (uint32 handle, char **Image, uint32 *ImageSize, PTPDeviceHandle  ptpHandle)
{
	
	PTPObjectInfo		oi;
	unsigned long stTick = 0;
	unsigned long edTick = 0;
	
	uint16	result = PTP_RC_OK;

	PTPDevContext *dc;
	dc=getDc(ptpHandle);
	if(dc==NULL)
	{
		PTP_Debug_Print("[ERROR] PTP_API_Get_JpegImage_MPTP : PTPDevContext is null.\n");
		return PTPAPI_ERROR;//added by Niraj   
	}
	if (PTP_API_CHKDEV_MPTP(ptpHandle) != true)
		return PTPAPI_ERROR; //just representing status error
	
#ifndef OPTIMIZED_PTP
	if (!PTP_API_Is_Handle_Vaild_MPTP(handle, ptpHandle))
		return PTPAPI_ERROR; //just representing handle error		
#endif		
	PTP_Init_Transaction (dc);
	
	PTP_API_Init_ObjectInfo (&oi);
	
	PTP_Debug_Print("PTP_API_Get_JpegImage : start(H:%x)\n", handle);
	
	PTP_BEGIN;
	
	
	result = ptp_getobjectinfo(&(dc->params), handle, &oi);
	
	if ( result != PTP_RC_OK )
	{
		PTP_Debug_Print("[Error] PTP_API_Get_JpegImage : ptp_getobjectinfo return(0x%04x)\n", result);
		PTP_END;
		return result;
	}
	
	if (oi.ObjectFormat != PTP_OFC_EXIF_JPEG) 
	{
		PTP_Debug_Print("[Error] PTP_API_Get_JpegImage : handle(%d) is not jpeg image.\n", handle);
		PTP_END;		
		return PTPAPI_ERROR; // just representing handle error
	}         
	
	if (PTP_Debug_Level >=  PTP_DBG_LVL2)
	{
		stTick = Tick() ;
	}
	
	result = ptp_getobject(&(dc->params),handle, Image);
	
	if (PTP_Debug_Level >= PTP_DBG_LVL2)
	{
		edTick = Tick() ;
		PTP_Debug_Print ("0x%08x: %9i\t%16s\t0x%08x\t0x%04x\t0x%08x speed:%d(ms)\n", handle,	\
			oi.ObjectCompressedSize, oi.Filename, 										\
			oi.StorageID, oi.ObjectFormat, oi.ParentObject, edTick -stTick  );
	}
	else
	{
		PTP_Debug_Print ("0x%08x: %9i\t%16s\t0x%08x\t0x%04x\t0x%08x\n", handle,		\
			oi.ObjectCompressedSize, oi.Filename, 										\
			oi.StorageID, oi.ObjectFormat, oi.ParentObject);
	}
	
	if (result != PTP_RC_OK) 
	{
		PTP_Debug_Print ("[Error] PTP_API_Get_JpegImage : ptp_getobject return(0x%04x)\n", result);
		//*ImageSize = -1;
		*ImageSize=0;//Niraj
	} 
	else 
	{	
		*ImageSize = oi.ObjectCompressedSize;
		
	}
	
	PTP_API_Clear_ObjectInfo(&oi);	
	
	PTP_END;	
	
	PTP_Debug_Print("PTP_API_Get_JpegImage : end(%x)\n\n", result);
	
	return result;	
}

/**
* @brief	      Get MP3 file
* @remarks		none
* @param		uint32 handle				: handle
* @param		char **Image				: the store of an mp3
* @param		uint32 *uint32 *ImageSize	: Compressed size of an mp3
* @return		uint16
PTP_RC_OK : success, else : failure
*/ 
 uint16 PTP_API_Get_MP3File(uint32 handle, char **Image, uint32 *ImageSize)
 {
#if 0
 	PTPObjectInfo           oi;
 	
 	char    sztemp[100]  = {0};
 	long    start_sec = 0, end_sec = 0;
 	double  start_msec = 0, end_msec = 0;
 	
 	uint16  result = PTP_RC_OK;
 	
 	if (PTP_API_CHKDEV() != true)
 		return PTPAPI_ERROR; //just representing status error
 #ifndef OPTIMIZED_PTP
 	if (!PTP_API_Is_Handle_Vaild(handle))
 		return PTPAPI_ERROR; //just representing handle error           
 #endif
 	PTP_Init_Transaction (&gdc);
 	
 	PTP_API_Init_ObjectInfo (&oi);
 	
 	PTP_Debug_Print("PTP_API_Get_MP3 : start(H:%x)\n", handle);
 	
 	PTP_BEGIN;
 	
 	result = ptp_getobjectinfo(&(gdc.params), handle, &oi);
 	if ( result != PTP_RC_OK )
 	{
 		PTP_Debug_Print("[Error] PTP_API_Get_MP3 : ptp_getobjectinfo return(0x%04x)\n", result);
 		PTP_END;
 		return result;
 	}
 	
 	if (oi.ObjectFormat != PTP_OFC_MP3)
 	{
 		PTP_Debug_Print("[Error] PTP_API_Get_MP3 : handle(%d) is not jpeg image.\n", handle);
 		PTP_END;
 		return PTPAPI_ERROR; //just representing handle error
 	}
 	
 	result = ptp_getobject(&(gdc.params),handle, Image);
 	
 	PTP_Debug_Print ("0x%08x: %9i\t%16s\t0x%08x\t0x%04x\t0x%08x\n", handle,         \
 		oi.ObjectCompressedSize, oi.Filename,                                                                           \
 		oi.StorageID, oi.ObjectFormat, oi.ParentObject);
 	
 	if (result != PTP_RC_OK)
 	{
 		PTP_Debug_Print ("[Error] PTP_API_Get_MP3 : ptp_getobject return(0x%04x)\n", result);
 		*ImageSize = -1;
 	}
 	else
 	{
 		*ImageSize = oi.ObjectCompressedSize;
 	}
 	
 	PTP_API_Clear_ObjectInfo(&oi);
 	
 	PTP_END;
 	
 	PTP_Debug_Print("PTP_API_Get_MP3 : end(%x)\n\n", result);
 	
 	return result;
#endif

	// intialising unused arguments
	handle = 0;
	Image = NULL;
	*ImageSize = 0;

	return PTPAPI_ERROR; 	
 }

/**
* @brief	      Get MP3 file for the given ptp device
* @remarks		none
* @param		uint32 handle				: handle
* @param		char **Image				: the store of an mp3
* @param		uint32 *uint32 *ImageSize	: Compressed size of an mp3
* @param	      PTPDeviceHandle ptpHandle:	Handle for the given ptp device
* @return		uint16
PTP_RC_OK : success, else : failure
*/ 
uint16 PTP_API_Get_MP3File_MPTP(uint32 handle, char **Image, uint32 *ImageSize, PTPDeviceHandle  ptpHandle)
{
	PTPObjectInfo		oi;
	
	//char 	sztemp[100]  = {0};
	//long 	start_sec = 0, end_sec = 0;
	//double 	start_msec = 0, end_msec = 0;
	
	uint16	result = PTP_RC_OK;

	PTPDevContext *dc;
	dc=getDc(ptpHandle);
	if(dc==NULL)
	{
		PTP_Debug_Print("[ERROR] PTP_API_Get_MP3File_MPTP : PTPDevContext is null.\n");
		return PTPAPI_ERROR;//added by Niraj   
	}
	if (PTP_API_CHKDEV_MPTP(ptpHandle) != true)
		return PTPAPI_ERROR; //just representing status error
#ifndef OPTIMIZED_PTP
	if (!PTP_API_Is_Handle_Vaild_MPTP(handle, ptpHandle))
		return PTPAPI_ERROR; //just representing handle error		
#endif		
	PTP_Init_Transaction (dc);
	
	PTP_API_Init_ObjectInfo (&oi);
	
	PTP_Debug_Print("PTP_API_Get_MP3 : start(H:%x)\n", handle);
	
	PTP_BEGIN;	
	
	result = ptp_getobjectinfo(&(dc->params), handle, &oi);
	if ( result != PTP_RC_OK )
	{
		PTP_Debug_Print("[Error] PTP_API_Get_MP3 : ptp_getobjectinfo return(0x%04x)\n", result);
		PTP_END;
		return result;
	}
	
	if (oi.ObjectFormat != PTP_OFC_MP3) 
	{
		PTP_Debug_Print("[Error] PTP_API_Get_MP3 : handle(%d) is not jpeg image.\n", handle);
		PTP_END;		
		return PTPAPI_ERROR; //just representing handle error
	}         
	
	result = ptp_getobject(&(dc->params),handle, Image);
	
	PTP_Debug_Print ("0x%08x: %9i\t%16s\t0x%08x\t0x%04x\t0x%08x\n", handle,		\
		oi.ObjectCompressedSize, oi.Filename, 										\
		oi.StorageID, oi.ObjectFormat, oi.ParentObject);
	
	if (result != PTP_RC_OK) 
	{
		PTP_Debug_Print ("[Error] PTP_API_Get_MP3 : ptp_getobject return(0x%04x)\n", result);
		//*ImageSize = -1;
		*ImageSize=0;//Niraj
	} 
	else 
	{
		*ImageSize = oi.ObjectCompressedSize;
	}
	
	PTP_API_Clear_ObjectInfo(&oi);		
	
	PTP_END;	
	
	PTP_Debug_Print("PTP_API_Get_MP3 : end(%x)\n\n", result);
	
	return result;	
}

/**
* @brief	      Get information of a jpeg image
* @remarks		none
* @param		uint32 handle				: handle
* @param		PTPObjectInfo *oi			: the store of an object information
* @return		uint16
PTP_RC_OK : success, else : failure
*/ 
 uint32 PTP_API_Get_JpegImageInfo(uint32 handle, PTPObjectInfo *oi)
 {
#if 0
 	uint16  result = PTP_RC_OK;
 	
 	if (PTP_API_CHKDEV() != true)
 		return PTPAPI_ERROR; //just representing status error
 #ifndef OPTIMIZED_PTP
 	if (!PTP_API_Is_Handle_Vaild(handle))
 		return PTPAPI_ERROR; //just representing handle error                           
 #endif
 	PTP_Init_Transaction (&gdc);
 	
 	PTP_Debug_Print("PTP_API_Get_JpegImageInfo : start(H:%x)\n", handle);
 	
 	PTP_BEGIN;
 	
 	result = ptp_getobjectinfo(&(gdc.params), handle, gdc.params.objectinfo);
 	if ( result != PTP_RC_OK )
 	{
 		PTP_Debug_Print("[Error] PTP_API_Get_JpegImageInfo : ptp_getobjectinfo return(%x)\n", result);
 		PTP_END;
 		return result;
 	}
 	
 	if (gdc.params.objectinfo->ObjectFormat != PTP_OFC_EXIF_JPEG)
 	{
 		PTP_Debug_Print("[Error] PTP_API_Get_JpegImageInfo : handle(%x) is not jpeg image.\n", handle);
 		PTP_END;
 		return PTPAPI_ERROR; //just representing handle error
 	}
 	
 	PTP_Debug_Print ("0x%08x: % 9i\t%16s\t0x%08x\t0x%04x\t0x%08x\n",handle,         \
 		gdc.params.objectinfo->ObjectCompressedSize, gdc.params.objectinfo->Filename,                                   \
 		gdc.params.objectinfo->StorageID, gdc.params.objectinfo->ObjectFormat, gdc.params.objectinfo->ParentObject);
 	
	// ÀÌ ÇÔ¼ö¸¦ È£ÃâÇÑ ÂÊ¿¡¼­ objectinfoÀÇ ¸Þ¸ð¸®¸¦ ÇØÁ¦ÇØÁÖ¾î¾ß ÇÑ´Ù.
 	ptp_objectinfo_copy(oi, gdc.params.objectinfo);
 	
 	PTP_END;
 	
 	PTP_Debug_Print("PTP_API_Get_JpegImageInfo : end(%x)\n\n", result);
 	
 	return result;
#endif

	// intialising unused arguments
	handle = 0;
	oi = NULL;

	return PTPAPI_ERROR;
 }


/**
* @brief	      Get information of a jpeg image for the given ptp device
* @remarks		none
* @param		uint32 handle				: handle
* @param		PTPObjectInfo *oi			: the store of an object information
* @param	      PTPDeviceHandle ptpHandle:	Handle for the given ptp device
* @return		uint16
PTP_RC_OK : success, else : failure
*/
uint32 PTP_API_Get_JpegImageInfo_MPTP (uint32 handle, PTPObjectInfo *oi,PTPDeviceHandle  ptpHandle)
{
	uint16	result = PTP_RC_OK;

	PTPDevContext *dc;
	dc=getDc(ptpHandle);
	if(dc==NULL)
	{
		PTP_Debug_Print("[ERROR] PTP_API_Get_JpegImageInfo_MPTP : PTPDevContext is null.\n");
		return PTPAPI_ERROR;//added by Niraj   
	}
	if (PTP_API_CHKDEV_MPTP(ptpHandle)  != true)
		return PTPAPI_ERROR; //just representing status error
#ifndef OPTIMIZED_PTP
	if (!PTP_API_Is_Handle_Vaild_MPTP(handle, ptpHandle))
		return PTPAPI_ERROR; //just representing handle error				
#endif
	PTP_Init_Transaction (dc);
	
	PTP_Debug_Print("PTP_API_Get_JpegImageInfo : start(H:%x)\n", handle);
	
	PTP_BEGIN;	
	
	
	result = ptp_getobjectinfo(&(dc->params), handle, oi); //modified by Niraj to get object info directly in oi
	if ( result != PTP_RC_OK )
	{
		PTP_Debug_Print("[Error] PTP_API_Get_JpegImageInfo : ptp_getobjectinfo return(%x)\n", result);
		PTP_END;		
	      	return result;
	}
	
	
	
	if (oi->ObjectFormat != PTP_OFC_EXIF_JPEG) //modified by Niraj
	{
		PTP_Debug_Print("[Error] PTP_API_Get_JpegImageInfo : handle(%x) is not jpeg image.\n", handle);
		PTP_END;				
		return PTPAPI_ERROR; //just representing handle error
	}         
	
	
	
	//added by Niraj to allocate space for objectinfo which is NULL before this point
	if(dc->params.objectinfo==NULL)
	{
		dc->params.objectinfo=(PTPObjectInfo*)malloc(sizeof(PTPObjectInfo));//vishal g++ typecasting
	}
	
	ptp_objectinfo_copy(dc->params.objectinfo,oi);
	//added by Niraj..end
	
	PTP_Debug_Print ("0x%08x: % 9i\t%16s\t0x%08x\t0x%04x\t0x%08x\n",handle,         \
		dc->params.objectinfo->ObjectCompressedSize, dc->params.objectinfo->Filename,                                   \
		dc->params.objectinfo->StorageID, dc->params.objectinfo->ObjectFormat, dc->params.objectinfo->ParentObject);
	
	
// ÀÌ ÇÔ¼ö¸¦ È£ÃâÇÑ ÂÊ¿¡¼­ objectinfoÀÇ ¸Þ¸ð¸®¸¦ ÇØÁ¦ÇØÁÖ¾î¾ß ÇÑ´Ù.	
	PTP_END;			
	
	PTP_Debug_Print("PTP_API_Get_JpegImageInfo : end(%x)\n\n", result);
	
	return result;	
}


/**
* @brief	      Get information of a device
* @remarks		none
* @param		PTPDeviceInfo *deviceinfo	: the store of a device information
* @return		uint16
PTP_RC_OK : success, else : failure
*/ 
 uint16 PTP_API_Get_DeviceInfo(PTPDeviceInfo *deviceinfo)
 {
#if 0
 	uint16 result = PTP_RC_OK;
 	
 	if (PTP_API_CHKDEV() != true)
 		return PTPAPI_ERROR; //just representing status error
 	
 	PTP_Init_Transaction (&gdc);
 	
 	PTP_Debug_Print("PTP_API_Get_DeviceInfo : start\n");
 	
 	PTP_BEGIN;
 	
 	result = ptp_getdeviceinfo (&(gdc.params), &(gdc.params.deviceinfo));
 	if( result != PTP_RC_OK)
 	{
 		PTP_Debug_Print("[Error] PTP_API_Get_DeviceInfo : ptp_getdeviceinfo return (%x)\n", result);
 		PTP_END;
 		return result;
 	}
 	
	// ÀÌ ÇÔ¼ö¸¦ È£ÃâÇÑ ÂÊ¿¡¼­ deviceinfoÀÇ ¸Þ¸ð¸®¸¦ ÇØÁ¦ÇØÁÖ¾î¾ß ÇÑ´Ù.
 	ptp_deviceinfo_copy(deviceinfo, &(gdc.params.deviceinfo));
 	
 	PTP_END;
 	
 	PTP_Debug_Print("PTP_API_Get_DeviceInfo : end(%x)\n\n", result);
 	
 	return result;
 #endif

	// intialising unused arguments
	deviceinfo = NULL;

	return PTPAPI_ERROR;	
 }

/**
* @brief	      Get information of the given ptp device
* @remarks		none
* @param		PTPDeviceInfo *deviceinfo	: the store of a device information
* @param	      PTPDevContext *dc		:Device context of the given device
* @return		uint16
PTP_RC_OK : success, else : failure
*/ 
uint16 PTP_API_Get_DeviceInfo_MPTP(PTPDeviceInfo *deviceinfo, PTPDeviceHandle  ptpHandle)
{
	uint16 result = PTP_RC_OK;

	PTPDevContext *dc;
	dc=getDc(ptpHandle);
	if(dc==NULL)
	{
		PTP_Debug_Print("[ERROR] PTP_API_Get_DeviceInfo_MPTP : PTPDevContext is null.\n");
//printf("[ERROR] PTP_API_Get_DeviceInfo_MPTP : PTPDevContext is null.\n");
		return PTPAPI_ERROR;//added by Niraj   
	}
	if (PTP_API_CHKDEV_MPTP(ptpHandle)  != true)
	{
//printf("[ERROR] PTP_API_Get_DeviceInfo_MPTP : PTP_API_CHKDEV_MPTP failed....\n");
		return PTPAPI_ERROR; //just representing status error
	}
	
	PTP_Init_Transaction (dc);
	
	PTP_Debug_Print("PTP_API_Get_DeviceInfo : start\n");
	
	PTP_BEGIN;	
	
	result = ptp_getdeviceinfo (&(dc->params), &(dc->params.deviceinfo));
	if( result != PTP_RC_OK) 
	{
		PTP_Debug_Print("[Error] PTP_API_Get_DeviceInfo : ptp_getdeviceinfo return (%x)\n", result);
//printf("[ERROR] PTP_API_Get_DeviceInfo_MPTP :ptp_getdeviceinfo return (%x)\n", result);
		PTP_END;
		return result;
	}
	
// ÀÌ ÇÔ¼ö¸¦ È£ÃâÇÑ ÂÊ¿¡¼­ objectinfoÀÇ ¸Þ¸ð¸®¸¦ ÇØÁ¦ÇØÁÖ¾î¾ß ÇÑ´Ù.	ptp_deviceinfo_copy(deviceinfo, &(dc->params.deviceinfo));
	ptp_deviceinfo_copy(deviceinfo, &(dc->params.deviceinfo));

	PTP_END;
	
	PTP_Debug_Print("PTP_API_Get_DeviceInfo : end(%x)\n\n", result);
	
	return result;	
	
}

/**
* @brief	      Get devpath of a device
* @remarks		none
* @param		PTPDeviceHandle  ptpHandle	: the store of a device path
* @return		uint16
PTP_RC_OK : success, else : failure
*/ 
 uint16 PTP_API_Get_DevPath(char *devpath)
 {
#if 0
 	uint16 result = PTP_RC_OK;
 	
 	if (PTP_API_CHKDEV() != true)
 		return PTPAPI_ERROR; //just representing status error
 	
 	PTP_Debug_Print("PTP_API_Get_DevPath : start\n");
 	
 	PTP_BEGIN;
 	
 	if(gdc.dev)
 		strcpy(devpath, gdc.dev->devpath);
 	else
 		result = PTP_RC_GeneralError;
 	
 	PTP_END;
 	
 	PTP_Debug_Print("PTP_API_Get_DevPath : end(%x)\n\n", result);
 	
 	return result;
#endif

	// intialising unused arguments
	devpath = NULL;

	return PTPAPI_ERROR;
 	
 }

/**
* @brief	      Get devpath of the given device
* @remarks		none
* @param		PTPDeviceHandle *ptpHandle	: the store of a device path
* @param	      PTPDevContext *dc		:Device context of the given device
* @return		uint16
PTP_RC_OK : success, else : failure
*/ 
#if 0
uint16 PTP_API_Get_DevPath_MPTP(char *devpath, struct usb_device* dev)
{
	
	uint16 result = PTP_RC_OK;
	
// 	if(dc==NULL)
// 	{
// 		PTP_Debug_Print("[ERROR] PTP_API_Get_DevPath_MPTP : PTPDevContext is null.\n");
// 		return PTPAPI_ERROR;//added by Niraj   
// 	}
// 	if (PTP_API_CHKDEV_MPTP(devpath)  != true)
// 		return PTPAPI_ERROR; //just representing status error
	
	PTP_Debug_Print("PTP_API_Get_DevPath : start\n");
	
	PTP_BEGIN;	
	
	if(dev)
	{
		sprintf(dev->devpath,"/%s/%s",dev->bus->dirname,dev->filename);
		strcpy(devpath,"/dev/bus/usb");
// 		devpath[0]= '\0';
		strcat(devpath, dev->devpath);
		MPTP_Debug_Print ("PTP_API_Get_DevPath_MPTP: device path is %s\n", dev->devpath);
	}
	else
		result = PTP_RC_GeneralError;
	
	PTP_END;
	
	PTP_Debug_Print("PTP_API_Get_DevPath : end(%x)\n\n", result);
	
	return result;	
	
}
#endif

uint16 PTP_API_Get_DevPath_MPTP(char *devpath, PTPDeviceHandle  ptpHandle)
{

        uint16 result = PTP_RC_OK;
        char strTemp[15];

        PTPDevContext *dc;
        dc=getDc(ptpHandle);
        if(dc==NULL)
        {
                PTP_Debug_Print("[ERROR] PTP_API_Get_DevPath_MPTP : PTPDevContext is null.\n");
                return PTPAPI_ERROR;//added by Niraj   
        }
        if (PTP_API_CHKDEV_MPTP(ptpHandle) != true)
                return PTPAPI_ERROR;

        PTP_Debug_Print("PTP_API_Get_DevPath : start\n");

        PTP_BEGIN;

                sprintf(strTemp,"/%s/%s",dc->bus->dirname,dc->dev->filename);
  //              strcpy(devpath,"/dev/bus/usb");
//              devpath[0]= '\0';
//                strcat(devpath, strTemp);
                strcpy(devpath,strTemp);
                MPTP_Debug_Print ("PTP_API_Get_DevPath_MPTP: device path is %s\n", devpath);
			//	printf("\nPTP_API_Get_DevPath_MPTP: device path is %s strTemp is %s\n", devpath,strTemp);


        PTP_END;

        PTP_Debug_Print("PTP_API_Get_DevPath : end(%x)\n\n", result);

        return result;

}



/**
    * @brief			PTP_API_Get_StorageIDs´Â ÇØ´ç µðÁöÅÐ Ä«¸Þ¶ó¿¡¼­ °ü¸®ÇÏ°í ÀÖ´Â storage id µé ¸®½ºÆ®¸¦ ¹Þ¾Æ¿Â´Ù.
    * @remarks		PTP_API_Get_StorageInfos ÇÔ¼ö¸¦ È£ÃâÇÏ±â Àü¿¡ ÀÌ ÇÔ¼ö°¡ ¸ÕÀú È£ÃâµÇ¾î¾ß ÇÔ.
    					PTP ½ºÆå¿¡¼­ getstorageids API ¸¦ Á¤ÀÇÇÔ.

    					ÁÖÀÇ) ÀÌ ÇÔ¼ö¸¦ È£ÃâÇÏ±â Àü¿¡ PTP_API_Init_StorageIDs ÇÔ¼ö¸¦ È£ÃâÇØ¾ß ÇÔ.
    					ÀÌ ÇÔ¼ö°¡ È£ÃâµÇ¸é PTPStorageIDs¿¡ Id  µéÀÌ 1Â÷¿ø ¹è¿­·Î ÀúÀåµÊ.
    					PTPStorageIDµéÀ» »ç¿ëÇÑ ÈÄ PTP_API_Clear_StorageIDs () ¸¦ È£ÃâÇØ ¸Þ¸ð¸®¸¦ ÇØÁ¦ÇØ¾ß ÇÔ.
  
				-----------------
				| usb handle open  |
				-----------------
				|
				V
				-----------------
				| ptp session open  |
				-----------------
				|
				V
				-----------------------
				| send ptp_getstorageids   |
				-----------------------
				|
				V
				-----------------
				| ptp session close |
				-----------------
				|
				V
				-----------------
				| usb handle close  |
				-----------------
				
    * @param         storage id ÀÇ ¹è¿­ ±¸Á¶Ã¼.
    * @return		¾Æ·¡ ÇÔ¼ö°¡ ¼º°øÀûÀ¸·Î ¼öÇàµÇ¸é PTP_RC_OK ¸¦ ¸®ÅÏÇÏ°í,
    				¼öÇà µµÁß¿¡ ¿¡·¯¸¦ ¸¸³ª¸é PTP_RC_Undefined ¸¦ ¸®ÅÏÇÔ.
    * @see		ptp_opensession ´Â usb handleÀ» openÇÑ ÈÄ ptp sessionÀ» openÇÒ¶§ »ç¿ëÇÔ.
    				ptp_getstorageids´Â PTP ½ºÆå¿¡¼­ Á¤ÀÇÇÑ storage idµéÀ» °¡Á®¿À±â À§ÇÑ API.
    				ptp_closesession ´Â openµÈ ptp sessionÀ» closeÇÒ¶§ »ç¿ëÇÔ.
*/
 uint16 PTP_API_Get_StorageIDs(PTPStorageIDs *storageids)
 {
#if 0
 	uint16  result = PTP_RC_OK;
 	
 	if (PTP_API_CHKDEV() != true)
 		return PTPAPI_ERROR; //just representing status error
 	
 	PTP_Init_Transaction (&gdc);
 	
 	PTP_Debug_Print("PTP_API_Get_StorageIDs : start\n");
 	
 	PTP_BEGIN;
 	
 	result = ptp_getstorageids (&(gdc.params), storageids);
 	if( result != PTP_RC_OK)
 	{
 		PTP_Debug_Print("[Error] PTP_API_Get_StorageIDs : ptp_getstorageids return (%x)\n", result);
 		PTP_END;
 		return result;
 	}
 	
 	PTP_END;
 	
 	PTP_Debug_Print("PTP_API_Get_StorageIDs : end(%x)\n\n", result);
 	
 	return result;
#endif

	// intialising unused arguments
	storageids = NULL;	

	return PTPAPI_ERROR;
 	
 }

/**
* @brief	      Get list of storageIds from the given ptp device
* @remarks		none
* @param		PTPStorageIDs *storageids:		list of storage ids
* @param	      PTPDeviceHandle ptpHandle:	Handle for the given ptp device
* @return		uint16
PTP_RC_OK : success, else : failure
*/ 
uint16 PTP_API_Get_StorageIDs_MPTP(PTPDeviceHandle  ptpHandle,PTPStorageIDs *storageids)
{
	uint16  result = PTP_RC_OK;

	PTPDevContext *dc;

	dc = getDc(ptpHandle);
	if(dc == NULL)
	{
		PTP_Debug_Print("[ERROR] PTP_API_Get_StorageIDs_MPTP : PTPDevContext is null.\n");
				printf("[ERROR] PTP_API_Get_StorageIDs_MPTP : PTPDevContext is null.\n");

		return PTPAPI_ERROR;
	}

	

	if (PTP_API_CHKDEV_MPTP(ptpHandle) != true)
	{
		PTP_Debug_Print("[ERROR] PTP_API_Get_StorageIDs_MPTP :PTP_API_CHKDEV_MPTP failed \n");

		printf("[ERROR] PTP_API_Get_StorageIDs_MPTP :PTP_API_CHKDEV_MPTP failed \n");
		return PTPAPI_ERROR;
	}
	
	//	PTP_Init_Transaction (&gdc);
	PTP_Init_Transaction (dc);
	   
	PTP_Debug_Print("PTP_API_Get_StorageIDs : start\n");
	
	PTP_BEGIN;	

	  	
	//	result = ptp_getstorageids (&(gdc.params), storageids);
	result = ptp_getstorageids (&(dc->params), storageids);
	
	if( result != PTP_RC_OK)
	{
		PTP_Debug_Print("[Error] PTP_API_Get_StorageIDs : ptp_getstorageids return (%x)\n", result);
		printf("[Error] PTP_API_Get_StorageIDs : ptp_getstorageids return (%x)\n", result);
		PTP_END;
		return result;
	}       
	
	PTP_END;
	
	PTP_Debug_Print("PTP_API_Get_StorageIDs : end(%x)\n\n", result);
	
	return result;
}       

/**
    * @brief		PTP_API_Get_StorageInfos ÇÔ¼ö´Â ptp ÀåºñÀÇ Æ¯Á¤ storage id ¿¡ ´ëÇÑ Á¤º¸¸¦ ¾ò¾î¿È.
    * @remarks	PTP_API_Get_StorageInfos ÇÔ¼ö´Â µðÁöÅÐ Ä«¸Þ¶ó¿¡ ¿¬°áµÈ ÀúÀåÀåÄ¡ÀÇ Á¤º¸¸¦ °¡Á®¿Ã¶§ »ç¿ëÇÔ.
    				PTP ½ºÆå¿¡¼­ getstorageinfo API ¸¦ »ç¿ëÇÔ.
    				
    				ÁÖÀÇ) ÀÌ ÇÔ¼ö¸¦ È£ÃâÇÏ±â Àü¿¡ PTP_API_Init_PTPStorageInfo () ÇÔ¼ö¸¦ È£ÃâÇØ¾ß ÇÔ. 
    					PTPStorageInfo ÀÇ StorageDescription, VolumeLabelÀº »ç¿ëÈÄ ¸Þ¸ð¸®¸¦ ÇØÁ¦ÇØ¾ß ÇÔ.
    					Áï, PTP_API_Clear_PTPStorageInfo () ÇÔ¼ö¸¦ È£ÃâÇÏ¸é µÊ.
  
				-----------------
				| usb handle open  |
				-----------------
				|
				V
				-----------------
				| ptp session open  |
				-----------------
				|
				V
				------------------------
				| send ptp_getstorageinfo   |
				------------------------
				|
				V
				-----------------
				| ptp session close |
				-----------------
				|
				V
				-----------------
				| usb handle close  |
				-----------------
				
    * @param		storageid	µðÁöÅÐ Ä«¸Þ¶ó¿¡¼­ °ü¸®ÇÏ´Â ÀúÀåÀåÄ¡ÀÇ id. PTP ½ºÆå¿¡¼­ Á¤ÀÇÇÑ 4¹ÙÀÌÆ® Á¤¼ö.
    * @param		storageinfo	PTP ½ºÆå¿¡¼­ Á¤ÀÇÇÑ ÀúÀåÀåÄ¡ ±¸Á¶Ã¼·Î ¿ë·®, ¿©À¯°ø°£ Å©±â, º¼·ý¸í ÆÄÀÏ½Ã½ºÅÛ Å¸ÀÔµîÀÇ Á¤º¸¸¦ ÀúÀåÇÔ.
    * @return		¾Æ·¡ ÇÔ¼ö°¡ ¼º°øÀûÀ¸·Î ¼öÇàµÇ¸é PTP_RC_OK ¸¦ ¸®ÅÏÇÏ°í,
    				¼öÇà µµÁß¿¡ ¿¡·¯¸¦ ¸¸³ª¸é PTP_RC_Undefined ¸¦ ¸®ÅÏÇÔ.
    * @see		ptp_opensession ´Â usb handleÀ» openÇÑ ÈÄ ptp sessionÀ» openÇÒ¶§ »ç¿ëÇÔ.
    				ptp_getstorageinfo´Â PTP ½ºÆå¿¡¼­ Á¤ÀÇÇÑ storageinfo Á¤º¸¸¦ °¡Á®¿À±â À§ÇÑ API.
    				ptp_closesession ´Â openµÈ ptp sessionÀ» closeÇÒ¶§ »ç¿ëÇÔ.
*/

 uint16 PTP_API_Get_StorageInfos(uint32_t storageid, PTPStorageInfo* storageinfo)
 { 
#if 0
 	uint16  result = PTP_RC_OK;
 	
 	if (PTP_API_CHKDEV() != true)
 		return PTPAPI_ERROR; //just representing status error
 	
 	PTP_Init_Transaction (&gdc);
 	
 	PTP_Debug_Print("PTP_API_Get_StorageInfos : start\n");
 	
 	PTP_BEGIN;
 	
 	result = ptp_getstorageinfo (&(gdc.params), storageid, storageinfo);
 	if( result != PTP_RC_OK)
 	{
 		PTP_Debug_Print("[Error] PTP_API_Get_StorageInfos : ptp_getstorageinfo return (%x)\n", result);
 		PTP_END;
 		return result;
 	}
 	
 	PTP_END;
 	
 	PTP_Debug_Print("PTP_API_Get_StorageInfos : end(%x)\n\n", result);
 	
 	return result;
#endif
	return PTPAPI_ERROR;
 }

/**
* @brief	      Get storage information of the given storage id from the given  device
* @remarks		none
* @param		PTPStorageInfo* storageinfo:	structure for storage infomation		
* @param	      PTPDeviceHandle ptpHandle:	Handle for the given ptp device
* @return		uint16
PTP_RC_OK : success, else : failure
*/
uint16 PTP_API_Get_StorageInfos_MPTP(PTPDeviceHandle  ptpHandle, uint32_t storageid, PTPStorageInfo* storageinfo)
{
	uint16  result = PTP_RC_OK;
	PTPDevContext *dc = NULL;

	dc = getDc(ptpHandle);
	if(dc == NULL)
	{
		PTP_Debug_Print("[ERROR] PTP_API_Get_StorageInfos_MPTP : PTPDevContext is null.\n");
		return PTPAPI_ERROR;//added by Niraj   
	}

	if (PTP_API_CHKDEV_MPTP(ptpHandle) != true)
		return PTPAPI_ERROR; //just representing status error
	
	PTP_Init_Transaction (dc);
	
	PTP_Debug_Print("PTP_API_Get_StorageInfos : start\n");
	
	PTP_BEGIN;	
	
	if(PTP_API_CheckValidStorageID(storageid) ==PTP_RC_OK)
	{
		result = ptp_getstorageinfo (&(dc->params), storageid, storageinfo);
		if( result != PTP_RC_OK)
		{
			  
			PTP_Debug_Print("[Error] PTP_API_Get_StorageInfos : ptp_getstorageinfo return (%x)\n", result);
			PTP_END;
			return result;
		}
	}
	else
	{
		PTP_END;
		return PTP_RC_Undefined;
	}
	
	PTP_END;
	
	PTP_Debug_Print("PTP_API_Get_StorageInfos : end(%x)\n\n", result);
	
	return result;
}

/**
    * @brief	PTP_API_Send_Reset ÇÔ¼ö´Â usb device ¿¡°Ô Reset ¸í·ÉÀ» º¸³¾¶§ »ç¿ëÇÔ.
    * @remarks	usb device ÀåºñµéÀÌ host ¿¡ ¿¬°áµÇ¸é host´Â ÀÓÀÇ ½ÃÁ¡¿¡ Reset ¸í·ÉÀ» º¸³»¼­
    				usb device ¿¡°Ô ÃÊ±âÈ­ ÇÒ  °ÍÀ» ¸í·ÉÇÔ.

  -----------------
  | usb handle open  |
  -----------------
  |
  V
  -----------------
  | send reset cmd    |
  -----------------
  |
  V
  -----------------
  | usb handle close  |
  -----------------
  
    * @param	¾øÀ½.
    * @return	¾Æ·¡ ÇÔ¼ö°¡ ¼º°øÀûÀ¸·Î ¼öÇàµÇ¸é PTP_RC_OK ¸¦ ¸®ÅÏÇÏ°í,
    		¼öÇà µµÁß¿¡ ¿¡·¯¸¦ ¸¸³ª¸é PTP_RC_Undefined ¸¦ ¸®ÅÏÇÔ.
    * @see	PTP_Init_Transaction ÇÔ¼ö´Â usb/ptp Åë½Åµ¿¾È »ç¿ëÇÑ º¯¼ö ÃÊ±âÈ­.
    		PTP_Open_Device ÇÔ¼ö´Â usb handleÀ» open ÇÔ.
    		get_usb_endpoint_status ÇÔ¼ö´Â bulk in/bulk out/interrupt endpointÀÇ »óÅÂ¸¦ ÀÐ¾î¿È.
    		usb_clear_stall ÇÔ¼ö´Â bulk in/bulk out/interrupt endpointÀÇ ¿¬°áÀ» ²÷À½.
    		set_usb_device_reset ÇÔ¼ö´Â usb device¿¡°Ô Reset ¸í·ÉÀ» º¸³¿.
    		PTP_Close_Device ÇÔ¼ö´Â usb handleÀ» ´ÝÀ½.
    		PTP_Terminate_Transaction ÇÔ¼ö´Â usb/ptp Åë½Åµ¿¾È »ç¿ëÇÑ ¸Þ¸ð¸® ÇØÁ¦.
*/
 uint16 PTP_API_Send_Reset(void)
 {
#if 0
 	
 	uint16  result = PTP_RC_OK;
 	uint16  status = 0;
 	uint16  device_status[2] = {0,0};
 	
 	PTP_Init_Transaction (&gdc);
 	PTP_Debug_Print("PTP_API_Send_Reset : start\n");
 	
 	PTP_BEGIN;
 	
 	result = get_usb_endpoint_status(&gdc, &status);
 	if (result != PTP_RC_OK)
 	{
 		PTP_Debug_Print("[Error] PTP_API_Send_Reset : usb_get_endpoint_status return (%x)\n", result);
 		PTP_END;
 		return PTPAPI_ERROR; //just representing status error
 	}
 	else
 	{
 		if (status)
 		{
 			result = usb_clear_stall(&gdc);
 			if (result != PTP_RC_OK)
 			{
 				PTP_Debug_Print("[Error] PTP_API_Send_Reset : usb_clear_stall_feature return (%x)\n", result);
 				PTP_END;
 				return PTPAPI_ERROR; //just representing status error
 			}
 		}
 	}
 	
 	result = get_usb_device_status(&gdc, device_status);
 	if (result != PTP_RC_OK )
 	{
 		PTP_Debug_Print ("[Error] PTP_API_Send_Reset : get_usb_device_status return fail.\n");
 		PTP_END;
 		return PTPAPI_ERROR; //just representing status error
 	}
 	else
 	{
 		PTP_Debug_Print ("PTP_API_Send_Reset : get_usb_device_status return(0x%04x/0x%04x)\n", device_status[0], device_status[1]);
 		if (device_status[0] == PTP_RC_OK)
 		{
 			PTP_Debug_Print ("PTP_API_Send_Reset : device status OK\n");
 		}
 		else
 		{
 			PTP_Debug_Print ("[Error] PTP_API_Send_Reset : device status(0x%04x/0x%04x)\n",device_status[0], device_status[1]);
 			PTP_END;
 			return PTPAPI_ERROR; //just representing status error
 		}
 	}
 	
 	result = set_usb_device_reset(&gdc);
 	if (result != PTP_RC_OK)
 	{
 		PTP_Debug_Print ("[Error] PTP_API_Send_Reset : set_ptp_device_reset return(0x%4x)\n", result);
 		PTP_END;
 		return PTPAPI_ERROR; //just representing status error
 	}
 	
 	result = get_usb_device_status(&gdc, device_status);
 	if (result != PTP_RC_OK )
 	{
 		PTP_Debug_Print ("[Error] PTP_API_Send_Reset : get_usb_device_status return(%x)\n", device_status[0]);
 		PTP_END;
 		return PTPAPI_ERROR; //just representing status error
 	}
 	else
 	{
 		if (device_status[0] == PTP_RC_OK)
 		{
 			PTP_Debug_Print ("PTP_API_Send_Reset : device status OK\n");
 		}
 	}
 	
 	PTP_END;
 	
 	PTP_Debug_Print("PTP_API_Send_Reset : end(%x)\n\n", result);
 	
 	return result;
#endif
	return PTPAPI_ERROR;
 }

/**
* @brief	      sends reset command to the given ptp device
* @remarks		none
* @param	      PTPDeviceHandle ptpHandle:	Handle for the given ptp device
* @return		uint16
PTP_RC_OK : success, else : failure
*/ 
uint16 PTP_API_Send_Reset_MPTP(PTPDeviceHandle  ptpHandle)
{
	
	uint16	result = PTP_RC_OK;
	uint16	status = 0;
	uint16	/*device_status[2]*/device_status[4] = {0,0};

	PTPDevContext *dc;
	dc=getDc(ptpHandle);
	
	if(dc==NULL)
	{
		PTP_Debug_Print("[ERROR] PTP_API_Send_Reset_MPTP : PTPDevContext is null.\n");
		return PTPAPI_ERROR;//added by Niraj   
	}
	PTP_Init_Transaction (dc);
	PTP_Debug_Print("PTP_API_Send_Reset : start\n");
	PTP_BEGIN;		
	      
	result = get_usb_endpoint_status(dc, &status);
	if (result != PTP_RC_OK)
	{
		PTP_Debug_Print("[Error] PTP_API_Send_Reset : usb_get_endpoint_status return (%x)\n", result);
		PTP_END;
		return PTPAPI_ERROR; //just representing status error
	}
	else
	{
		if (status) 
		{
			result = usb_clear_stall(dc);
			if (result != PTP_RC_OK)
			{
				PTP_Debug_Print("[Error] PTP_API_Send_Reset : usb_clear_stall_feature return (%x)\n", result);
				PTP_END;
				return PTPAPI_ERROR; //just representing status error
			}
		}
	}	
	
	result = get_usb_device_status(dc, device_status);
	if (result != PTP_RC_OK )
	{
		PTP_Debug_Print ("[Error] PTP_API_Send_Reset : get_usb_device_status return fail.\n");
		PTP_END;
		return PTPAPI_ERROR; //just representing status error
	}
	else
	{
		PTP_Debug_Print ("PTP_API_Send_Reset : get_usb_device_status return(0x%04x/0x%04x)\n", device_status[0], device_status[1]);
		if (/*device_status[0]*/device_status[3] == PTP_RC_OK) //vishal
		{
			PTP_Debug_Print ("PTP_API_Send_Reset : device status OK\n");
		}
		else
		{
			PTP_Debug_Print ("[Error] PTP_API_Send_Reset : device status(0x%04x/0x%04x)\n",device_status[0], device_status[1]);
			PTP_END;			
			return PTPAPI_ERROR; //just representing status error
		}
	}
	
	result = set_usb_device_reset(dc);
	if (result != PTP_RC_OK)
	{
		PTP_Debug_Print ("[Error] PTP_API_Send_Reset : set_ptp_device_reset return(0x%4x)\n", result);
		PTP_END;		
		return PTPAPI_ERROR; //just representing status error
	}
	
	result = get_usb_device_status(dc, device_status);
	if (result != PTP_RC_OK )
	{
		PTP_Debug_Print ("[Error] PTP_API_Send_Reset : get_usb_device_status return(%x)\n", device_status[0]);
		PTP_END;		
		return PTPAPI_ERROR; //just representing status error
	}
	else
	{
		if (/*device_status[0]*/device_status[3] == PTP_RC_OK) 
		{
			PTP_Debug_Print ("PTP_API_Send_Reset : device status OK\n");
		}
	}
	
	PTP_END;
	
	PTP_Debug_Print("PTP_API_Send_Reset : end(%x)\n\n", result);
	
	return result;
}



/**

  * @brief  
  
         : Storage IDÀÇ À¯È¿¼ºÀ» ÆÇ´ÜÇÑ´Ù (physical,logical masking °ªÀÌ Á¸ÀçÇÏ¸é À¯È¿ÇÑ storageID ÀÌ´Ù)
	
	  * @remarks 
	  
		* @param    AlDevContext *dc
		
		  * @return   
		  
			* @see
			
*/
uint16  PTP_API_CheckValidStorageID(unsigned int storageID)

{
	
    if(storageID & PTP_PHYSICAL_STORAGE_MASK)
    {
        if(storageID & PTP_LOGICAL_STORAGE_MASK)
        {
            PTP_Debug_Print("PTP_LOGICAL_STORAGE existing\n ");
            return PTP_RC_OK;
        }
        else
        {
            PTP_Debug_Print("PTP_LOGICAL_STORAGE not existing\n ");
            return PTPAPI_ERROR; //just representing status error;          
        }
    }
    else
    {
        PTP_Debug_Print("PTP_PHYSICAL_STORAGE not existing\n ");
        return PTPAPI_ERROR; //just representing status error
    }
}


/**
* @brief         This function maps the ptpHandle of a ptp device to its DeviceContext
* @remarks       pDC is set to NULL if the device is not in the list
* @param         PTPDeviceHandle ptpHandle - ptpHandle of the camera  
* @return		 PTPDevContext*:  
				 DeviceContext of the respective Handle
* @see           
*/
static PTPDevContext *getDc(PTPDeviceHandle  ptpHandle)
{ 
	PTPDevContext* pDC;

	CPTPDeviceListUpdate::Get().GetPTPDeviceContext(ptpHandle, &pDC);

	return pDC;
}



/**
* @brief	    fetches the DeviceList of the connected PTP Devices
* @remarks		none
* @param		UsbDev* deviceList[]    :Device List
* @param		uint32_t* count:	number of ptp Devices
* @return		void
*/ 
void PTP_API_GetDeviceList(UsbDev* deviceList[],uint32_t* count)
{
	#define dev_max_cnt 20

	CPTPDeviceListUpdate::Get().GetPTPDeviceList(dev_max_cnt, deviceList, count);
}	
	
    
