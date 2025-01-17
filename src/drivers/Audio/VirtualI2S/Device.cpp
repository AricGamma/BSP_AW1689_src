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
#include "DMA.h"
#include "wdm.h"
#include "Registry.h"
#include "Trace.h"
#include "Device.tmh"

#ifdef ALLOC_PRAGMA

#endif

NTSTATUS I2SCreateDevice(
	_In_ PWDFDEVICE_INIT pDeviceInit) 
{
	NTSTATUS Status = STATUS_SUCCESS;
	WDF_PNPPOWER_EVENT_CALLBACKS PnpPowerCallbacks = { 0 };
	WDF_OBJECT_ATTRIBUTES ObjectAttributes = { 0 };
	PDEVICE_CONTEXT pDevExt = NULL;
	WDFDEVICE pDevice = NULL;
	WDF_QUERY_INTERFACE_CONFIG QueryInterfaceConfig = { 0 };
	WDF_DEVICE_POWER_POLICY_IDLE_SETTINGS IdleSettings = { 0 };
	ULONG IdleTimeout = 0;
	WDF_POWER_FRAMEWORK_SETTINGS PoFxSettings = { 0 };

	FunctionEnter();

	//
	// Create the WDF device object
	//

	// Add PNP and PM callbacks 
	WDF_PNPPOWER_EVENT_CALLBACKS_INIT(&PnpPowerCallbacks);
	PnpPowerCallbacks.EvtDevicePrepareHardware = I2SEvtPrepareHardware;
	PnpPowerCallbacks.EvtDeviceReleaseHardware = I2SEvtReleaseHardware;
	PnpPowerCallbacks.EvtDeviceD0Entry = I2SEvtD0Entry;
	PnpPowerCallbacks.EvtDeviceD0Exit = I2SEvtD0Exit;
	WdfDeviceInitSetPnpPowerEventCallbacks(pDeviceInit, &PnpPowerCallbacks);

	// Declare as the PPO
	WdfDeviceInitSetPowerPolicyOwnership(pDeviceInit, TRUE);

	// Allocate device extension
	WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&ObjectAttributes, DEVICE_CONTEXT);

	Status = WdfDeviceCreate(&pDeviceInit, 
		&ObjectAttributes,
		&pDevice);
	if (!NT_SUCCESS(Status))
	{
		DbgPrint_E("WdfDeviceCreate failed with 0x%lx.", Status);
		goto Exit;
	}

	//
	// Get a pointer to the device context structure that we just associated
	// with the device object. We define this structure in the device.h
	// header file. DeviceGetContext is an inline function generated by
	// using the WDF_DECLARE_CONTEXT_TYPE_WITH_NAME macro in device.h.
	// This function will do the type checking and return the device context.
	// If you pass a wrong object handle it will return NULL and assert if
	// run under framework verifier mode.
	//
	pDevExt = DeviceGetContext(pDevice);

	//
	// Initialize the context.
	//

	pDevExt->pDevice = pDevice;
	pDevExt->PowerState = WdfPowerDeviceD0;
	pDevExt->IsFirstD0Entry = TRUE;
	pDevExt->IdleState = 0;
	pDevExt->hPoHandle = NULL;
	//KeInitializeEvent(&pDevExt->ClockFrequencyChangedEvent, SynchronizationEvent, FALSE);

	memset(&pDevExt->PowerDownCompletionCallback, 0, sizeof(pDevExt->PowerDownCompletionCallback));

	memset(pDevExt->pDmaResourceDescriptor, 0, sizeof(pDevExt->pDmaResourceDescriptor));

	WDF_OBJECT_ATTRIBUTES_INIT(&ObjectAttributes);
	ObjectAttributes.ParentObject = pDevice;

	WdfSpinLockCreate(&ObjectAttributes, &pDevExt->pDmaTransmissionLock);

	pDevExt->DmaState = DMA_STATE_NONE;
	pDevExt->pTxDmaEnabler = NULL;
	pDevExt->pRxDmaEnabler = NULL;
	pDevExt->pDmaTransmitTransaction = NULL;
	pDevExt->pDmaReceiveTransaction = NULL;
	pDevExt->pReadAdapter = NULL;
	pDevExt->pWriteAdapter = NULL;
	memset(&pDevExt->DmaReadInfo, 0, sizeof(pDevExt->DmaReadInfo));
	memset(&pDevExt->DmaWriteInfo, 0, sizeof(pDevExt->DmaWriteInfo));
	pDevExt->DmaTransmitTotalLength = 0;
	pDevExt->DmaTransmitLengthPerOperation = 0;
	pDevExt->DmaTransmittedLength = 0;
	pDevExt->DmaReceiveTotalLength = 0;
	pDevExt->DmaReceiveLengthPerOperation = 0;
	pDevExt->DmaReceivedLength = 0;
	pDevExt->pTransmitMdl = NULL;
	pDevExt->pReceiveMdl = NULL;
	pDevExt->DmaInterruptCounter = 0;
	memset(&pDevExt->DmaTransmitCallback, 0, sizeof(pDevExt->DmaTransmitCallback));
	memset(&pDevExt->DmaReceiveCallback, 0, sizeof(pDevExt->DmaReceiveCallback));

	//
	// Register remote device function interface
	//
	pDevExt->I2sInterfaceRef = 0;
	pDevExt->I2sFunctionInterface.InterfaceHeader.Size = sizeof(pDevExt->I2sFunctionInterface);
	pDevExt->I2sFunctionInterface.InterfaceHeader.Version = 1;
	pDevExt->I2sFunctionInterface.InterfaceHeader.Context = (PVOID)pDevice;

	pDevExt->I2sFunctionInterface.I2STransmit = I2STransmit;
	pDevExt->I2sFunctionInterface.I2SReceive = I2SReceive;
	pDevExt->I2sFunctionInterface.I2SStopTransfer = I2SStopTransfer;
	pDevExt->I2sFunctionInterface.I2SSetTransmitCallback = I2SSetTransmitCallback;
	pDevExt->I2sFunctionInterface.I2SSetReceiveCallback = I2SSetReceiveCallback;
	pDevExt->I2sFunctionInterface.I2SSetStreamFormat = I2SSetStreamFormat;
	pDevExt->I2sFunctionInterface.I2SSetPowerState = I2SSetPowerState;
	pDevExt->I2sFunctionInterface.I2SSetPowerDownCompletionCallback = I2SSetPowerDownCompletionCallback;
	pDevExt->I2sFunctionInterface.InterfaceHeader.InterfaceReference =
		I2SInterfaceReference;
	pDevExt->I2sFunctionInterface.InterfaceHeader.InterfaceDereference =
		I2SInterfaceDereference;

	WDF_QUERY_INTERFACE_CONFIG_INIT(&QueryInterfaceConfig,
		(PINTERFACE)&pDevExt->I2sFunctionInterface,
		&GUID_I2S_FUNCTION_INTERFACE,
		NULL);

	Status = WdfDeviceAddQueryInterface(pDevice, &QueryInterfaceConfig);
	if (!NT_SUCCESS(Status))
	{
		DbgPrint_E("WdfDeviceAddQueryInterface failed with 0x%lx.", Status);
		goto Exit;
	}

	//
	// Register device interface 
	//

	Status = WdfDeviceCreateDeviceInterface(pDevice,
		&GUID_I2S_INTERFACE,
		NULL);
	if (!NT_SUCCESS(Status))
	{
		DbgPrint_E("WdfDeviceCreateDeviceInterface failed with 0x%lx.", Status);
	}

	//
	// Register idle capabilities
	//

	WDF_DEVICE_POWER_POLICY_IDLE_SETTINGS_INIT(
		&IdleSettings,
		IdleCanWakeFromS0);

	Status = RegQueryDeviceDwordValue(pDevice, IDLE_TIMEOUT_VALUE_NAME, &IdleTimeout);
	if (!NT_SUCCESS(Status))
	{
		DbgPrint_W("Failed to query device idle timeout value with 0x%lx.", Status);
		Status = STATUS_SUCCESS;
		IdleTimeout = DEFAULT_IDLE_TIMEOUT_VALUE_MS;
	}

	IdleSettings.IdleTimeout = IdleTimeout;
	IdleSettings.IdleTimeoutType = SystemManagedIdleTimeoutWithHint;

	Status = WdfDeviceAssignS0IdleSettings(pDevice, &IdleSettings);
	if (STATUS_POWER_STATE_INVALID == Status)
	{
		DbgPrint_W("WdfDeviceAssignS0IdleSettings with IdleCanWakeFromS0 failed.");

		IdleSettings.IdleCaps = IdleCannotWakeFromS0;
		Status = WdfDeviceAssignS0IdleSettings(pDevice, &IdleSettings);
		if (!NT_SUCCESS(Status))
		{
			DbgPrint_E("WdfDeviceAssignS0IdleSettings with IdleCannotWakeFromS0 failed with 0x%lx.", Status);
		}
	}

	//
	// Register a device component for performance state management by the power management framework (PoFx).
	// Currently, we don't implement the Fx state now, and only register the F0 state for PoFx.
	//

	WDF_POWER_FRAMEWORK_SETTINGS_INIT(&PoFxSettings);
	PoFxSettings.EvtDeviceWdmPostPoFxRegisterDevice =
		I2SSingleCompWdmEvtDeviceWdmPostPoFxRegisterDevice;
	PoFxSettings.EvtDeviceWdmPrePoFxUnregisterDevice =
		I2SSingleCompWdmEvtDeviceWdmPrePoFxUnregisterDevice;
	PoFxSettings.Component = NULL;
	PoFxSettings.ComponentActiveConditionCallback = NULL;
	PoFxSettings.ComponentIdleConditionCallback = NULL;
	PoFxSettings.ComponentIdleStateCallback = NULL;
	PoFxSettings.PoFxDeviceContext = (PVOID)pDevice;

	Status = WdfDeviceWdmAssignPowerFrameworkSettings(pDevice, &PoFxSettings);
	if (!NT_SUCCESS(Status))
	{
		DbgPrint_E("WdfDeviceWdmAssignPowerFrameworkSettings failed with 0x%lx.", Status);
	}

Exit:
	FunctionExit(Status);
	return Status;
}

NTSTATUS I2SEvtPrepareHardware(
	_In_ WDFDEVICE pDevice,
	_In_ WDFCMRESLIST pResources,
	_In_ WDFCMRESLIST pResourcesTranslated)
{
	PDEVICE_CONTEXT pDevExt = NULL;
	NTSTATUS Status = STATUS_SUCCESS;

	PCM_PARTIAL_RESOURCE_DESCRIPTOR pPartialResourceDescRaw = NULL;
	PCM_PARTIAL_RESOURCE_DESCRIPTOR pPartialResourceDescTrans = NULL;
	ULONG DmaResourcesFound = 0;
	ULONG ResourceListCountRaw = 0;
	ULONG ResourceListCountTrans = 0;
	ULONG i = 0;

	FunctionEnter();

	pDevExt = DeviceGetContext(pDevice);

	//
	// TODO: Initialize the DMA HW here
	//

	// Determines how many resources we will iterate over.
	ResourceListCountRaw = WdfCmResourceListGetCount(pResources);
	ResourceListCountTrans = WdfCmResourceListGetCount(pResourcesTranslated);

	if (ResourceListCountRaw != ResourceListCountTrans)
	{
		Status = STATUS_UNSUCCESSFUL;
		goto Exit;
	}

	for (i = 0; i < ResourceListCountRaw; i++)
	{
		// Gets the raw partial resource descriptor.
		pPartialResourceDescRaw =
			WdfCmResourceListGetDescriptor(pResources, i);

		// Gets the translated partial resource descriptor.
		pPartialResourceDescTrans =
			WdfCmResourceListGetDescriptor(pResourcesTranslated, i);

		// Switch based upon the type of resource.
		switch (pPartialResourceDescTrans->Type)
		{
			case CmResourceTypeDma:
			{
				if (DmaResourcesFound < 2)
				{
					pDevExt->pDmaResourceDescriptor[DmaResourcesFound] = pPartialResourceDescTrans;
				}

				DmaResourcesFound++;
				DbgPrint_E("DMA resource [%d] found.", DmaResourcesFound);
				break; 
			}

			default:
				break;
		}
	}

	// Initialize DMA if a resource was found
	if (DMA_ENTRY_NUMBER != DmaResourcesFound)
	{
		DbgPrint_E("Invalid DMA resouce count.");
		Status = STATUS_UNSUCCESSFUL;
		goto Exit;
	}

	Status = I2SDmaInitialize(pDevExt);
	if (!NT_SUCCESS(Status)) 
	{
		DbgPrint_E("Failed to initialize DMA with 0x%lx.", Status);
	}

Exit:
	FunctionExit(Status);
	return Status;
}

NTSTATUS I2SEvtReleaseHardware(
	_In_ WDFDEVICE pDevice,
	_In_ WDFCMRESLIST pResourcesTranslated)
{
	NTSTATUS Status = STATUS_SUCCESS;
	PDEVICE_CONTEXT pDevExt = NULL;

	UNREFERENCED_PARAMETER(pResourcesTranslated);

	FunctionEnter();

	pDevExt = DeviceGetContext(pDevice);

	I2SDmaStop(pDevExt, TRUE);
	I2SDmaStop(pDevExt, FALSE);

	pDevExt->DmaState = DMA_STATE_NONE;

	if (NULL != pDevExt->pDmaTransmissionLock) 
	{
		WdfObjectDelete(pDevExt->pDmaTransmissionLock);
		pDevExt->pDmaTransmissionLock = NULL;
	}

	//
	// TODO: Check the function interface reference here???
	//

	FunctionExit(Status);
	return Status;
}

NTSTATUS I2SEvtD0Entry(
	_In_ WDFDEVICE pDevice,
	_In_ WDF_POWER_DEVICE_STATE PreviousState)
{
	NTSTATUS Status = STATUS_SUCCESS;
	PDEVICE_CONTEXT pDevExt = NULL;

	UNREFERENCED_PARAMETER(PreviousState);

	FunctionEnter();
	DbgPrint_I("Entered, device is coming from [D %d] state.", PreviousState - WdfPowerDeviceD0);

	pDevExt = DeviceGetContext(pDevice);

	//
	// TODO: Power up the device here
	//

	pDevExt->PowerState = WdfPowerDeviceD0;

	if (TRUE == pDevExt->IsFirstD0Entry)
	{
		WdfDeviceStopIdle(pDevExt->pDevice, FALSE);
		pDevExt->IdleState = 1;
		pDevExt->IsFirstD0Entry = FALSE;
	}

	FunctionExit(Status);
	return Status;
}

NTSTATUS I2SEvtD0Exit(
	_In_ WDFDEVICE pDevice,
	_In_ WDF_POWER_DEVICE_STATE TargetState)
{
	NTSTATUS Status = STATUS_SUCCESS;
	PDEVICE_CONTEXT pDevExt = NULL;

	FunctionEnter();
	DbgPrint_I("Entered, device is going to [D %d] state.", TargetState - WdfPowerDeviceD0);

	pDevExt = DeviceGetContext(pDevice);

	//Status = I2SDmaStop(pDevExt);

	//
	// TODO: Power down the device here
	//

	pDevExt->PowerState = TargetState;

	//
	// Notify the audio adapter after the D0 exit
	//
	if ((NULL != pDevExt->PowerDownCompletionCallback.pCallbackContext)
		&& (NULL != pDevExt->PowerDownCompletionCallback.pCallbackRoutine))
	{
		pDevExt->PowerDownCompletionCallback.pCallbackRoutine(pDevExt->PowerDownCompletionCallback.pCallbackContext);
	}

	FunctionExit(Status);
	return Status;
}

NTSTATUS I2SSingleCompWdmEvtDeviceWdmPostPoFxRegisterDevice(
	_In_ WDFDEVICE pDevice,
	_In_ POHANDLE hPoHandle)
{
	NTSTATUS Status = STATUS_SUCCESS;
	PDEVICE_CONTEXT pDevExt = NULL;

	FunctionEnter();

	if ((NULL == pDevice) || (NULL == hPoHandle))
	{
		DbgPrint_E("Invalid parameters.");
		Status = STATUS_INVALID_PARAMETER;
		goto Exit;
	}

	pDevExt = DeviceGetContext(pDevice);
	pDevExt->hPoHandle = hPoHandle;

Exit:
	FunctionExit(Status);
	return Status;
}

void I2SSingleCompWdmEvtDeviceWdmPrePoFxUnregisterDevice(
	_In_ WDFDEVICE pDevice,
	_In_ POHANDLE hPoHandle)
{
	PDEVICE_CONTEXT pDevExt = NULL;

	UNREFERENCED_PARAMETER(hPoHandle);

	FunctionEnter();

	if (NULL == pDevice)
	{
		DbgPrint_E("Invalid parameters.");
		goto Exit;
	}

	pDevExt = DeviceGetContext(pDevice);
	pDevExt->hPoHandle = NULL;

Exit:
	FunctionExit(0);
	return;
}

