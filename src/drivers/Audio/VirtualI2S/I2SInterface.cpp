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

#include "I2SInterface.h"
#include "Device.h"
#include "DMA.h"
#include "Trace.h"
#include "I2SInterface.tmh"

void I2SInterfaceReference(
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

	InterlockedIncrement((LONG *)&pDevExt->I2sInterfaceRef);

Exit:
	FunctionExit(STATUS_SUCCESS);
	return;
}

void I2SInterfaceDereference(
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

	InterlockedDecrement((LONG *)&pDevExt->I2sInterfaceRef);

Exit:
	FunctionExit(STATUS_SUCCESS);
	return;
}

NTSTATUS I2SSetTransmitCallback(
	_In_ PVOID pContext,
	_In_ PI2S_TRANSMISSION_CALLBACK pTransmissionCallback)
{
	NTSTATUS Status = STATUS_SUCCESS;
	PDEVICE_CONTEXT pDevExt = NULL;

	FunctionEnter();

	if ((NULL == pContext)
		|| (NULL == pTransmissionCallback))
	{
		DbgPrint_E("Invalid parameters.");
		Status = STATUS_INVALID_PARAMETER;
		goto Exit;
	}

	pDevExt = DeviceGetContext((WDFDEVICE)pContext);

	pDevExt->DmaTransmitCallback.pCallbackContext = pTransmissionCallback->pCallbackContext;
	pDevExt->DmaTransmitCallback.pCallbackRoutine = pTransmissionCallback->pCallbackRoutine;

Exit:
	FunctionExit(Status);
	return Status;
}

NTSTATUS I2SSetReceiveCallback(
	_In_ PVOID pContext,
	_In_ PI2S_TRANSMISSION_CALLBACK pTransmissionCallback)
{
	NTSTATUS Status = STATUS_SUCCESS;
	PDEVICE_CONTEXT pDevExt = NULL;

	FunctionEnter();

	if ((NULL == pContext)
		|| (NULL == pTransmissionCallback))
	{
		DbgPrint_E("Invalid parameters.");
		Status = STATUS_INVALID_PARAMETER;
		goto Exit;
	}

	pDevExt = DeviceGetContext((WDFDEVICE)pContext);

	pDevExt->DmaReceiveCallback.pCallbackContext = pTransmissionCallback->pCallbackContext;
	pDevExt->DmaReceiveCallback.pCallbackRoutine = pTransmissionCallback->pCallbackRoutine;

Exit:
	FunctionExit(Status);
	return Status;
}

NTSTATUS I2STransmit(
	_In_ PVOID pContext,
	_In_ PMDL pTransferMdl,
	_In_ ULONG TransferSize,
	_In_ ULONG NotificationsPerBuffer)
{
	NTSTATUS Status = STATUS_SUCCESS;
	PDEVICE_CONTEXT pDevExt = NULL;

	FunctionEnter();

	if ((NULL == pContext)
		|| (NULL == pTransferMdl))
	{
		DbgPrint_E("Invalid parameters.");
		Status = STATUS_INVALID_PARAMETER;
		goto Exit;
	}

	pDevExt = DeviceGetContext((WDFDEVICE)pContext);

	Status = I2SDmaTransmit(pDevExt, pTransferMdl, TransferSize, NotificationsPerBuffer);
	if (!NT_SUCCESS(Status)) 
	{
		DbgPrint_E("Failed to start transmit DMA with 0x%lx.", Status);
		goto Exit;
	}

	//
	// TODO: Start DMA HW TX here, if necessary
	//

Exit:
	FunctionExit(Status);
	return Status;
}

NTSTATUS I2SReceive(
	_In_ PVOID pContext,
	_In_ PMDL pTransferMdl,
	_In_ ULONG TransferSize,
	_In_ ULONG NotificationsPerBuffer)
{
	NTSTATUS Status = STATUS_SUCCESS;
	PDEVICE_CONTEXT pDevExt = NULL;

	FunctionEnter();

	if ((NULL == pContext)
		|| (NULL == pTransferMdl))
	{
		DbgPrint_E("Invalid parameters.");
		Status = STATUS_INVALID_PARAMETER;
		goto Exit;
	}

	pDevExt = DeviceGetContext((WDFDEVICE)pContext);

	Status = I2SDmaReceive(pDevExt, pTransferMdl, TransferSize, NotificationsPerBuffer);
	if (!NT_SUCCESS(Status))
	{
		DbgPrint_E("Failed to start receive DMA with 0x%lx.", Status);
		goto Exit;
	}

	//
	// TODO: Start DMA HW RX here, if necessary
	//

Exit:
	FunctionExit(Status);
	return Status;
}

NTSTATUS I2SStopTransfer(
	_In_ PVOID pContext,
	_In_ BOOL IsCapture)
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

	Status = I2SDmaStop(pDevExt, IsCapture);

Exit:
	FunctionExit(Status);
	return Status;
}

NTSTATUS I2SSetStreamFormat(
	_In_ PVOID pContext, 
	_In_ BOOL IsCapture, 
	_In_ PWAVEFORMATEXTENSIBLE pWaveFormatExt)
{
	NTSTATUS Status = STATUS_SUCCESS;
	PDEVICE_CONTEXT pDevExt = NULL;

	FunctionEnter();

	if ((NULL == pContext) || (NULL == pWaveFormatExt)) 
	{
		DbgPrint_E("Invalid parameters.");
		Status = STATUS_INVALID_PARAMETER;
		goto Exit;
	}

	pDevExt = DeviceGetContext((WDFDEVICE)pContext);

	//
	// TODO: Set the format to HW here
	//

	//
	// TODO: Set the I2S clock here, if necessary
	//
	UNREFERENCED_PARAMETER(IsCapture);

Exit:
	FunctionExit(Status);
	return Status;
}

NTSTATUS I2SSetPowerState(
	_In_ PVOID pContext, 
	_In_ DEVICE_POWER_STATE PowerState) 
{
	NTSTATUS Status = STATUS_SUCCESS;
	PDEVICE_CONTEXT pDevExt = NULL;

	UNREFERENCED_PARAMETER(PowerState);

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
			DbgPrint_E("FIXED : WdfDeviceStopIdle ...");
			Status = WdfDeviceStopIdle(pDevExt->pDevice, TRUE);
			pDevExt->IdleState++;
		}

		if (WdfPowerDeviceD3 == PowerState)
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
		if (!NT_SUCCESS(Status))
		{
			DbgPrint_E("Failed to stop idle with 0x%lx.", Status);
			goto Exit;
		}
		pDevExt->IdleState++;
	}
	else if (WdfPowerDeviceD0 == pDevExt->PowerState)
	{
		Status = I2SDmaStop(pDevExt, TRUE);
		Status |= I2SDmaStop(pDevExt, FALSE);
		if (!NT_SUCCESS(Status))
		{
			DbgPrint_E("Failed to stop DMA with 0x%lx.", Status);
			goto Exit;
		}

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

NTSTATUS I2SSetPowerDownCompletionCallback(
	_In_ PVOID pContext,
	_In_ PI2S_POWER_DOWN_COMPLETION_CALLBACK pPowerDownCompletionCallback)
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
