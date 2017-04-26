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

#include "Registry.h"
#include "Trace.h"
#include "Registry.tmh"

// To open the hardware Device Parameters key or software key
NTSTATUS RegKernelDeviceRootKeyOpen(_In_ WDFDEVICE pDevice, _In_ BOOLEAN IsDeviceKey, _Out_ WDFKEY *phKey);

NTSTATUS RegKernelDeviceRootKeyOpen(
	_In_ WDFDEVICE pDevice, 
	_In_ BOOLEAN IsDeviceKey, 
	_Out_ WDFKEY *phKey)
{
	ULONG KeyType = PLUGPLAY_REGKEY_DEVICE;

	if ((NULL == pDevice)
		|| (NULL == phKey))
	{
		return STATUS_INVALID_PARAMETER;
	}

	if (FALSE == IsDeviceKey)
	{
		KeyType = PLUGPLAY_REGKEY_DRIVER;
	}

	return WdfDeviceOpenRegistryKey(pDevice,
		KeyType,
		KEY_READ | KEY_WRITE,
		WDF_NO_OBJECT_ATTRIBUTES,
		phKey);
}

NTSTATUS RegQueryDeviceDwordValue(
	_In_ WDFDEVICE pDevice,
	_In_ PCWSTR pValueName,
	_Out_ ULONG *pValue)
{
	NTSTATUS Status = STATUS_SUCCESS;
	UNICODE_STRING ValueName = { 0 };
	WDFKEY hKey = NULL;

	FunctionEnter();

	if ((NULL == pDevice)
		|| (NULL == pValueName)
		|| (NULL == pValue))
	{
		DbgPrint_E("Invalid parameters.");
		Status = STATUS_INVALID_PARAMETER;
		goto Exit;
	}

	Status = RegKernelDeviceRootKeyOpen(pDevice, TRUE, &hKey);
	if (!NT_SUCCESS(Status))
	{
		DbgPrint_E("Failed to open the device parameter key with 0x%lx.", Status);
		goto Exit;
	}

	RtlInitUnicodeString(&ValueName, pValueName);

	Status = WdfRegistryQueryValue(hKey,
		&ValueName,
		sizeof(ULONG),
		pValue,
		NULL,
		NULL);
	if (!NT_SUCCESS(Status))
	{
		DbgPrint_E("Failed to query REG_DWORD value with 0x%lx.", Status);
	}

Exit:
	if (NULL != hKey)
	{
		WdfRegistryClose(hKey);
		hKey = NULL;
	}

	FunctionExit(Status);
	return Status;
}

NTSTATUS RegSetDeviceDwordValue(
	_In_ WDFDEVICE pDevice,
	_In_ PCWSTR pValueName,
	_In_ ULONG Value)
{
	NTSTATUS Status = STATUS_SUCCESS;
	UNICODE_STRING ValueName = { 0 };
	WDFKEY hKey = NULL;

	FunctionEnter();

	if (NULL == pValueName)
	{
		DbgPrint_E("Invalid parameter.");
		Status = STATUS_INVALID_PARAMETER;
		goto Exit;
	}

	Status = RegKernelDeviceRootKeyOpen(pDevice, TRUE, &hKey);
	if (!NT_SUCCESS(Status))
	{
		DbgPrint_E("Failed to open the device parameter key with 0x%lx.", Status);
		goto Exit;
	}

	RtlInitUnicodeString(&ValueName, pValueName);

	Status = WdfRegistryAssignULong(hKey,
		&ValueName,
		Value);
	if (!NT_SUCCESS(Status))
	{
		DbgPrint_E("Failed to assign REG_DWORD value with 0x%lx.", Status);
	}

Exit:
	if (NULL != hKey)
	{
		WdfRegistryClose(hKey);
		hKey = NULL;
	}

	FunctionExit(Status);
	return Status;
}