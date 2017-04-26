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

#include "Device.h"
#include "Trace.h"
#include <ntintsafe.h>
#include "CodecInterface.tmh"
#include "awcodec.h"

void CodecInterfaceReference(
	_In_ PVOID pContext) 
{

	PDEVICE_CONTEXT pDevExt = NULL;

	FunctionEnter();

	if (NULL == pContext)
	{
		DbgPrint_E("Invalid parameter.");
		goto Exit;
	}

	pDevExt = DeviceGetContext((WDFDEVICE)pContext);

	InterlockedIncrement((LONG *)&pDevExt->CodecInterfaceRef);

Exit:
	FunctionExit(STATUS_SUCCESS);
	return;
}

void CodecInterfaceDereference(
	_In_ PVOID pContext) 
{
	PDEVICE_CONTEXT pDevExt = NULL;

	FunctionEnter();

	if (NULL == pContext)
	{
		DbgPrint_E("Invalid parameter.");
		goto Exit;
	}

	pDevExt = DeviceGetContext((WDFDEVICE)pContext);

	InterlockedDecrement((LONG *)&pDevExt->CodecInterfaceRef);

Exit:
	FunctionExit(STATUS_SUCCESS);
	return;
}

NTSTATUS CodecRegisterEndpointNotification(
	_In_ PVOID pContext, 
	_In_ PENDPOINT_NOTIFICATION pNotification)
{
	NTSTATUS Status = STATUS_SUCCESS;
	PDEVICE_CONTEXT pDevExt = NULL;
	PENDPOINT_NOTIFICATION_LIST_ITEM pItem = NULL;

	FunctionEnter();

	if ((NULL == pContext)
		|| (NULL == pNotification)) 
	{
		DbgPrint_E("Invalid parameters.");
		Status = STATUS_INVALID_PARAMETER;
		goto Exit;
	}

	if (pNotification->Header.Size != sizeof(ENDPOINT_NOTIFICATION))
	{
		DbgPrint_E("Invalid notification callback size.");
		Status = STATUS_INVALID_PARAMETER;
		goto Exit;
	}

	pDevExt = DeviceGetContext((WDFDEVICE)pContext);

	pItem = (PENDPOINT_NOTIFICATION_LIST_ITEM)ExAllocatePoolWithTag(NonPagedPool, 
		sizeof(ENDPOINT_NOTIFICATION_LIST_ITEM), 
		CODEC_POOL_TAG);
	if (NULL == pItem) 
	{
		DbgPrint_E("Failed to allocate endpoint notification callback list item.");
		Status = STATUS_INSUFFICIENT_RESOURCES;
		goto Exit;
	}

	pItem->NotificationType = pNotification->Header.NotificationType;
	pItem->DeviceType = pNotification->Header.DeviceType;
	pItem->pCallbackRoutine = pNotification->pCallbackRoutine;
	pItem->pCallbackContex = pNotification->pCallbackContex;
	
	InsertEndpointNotificationItem(pDevExt, pItem);

Exit:
	FunctionExit(Status);
	return Status;
}

NTSTATUS CodecSetPowerState(
	_In_ PVOID pContext, 
	_In_ DEVICE_POWER_STATE PowerState) 
{
	NTSTATUS Status = STATUS_SUCCESS;
	PDEVICE_CONTEXT pDevExt = NULL;

	FunctionEnter();

	if (NULL == pContext)
	{
		DbgPrint_E("Invalid parameter.");
		Status = STATUS_INVALID_PARAMETER;
		goto Exit;
	}

	pDevExt = DeviceGetContext((WDFDEVICE)pContext);

	DbgPrint_I("Set power state to [D %d].", PowerState - PowerDeviceD0);

	if ((PowerState - PowerDeviceD0) == (pDevExt->PowerState - WdfPowerDeviceD0))
	{
		DbgPrint_I("The device is already in the state, do nothing.");
		if (pDevExt->IdleState <= 0)
		{
			DbgPrint_E("----------WdfDeviceStopIdle ELSE-----------");
			Status = WdfDeviceStopIdle(pDevExt->pDevice, TRUE);
			pDevExt->IdleState++;
		}

		if (PowerDeviceD3 == PowerState)
		{
			if ((NULL != pDevExt->PowerDownCompletionCallback.pCallbackContext)
				&& (NULL != pDevExt->PowerDownCompletionCallback.pCallbackRoutine))
			{
				pDevExt->PowerDownCompletionCallback.pCallbackRoutine(pDevExt->PowerDownCompletionCallback.pCallbackContext);
			}
		}
		goto Exit;
	}

	if (PowerDeviceD0 == PowerState)
	{
		// Wake up the device once the power state changed Dx->D0
		Status = WdfDeviceStopIdle(pDevExt->pDevice, TRUE);
		pDevExt->IdleState++;
	}
	else if (WdfPowerDeviceD0 == pDevExt->PowerState)
	{
		// Power down the device once the power state changed D0->Dx 
		ASSERT(pDevExt->IdleState > 0);
		WdfDeviceResumeIdle(pDevExt->pDevice);
		pDevExt->IdleState--;
	}
	else
	{
		// TODO :: Uncover state
		DbgPrint_E("Uncovered !! Set power state to [D %d]. Current State is [D %d]",
			PowerState - PowerDeviceD0, pDevExt->PowerState - WdfPowerDeviceD0);
	}

Exit:
	FunctionExit(Status);
	return Status;
}

NTSTATUS CodecSetPowerDownCompletionCallback(
	_In_ PVOID pContext,
	_In_ PCODEC_POWER_DOWN_COMPLETION_CALLBACK pPowerDownCompletionCallback)
{
	NTSTATUS Status = STATUS_SUCCESS;
	PDEVICE_CONTEXT pDevExt = NULL;

	FunctionEnter();

	if ((NULL == pContext)
		|| (NULL == pPowerDownCompletionCallback))
	{
		DbgPrint_E("Invalid parameters.");
		Status = STATUS_INVALID_PARAMETER;
		goto Exit;
	}

	pDevExt = DeviceGetContext((WDFDEVICE)pContext);

	pDevExt->PowerDownCompletionCallback.pCallbackContext = pPowerDownCompletionCallback->pCallbackContext;
	pDevExt->PowerDownCompletionCallback.pCallbackRoutine = pPowerDownCompletionCallback->pCallbackRoutine;

Exit:
	FunctionExit(Status);
	return Status;
}


static PTCHAR s[4] = {"stop","acquire","pause","run"};
NTSTATUS CodecSetStreamState(
	_In_ PVOID pContext, 
	_In_ BOOL IsCapture, 
	_In_ KSSTATE State)
{
	NTSTATUS Status = STATUS_SUCCESS;
	PDEVICE_CONTEXT pDevExt = NULL;

	DbgPrint_V("Set KsState to %s\n", s[State]);

	if (NULL == pContext)
	{
		DbgPrint_E("Invalid parameter.");
		Status = STATUS_INVALID_PARAMETER;
		goto Exit;
	}

	pDevExt = DeviceGetContext((WDFDEVICE)pContext);
	Status = AWCodecStateControl(IsCapture, State);
Exit:
	return Status;
}

NTSTATUS CodecSetStreamMode(
	_In_ PVOID pContext, 
	_In_ BOOL IsCapture, 
	_In_ ULONG Mode) 
{
	NTSTATUS Status = STATUS_SUCCESS;
	PDEVICE_CONTEXT pDevExt = NULL;

	FunctionEnter();

	if (NULL == pContext)
	{
		DbgPrint_E("Invalid parameter.");
		Status = STATUS_INVALID_PARAMETER;
		goto Exit;
	}

	pDevExt = DeviceGetContext((WDFDEVICE)pContext);

	//
	// TODO: Set the mode to HW, if necessary
	//
	UNREFERENCED_PARAMETER(IsCapture);
	UNREFERENCED_PARAMETER(Mode);

Exit:
	FunctionExit(Status);
	return Status;
}

NTSTATUS CodecSetStreamFormat(
	_In_ PVOID pContext, 
	_In_ BOOL IsCapture, 
	_In_ PWAVEFORMATEXTENSIBLE pWaveFormatExt) 
{
	NTSTATUS Status = STATUS_SUCCESS;
	PDEVICE_CONTEXT pDevExt = NULL;

	FunctionEnter();

	if ((NULL == pContext)
		|| (NULL == pWaveFormatExt))
	{
		DbgPrint_E("Invalid parameters.");
		Status = STATUS_INVALID_PARAMETER;
		goto Exit;
	}

	pDevExt = DeviceGetContext((WDFDEVICE)pContext);

	//
	// TODO: Set format to HW, if necessary
	//
	UNREFERENCED_PARAMETER(IsCapture);

Exit:
	FunctionExit(Status);
	return Status;
}

NTSTATUS CodecSetVolume(
	_In_ PVOID pContext, 
	_In_ BOOL IsCapture, 
	_In_ ULONG Channel, 
	_In_ LONG Volume) 
{
	NTSTATUS Status = STATUS_SUCCESS;
	PDEVICE_CONTEXT pDevExt = NULL;

	FunctionEnter();

	if (NULL == pContext)
	{
		DbgPrint_E("Invalid parameter.");
		Status = STATUS_INVALID_PARAMETER;
		goto Exit;
	}

	pDevExt = DeviceGetContext((WDFDEVICE)pContext);

	//
	// TODO: Set the volume to HW here
	//
	UNREFERENCED_PARAMETER(IsCapture);
	UNREFERENCED_PARAMETER(Channel);
	UNREFERENCED_PARAMETER(Volume);

Exit:
	FunctionExit(Status);
	return Status;
}

NTSTATUS CodecGetVolume(
	_In_ PVOID pContext, 
	_In_ BOOL IsCapture, 
	_In_ ULONG Channel, 
	_Out_ LONG *pVolume) 
{
	NTSTATUS Status = STATUS_SUCCESS;
	PDEVICE_CONTEXT pDevExt = NULL;

	FunctionEnter();

	if ((NULL == pContext)
		|| (NULL == pVolume))
	{
		DbgPrint_E("Invalid parameters.");
		Status = STATUS_INVALID_PARAMETER;
		goto Exit;
	}

	pDevExt = DeviceGetContext((WDFDEVICE)pContext);
	
	//
	// TODO: Read the volume from HW here
	//
	UNREFERENCED_PARAMETER(IsCapture);
	UNREFERENCED_PARAMETER(Channel);

Exit:
	FunctionExit(Status);
	return Status;
}

NTSTATUS CodecSetMute(
	_In_ PVOID pContext,
	_In_ BOOL IsCapture, 
	_In_ ULONG Channel, 
	_In_ BOOL IsMute)
{
	NTSTATUS Status = STATUS_SUCCESS;
	PDEVICE_CONTEXT pDevExt = NULL;

	FunctionEnter();

	if (NULL == pContext)
	{
		DbgPrint_E("Invalid parameter.");
		Status = STATUS_INVALID_PARAMETER;
		goto Exit;
	}

	pDevExt = DeviceGetContext((WDFDEVICE)pContext);

	//
	// TODO: Mute the HW here
	//
	UNREFERENCED_PARAMETER(IsCapture);
	UNREFERENCED_PARAMETER(Channel);
	UNREFERENCED_PARAMETER(IsMute);

Exit:
	FunctionExit(Status);
	return Status;
}

NTSTATUS CodecGetMute(
	_In_ PVOID pContext, 
	_In_ BOOL IsCapture, 
	_In_ ULONG Channel, 
	_Out_ BOOL *pIsMute) 
{
	NTSTATUS Status = STATUS_SUCCESS;
	PDEVICE_CONTEXT pDevExt = NULL;

	FunctionEnter();

	if ((NULL == pContext)
		|| (NULL == pIsMute))
	{
		DbgPrint_E("Invalid parameters.");
		Status = STATUS_INVALID_PARAMETER;
		goto Exit;
	}

	pDevExt = DeviceGetContext((WDFDEVICE)pContext);

	//
	// TODO: Read the mute state from HW here
	//
	UNREFERENCED_PARAMETER(IsCapture);
	UNREFERENCED_PARAMETER(Channel);

Exit:
	FunctionExit(Status);
	return Status;
}

NTSTATUS CodecGetPeakMeter(
	_In_ PVOID pContext, 
	_In_ BOOL IsCapture, 
	_In_ ULONG Channel, 
	_Out_ LONG *pPeakMeter) 
{
	NTSTATUS Status = STATUS_SUCCESS;
	PDEVICE_CONTEXT pDevExt = NULL;

	FunctionEnter();

	if ((NULL == pContext)
		|| (NULL == pPeakMeter))
	{
		DbgPrint_E("Invalid parameters.");
		Status = STATUS_INVALID_PARAMETER;
		goto Exit;
	}

	pDevExt = DeviceGetContext((WDFDEVICE)pContext);

	//
	// TODO: Read the peakmeter value from HW here
	//
	UNREFERENCED_PARAMETER(IsCapture);
	UNREFERENCED_PARAMETER(Channel);

Exit:
	FunctionExit(Status);
	return Status;
}