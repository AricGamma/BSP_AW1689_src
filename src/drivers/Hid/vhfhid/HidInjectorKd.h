/*++

Copyright (C) Microsoft Corporation, All Rights Reserved

Module Name:

hidinjectorkd.h

Abstract:

This module contains the type definitions for the driver

Environment:

Windows Driver Framework (WDF)

--*/

#ifdef _KERNEL_MODE
#include <ntddk.h>
#else
#include <windows.h>
#endif

#include <wdf.h>
#include <hidport.h>
#include <vhf.h>
#include <initguid.h>
#include "common.h"

typedef UCHAR HID_REPORT_DESCRIPTOR, *PHID_REPORT_DESCRIPTOR;

DRIVER_INITIALIZE                   HIDINJECTOR_DriverEntry;
EVT_WDF_DRIVER_DEVICE_ADD           HIDINJECTOR_EvtDeviceAdd;
EVT_WDF_DEVICE_SELF_MANAGED_IO_INIT HIDINJECTOR_EvtDeviceSelfManagedIoInit;
EVT_WDF_IO_QUEUE_IO_WRITE			HIDINJECTOR_EvtIoWriteForRawPdo;
EVT_WDF_IO_QUEUE_IO_WRITE			HIDINJECTOR_EvtIoWriteFromRawPdo;

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
	WDFQUEUE                Queue;
	PHID_DEVICE_CONTEXT     DeviceContext;
} QUEUE_CONTEXT, *PQUEUE_CONTEXT;

WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(QUEUE_CONTEXT, GetQueueContext);


// Raw PDO context.  
typedef struct _RAWPDO_DEVICE_CONTEXT
{

	// TODO; is this used
	ULONG InstanceNo;

	//
	// Queue of the parent device we will forward requests to
	//
	WDFQUEUE ParentQueue;

} RAWPDO_DEVICE_CONTEXT, *PRAWPDO_DEVICE_CONTEXT;

WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(RAWPDO_DEVICE_CONTEXT, GetRawPdoDeviceContext)

// {EB036691-9915-4B73-B66B-E5EB9500CE61}
DEFINE_GUID(GUID_DEVCLASS_HIDINJECTOR,
	0xeb036691, 0x9915, 0x4b73, 0xb6, 0x6b, 0xe5, 0xeb, 0x95, 0x0, 0xce, 0x61);

// {9EDB43C3-28F9-4111-98F9-0CBB390241BE}
DEFINE_GUID(GUID_DEVINTERFACE_HIDINJECTOR,
	0x9edb43c3, 0x28f9, 0x4111, 0x98, 0xf9, 0xc, 0xbb, 0x39, 0x2, 0x41, 0xbe);


// {0B438283-FBAE-44CC-8958-D4F00722CB46}
DEFINE_GUID(GUID_BUS_HIDINJECTOR,
	0xb438283, 0xfbae, 0x44cc, 0x89, 0x58, 0xd4, 0xf0, 0x7, 0x22, 0xcb, 0x46);


#define  HIDINJECTOR_DEVICE_ID L"{0B438283-FBAE-44CC-8958-D4F00722CB46}\\VhfHidDevice\0"

NTSTATUS
RawQueueCreate(
	_In_ WDFDEVICE										Device,
	_Out_ WDFQUEUE										*Queue
);

NTSTATUS
HIDINJECTOR_CreateRawPdo(
	_In_  WDFDEVICE         Device
);

EVT_WDF_DEVICE_SELF_MANAGED_IO_INIT     HIDINJECTOR_EvtDeviceSelfManagedIoInit;
EVT_WDF_DEVICE_SELF_MANAGED_IO_INIT     RAWPDO_EvtDeviceSelfManagedIoInit;
EVT_WDF_DEVICE_SELF_MANAGED_IO_CLEANUP  HIDINJECTOR_EvtDeviceSelfManagedIoCleanup;

_IRQL_requires_(PASSIVE_LEVEL)
NTSTATUS
HIDINJECTOR_VhfInitialize(
	WDFDEVICE WdfDevice
);

VOID
HIDINJECTOR_VhfSubmitReadReport(
	_In_ PHID_DEVICE_CONTEXT DeviceContext,
	_In_ PUCHAR Report,
	_In_ size_t ReportSize
);

NTSTATUS
HIDINJECTOR_GetFeatureReport(
	_In_ PHID_DEVICE_CONTEXT DeviceContext,
	_In_ PHID_XFER_PACKET    HidTransferPacket
);

VOID
HIDINJECTOR_VhfAsyncOperationGetFeature(
	_In_     PVOID              VhfClientContext,
	_In_     VHFOPERATIONHANDLE VhfOperationHandle,
	_In_opt_ PVOID              VhfOperationContext,
	_In_     PHID_XFER_PACKET   HidTransferPacket
);

