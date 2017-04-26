#pragma once
#include <ntddk.h>

#include <wdf.h>
#include <hidport.h>
#include <vhf.h>
#include <initguid.h>
#include "common.h"


typedef UCHAR HID_REPORT_DESCRIPTOR, *PHID_REPORT_DESCRIPTOR;

DRIVER_INITIALIZE DriverEntry;


EVT_WDF_DRIVER_DEVICE_ADD VhfHidEvtDriverDeviceAdd;
EVT_WDF_DEVICE_SELF_MANAGED_IO_INIT VhfHidEvtDeviceSelfManagedIoInit;
EVT_WDF_DEVICE_SELF_MANAGED_IO_CLEANUP VhfHidEvtDeviceSelfManagedIoCleanup;
EVT_WDF_IO_QUEUE_IO_WRITE VhfHidEvtIoWriteFromRawPdo;

typedef struct _HID_MAX_COUNT_REPORT
{
	UCHAR ReportID;
	UCHAR MaxCount;
} HIDINJECTOR_MAX_COUNT_REPORT, *PHIDINJECTOR_MAX_COUNT_REPORT;


typedef struct _HID_DEVICE_CONTEXT
{
	WDFDEVICE               Device;
	WDFQUEUE                DefaultQueue;
	WDFQUEUE                ManualQueue;
	HID_DESCRIPTOR          HidDescriptor;
	PHID_REPORT_DESCRIPTOR  ReportDescriptor;
	WDFDEVICE				RawPdo;
	WDFQUEUE				RawPdoQueue;	// Queue for handling requests that come from the rawPdo
	VHFHANDLE               VhfHandle;
} HID_DEVICE_CONTEXT, *PHID_DEVICE_CONTEXT;

WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(HID_DEVICE_CONTEXT, GetHidDeviceContext);


typedef struct _QUEUE_CONTEXT
{
	WDFQUEUE Queue;
	PHID_DEVICE_CONTEXT DeviceContext;
}QUEUE_CONTEXT, *PQUEUE_CONTEXT;

WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(QUEUE_CONTEXT, GetQueueContext);

typedef struct _RAWPDO_DEVICE_CONTEXT
{
	ULONG InstanceNo;

	WDFQUEUE ParentQueue;
}RAWPDO_DEVICE_CONTEXT, *PRAWPDO_DEVICE_CONTEXT;

WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(RAWPDO_DEVICE_CONTEXT, GetRawPdoDeviceContext);


// {F990ABA9-C9C0-4CF8-A4A2-5B06D97F6871}
DEFINE_GUID(GUID_DEVCLASS_HIDINJECTOR,
	0xf990aba9, 0xc9c0, 0x4cf8, 0xa4, 0xa2, 0x5b, 0x6, 0xd9, 0x7f, 0x68, 0x71);

// {C65DDC4B-C013-4113-970A-9C7D0B07F05A}
DEFINE_GUID(GUID_DEVINTERFACE_HIDINJECTOR,
	0xc65ddc4b, 0xc013, 0x4113, 0x97, 0xa, 0x9c, 0x7d, 0xb, 0x7, 0xf0, 0x5a);

// {FFD216E4-A560-4676-8BD5-4F26A5BFF214}
DEFINE_GUID(GUID_BUS_HIDINJECTOR,
	0xffd216e4, 0xa560, 0x4676, 0x8b, 0xd5, 0x4f, 0x26, 0xa5, 0xbf, 0xf2, 0x14);

#define HIDINJECTOR_DEVICE_ID L"{FFD216E4-A560-4676-8BD5-4F26A5BFF214}\\VhfHidDevice\0"
#define MAX_ID_LEN 128



NTSTATUS VhfInitialize(_In_ WDFDEVICE Device);

VOID VhfHidEvtVhfAsyncOperationGetFeature(
	_In_ PVOID VhfClientContext,
	_In_     VHFOPERATIONHANDLE VhfOperationHandle,
	_In_opt_ PVOID              VhfOperationContext,
	_In_     PHID_XFER_PACKET   HidTransferPacket
);

NTSTATUS RawQueueCreate(_In_ WDFDEVICE Device, _Out_ WDFQUEUE *Queue);

VOID VhfHidSubmitReadReport(_In_ PHID_DEVICE_CONTEXT DeviceContext, _In_ PUCHAR Report, _In_ size_t ReportSize);
