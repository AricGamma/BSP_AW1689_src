#include "Device.h"
#include "AwCIR.h"
#include "Interrupt.h"
#include "HidInterface.h"
#include <vhf.h>

#define RESHUB_USE_HELPER_ROUTINES
#include <reshub.h>

#include "Trace.h"
#include "Device.tmh"

NTSTATUS CirDeviceCreate(_In_ PWDFDEVICE_INIT DeviceInit)
{
	NTSTATUS status;
	WDFDEVICE device;
	PDEVICE_CONTEXT pDevContext = NULL;
	WDF_PNPPOWER_EVENT_CALLBACKS pnpCallbacks = { 0 };
	WDF_OBJECT_ATTRIBUTES attr = { 0 };

	PAGED_CODE();
	FunctionEnter();

	WDF_PNPPOWER_EVENT_CALLBACKS_INIT(&pnpCallbacks);
	pnpCallbacks.EvtDevicePrepareHardware = SunxicirEvtDevicePrepareHardware;
	pnpCallbacks.EvtDeviceReleaseHardware = SunxicirEvtDeviceReleaseHardware;
	pnpCallbacks.EvtDeviceD0Entry = SunxicirEvtDeviceD0Entry;
	pnpCallbacks.EvtDeviceD0Exit = SunxicirEvtDeviceD0Exit;

	WdfDeviceInitSetPnpPowerEventCallbacks(DeviceInit, &pnpCallbacks);

	WdfDeviceInitSetPowerPolicyOwnership(DeviceInit, TRUE);

	WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&attr, DEVICE_CONTEXT);

	status = WdfDeviceCreate(&DeviceInit, &attr, &device);
	if (!NT_SUCCESS(status))
	{
		DebugPrint(DEBUG_LEVEL_ERROR, "Error: WdfDeviceCreate failed with error 0x%x\n", status);
		goto Exit;
	}

	pDevContext = DeviceGetContext(device);
	pDevContext->FxDevice = device;

	status = WdfDeviceCreateDeviceInterface(device, &GUID_DEVINTERFACE_IRPORT, NULL);
	if (!NT_SUCCESS(status))
	{
		DebugPrint(DEBUG_LEVEL_ERROR, "Error: WdfDeviceCreateDeviceInterface failed with error 0x%x\n", status);
		goto Exit;
	}

Exit:

	return status;
}

NTSTATUS SunxicirEvtDevicePrepareHardware(_In_ WDFDEVICE Device, _In_ WDFCMRESLIST ResourcesRaw, _In_ WDFCMRESLIST ResourcesTrans)
{
	NTSTATUS status = STATUS_SUCCESS;
	PCM_PARTIAL_RESOURCE_DESCRIPTOR pResDescRaw = NULL;
	PCM_PARTIAL_RESOURCE_DESCRIPTOR pResDescTrans = NULL;
	PDEVICE_CONTEXT pDevContext = NULL;

	ULONG resListCountRaw = 0;
	ULONG resListCountTrans = 0;

	UINT i = 0;

	DECLARE_UNICODE_STRING_SIZE(DevicePath, RESOURCE_HUB_PATH_SIZE);

	FunctionEnter();

	resListCountRaw = WdfCmResourceListGetCount(ResourcesRaw);
	resListCountTrans = WdfCmResourceListGetCount(ResourcesTrans);

	pDevContext = DeviceGetContext(Device);

	if (resListCountRaw != resListCountTrans)
	{
		DbgPrint_E("Error: Raw resource count is not equal to translated resources");
		status = STATUS_UNSUCCESSFUL;
		goto Exit;
	}

	for (i = 0; i < resListCountTrans; i++)
	{
		pResDescRaw = WdfCmResourceListGetDescriptor(ResourcesRaw, i);
		pResDescTrans = WdfCmResourceListGetDescriptor(ResourcesTrans, i);

		switch (pResDescTrans->Type)
		{
		case CmResourceTypeMemory:

			if (pResDescTrans->u.Memory.Length == CIR_REG_LENGTH)
			{
				pDevContext->RegisterBase = (volatile ULONG*)MmMapIoSpace(pResDescTrans->u.Memory.Start, pResDescTrans->u.Memory.Length, MmNonCached);
				pDevContext->RegisterLength = pResDescTrans->u.Memory.Length;
			}

			break;
			
		case CmResourceTypeInterrupt:
			CirInterruptCreate(Device, pResDescRaw, pResDescTrans);
			break;

		default:
			break;
		}
	}



Exit:

	return status;
}

NTSTATUS SunxicirEvtDeviceReleaseHardware(_In_ WDFDEVICE Device, _In_ WDFCMRESLIST ResourcesTrans)
{
	PDEVICE_CONTEXT pDevContext = NULL;
	FunctionEnter();

	UNREFERENCED_PARAMETER(ResourcesTrans);

	pDevContext = DeviceGetContext(Device);

	//Unmap register
	if (pDevContext->RegisterBase != NULL)
	{
		MmUnmapIoSpace(PVOID(pDevContext->RegisterBase), pDevContext->RegisterLength);

		pDevContext->RegisterBase = NULL;
		pDevContext->RegisterLength = 0;
	}


	return STATUS_SUCCESS;
}

NTSTATUS SunxicirEvtDeviceD0Entry(_In_ WDFDEVICE Device, _In_ WDF_POWER_DEVICE_STATE PreviousState)
{
	NTSTATUS status;

	FunctionEnter();

	UNREFERENCED_PARAMETER(PreviousState);

	status = RegisterVhidReadyNotification(Device);
	if (!NT_SUCCESS(status))
	{
		return status;
	}

	status = CIRInitialize(Device);
	if (!NT_SUCCESS(status))
	{
		return status;
	}


	return STATUS_SUCCESS;
}

NTSTATUS SunxicirEvtDeviceD0Exit(_In_ WDFDEVICE Device, _In_ WDF_POWER_DEVICE_STATE TargetState)
{

	FunctionEnter();

	UNREFERENCED_PARAMETER(Device);
	UNREFERENCED_PARAMETER(TargetState);

	return NTSTATUS();
}