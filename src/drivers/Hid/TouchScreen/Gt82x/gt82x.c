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
#include <ntddk.h>
#include <wdm.h>
#include <wdf.h>
#include <ntstrsafe.h>
#include <gpio.h>

#include "spb.h"

#define RESHUB_USE_HELPER_ROUTINES
#include "reshub.h"


#if defined(EVENT_TRACING)
#include "gt82x.tmh"
#endif

#define DELAY_ONE_MICROSECOND   (-10)  
#define DELAY_ONE_MILLISECOND	(DELAY_ONE_MICROSECOND * 1000)

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, HidCtpEvtDevicePrepareHardware)
#pragma alloc_text(PAGE, HidCtpEvtDeviceD0Exit)
#endif

NTSTATUS
HidCtpEvtDevicePrepareHardware	(
	IN WDFDEVICE    Device,
	IN WDFCMRESLIST ResourceList,
	IN WDFCMRESLIST ResourceListTranslated
)
/*++

Routine Description:

	In this callback, the driver does whatever is necessary to make the
	hardware ready to use.  In the case of a USB device, this involves
	reading and selecting descriptors.

Arguments:

	Device - handle to a device

	ResourceList - A handle to a framework resource-list object that
		identifies the raw hardware resourcest

	ResourceListTranslated - A handle to a framework resource-list object
		that identifies the translated hardware resources

Return Value:

	NT status value

--*/
{

	ULONG interruptIndex = 0;
	NTSTATUS status = STATUS_SUCCESS;
	BOOLEAN fSpbResourceFound = FALSE;
	BOOLEAN fInterruptResourceFound = FALSE;
	PDEVICE_EXTENSION pDevice = GetDeviceContext(Device);

	PAGED_CODE();

	TraceEvents(TRACE_LEVEL_ERROR, DBG_CTP,
		"HidCtpEvtDevicePrepareHardware");

	//
	// Parse the peripheral's resources.
	//

	ULONG resourceCount = WdfCmResourceListGetCount(ResourceListTranslated);

	for (ULONG i = 0; i < resourceCount; i++) {

		UCHAR Class;
		UCHAR Type;
		PCM_PARTIAL_RESOURCE_DESCRIPTOR pDescriptor;

		pDescriptor = WdfCmResourceListGetDescriptor(ResourceListTranslated, i);

		switch (pDescriptor->Type) {
		case CmResourceTypeConnection:

			//
			// Look for I2C or SPI resource and save connection ID.
			//

			Class = pDescriptor->u.Connection.Class;
			Type = pDescriptor->u.Connection.Type;

			if ((Class == CM_RESOURCE_CONNECTION_CLASS_SERIAL) &&
				((Type == CM_RESOURCE_CONNECTION_TYPE_SERIAL_I2C) ||
				(Type == CM_RESOURCE_CONNECTION_TYPE_SERIAL_SPI))) {
				if (fSpbResourceFound == FALSE) {
					pDevice->PeripheralId.LowPart = pDescriptor->u.Connection.IdLowPart;
					pDevice->PeripheralId.HighPart = pDescriptor->u.Connection.IdHighPart;
					fSpbResourceFound = TRUE;
				}
				else {
					TraceEvents(TRACE_LEVEL_ERROR, DBG_CTP,
						"******not find i2c sources: line:%d******\n", __LINE__);
				}
			}
			if (Class == CM_RESOURCE_CONNECTION_CLASS_GPIO) {
				// Check for GPIO pin resource.
				pDevice->ConnectionIds.LowPart = pDescriptor->u.Connection.IdLowPart;
				pDevice->ConnectionIds.HighPart = pDescriptor->u.Connection.IdHighPart;
			}
			else {
				TraceEvents(TRACE_LEVEL_ERROR, DBG_CTP,
					"******not find gpio sources: line:%d******\n", __LINE__);
			}

			break;

		case CmResourceTypeInterrupt:

			if (fInterruptResourceFound == FALSE) {
				fInterruptResourceFound = TRUE;
				interruptIndex = i;
			}
			else {
				TraceEvents(TRACE_LEVEL_ERROR, DBG_CTP,
					"******not find int sources: line:%d******\n", __LINE__);
			}
			break;

		default:

			//
			// Ignoring all other resource types.
			//
			break;
		}
	}

	//
	// An SPB resource is required.
	//

	if (fSpbResourceFound == FALSE)
	{
		status = STATUS_NOT_FOUND;
		TraceEvents(TRACE_LEVEL_ERROR, DBG_CTP,
			"******fail: line:%d******\n", __LINE__);
	}

	//
	// Create the interrupt if an interrupt
	// resource was found.
	//

	if (NT_SUCCESS(status))
	{
		if (fInterruptResourceFound == TRUE)
		{
			WDF_INTERRUPT_CONFIG interruptConfig;
			WDF_INTERRUPT_CONFIG_INIT(
				&interruptConfig,
				OnInterruptIsr,
				OnInterruptDpc);

			interruptConfig.PassiveHandling = TRUE;
			interruptConfig.InterruptTranslated = WdfCmResourceListGetDescriptor(
				ResourceListTranslated,
				interruptIndex);
			interruptConfig.InterruptRaw = WdfCmResourceListGetDescriptor(
				ResourceList,
				interruptIndex);

			status = WdfInterruptCreate(
				pDevice->FxDevice,
				&interruptConfig,
				WDF_NO_OBJECT_ATTRIBUTES,
				&pDevice->Interrupt);

			if (!NT_SUCCESS(status)) {
				TraceEvents(TRACE_LEVEL_ERROR, DBG_CTP,
					"****** create int fail: line:%d******\n", __LINE__);
			}
		}
	}

	return status;
}

NTSTATUS
SpbRequestWrite (
	_In_ PDEVICE_EXTENSION pDevice,
	_In_ PUCHAR	InBuffer,
	_In_ UCHAR	InBufferlength
)
/*++

Routine Description:

	The write operation

Arguments:
	pDevice - handle to the device extension.

	InBuffer - Pointer to the write data

	InBufferlength - The length of the data

Return Value:

	NT status value

--*/

{
	NTSTATUS Status;
	WDFMEMORY Memory;
	WDFREQUEST IoctlRequest;
	WDF_OBJECT_ATTRIBUTES Attributes;
	WDF_REQUEST_SEND_OPTIONS SendOptions;
	WDF_OBJECT_ATTRIBUTES RequestAttributes;


	IoctlRequest = NULL;
	Memory = WDF_NO_HANDLE;

	WDF_OBJECT_ATTRIBUTES_INIT(&RequestAttributes);
	Status = WdfRequestCreate(&RequestAttributes, pDevice->SpbController, &IoctlRequest);
	if (!NT_SUCCESS(Status)) {
		TraceEvents(TRACE_LEVEL_ERROR, DBG_CTP,
			"request fail *****line:%d*****\n", __LINE__);
		goto Done;
	}

	if (InBuffer == NULL) {
		TraceEvents(TRACE_LEVEL_ERROR, DBG_CTP,
			"%d: InBuffer is null!\n", __LINE__);
		return STATUS_UNSUCCESSFUL;
	}

	NT_ASSERT(Memory == WDF_NO_HANDLE);

	WDF_OBJECT_ATTRIBUTES_INIT(&Attributes);
	Attributes.ParentObject = IoctlRequest;
	Status = WdfMemoryCreatePreallocated(
		&Attributes,
		InBuffer,
		InBufferlength,
		&Memory);

	if (!NT_SUCCESS(Status)) {
		TraceEvents(TRACE_LEVEL_ERROR, DBG_CTP,
			"WdfMemoryCreatePreallocated fail *****line:%d*****\n", __LINE__);
		goto Done;
	}

	Status = WdfIoTargetFormatRequestForWrite(
		pDevice->SpbController,
		IoctlRequest,
		Memory,
		NULL,
		NULL
		);

	if (!NT_SUCCESS(Status)) {
		TraceEvents(TRACE_LEVEL_ERROR, DBG_CTP,
			"*****line:%d*****\n", __LINE__);
		return Status;
	}

	WDF_REQUEST_SEND_OPTIONS_INIT(&SendOptions,
		WDF_REQUEST_SEND_OPTION_SYNCHRONOUS);

	WDF_REQUEST_SEND_OPTIONS_SET_TIMEOUT(&SendOptions,
		WDF_REL_TIMEOUT_IN_SEC(60));

	Status = WdfRequestAllocateTimer(IoctlRequest);
	if (!NT_SUCCESS(Status)) {
		TraceEvents(TRACE_LEVEL_ERROR, DBG_CTP,
			"*****line:%d*****\n", __LINE__);
		goto Done;
	}

	if (!WdfRequestSend(IoctlRequest, pDevice->SpbController, &SendOptions)) {
		Status = WdfRequestGetStatus(IoctlRequest);
	}

	if (!NT_SUCCESS(Status)) {
		TraceEvents(TRACE_LEVEL_ERROR, DBG_CTP,
			"*****line:%d status£º0x%x*****\n", __LINE__, Status);
	}

Done:
	if (IoctlRequest != NULL) {
		WdfObjectDelete(IoctlRequest);
	}
	return Status;
}


NTSTATUS
SpbRequestRead (
	_In_ PDEVICE_EXTENSION  pDevice,
	_Out_ PUCHAR OutBuffer,
	_In_ UCHAR	OutBufferlength
)
/*++

Routine Description:
	The Read operation

Arguments:
	pDevice - handle to the device extension.
	OutBuffer - Pointer to the Read data
	OutBufferlength - The length of the data

Return Value:
	NT status value

--*/
{
	NTSTATUS Status;
	WDFMEMORY Memory;
	WDFREQUEST IoctlRequest;
	WDF_OBJECT_ATTRIBUTES Attributes;
	WDF_REQUEST_SEND_OPTIONS SendOptions;
	WDF_OBJECT_ATTRIBUTES RequestAttributes;

	IoctlRequest = NULL;
	Memory = WDF_NO_HANDLE;

	WDF_OBJECT_ATTRIBUTES_INIT(&RequestAttributes);
	Status = WdfRequestCreate(&RequestAttributes, pDevice->SpbController, &IoctlRequest);
	if (!NT_SUCCESS(Status)) {
		TraceEvents(TRACE_LEVEL_ERROR, DBG_CTP,
			"request fail *****line:%d*****\n", __LINE__);
		goto Done;
	}

	if (OutBuffer == NULL) {
		TraceEvents(TRACE_LEVEL_ERROR, DBG_CTP,
			"%d: OutBuffer is null!\n", __LINE__);
		return STATUS_UNSUCCESSFUL;
	}

	//
	// Create preallocated WDFMEMORY. The IOCTL is METHOD_BUFFERED,
	// so the memory doesn't have to persist until the request is
	// completed.
	//

	NT_ASSERT(Memory == WDF_NO_HANDLE);

	WDF_OBJECT_ATTRIBUTES_INIT(&Attributes);
	Attributes.ParentObject = IoctlRequest;
	Status = WdfMemoryCreatePreallocated(
		&Attributes,
		OutBuffer,
		OutBufferlength,
		&Memory);

	if (!NT_SUCCESS(Status))
	{
		TraceEvents(TRACE_LEVEL_ERROR, DBG_CTP,
			"WdfMemoryCreatePreallocated fail *****line:%d*****\n", __LINE__);
		goto Done;
	}

	Status = WdfIoTargetFormatRequestForRead(
		pDevice->SpbController,
		IoctlRequest,
		Memory,
		NULL,
		NULL
		);

	if (!NT_SUCCESS(Status)) {
		TraceEvents(TRACE_LEVEL_ERROR, DBG_CTP,
			"forIoctl error *****line:%d*****\n", __LINE__);
		goto Done;
	}

	WDF_REQUEST_SEND_OPTIONS_INIT(&SendOptions,
		WDF_REQUEST_SEND_OPTION_SYNCHRONOUS);

	WDF_REQUEST_SEND_OPTIONS_SET_TIMEOUT(&SendOptions,
		WDF_REL_TIMEOUT_IN_SEC(60));

	Status = WdfRequestAllocateTimer(IoctlRequest);
	if (!NT_SUCCESS(Status)) {
		TraceEvents(TRACE_LEVEL_ERROR, DBG_CTP,
			"*****line:%d*****\n", __LINE__);
		goto Done;
	}

	if (!WdfRequestSend(IoctlRequest, pDevice->SpbController, &SendOptions)) {
		Status = WdfRequestGetStatus(IoctlRequest);
	}

	if (!NT_SUCCESS(Status)) {
		TraceEvents(TRACE_LEVEL_ERROR, DBG_CTP,
			"*****line:%d Status:0x%x*****\n", __LINE__, Status);
	}

Done:
	if (IoctlRequest != NULL) {
		WdfObjectDelete(IoctlRequest);
	}
	return Status;
}


NTSTATUS
SpbRequestWriteRead (
	_In_ PDEVICE_EXTENSION pDevice,
	_In_ PUCHAR	InBuffer,
	_In_ UCHAR	InBufferlength,
	_Out_ PUCHAR OutBuffer,
	_In_ UCHAR	OutBufferlength
)
/*++

Routine Description:
	The Write&&Read operation

Arguments:
	pDevice - handle to the device extension.
	InBuffer - Pointer to the write data
	InBufferlength - The length of the data
	OutBuffer - Pointer to the Read data
	OutBufferlength - The length of the data

Return Value:
	NT status value

--*/
{
	NTSTATUS Status;
	WDFMEMORY Memory;
	WDFREQUEST IoctlRequest;
	WDF_OBJECT_ATTRIBUTES Attributes;
	WDF_REQUEST_SEND_OPTIONS SendOptions;
	WDF_OBJECT_ATTRIBUTES RequestAttributes;

	IoctlRequest = NULL;
	Memory = WDF_NO_HANDLE;

	WDF_OBJECT_ATTRIBUTES_INIT(&RequestAttributes);
	Status = WdfRequestCreate(&RequestAttributes, pDevice->SpbController, &IoctlRequest);
	if (!NT_SUCCESS(Status)) {
		TraceEvents(TRACE_LEVEL_ERROR, DBG_CTP,
			"request fail *****line:%d status:0x%x*****\n", __LINE__, Status);
		goto Done;
	}


	if ((InBuffer == NULL) || (OutBuffer == NULL)) {
		TraceEvents(TRACE_LEVEL_ERROR, DBG_CTP,
			"%d: InBuffer or OutBuffer is null!\n", __LINE__);
		return STATUS_UNSUCCESSFUL;
	}

	//
	// Build SPB sequence.
	//

	const ULONG transfers = 2;

	SPB_TRANSFER_LIST_AND_ENTRIES(2) seq;
	SPB_TRANSFER_LIST_INIT(&(seq.List), transfers);

	{
		//
		// PreFAST cannot figure out the SPB_TRANSFER_LIST_ENTRY
		// "struct hack" size but using an index variable quiets 
		// the warning. This is a false positive from OACR.
		// 

		ULONG index = 0;
		seq.List.Transfers[index] = SPB_TRANSFER_LIST_ENTRY_INIT_SIMPLE(
			SpbTransferDirectionToDevice,
			0,
			InBuffer,
			InBufferlength);

		seq.List.Transfers[index + 1] = SPB_TRANSFER_LIST_ENTRY_INIT_SIMPLE(
			SpbTransferDirectionFromDevice,
			0,
			OutBuffer,
			OutBufferlength);
	}

	//
	// Create preallocated WDFMEMORY. The IOCTL is METHOD_BUFFERED,
	// so the memory doesn't have to persist until the request is
	// completed.
	//

	NT_ASSERT(Memory == WDF_NO_HANDLE);

	WDF_OBJECT_ATTRIBUTES_INIT(&Attributes);
	Attributes.ParentObject = IoctlRequest;
	Status = WdfMemoryCreatePreallocated(
		&Attributes,
		(PVOID) &seq,
		sizeof(seq),
		&Memory);

	if (!NT_SUCCESS(Status))
	{
		TraceEvents(TRACE_LEVEL_ERROR, DBG_CTP,
			"WdfMemoryCreatePreallocated fail *****line:%d Status:0x%x*****\n", __LINE__, Status);
		goto Done;
	}

	//
	// Format and send the SPB sequence request.
	//

	Status = WdfIoTargetFormatRequestForIoctl(
		pDevice->SpbController,
		IoctlRequest,
		IOCTL_SPB_EXECUTE_SEQUENCE,
		Memory,
		NULL,
		NULL,
		NULL);

	if (!NT_SUCCESS(Status))
	{
		TraceEvents(TRACE_LEVEL_ERROR, DBG_CTP,
			"forIoctl error *****line:%d Status:0x%x*****\n", __LINE__, Status);
		goto Done;
	}

	WDF_REQUEST_SEND_OPTIONS_INIT(&SendOptions,
		WDF_REQUEST_SEND_OPTION_SYNCHRONOUS);

	WDF_REQUEST_SEND_OPTIONS_SET_TIMEOUT(&SendOptions,
		WDF_REL_TIMEOUT_IN_SEC(60));

	Status = WdfRequestAllocateTimer(IoctlRequest);
	if (!NT_SUCCESS(Status)) {
		TraceEvents(TRACE_LEVEL_ERROR, DBG_CTP,
			"*****line:%d Status:0x%x*****\n", __LINE__, Status);
		goto Done;
	}

	if (!WdfRequestSend(IoctlRequest, pDevice->SpbController, &SendOptions)) {
		Status = WdfRequestGetStatus(IoctlRequest);
	}

	if (!NT_SUCCESS(Status)) {
		TraceEvents(TRACE_LEVEL_ERROR, DBG_CTP,
			"*****line:%d Status£º0x%x*****\n", __LINE__, Status);
	}

Done:
	if (IoctlRequest != NULL) {
		WdfObjectDelete(IoctlRequest);
	}
	return Status;
}

NTSTATUS
EndCmd(
	_In_ PDEVICE_EXTENSION pDevice
)
/*++

Routine Description:
	Ctp equipment suffix signal, write 0x8000 said data has been written to or read

Arguments:
	pDevice - handle to the device extension.

Return Value:
	NT status value

--*/
{
	NTSTATUS Status;
	UCHAR InPutBufferEnd[] = { 0x80, 0x00 };
	Status = SpbRequestWrite(pDevice, InPutBufferEnd, 2);
	return Status;

}

NTSTATUS
CtpInit(
	_In_ PDEVICE_EXTENSION pDevice
)
/*++

Routine Description:
	Write device initialization data

Arguments:
	pDevice - handle to the device extension.

Return Value:
	NT status value

--*/
{
	UCHAR InputBuffer[114] = { 0x0F, 0x80,
		0x02, 0x11, 0x03, 0x12, 0x04, 0x13, 0x05, 0x14,
		0x06, 0x15, 0x07, 0x16, 0x08, 0x17, 0x09, 0x18,
		0x0A, 0x19, 0x0B, 0x1A, 0xFF, 0x15, 0x16, 0x17,
		0x18, 0x19, 0x1A, 0x1B, 0x1C, 0x1D, 0x03, 0x0D,
		0x04, 0x0E, 0x05, 0x0F, 0x06, 0x10, 0x07, 0x11,
		0x08, 0x12, 0xFF, 0x0D, 0xFF, 0x0F, 0x10, 0x11,
		0x12, 0x13, 0x0F, 0x03, 0x10, 0x88, 0x88, 0x20,
		0x00, 0x00, 0x06, 0x00, 0x00, 0x02, 0x50, 0x3C,
		0x35, 0x03, 0x00, 0x05, 0x00, 0x03, 0x20, 0x05,
		0x00, 0x5A, 0x5A, 0x46, 0x46, 0x08, 0x00, 0x03,
		0x19, 0x05, 0x14, 0x10, 0x00, 0x07, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01 };

	//UCHAR OutputBuffer[2] = { 0 };
	//UCHAR OutputBuffer1[112] = { 0 };
	NTSTATUS Status;


	AWPRINT("CtpInit");


	Status = SpbRequestWrite(pDevice, InputBuffer, 114);
	EndCmd(pDevice);

	Wait(100);

	//OutputBuffer[0] = 0x0f;
	//OutputBuffer[1] = 0x80;
	//SpbRequestWriteRead(pDevice, OutputBuffer, 2, OutputBuffer1, 112);

	//for (UCHAR i = 0; i < 112; i++){
	//	KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "*****OutputBuffer1[%d]:0x%2x*****\n", i, OutputBuffer1[i]));
	//}

	return Status;

}


VOID
ReadChipId(
	_In_ PDEVICE_EXTENSION pDevice
)
/*++

Routine Description:
	Read the device chip id value

Arguments:
	pDevice - handle to the device extension.

--*/
{
	UCHAR InputBuffer[2] = { 0x0f, 0x7d };
	UCHAR OutputBuffer[3] = { 0 };

	SpbRequestWriteRead(pDevice, InputBuffer, 2, OutputBuffer, 1);
	KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "******\nCHIP ID : 0x%x \n******", OutputBuffer[0]));
	return;

}

NTSTATUS
SetGpioLevel(
	_In_ WDFDEVICE Device,
	_In_ PCUNICODE_STRING RequestString,
	_In_ BOOLEAN ReadOperation,
	_Inout_ PUCHAR Data,
	_In_ _In_range_(>, 0) ULONG Size,
	_Out_ WDFIOTARGET *IoTargetOut
)

/*++

Routine Description:

	This is a utility routine to read or write on a GPIO pins level.

Arguments:

	Device - Supplies a handle to the framework device object.

	RequestString - Supplies a pointer to the unicode string to be opened.

	ReadOperation - Supplies a boolean that identifies whether read (TRUE) or
		write (FALSE) should be performed.

	Data - Supplies a pointer containing the buffer that should be read from
		or written to.

	Size - Supplies the size of the data buffer in bytes.

	IoTargetOut - Supplies a pointer that receives the IOTARGET created by
		WDF.

Return Value:

	None.

--*/

{
	ULONG DesiredAccess;
	NTSTATUS Status;
	WDFREQUEST IoctlRequest;
	WDFIOTARGET IoTarget;
	WDFMEMORY WdfMemory;
	WDF_OBJECT_ATTRIBUTES Attributes;
	WDF_OBJECT_ATTRIBUTES RequestAttributes;
	WDF_REQUEST_SEND_OPTIONS SendOptions;
	WDF_OBJECT_ATTRIBUTES ObjectAttributes;
	WDF_IO_TARGET_OPEN_PARAMS OpenParams;

	WDF_OBJECT_ATTRIBUTES_INIT(&ObjectAttributes);
	ObjectAttributes.ParentObject = Device;

	IoctlRequest = NULL;
	IoTarget = NULL;

	if ((Data == NULL) || (Size == 0)) {
		TraceEvents(TRACE_LEVEL_ERROR, DBG_CTP,
			"*****line:%d Data or size is error!*****\n", __LINE__);
		Status = STATUS_INVALID_PARAMETER;
		goto End;
	}

	Status = WdfIoTargetCreate(Device,
		&ObjectAttributes,
		&IoTarget);

	if (!NT_SUCCESS(Status)) {
		TraceEvents(TRACE_LEVEL_ERROR, DBG_CTP,
			"*****line:%d status:0x%x *****\n", __LINE__, Status);
		goto End;
	}

	//
	//  Specify desired file access
	//

	if (ReadOperation != FALSE) {
		DesiredAccess = FILE_GENERIC_READ;

	}
	else {
		DesiredAccess = FILE_GENERIC_WRITE;
	}

	WDF_IO_TARGET_OPEN_PARAMS_INIT_OPEN_BY_NAME(&OpenParams,
		RequestString,
		DesiredAccess);

	//
	//  Open the IoTarget for I/O operation
	//

	Status = WdfIoTargetOpen(IoTarget, &OpenParams);
	if (!NT_SUCCESS(Status)) {
		TraceEvents(TRACE_LEVEL_ERROR, DBG_CTP,
			"*****line:%d Status:0x%x*****\n", __LINE__, Status);
		goto End;
	}

	WDF_OBJECT_ATTRIBUTES_INIT(&RequestAttributes);
	Status = WdfRequestCreate(&RequestAttributes, IoTarget, &IoctlRequest);
	if (!NT_SUCCESS(Status)) {
		TraceEvents(TRACE_LEVEL_ERROR, DBG_CTP,
			"*****line:%d Status:0x%x*****\n", __LINE__, Status);
		goto End;
	}

	//
	// Set up a WDF memory object for the IOCTL request
	//

	WDF_OBJECT_ATTRIBUTES_INIT(&Attributes);
	Attributes.ParentObject = IoctlRequest;
	Status = WdfMemoryCreatePreallocated(&Attributes, Data, Size, &WdfMemory);
	if (!NT_SUCCESS(Status)) {
		TraceEvents(TRACE_LEVEL_ERROR, DBG_CTP,
			"*****line:%d Status:0x%x*****\n", __LINE__, Status);
		goto End;
	}

	//
	// Format the request as read or write operation
	//

	if (ReadOperation != FALSE) {
		Status = WdfIoTargetFormatRequestForIoctl(IoTarget,
			IoctlRequest,
			IOCTL_GPIO_READ_PINS,
			NULL,
			0,
			WdfMemory,
			0);

	}
	else {
		Status = WdfIoTargetFormatRequestForIoctl(IoTarget,
			IoctlRequest,
			IOCTL_GPIO_WRITE_PINS,
			WdfMemory,
			0,
			WdfMemory,
			0);
	}

	if (!NT_SUCCESS(Status)) {
		TraceEvents(TRACE_LEVEL_ERROR, DBG_CTP,
			"*****line:%d Status:0x%x *****\n", __LINE__, Status);
		goto End;
	}

	//
	// Send the request synchronously with an arbitrary timeout of 60 seconds
	//

	WDF_REQUEST_SEND_OPTIONS_INIT(&SendOptions,
		WDF_REQUEST_SEND_OPTION_SYNCHRONOUS);

	WDF_REQUEST_SEND_OPTIONS_SET_TIMEOUT(&SendOptions,
		WDF_REL_TIMEOUT_IN_SEC(60));

	Status = WdfRequestAllocateTimer(IoctlRequest);
	if (!NT_SUCCESS(Status)) {
		TraceEvents(TRACE_LEVEL_ERROR, DBG_CTP,
			"*****line:%d Status:0x%x*****\n", __LINE__, Status);
		goto End;
	}

	if (!WdfRequestSend(IoctlRequest, IoTarget, &SendOptions)) {
		Status = WdfRequestGetStatus(IoctlRequest);
	}

	if (NT_SUCCESS(Status)) {
		*IoTargetOut = IoTarget;
	}

End:
	if (IoctlRequest != NULL) {
		WdfObjectDelete(IoctlRequest);
	}

	if (!NT_SUCCESS(Status) && (IoTarget != NULL)) {
		WdfIoTargetClose(IoTarget);
		WdfObjectDelete(IoTarget);
	}

	return Status;
}



VOID Wait(
	LONG msec
	)
/*++
Routine Description:
	Set the delay time of millisecond

Arguments:
	msec - Delay time

*/
{
	LARGE_INTEGER my_interval;
	my_interval.QuadPart = DELAY_ONE_MILLISECOND;
	my_interval.QuadPart *= msec;

	KeDelayExecutionThread(KernelMode, 0, &my_interval);

}



VOID
SetWakeupHigh(
	_In_ PDEVICE_EXTENSION pDevice,
	_In_ LONG msec
)
/*++

Routine Description:
	Set the Wakeup pin

Arguments:
	pDevice - handle to the device extension.
	msec - Wakeup pin sets the low level of the time

--*/

{
	UNICODE_STRING WriteString;
	WCHAR WriteStringBuffer[100];
	WDFIOTARGET WriteTarget;
	NTSTATUS Status;
	BYTE Data;

	WriteTarget = NULL;

	RtlInitEmptyUnicodeString(&WriteString,
		WriteStringBuffer,
		sizeof(WriteStringBuffer));

	Status = RESOURCE_HUB_CREATE_PATH_FROM_ID(&WriteString,
		pDevice->ConnectionIds.LowPart,
		pDevice->ConnectionIds.HighPart);

	if (!NT_SUCCESS(Status)) {
		goto Cleanup;
	}

	Data = 1;
	Status = SetGpioLevel(pDevice->FxDevice, &WriteString, FALSE, &Data, sizeof(Data), &WriteTarget);
	if (!NT_SUCCESS(Status)) {
		goto Cleanup;
	}

	if (WriteTarget != NULL) {
		WdfIoTargetClose(WriteTarget);
		WdfObjectDelete(WriteTarget);
	}
	WriteTarget = NULL;
	Wait(10);

	Data = 0;
	Status = SetGpioLevel(pDevice->FxDevice, &WriteString, FALSE, &Data, sizeof(Data), &WriteTarget);
	if (!NT_SUCCESS(Status)) {
		goto Cleanup;
	}
	Wait(msec);

	if (WriteTarget != NULL) {
		WdfIoTargetClose(WriteTarget);
		WdfObjectDelete(WriteTarget);
	}
	WriteTarget = NULL;

	Data = 1;
	Status = SetGpioLevel(pDevice->FxDevice, &WriteString, FALSE, &Data, sizeof(Data), &WriteTarget);
	if (!NT_SUCCESS(Status)) {
		goto Cleanup;
	}

	Wait(10);
Cleanup:

	if (WriteTarget != NULL) {
		WdfIoTargetClose(WriteTarget);
		WdfObjectDelete(WriteTarget);
	}

}

VOID GoodixTsVersion(
	_In_ PDEVICE_EXTENSION pDevice
)
/*++

Routine Description:
	Read the device firmware version

Arguments:
	pDevice - handle to the device extension.

--*/
{
	UCHAR buf[8];
	UCHAR buf1[8] = { 0 };
	buf[0] = 0x0f;
	buf[1] = 0x7d;

	SpbRequestWriteRead(pDevice, buf, 2, buf1, 3);
	EndCmd(pDevice);
	KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "PID:%02x, VID:%02x%02x\n", buf1[0], buf1[1], buf1[2]));
	return;
}


NTSTATUS
SpbPeripheralOpen(
	_In_  PDEVICE_EXTENSION pDevice
)
/*++

Routine Description:

	This routine opens a handle to the SPB controller.

Arguments:

	pDevice - a pointer to the device context

Return Value:

	Status

--*/
{
	WDF_IO_TARGET_OPEN_PARAMS  openParams;
	NTSTATUS status;

	//
	// Create the device path using the connection ID.
	//

	DECLARE_UNICODE_STRING_SIZE(DevicePath, RESOURCE_HUB_PATH_SIZE);

	RESOURCE_HUB_CREATE_PATH_FROM_ID(
		&DevicePath,
		pDevice->PeripheralId.LowPart,
		pDevice->PeripheralId.HighPart);

	//
	// Open a handle to the SPB controller.
	//

	WDF_IO_TARGET_OPEN_PARAMS_INIT_OPEN_BY_NAME(
		&openParams,
		&DevicePath,
		(GENERIC_READ | GENERIC_WRITE));

	openParams.ShareAccess = 0;
	openParams.CreateDisposition = FILE_OPEN;
	openParams.FileAttributes = FILE_ATTRIBUTE_NORMAL;

	status = WdfIoTargetOpen(
		pDevice->SpbController,
		&openParams);

	if (!NT_SUCCESS(status))
	{
		KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "*****open fail! line:%d*****\n", __LINE__));
	}

	return status;
}

NTSTATUS
SpbPeripheralClose (
	_In_  PDEVICE_EXTENSION  pDevice
)
/*++

Routine Description:

	This routine closes a handle to the SPB controller.

Arguments:

	pDevice - a pointer to the device context

Return Value:

	Status

--*/
{
	KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "*****close handle to SPB target:%d*****\n", __LINE__));

	WdfIoTargetClose(pDevice->SpbController);

	return STATUS_SUCCESS;
}


NTSTATUS
HidCtpEvtDeviceD0Entry(
	IN  WDFDEVICE Device,
	IN  WDF_POWER_DEVICE_STATE PreviousState
)
/*++

Routine Description:

	EvtDeviceD0Entry event callback must perform any operations that are
	necessary before the specified device is used.  It will be called every
	time the hardware needs to be (re-)initialized.

	This function is not marked pageable because this function is in the
	device power up path. When a function is marked pagable and the code
	section is paged out, it will generate a page fault which could impact
	the fast resume behavior because the client driver will have to wait
	until the system drivers can service this page fault.

	This function runs at PASSIVE_LEVEL, even though it is not paged.  A
	driver can optionally make this function pageable if DO_POWER_PAGABLE
	is set.  Even if DO_POWER_PAGABLE isn't set, this function still runs
	at PASSIVE_LEVEL.  In this case, though, the function absolutely must
	not do anything that will cause a page fault.

Arguments:

	Device - Handle to a framework device object.

	PreviousState - Device power state which the device was in most recently.
		If the device is being newly started, this will be
		PowerDeviceUnspecified.

Return Value:

	NTSTATUS

--*/
{


	UNREFERENCED_PARAMETER(PreviousState);

	PDEVICE_EXTENSION pDevice = GetDeviceContext(Device);
	NTSTATUS status;

	//
	// Create the SPB target.
	//

	WDF_OBJECT_ATTRIBUTES targetAttributes;
	WDF_OBJECT_ATTRIBUTES_INIT(&targetAttributes);

	status = WdfIoTargetCreate(
		pDevice->FxDevice,
		&targetAttributes,
		&pDevice->SpbController);

	if (!NT_SUCCESS(status)) {
		TraceEvents(TRACE_LEVEL_VERBOSE, DBG_CTP,
			"******WdfIOTargetCreate fail: line:%d Status:0x%x******\n", __LINE__, status);

	}

	pDevice->TouchNum = 0;
	pDevice->Report = 0;
	pDevice->DataReady = FALSE;
	pDevice->ReportTime = 0;
	RtlZeroMemory(pDevice->Data, sizeof(pDevice->Data));

	SetWakeupHigh(pDevice, 20); //Wakeup pin down up after 20 ms
	SpbPeripheralOpen(pDevice); //Open the spb equipment
	Wait(100);
	ReadChipId(pDevice);//Read the chip id
	CtpInit(pDevice); //write Initialize the data
	GoodixTsVersion(pDevice);//Reading device version

	return status;
}


NTSTATUS
HidCtpEvtDeviceD0Exit(
	IN  WDFDEVICE Device,
	IN  WDF_POWER_DEVICE_STATE TargetState
)
/*++

Routine Description:

	This routine undoes anything done in EvtDeviceD0Entry.  It is called
	whenever the device leaves the D0 state, which happens when the device is
	stopped, when it is removed, and when it is powered off.

	The device is still in D0 when this callback is invoked, which means that
	the driver can still touch hardware in this routine.


	EvtDeviceD0Exit event callback must perform any operations that are
	necessary before the specified device is moved out of the D0 state.  If the
	driver needs to save hardware state before the device is powered down, then
	that should be done here.

	This function runs at PASSIVE_LEVEL, though it is generally not paged.  A
	driver can optionally make this function pageable if DO_POWER_PAGABLE is set.

	Even if DO_POWER_PAGABLE isn't set, this function still runs at
	PASSIVE_LEVEL.  In this case, though, the function absolutely must not do
	anything that will cause a page fault.

Arguments:

	Device - Handle to a framework device object.

	TargetState - Device power state which the device will be put in once this
		callback is complete.

Return Value:

	Success implies that the device can be used.  Failure will result in the
	device stack being torn down.

--*/
{
	PDEVICE_EXTENSION         devContext;

	PAGED_CODE();

	TraceEvents(TRACE_LEVEL_VERBOSE, DBG_PNP,
		"HidCtpEvtDeviceD0Exit Enter- moving to %s\n",
		DbgDevicePowerString(TargetState));

	devContext = GetDeviceContext(Device);

	SpbPeripheralClose(devContext);

	TraceEvents(TRACE_LEVEL_ERROR, DBG_PNP,
		"HidCtpEvtDeviceD0Exit Exit\n");

	return STATUS_SUCCESS;
}

VOID ProcessCtpDataUp(
	_In_ PDEVICE_EXTENSION pDevice
)
	// Processing the data up
{


	PHIDFX2_INPUT_REPORT inputReport = NULL;
	ULONG				 bytesToCopy = 0;
	size_t				 bytesReturned = 0;
	WDFREQUEST           request;
	NTSTATUS status;


	status = WdfIoQueueRetrieveNextRequest(pDevice->InterruptMsgQueue, &request);
	if (NT_SUCCESS(status)) {

		bytesToCopy = sizeof(HIDFX2_INPUT_REPORT);

		if (bytesToCopy == 0) {
			TraceEvents(TRACE_LEVEL_ERROR, DBG_CTP,
				"bytesToCopy == 0");
			goto exit_bytes;
		}

		status = WdfRequestRetrieveOutputBuffer(request,
			bytesToCopy,
			&inputReport,
			&bytesReturned);// BufferLength
		RtlZeroMemory(inputReport, sizeof(inputReport));

		if (!NT_SUCCESS(status)) {
			TraceEvents(TRACE_LEVEL_ERROR, DBG_CTP,
				"line:%d WdfRequestRetrieveOutputBuffer fail\n", __LINE__);
		}
		else {

			inputReport->ReportTouchId = 0x01;
			inputReport->ScanTimeLSB = pDevice->ReportTime & 0x00ff;
			inputReport->ScanTimeMSB = (pDevice->ReportTime & 0xff00) >> 8;
			inputReport->ContactCount = 0;
		}
		bytesReturned = bytesToCopy;
		WdfRequestCompleteWithInformation(request, status, bytesReturned);
	}
	else if (status != STATUS_NO_MORE_ENTRIES) {
		TraceEvents(TRACE_LEVEL_ERROR, DBG_CTP,
			"line:%d  status : 0x%08x\n", __LINE__, status);
	}

exit_bytes:
	pDevice->ReportTime += 125;
	if (pDevice->ReportTime > 0xffff)
		pDevice->ReportTime = 0;

	return;
}


VOID ReadCTPData(
	_In_ PDEVICE_EXTENSION pDevice
	)
	// Read data of CTP
{
	UCHAR  point_data[2] = { 0x0f, 0x40 };
	UCHAR  finger = 0;
	UCHAR KeyValue = 0;
	UCHAR TouchNum = 0;
	NTSTATUS Status;

	RtlZeroMemory(pDevice->Data, sizeof(pDevice->Data));

	SpbRequestWriteRead(pDevice, point_data, 2, &pDevice->Data[0], 8);
	finger = pDevice->Data[0];
	TouchNum = (finger & 0x01) + !!(finger & 0x02) + !!(finger & 0x04) + !!(finger & 0x08) + !!(finger & 0x10);

	if (TouchNum > 1){
		UCHAR buf[2];
		buf[0] = 0x0f;
		buf[1] = 0x40 + 8;
		SpbRequestWriteRead(pDevice, buf, 2, &pDevice->Data[8], 5 * (TouchNum - 1));
	}

	EndCmd(pDevice);


	if ((finger & 0xC0) != 0x80){ // Check to see if the data is ready
		TraceEvents(TRACE_LEVEL_ERROR, DBG_CTP,
			"ReadData %d: Data not ready!", __LINE__);
		return;
	}

	KeyValue = pDevice->Data[1] & 0x0f;

	if ((KeyValue & 0x0f) == 0x0f){	//ther there is any abnormal data
		Status = CtpInit(pDevice);
		if (!NT_SUCCESS(Status)) {
			TraceEvents(TRACE_LEVEL_ERROR, DBG_PNP,
				"load init files fail!\n");
		}
		return;
	}

	//if ((TouchNum != 0) || ((pDevice->TouchNum & 0x1f) != 0))
	//	pDevice->DataReady = TRUE; // Have read correctly to the data
	//else {
	//	pDevice->DataReady = FALSE;
	//	ProcessCtpDataUp(pDevice);
	//}

	return;
}

VOID ProcessCtpDataDown(
	_In_ PDEVICE_EXTENSION pDevice
	)
	// Processing the data, and data will be reported to the system
{
	UCHAR* coor_data = NULL;
	UCHAR  check_sum = 0;
	UCHAR  touch_num = 0;
	UCHAR  pre_num = 0;
	UCHAR  finger = 0;

	PHIDFX2_INPUT_REPORT inputReport = NULL;
	ULONG				 bytesToCopy = 0;
	size_t				 bytesReturned = 0;
	WDFREQUEST           request;
	NTSTATUS status = STATUS_SUCCESS;

	long input_x = 0;
	long input_y = 0;
	long input_w = 0;
	UCHAR idx = 0;

	finger = pDevice->Data[0];
	touch_num = (finger & 0x01) + !!(finger & 0x02) + !!(finger & 0x04) + !!(finger & 0x08) + !!(finger & 0x10);
	pre_num = (pDevice->TouchNum & 0x01) + !!(pDevice->TouchNum & 0x02) + !!(pDevice->TouchNum & 0x04) +
		!!(pDevice->TouchNum & 0x08) + !!(pDevice->TouchNum & 0x10);

	if (touch_num == 0)
		return;

	coor_data = &pDevice->Data[2];
	check_sum = 0;
	for (idx = 0; idx < 5 * touch_num; idx++){
		check_sum += coor_data[idx];
	}

	if (check_sum != coor_data[5 * touch_num]){
		TraceEvents(TRACE_LEVEL_ERROR, DBG_CTP,
			"ReadData %d:Check sum error!", __LINE__);
		goto exit_work_func;
	}

	status = WdfIoQueueRetrieveNextRequest(pDevice->InterruptMsgQueue, &request);
	if (NT_SUCCESS(status)) {

		bytesToCopy = sizeof(HIDFX2_INPUT_REPORT);

		if (bytesToCopy == 0) {
			TraceEvents(TRACE_LEVEL_ERROR, DBG_CTP,
				"bytesToCopy == 0");
			goto exit_bytes;
		}

		status = WdfRequestRetrieveOutputBuffer(request,
			bytesToCopy,
			&inputReport,
			&bytesReturned);// BufferLength
		RtlZeroMemory(inputReport, sizeof(HIDFX2_INPUT_REPORT));

		if (!NT_SUCCESS(status)) {
			TraceEvents(TRACE_LEVEL_ERROR, DBG_CTP,
				"line:%d WdfRequestRetrieveOutputBuffer fail\n", __LINE__);
		}
		else {
			long  pos = 0;
			for (idx = 0; idx < 5; idx++){
				if (idx < touch_num) {
					input_x = coor_data[pos] << 8;
					input_x |= coor_data[pos + 1];

					input_x = 800 - input_x;

					input_y = coor_data[pos + 2] << 8;
					input_y |= coor_data[pos + 3];

					input_y = 1280 - input_y;

					input_w = coor_data[pos + 4];
				}

				switch (idx)
				{
				case 0:
					if (!(finger & (0x01 << idx))) {
						inputReport->Tip0 = 0x00;
						inputReport->ContactId0 = idx;
						break;
					}

					inputReport->Tip0 = 0x01;
					inputReport->ContactId0 = idx;
					inputReport->XLSB0 = input_y & 0x00ff;
					inputReport->XMSB0 = (input_y & 0xff00) >> 8;
					inputReport->YLSB0 = input_x & 0x00ff;
					inputReport->YMSB0 = (input_x & 0xff00) >> 8;

					break;
				case 1:
					if (!(finger & (0x01 << idx))) {
						inputReport->Tip1 = 0x00;
						inputReport->ContactId1 = idx;
						break;
					}

					inputReport->Tip1 = 0x01;
					inputReport->ContactId1 = idx;
					inputReport->XLSB1 = input_y & 0x00ff;
					inputReport->XMSB1 = (input_y & 0xff00) >> 8;
					inputReport->YLSB1 = input_x & 0x00ff;
					inputReport->YMSB1 = (input_x & 0xff00) >> 8;

					break;
				case 2:
					if (!(finger & (0x01 << idx))) {
						inputReport->Tip2 = 0x00;
						inputReport->ContactId2 = idx;
						break;
					}

					inputReport->Tip2 = 0x01;
					inputReport->ContactId2 = idx;
					inputReport->XLSB2 = input_y & 0x00ff;
					inputReport->XMSB2 = (input_y & 0xff00) >> 8;
					inputReport->YLSB2 = input_x & 0x00ff;
					inputReport->YMSB2 = (input_x & 0xff00) >> 8;
					break;
				case 3:
					if (!(finger & (0x01 << idx))) {
						inputReport->Tip3 = 0x00;
						inputReport->ContactId3 = idx;
						break;
					}

					inputReport->Tip3 = 0x01;
					inputReport->ContactId3 = idx;
					inputReport->XLSB3 = input_y & 0x00ff;
					inputReport->XMSB3 = (input_y & 0xff00) >> 8;
					inputReport->YLSB3 = input_x & 0x00ff;
					inputReport->YMSB3 = (input_x & 0xff00) >> 8;

					break;
				case 4:
					if (!(finger & (0x01 << idx))) {
						inputReport->Tip4 = 0x00;
						inputReport->ContactId4 = idx;
						break;
					}

					inputReport->Tip4 = 0x01;
					inputReport->ContactId4 = idx;
					inputReport->XLSB4 = input_y & 0x00ff;
					inputReport->XMSB4 = (input_y & 0xff00) >> 8;
					inputReport->YLSB4 = input_x & 0x00ff;
					inputReport->YMSB4 = (input_x & 0xff00) >> 8;
					break;
				default:
					break;
				}
				pos += 5;
			}
			inputReport->ReportTouchId = 0x01;
			inputReport->ScanTimeLSB = pDevice->ReportTime & 0x00ff;
			inputReport->ScanTimeMSB = (pDevice->ReportTime & 0xff00) >> 8;
			inputReport->ContactCount = touch_num;
		}
	}
	else if (status != STATUS_NO_MORE_ENTRIES) {
		TraceEvents(TRACE_LEVEL_ERROR, DBG_CTP,
			"line:%d  status : 0x%08x\n", __LINE__, status);
	}

exit_bytes:
	bytesReturned = bytesToCopy;
	WdfRequestCompleteWithInformation(request, status, bytesReturned);
	pDevice->TouchNum = finger;
	pDevice->ReportTime += 125;
	if (pDevice->ReportTime > 0xffff)
		pDevice->ReportTime = 0;

	pDevice->DataReady = FALSE;
exit_work_func:
	return;
}


BOOLEAN
OnInterruptIsr(
	_In_  WDFINTERRUPT FxInterrupt,
	_In_  ULONG        MessageID
)
/*++

Routine Description:

	This routine responds to interrupts generated by the H/W.
	It then waits indefinitely for the user to signal that
	the interrupt has been acknowledged, allowing the ISR to
	return. This ISR is called at PASSIVE_LEVEL.

Arguments:

	Interrupt - a handle to a framework interrupt object
	MessageID - message number identifying the device's
	hardware interrupt message (if using MSI)

Return Value:

	TRUE if interrupt recognized.

--*/
{
	BOOLEAN fInterruptRecognized = TRUE;
	WDFDEVICE device;
	PDEVICE_EXTENSION pDevice;

	UNREFERENCED_PARAMETER(MessageID);
	AWPRINT("ctp int isr....\n");
	device = WdfInterruptGetDevice(FxInterrupt);
	pDevice = GetDeviceContext(device);
	if (pDevice == NULL) {
		TraceEvents(TRACE_LEVEL_ERROR, DBG_CTP,
			"isr pDevice is Null!");
		return fInterruptRecognized;
	}

	ReadCTPData(pDevice); //Read the data

	if (((pDevice->Report & 0x07) == 0x07))
		WdfInterruptQueueDpcForIsr(FxInterrupt);

	return fInterruptRecognized;
}

VOID
OnInterruptDpc(
_In_ WDFINTERRUPT WdfInterrupt,
_In_ WDFOBJECT  AssociatedObject

)

/*++

Routine Description:

	DPC callback for ISR.

Arguments:

	WdfInterrupt - Handle to the framework interrupt object

	WdfDevice - Associated device object.

Return Value:

--*/
{
	PDEVICE_EXTENSION pDevice = NULL;
	UCHAR finger = 0;
	UCHAR touch_num = 0;

	UNREFERENCED_PARAMETER(WdfInterrupt);

	pDevice = GetDeviceContext(AssociatedObject);

	if (pDevice == NULL) {
		TraceEvents(TRACE_LEVEL_ERROR, DBG_CTP,
			"Dpc pDevice is Null!");
		return;
	}

	finger = pDevice->Data[0];
	touch_num = (finger & 0x01) + !!(finger & 0x02) + !!(finger & 0x04) + !!(finger & 0x08) + !!(finger & 0x10);


	WdfSpinLockAcquire(pDevice->OnLock);

	if (touch_num != 0)
		ProcessCtpDataDown(pDevice);
	else
		ProcessCtpDataUp(pDevice);


	WdfSpinLockRelease(pDevice->OnLock);

	pDevice->IsInterruptEnable = TRUE;

}



PCHAR
DbgDevicePowerString(
IN WDF_POWER_DEVICE_STATE Type
)
/*++

Updated Routine Description:
DbgDevicePowerString does not change in this stage of the function driver.

--*/
{
	switch (Type)
	{
	case WdfPowerDeviceInvalid:
		return "WdfPowerDeviceInvalid";
	case WdfPowerDeviceD0:
		return "WdfPowerDeviceD0";
	case WdfPowerDeviceD1:
		return "WdfPowerDeviceD1";
	case WdfPowerDeviceD2:
		return "WdfPowerDeviceD2";
	case WdfPowerDeviceD3:
		return "WdfPowerDeviceD3";
	case WdfPowerDeviceD3Final:
		return "WdfPowerDeviceD3Final";
	case WdfPowerDevicePrepareForHibernation:
		return "WdfPowerDevicePrepareForHibernation";
	case WdfPowerDeviceMaximum:
		return "WdfPowerDeviceMaximum";
	default:
		return "UnKnown Device Power State";
	}
}
