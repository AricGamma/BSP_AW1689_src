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

#pragma once

#include <ntddk.h>
#include <wdf.h>
#include "CodecInterface.h"

#define CODEC_POOL_TAG 'EDOC'

//
// GPIO configs
//

#define INTERRUPT_PIN_NUMBER 1

//
// Registry configs
//
// Driver configured idle timeout value, not efficient in current design.
// Because we configure the SystemManagedIdleTimeoutWithHint in Idle Settings.
#define IDLE_TIMEOUT_VALUE_NAME L"IdleTimeout"
#define DEFAULT_IDLE_TIMEOUT_VALUE_MS 500

typedef struct _ENDPOINT_NOTIFICATION_LIST_ITEM
{
	LIST_ENTRY ListEntry;
	ENDPOINT_NOTIFICATION_TYPE NotificationType;
	ENDPOINT_DEVICE_TYPE DeviceType;
	PNotificationCallbackRoutine pCallbackRoutine;
	PVOID pCallbackContex;
} ENDPOINT_NOTIFICATION_LIST_ITEM, *PENDPOINT_NOTIFICATION_LIST_ITEM;

//
// The device context performs the same job as
// a WDM device extension in the driver frameworks 
//
typedef struct _DEVICE_CONTEXT
{
	WDFDEVICE pDevice;
	WDFWAITLOCK pWaitLock;
	WDFIOTARGET pIoTarget;
	WDFSPINLOCK pListSpinLock;
	LIST_ENTRY NotificationList;
	CODEC_FUNCTION_INTERFACE CodecFunctionInterface;
	ULONG CodecInterfaceRef;
	WDF_POWER_DEVICE_STATE PowerState;
	CODEC_POWER_DOWN_COMPLETION_CALLBACK PowerDownCompletionCallback;
	BOOLEAN IsFirstD0Entry;
	int	 IdleState;
} DEVICE_CONTEXT, *PDEVICE_CONTEXT;

//
// This macro will generate an inline function called DeviceGetContext
// which will be used to get a pointer to the device context memory
// in a type safe manner.
//
WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(DEVICE_CONTEXT, DeviceGetContext)

EXTERN_C_START

EVT_WDF_DEVICE_PREPARE_HARDWARE CodecEvtPrepareHardware;
EVT_WDF_DEVICE_RELEASE_HARDWARE CodecEvtReleaseHardware;
EVT_WDF_DEVICE_D0_ENTRY CodecEvtD0Entry;
EVT_WDF_DEVICE_D0_EXIT CodecEvtD0Exit;

EXTERN_C_END

NTSTATUS CodecCreateDevice(_In_ PWDFDEVICE_INIT pDeviceInit);
PENDPOINT_NOTIFICATION_LIST_ITEM GetEndpointNotificationItem(
	_In_ DEVICE_CONTEXT *pDevExt, _In_ ENDPOINT_NOTIFICATION_TYPE NotificationType, _In_ ENDPOINT_DEVICE_TYPE DeviceType);
void InsertEndpointNotificationItem(_In_ DEVICE_CONTEXT *pDevExt, _In_ PENDPOINT_NOTIFICATION_LIST_ITEM pItem);
void EmptyEndpointNotificationList(_In_ DEVICE_CONTEXT *pDevExt);