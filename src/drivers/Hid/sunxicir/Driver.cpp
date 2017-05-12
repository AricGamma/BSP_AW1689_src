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
#include "Trace.h"
#include "Device.h"
#include "Driver.tmh"

#ifdef ALLOC_PRAGMA
#pragma alloc_text (INIT, DriverEntry)
#pragma alloc_text (PAGE, SunxicirEvtDriverDeviceAdd)
#endif

unsigned long g_DebugLevel = DEBUG_LEVEL_ERROR;

NTSTATUS DriverEntry(_In_ _DRIVER_OBJECT* DriverObject, _In_ PUNICODE_STRING RegistryPath)
{
	NTSTATUS status;
	WDF_DRIVER_CONFIG config;

	WPP_INIT_TRACING(DriverObject, RegistryPath);
	FunctionEnter();

	WDF_DRIVER_CONFIG_INIT(&config, SunxicirEvtDriverDeviceAdd);

	status = WdfDriverCreate(DriverObject, RegistryPath, WDF_NO_OBJECT_ATTRIBUTES, &config, WDF_NO_HANDLE);
	if (!NT_SUCCESS(status))
	{
		DebugPrint(DEBUG_LEVEL_ERROR, "Error: WdfDriverCreate failed with error 0x%x\n", status);
		goto Exit;
	}

Exit:

	return status;
}

NTSTATUS SunxicirEvtDriverDeviceAdd(_In_ WDFDRIVER Driver, _Inout_ PWDFDEVICE_INIT DeviceInit)
{
	NTSTATUS status = STATUS_SUCCESS;

	UNREFERENCED_PARAMETER(Driver);
	FunctionEnter();

	status = CirDeviceCreate(DeviceInit);

	return status;
}