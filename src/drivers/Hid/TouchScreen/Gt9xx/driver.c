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

#include <hidctp.h>


#ifdef ALLOC_PRAGMA
#pragma alloc_text( INIT, DriverEntry )
#pragma alloc_text( PAGE, HidCtpEvtDeviceAdd)
#pragma alloc_text( PAGE, HidCtpEvtDriverContextCleanup)
#endif

NTSTATUS
DriverEntry(
	_In_ PDRIVER_OBJECT  DriverObject,
	_In_ PUNICODE_STRING RegistryPath
	)
/*++

Routine Description:

	Installable driver initialization entry point.
	This entry point is called directly by the I/O system.

Arguments:

	DriverObject - pointer to the driver object

	RegistryPath - pointer to a unicode string representing the path,
	to driver-specific key in the registry.

Return Value:

	STATUS_SUCCESS if successful,
	STATUS_UNSUCCESSFUL otherwise.

--*/
{
	NTSTATUS               status;
	WDF_DRIVER_CONFIG      config;
	WDF_OBJECT_ATTRIBUTES  attributes;

	WDF_DRIVER_CONFIG_INIT(&config, HidCtpEvtDeviceAdd);

	//
	// Register a cleanup callback so that we can call WPP_CLEANUP when
	// the framework driver object is deleted during driver unload.
	//
	WDF_OBJECT_ATTRIBUTES_INIT(&attributes);
	attributes.EvtCleanupCallback = HidCtpEvtDriverContextCleanup;

	//
	// Create a framework driver object to represent our driver.
	//
	status = WdfDriverCreate(DriverObject,
		RegistryPath,
		&attributes,      // Driver Attributes
		&config,          // Driver Config Info
		WDF_NO_HANDLE
		);

	if (!NT_SUCCESS(status)) {

		KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "gt9xx: WdfDriverCreate fail!\n"));
	}

	return status;
}


NTSTATUS
HidCtpEvtDeviceAdd(
	IN WDFDRIVER       Driver,
	IN PWDFDEVICE_INIT DeviceInit
	)
/*++
Routine Description:

	HidCtpEvtDeviceAdd is called by the framework in response to AddDevice
	call from the PnP manager. We create and initialize a WDF device object to
	represent a new instance of toaster device.

Arguments:

	Driver - Handle to a framework driver object created in DriverEntry

	DeviceInit - Pointer to a framework-allocated WDFDEVICE_INIT structure.

Return Value:

	NTSTATUS

--*/
{
	NTSTATUS                      status;
	WDFDEVICE                     hDevice;
	WDFQUEUE                      queue;
	PDEVICE_EXTENSION             devContext = NULL;
	WDF_IO_QUEUE_CONFIG           queueConfig;
	WDF_OBJECT_ATTRIBUTES         attributes;
	WDF_OBJECT_ATTRIBUTES         LockAttributes;
	WDF_PNPPOWER_EVENT_CALLBACKS  pnpPowerCallbacks;

	UNREFERENCED_PARAMETER(Driver);

	PAGED_CODE();

	//
	// Tell framework this is a filter driver. Filter drivers by default are  
	// not power policy owners. This works well for this driver because
	// HIDclass driver is the power policy owner for HID minidrivers.
	//
	WdfFdoInitSetFilter(DeviceInit);

	//
	// Initialize pnp-power callbacks, attributes and a context area for the device object.
	//
	//
	WDF_PNPPOWER_EVENT_CALLBACKS_INIT(&pnpPowerCallbacks);

	//
	// For usb devices, PrepareHardware callback is the to place select the
	// interface and configure the device.
	//
	pnpPowerCallbacks.EvtDevicePrepareHardware = HidCtpEvtDevicePrepareHardware;

	//
	// These two callbacks start and stop the wdfusb pipe continuous reader
	// as we go in and out of the D0-working state.
	//
	pnpPowerCallbacks.EvtDeviceD0Entry = HidCtpEvtDeviceD0Entry;
	pnpPowerCallbacks.EvtDeviceD0Exit = HidCtpEvtDeviceD0Exit;

	WdfDeviceInitSetPnpPowerEventCallbacks(DeviceInit, &pnpPowerCallbacks);

	WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&attributes, DEVICE_EXTENSION);

	//
	// Create a framework device object.This call will in turn create
	// a WDM device object, attach to the lower stack, and set the
	// appropriate flags and attributes.
	//
	status = WdfDeviceCreate(&DeviceInit, &attributes, &hDevice);
	if (!NT_SUCCESS(status)) {
		return status;
	}

	devContext = GetDeviceContext(hDevice);
	devContext->FxDevice = hDevice;

	WDF_IO_QUEUE_CONFIG_INIT_DEFAULT_QUEUE(&queueConfig, WdfIoQueueDispatchParallel);
	queueConfig.EvtIoInternalDeviceControl = HidCtpEvtInternalDeviceControl;

	status = WdfIoQueueCreate(hDevice,
		&queueConfig,
		WDF_NO_OBJECT_ATTRIBUTES,
		&queue
		);
	if (!NT_SUCCESS(status)) {
		return status;
	}

	//
	// Register a manual I/O queue for handling Interrupt Message Read Requests.
	// This queue will be used for storing Requests that need to wait for an
	// interrupt to occur before they can be completed.
	//
	WDF_IO_QUEUE_CONFIG_INIT(&queueConfig, WdfIoQueueDispatchManual);

	//
	// This queue is used for requests that dont directly access the device. The
	// requests in this queue are serviced only when the device is in a fully
	// powered state and sends an interrupt. So we can use a non-power managed
	// queue to park the requests since we dont care whether the device is idle
	// or fully powered up.
	//
	queueConfig.PowerManaged = WdfFalse;

	status = WdfIoQueueCreate(hDevice,
		&queueConfig,
		WDF_NO_OBJECT_ATTRIBUTES,
		&devContext->InterruptMsgQueue
		);

	if (!NT_SUCCESS(status)) {
		return status;
	}

	//
	// Create a lock to protect all the read-related buffer lists
	//

	WDF_OBJECT_ATTRIBUTES_INIT(&LockAttributes);
	attributes.ParentObject = devContext->FxDevice;
	status = WdfSpinLockCreate(&attributes, &devContext->OnLock);
	if (!NT_SUCCESS(status)){
		KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "line:%d WdfSpinLockCreate fail£¡\n", __LINE__));
		return status;
	}

	return status;
}


VOID
HidCtpEvtDriverContextCleanup(
	IN WDFOBJECT Object
	)
/*++
Routine Description:

	Free resources allocated in DriverEntry that are not automatically
	cleaned up framework.

Arguments:

	Driver - handle to a WDF Driver object.

Return Value:

	VOID.

--*/
{
	PAGED_CODE();
	UNREFERENCED_PARAMETER(Object);
}

