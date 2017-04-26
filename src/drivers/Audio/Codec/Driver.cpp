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

#include "Driver.h"
#include "Device.h"
#include "Trace.h"
#include "Driver.tmh"

#ifdef ALLOC_PRAGMA
#pragma alloc_text (INIT, DriverEntry)
#pragma alloc_text (PAGE, CodecEvtDeviceAdd)
#pragma alloc_text (PAGE, CodecEvtDriverContextCleanup)
#endif

//unsigned long g_DebugLevel = DEBUG_LEVEL_ERROR;
unsigned long g_DebugLevel = DEBUG_LEVEL_VERBOSE;

NTSTATUS DriverEntry(
    _In_ PDRIVER_OBJECT  pDriverObject,
    _In_ PUNICODE_STRING pRegistryPath)
/*++

Routine Description:
    DriverEntry initializes the driver and is the first routine called by the
    system after the driver is loaded. DriverEntry specifies the other entry
    points in the function driver, such as EvtDevice and DriverUnload.

Parameters Description:

    pDriverObject - represents the instance of the function driver that is loaded
    into memory. DriverEntry must initialize members of DriverObject before it
    returns to the caller. DriverObject is allocated by the system before the
    driver is loaded, and it is released by the system after the system unloads
    the function driver from memory.

    pRegistryPath - represents the driver specific path in the Registry.
    The function driver can use the path to store driver related data between
    reboots. The path does not store hardware instance specific data.

Return Value:

    STATUS_SUCCESS if successful,
    STATUS_UNSUCCESSFUL otherwise.

--*/
{
	NTSTATUS Status = STATUS_SUCCESS;
	WDF_DRIVER_CONFIG Config = { 0 };
	WDF_OBJECT_ATTRIBUTES Attributes = { 0 };

    //
    // Initialize WPP Tracing
    //
    WPP_INIT_TRACING(pDriverObject, pRegistryPath);

	FunctionEnter();

    TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DRIVER, "%!FUNC! Entry");

    //
    // Register a cleanup callback so that we can call WPP_CLEANUP when
    // the framework driver object is deleted during driver unload.
    //
    WDF_OBJECT_ATTRIBUTES_INIT(&Attributes);
	Attributes.EvtCleanupCallback = CodecEvtDriverContextCleanup;

    WDF_DRIVER_CONFIG_INIT(&Config, CodecEvtDeviceAdd);

    Status = WdfDriverCreate(pDriverObject,
		pRegistryPath,
		&Attributes,
		&Config,
		WDF_NO_HANDLE);

    if (!NT_SUCCESS(Status))
	{
		DbgPrint_E("WdfDriverCreate failed with 0x%lx.", Status);
        WPP_CLEANUP(pDriverObject);
        goto Exit;
    }

Exit:
	FunctionExit(Status);
    return Status;
}

NTSTATUS CodecEvtDeviceAdd(
    _In_    WDFDRIVER       pDriver,
    _Inout_ PWDFDEVICE_INIT pDeviceInit)
/*++
Routine Description:

    EvtDeviceAdd is called by the framework in response to AddDevice
    call from the PnP manager. We create and initialize a device object to
    represent a new instance of the device.

Arguments:

    pDriver - Handle to a framework driver object created in DriverEntry

    pDeviceInit - Pointer to a framework-allocated WDFDEVICE_INIT structure.

Return Value:

    NTSTATUS

--*/
{
	NTSTATUS Status = STATUS_SUCCESS;

    UNREFERENCED_PARAMETER(pDriver);

	FunctionEnter();

	Status = CodecCreateDevice(pDeviceInit);
	if (!NT_SUCCESS(Status)) 
	{
		DbgPrint_E("Failed to create device with 0x%lx.", Status);
	}

	FunctionExit(Status);
    return Status;
}

void CodecEvtDriverContextCleanup(
    _In_ WDFOBJECT pDriverObject)
/*++
Routine Description:

    Free all the resources allocated in DriverEntry.

Arguments:

    pDriverObject - handle to a WDF Driver object.

Return Value:

    void.

--*/
{
    UNREFERENCED_PARAMETER(pDriverObject);

	FunctionEnter();

    //
    // Stop WPP Tracing
    //
    WPP_CLEANUP(WdfDriverWdmGetDriverObject((WDFDRIVER)pDriverObject));
	FunctionExit(STATUS_SUCCESS);
}
