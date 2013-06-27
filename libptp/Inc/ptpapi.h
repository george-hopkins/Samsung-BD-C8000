
#ifndef __PTPAPI_H__
#define __PTPAPI_H__

#include "ptp.h"
#include "usb.h"
#include <pthread.h>

#define	OPTIMIZED_PTP

typedef unsigned char uint8;
typedef unsigned int uint16;
typedef unsigned long uint32;

typedef signed char int8;
typedef signed int int16;
typedef signed long int32;

//#define BOOL		unsigned char
#define Bool		unsigned char
#define true		1
#define false		0

#define PTP_START_INDEX	1

#define USB_CLASS_PTP		6

#define USB_DP_HTD		(0x00 << 7)
#define USB_DP_DTH		(0x01 << 7)

#define USB_REQ_DEVICE_RESET			0x66
#define USB_REQ_GET_DEVICE_STATUS	0x67

#define USB_FEATURE_HALT		0x00

#define USB_TIMEOUT				10000
#define USB_CAPTURE_TIMEOUT	20000

#define PTP_TIMEOUT		10000
#define PTP_USB_URB		2097152

#define MAX_FILENAME		255

#define PTP_PRINT	printf

//newly appended.
#define PTPAPI_ERROR		0xffff
#define DO_GET_DEVICE_INFO				1
#define DO_NOT_GET_DEVICE_INFO		0

#define	RETRY_CNT	3

// PTP 디바이스의 valid storage를 check하기 위한 mask 
#define PTP_LOGICAL_STORAGE_MASK    0x0000FFFF
#define PTP_PHYSICAL_STORAGE_MASK   0xFFFF0000

//#ifdef LINUX_OS
#if 0
	#define USB_BULK_READ ptp_bulk_read
	#define USB_BULK_WRITE ptp_bulk_write
	
	int ptp_bulk_read(usb_dev_handle *dev, int ep, char *bytes, int size,	int timeout);
	int ptp_bulk_write(usb_dev_handle *dev, int ep, char *bytes, int length, int timeout);
#else
	#define USB_BULK_READ usb_bulk_read		/* defined in linux.c */
	#define USB_BULK_WRITE usb_bulk_write		/* defined in linux.c */
#endif


// #define SIZE_VENDORNAME		30
// #define SIZE_PRODUCTNAME	30


	
enum {FILE_TYPE = 1, DIR_TYPE};

struct _PTP_USB {
	usb_dev_handle* handle;
	int inep;
	int outep;
	int intep;
};
typedef struct _PTP_USB PTP_USB;

struct _PTPDevContext
{
        PTPParams		params;
        PTP_USB			ptp_usb;
        struct usb_bus 		*bus;
        struct usb_device 	*dev;
};
typedef struct _PTPDevContext PTPDevContext;


struct _PTPDevHandle
{
	uint32 init_status;
	uint32 device_status;//attached or removed
	PTPDevContext *ptp_dev_ctx;
	struct _PTPDevHandle *next;
};
typedef struct _PTPDevHandle PTPDevHandle;


struct _PTPFileListInfo
{
	char dirname[MAX_FILENAME];
	char filename[MAX_FILENAME];
	char filetype;
	uint32 storageId;
	uint32 handle;
	uint32 ParentObject;
	int year;
	int month;
	int day;
	int hour;
	int min;
	int sec;
};
typedef struct _PTPFileListInfo PTPFileListInfo;


struct _PTPDeviceHandle
{
	int bus_num;
	int dev_num;
};
typedef struct _PTPDeviceHandle PTPDeviceHandle;


typedef struct  _UsbDev
{
	PTPDeviceHandle ptp_path;
// 	char devpath[24];
	char vendor[80];
	char model[80];
	int storage_id[2];
	PTPStorageIDs sids;
	PTPStorageInfo 	storageinfo[2];
	char	SerialNumber[80];
	
	uint16 DeviceId;
	
}UsbDev;


#ifdef __cplusplus
extern "C" {
#endif

typedef bool (*ptp_monitoring) (void);

uint16 PTP_Open_Device(PTPDevContext *dc);
uint16 PTP_Open_Device_MPTP(PTPDevContext *dc);
uint16 PTP_Close_Device(PTPDevContext *dc);
uint16 get_usb_endpoint_status(PTPDevContext *dc, uint16 *status);
uint16 clear_usb_stall_feature(PTPDevContext *dc, int ep);
uint16 get_usb_device_status(PTPDevContext *dc, uint16 *device_status);
uint16 set_usb_device_reset(PTPDevContext *dc);
Bool usb_find_device (PTPDevContext *dc);
Bool  usb_find_device_MPTP (PTPDevContext *dc);//vishal
uint16 usb_find_endpoints(PTPDevContext *dc);
uint16 usb_initialize_port (PTPDevContext *dc);
uint16 usb_clear_stall(PTPDevContext *dc);

uint16 ptp_deviceinfo_copy(PTPDeviceInfo *dest, PTPDeviceInfo *src);
uint16 ptp_objectinfo_copy(PTPObjectInfo *dest, PTPObjectInfo *src);
uint16 ptp_handles_copy(PTPObjectHandles *dest , PTPObjectHandles *src);



void PTP_API_Init_DeviceInfo(PTPDeviceInfo *di);
void PTP_API_Init_ObjectHandles(PTPObjectHandles *oh);
void PTP_API_Init_ObjectInfo(PTPObjectInfo *oi);
void PTP_API_Init_PTPFileListInfo(PTPFileListInfo **fileinfo);
void PTP_API_Init_StorageIDs(PTPStorageIDs *storageids);
void PTP_API_Init_PTPStorageInfo(PTPStorageInfo *storageInfo);
void PTP_Init_Transaction(PTPDevContext *dc);
void PTP_API_Clear_DeviceInfo(PTPDeviceInfo *di);
void PTP_API_Clear_ObjectHandles(PTPObjectHandles *oh);
void PTP_API_Clear_ObjectInfo(PTPObjectInfo *oi);
void PTP_API_Clear_PTPFileListInfoInfo(PTPFileListInfo **fileinfo);
void PTP_API_Clear_StorageIDs(PTPStorageIDs *storageids);
void PTP_API_Clear_PTPStorageInfo(PTPStorageInfo *storageInfo);
void PTP_Terminate_Transaction(PTPDevContext *dc);
Bool PTP_API_Check_Devices(void);  

Bool PTP_API_Check_Devices_MPTP (uint16 *ptp_dev_cnt);  //sheetal added

//////////////////////////////////////////////////////////////////////////////////
// newly updated by sang u shim 
// date : 02Nov 2006
//////////////////////////////////////////////////////////////////////////////////
Bool t_FileExist(const char* fileName);
Bool t_FindString(const char *pStr, const char *pFindStr, int bContinue);
Bool PTP_API_CHKDEV(void);
Bool PTP_API_CHKDEV_MPTP(PTPDeviceHandle  ptpHandle);
uint16 PTP_API_OPENSESSION(void);
uint16 PTP_API_OPENSESSION_MPTP(PTPDeviceHandle  ptpHandle);//vishal
uint16 PTP_API_CLOSESESSION(void);
uint16 PTP_API_CLOSESESSION_MPTP(PTPDeviceHandle  ptpHandle); //vishal
uint16 PTP_API_OPENDEVICE(Bool getdevinfo);
uint16 PTP_API_OPENDEVICE_MPTP(Bool isgetdeviceinfo, PTPDeviceHandle  ptpHandle);//vishal
uint16 PTP_API_CLOSEDEVICE(void);
uint16 PTP_API_CLOSEDEVICE_MPTP(PTPDeviceHandle  ptpHandle); //vishal
uint16 PTP_API_INIT_COMM(void);
uint16 PTP_API_INIT_COMM_MPTP(PTPDeviceHandle  ptpHandle); //vishal
uint16 PTP_API_FINISH_COMM(void);

uint16 PTP_API_FINISH_COMM_MPTP(PTPDeviceHandle  ptpHandle); //vishal

Bool PTP_API_Is_Handle_Vaild (uint32 ObjectHandle);
Bool PTP_API_Is_Handle_Vaild_MPTP (uint32 ObjectHandle, PTPDeviceHandle  ptpHandle);//sheetal added


Bool Is_PTP_device(PTPDeviceInfo *deviceinfo);
Bool Is_PTP_device_MPTP(PTPDeviceHandle  ptpHandle);//sheetal added
uint16 PTP_API_Get_FilesList (uint32 storageID, uint32 ParentObjectHandle, PTPFileListInfo **fileinfo, uint32 *nCount);
uint16 PTP_API_Get_FilesList_MPTP (uint32 storageID, uint32 ParentObjectHandle, PTPFileListInfo **fileinfo, uint32 *nCount, PTPDeviceHandle  ptpHandle); //vishal

uint16 PTP_API_Get_JpegFilesList(uint32 storageID, uint32 ParentObjectHandle, PTPFileListInfo **fileinfo, uint32 *nCount);
uint16 PTP_API_Get_JpegFilesList_MPTP (uint32 storageID, uint32 ParentObjectHandle, PTPFileListInfo **fileinfo, uint32 *nCount,PTPDeviceHandle  ptpHandle);

uint16 PTP_API_Get_MP3FilesList(uint32 storageID, uint32 ParentObjectHandle, PTPFileListInfo **fileinfo, uint32 *nCount);
uint16 PTP_API_Get_MP3FilesList_MPTP (uint32 storageID, uint32 ParentObjectHandle, PTPFileListInfo **fileinfo, uint32 *nCount, PTPDeviceHandle  ptpHandle);//sheetal added
uint16 PTP_API_Get_ParentHandle(uint32 ObjectHandle, uint32 *ParentHandle);
uint16 PTP_API_Get_ParentHandle_MPTP (uint32 ObjectHandle, uint32 *ParentHandle, PTPDeviceHandle  ptpHandle);//sheetal added
uint16 PTP_API_Get_DirName(uint32 ObjectHandle, char *ParentDir);
uint16 PTP_API_Get_DirName_MPTP (uint32 ObjectHandle, char *ParentDir, PTPDeviceHandle  ptpHandle);//sheetal added
uint16 PTP_API_Get_ObjectInfo(uint32 ObjectHandle, PTPObjectInfo *oi);
uint16 PTP_API_Get_ObjectInfo_MPTP (uint32 ObjectHandle, PTPObjectInfo *oi, PTPDeviceHandle  ptpHandle);//sheetal added
uint16 PTP_API_Get_NumOfObjects(uint32 storageID, uint32 ParentObjectHandle, uint32 ObjectFormat, uint32 *nNum);
uint16 PTP_API_Get_NumOfObjects_MPTP(uint32 storageID, uint32 ParentObjectHandle, uint32 ObjectFormat, uint32 *nNum, PTPDeviceHandle  ptpHandle);//sheetal added

uint16 PTP_API_Get_NumOfList(uint32 storageID, uint32 ParentObjectHandle, uint32 *nNum);
uint16 PTP_API_Get_NumOfList_MPTP(uint32 storageID, uint32 ParentObjectHandle, uint32 *nNum, PTPDeviceHandle  ptpHandle);//sheetal added

uint16 PTP_API_Get_Thumbnail(uint32 handle, char **Image, uint32 *ImageSize);
uint16 PTP_API_Get_Thumbnail_MPTP(uint32 handle, char **Image, uint32 *ImageSize,PTPDeviceHandle  ptpHandle);//sheetal added
uint16 PTP_API_Get_JpegImage(uint32 handle, char **Image, uint32 *ImageSize);
uint16 PTP_API_Get_JpegImage_MPTP(uint32 handle, char **Image, uint32 *ImageSize, PTPDeviceHandle  ptpHandle);//sheetal added
uint16 PTP_API_Get_JpegObjectHandles (uint32 storageID, PTPObjectHandles *oh1 );
uint16 PTP_API_Get_JpegObjectHandles_MPTP (uint32 storageID, PTPObjectHandles *oh1, PTPDeviceHandle  ptpHandle );//sheetal added

void PTP_API_PartialReadStart(void);
void PTP_API_PartialReadStop(void);	
uint16 PTP_API_PartialRead(uint32 handle, char** buf, uint32 length, uint32 StartOffset, uint32 *ReturnCurOffset);
uint16 PTP_API_PartialRead_MPTP(uint32 handle, char** buf, uint32 length, uint32 StartOffset, uint32 *ReturnCurOffset, PTPDeviceHandle  ptpHandle);//added by niraj

uint16 PTP_API_Get_MP3File(uint32 handle, char **Image, uint32 *ImageSize);
uint16 PTP_API_Get_MP3File_MPTP(uint32 handle, char **Image, uint32 *ImageSize, PTPDeviceHandle  ptpHandle);//sheetal added
uint32 PTP_API_Get_JpegImageInfo(uint32 handle, PTPObjectInfo *oi);
uint32 PTP_API_Get_JpegImageInfo_MPTP(uint32 handle, PTPObjectInfo *oi,PTPDeviceHandle  ptpHandle);//sheetal added
uint16 PTP_API_Get_DeviceInfo(PTPDeviceInfo *deviceinfo);
uint16 PTP_API_Get_DeviceInfo_MPTP(PTPDeviceInfo *deviceinfo, PTPDeviceHandle  ptpHandle);//sheetal added

uint16 PTP_API_Get_DevPath(char *devpath);
uint16 PTP_API_Get_DevPath_MPTP(char *devpath, PTPDeviceHandle  ptpHandle);

uint16 PTP_API_Get_StorageIDs(PTPStorageIDs *storageids);  
uint16 PTP_API_Get_StorageIDs_MPTP(PTPDeviceHandle  ptpHandle,PTPStorageIDs *storageids);  // sheetal added
uint16 PTP_API_Get_StorageInfos(uint32_t storageid, PTPStorageInfo* storageinfo); 
uint16 PTP_API_Get_StorageInfos_MPTP(PTPDeviceHandle  ptpHandle, uint32_t storageid, PTPStorageInfo* storageinfo);  // sheetal added
uint16 PTP_API_Send_Reset(void);
uint16 PTP_API_Send_Reset_MPTP(PTPDeviceHandle  ptpHandle);//sheetal added
uint16  PTP_API_CheckValidStorageID(unsigned int storageID);

//uint16 PTP_API_Init_DeviceList (ptp_monitoring cb);
uint16 PTP_API_Init_DeviceList ();
//static PTPDevContext *getDc(PTPDeviceHandle  ptpHandle );//to map from devpath to device context
void PTP_API_GetDeviceList(UsbDev* device[],uint32_t* count);//added by Niraj

//exsting before -> removed
#ifdef __cplusplus
}
#endif



#endif
