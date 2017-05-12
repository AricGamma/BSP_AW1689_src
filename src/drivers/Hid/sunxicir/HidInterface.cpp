/** @file
*
*  Copyright (c) 2007-2016, Allwinner Technology Co., Ltd. All rights reserved.
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
*      http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*
**/

#include "HidInterface.h"

#include "Trace.h"
#include "HidInterface.tmh"


#define DEVICE_INTERFACE_NOTIFICATION_EVENT_COUNT 1
#define VHID_INTERFACE_NOTIFICATION_INDEX 0

#define DEVICE_INTERFACE_NOTIFICATION_TIMEOUT_MS 30000	//timeout 30s

KEVENT g_VhidArrivedEvent;
PKEVENT g_pDeviceNotificationEvent[DEVICE_INTERFACE_NOTIFICATION_EVENT_COUNT];


NTSTATUS RegisterVhidReadyNotification(WDFDEVICE Device)
{
	NTSTATUS status;
	PDEVICE_CONTEXT pDevContext = NULL;
	PVOID pVhidNotificationEntry;
	LARGE_INTEGER notificationTimeout = { 0 };


	pDevContext = DeviceGetContext(Device);

	KeInitializeEvent(&g_VhidArrivedEvent, NotificationEvent, FALSE);

	g_pDeviceNotificationEvent[VHID_INTERFACE_NOTIFICATION_INDEX] = &g_VhidArrivedEvent;

	notificationTimeout.QuadPart = WDF_REL_TIMEOUT_IN_MS(DEVICE_INTERFACE_NOTIFICATION_TIMEOUT_MS);

	status = IoRegisterPlugPlayNotification(
		EventCategoryDeviceInterfaceChange,
		PNPNOTIFY_DEVICE_INTERFACE_INCLUDE_EXISTING_INTERFACES,
		(PVOID)&GUID_DEVINTERFACE_HIDINJECTOR,
		WdfDriverWdmGetDriverObject(WdfGetDriver()),
		(PDRIVER_NOTIFICATION_CALLBACK_ROUTINE)VhidReadyNotificationCallback,
		pDevContext,
		&pVhidNotificationEntry);
	if (!NT_SUCCESS(status))
	{
		DbgPrint_E("failed to register pnp notification for vhid 0x%x\n", status);
		goto RegisterExit;
	}

	//status = KeWaitForSingleObject((PVOID)&g_VhidArrivedEvent, Executive, WaitNotification, FALSE, &notificationTimeout);
	status = KeWaitForMultipleObjects(
		DEVICE_INTERFACE_NOTIFICATION_EVENT_COUNT,
		(PVOID *)g_pDeviceNotificationEvent,
		WaitAll,
		Executive,
		KernelMode,
		FALSE,
		&notificationTimeout,
		NULL
	);
	 //TODO: maybe the timeout setting will be changed after the compatible test
	if (STATUS_TIMEOUT == status)
	{
		DbgPrint_E("Timeout of the VHid device interfaces waiting.\n");
		goto RegisterExit;
	}


RegisterExit:


	return status;
}


NTSTATUS VhidReadyNotificationCallback(PVOID pDeviceChange, PVOID pContext)
{
	NTSTATUS status;
	PDEVICE_CONTEXT pDevContext = NULL;
	WDF_OBJECT_ATTRIBUTES ioTargetAttr;
	WDF_IO_TARGET_OPEN_PARAMS openParams;

	FunctionEnter();

	if (pContext == NULL)
	{
		DbgPrint_E("invalid parameter: pContext\n");
		status = STATUS_INVALID_PARAMETER;
		goto Exit;
	}

	pDevContext = (PDEVICE_CONTEXT)pContext;

	WDF_OBJECT_ATTRIBUTES_INIT(&ioTargetAttr);

	status = WdfIoTargetCreate(pDevContext->FxDevice, &ioTargetAttr, &pDevContext->HidInjectorIoTarget);
	if (!NT_SUCCESS(status))
	{
		DbgPrint_E("Failed to create io target for device 0x%x\n", status);
		goto Exit;
	}

	WDF_IO_TARGET_OPEN_PARAMS_INIT_CREATE_BY_NAME(
		&openParams,
		((PDEVICE_INTERFACE_CHANGE_NOTIFICATION)pDeviceChange)->SymbolicLinkName,
		GENERIC_READ | GENERIC_WRITE);
	openParams.ShareAccess = FILE_SHARE_READ | FILE_SHARE_WRITE;

	status = WdfIoTargetOpen(pDevContext->HidInjectorIoTarget, &openParams);
	if (!NT_SUCCESS(status))
	{
		DbgPrint_E("Failed to open io target 0x%x\n", status);
		goto Exit;
	}

	KeSetEvent(&g_VhidArrivedEvent, 100, FALSE);

	return status;

Exit:

	if (pDevContext->HidInjectorIoTarget != NULL)
	{
		WdfObjectDelete(pDevContext->HidInjectorIoTarget);
		pDevContext->HidInjectorIoTarget = NULL;
	}

	return status;

}



