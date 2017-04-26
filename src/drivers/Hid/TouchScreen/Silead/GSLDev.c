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

#ifdef DEBUG_C_TMH
#include "GSLDev.tmh"
#endif
#include "Device.h"
#define RESHUB_USE_HELPER_ROUTINES
#include "reshub.h"
#include "Idle.h"

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, HidGSLEvtDevicePrepareHardware)
#pragma alloc_text(PAGE, HidGSLEvtDeviceD0Exit)
//#pragma alloc_text(PAGE, HidFx2ConfigContReaderForInterruptEndPoint)
//#pragma alloc_text(PAGE, HidFx2ValidateConfigurationDescriptor)
#endif

NTSTATUS
GSLEvtDeviceSetupI2CConnection(
IN WDFDEVICE    Device
)
{
	NTSTATUS Status;
	WDF_OBJECT_ATTRIBUTES Attributes;
	WDF_IO_TARGET_OPEN_PARAMS Parameters;
	PDEVICE_EXTENSION             devContext = NULL;
	UNICODE_STRING GSLDevicePath;
	WCHAR GSLDevicePathBuffer[100];

	devContext = GetDeviceContext(Device);

	WDF_OBJECT_ATTRIBUTES_INIT(&Attributes);
	Status = WdfIoTargetCreate(Device,
		&Attributes,
		&devContext->I2CIOTarget);
	if (!NT_SUCCESS(Status)) {
		DbgPrint("%s: WdfIoTargetCreate failed to create GSLDev_i2c IoTarget! "
			"Device = %p, Status:%#x\n",
			__FUNCTION__,
			Device,
			Status);
		if (devContext->I2CIOTarget != NULL) {
			WdfObjectDelete(devContext->I2CIOTarget);
		}
		goto SetupI2CConnectionFailed;
	}

	RtlInitEmptyUnicodeString(&GSLDevicePath,
		GSLDevicePathBuffer,
		sizeof(GSLDevicePathBuffer));
	//
	// Use the connection ID supplied to create the full device path. This
	// device path (indirectly) represents the path for the I2C controller.
	//
	Status = RESOURCE_HUB_CREATE_PATH_FROM_ID(
		&GSLDevicePath,
		devContext->ConnectionIds[I2C_RESOURCE_INDEX].LowPart,
		devContext->ConnectionIds[I2C_RESOURCE_INDEX].HighPart);
	DbgPrintGSL("ResourceHub create device path (%wZ)\n", &GSLDevicePath);
	
	if (!NT_SUCCESS(Status)) {
		DbgPrint("ResourceHub create device path (%wZ) failed Status:%#x\n",
			&GSLDevicePath,
			Status);
		goto SetupI2CConnectionFailed;
	}
	
	//
	// Initialize the parameters for the SPB IO target.
	//
	WDF_IO_TARGET_OPEN_PARAMS_INIT_OPEN_BY_NAME(&Parameters,
		&GSLDevicePath,
		GENERIC_WRITE | GENERIC_READ);

	Parameters.ShareAccess = 0;
	Parameters.CreateDisposition = FILE_OPEN;
	Parameters.FileAttributes = FILE_ATTRIBUTE_NORMAL;

	//
	// Open the SPB IO target. This creates a handle to the I2C controller
	// behind which SimGPO resides.
	//
	Status = WdfIoTargetOpen(devContext->I2CIOTarget, &Parameters);
	if (!NT_SUCCESS(Status)) {
		DbgPrint("WdfIoTargetOpen failed to open GSLDev target Status:%x\n",
			Status);
		goto SetupI2CConnectionFailed;
	}

SetupI2CConnectionFailed:
	return Status;
}

VOID
GSLEvtDeviceDestroyI2CConnection(
IN WDFDEVICE    Device
)
{
	PDEVICE_EXTENSION             devContext = NULL;
	devContext = GetDeviceContext(Device);
	if (devContext != NULL)
	{
		if (devContext->I2CIOTarget != NULL)
		{
			WdfIoTargetClose(devContext->I2CIOTarget);
			WdfObjectDelete(devContext->I2CIOTarget);
		}
	}
}

NTSTATUS
HidGSLEvtDeviceReleaseHardware(
_In_
WDFDEVICE Device,
_In_
WDFCMRESLIST ResourcesTranslated)
{
	PDEVICE_EXTENSION             devContext = NULL;
	
	DbgPrintGSL("HidGSLEvtDeviceReleaseHardware Entry\n");
	UNREFERENCED_PARAMETER(ResourcesTranslated);
	
	devContext = GetDeviceContext(Device);
#ifdef GSL_TIMER
	KeCancelTimer(&devContext->EsdTimer);
	devContext->terminate_thread = TRUE;//¨ª¡ê?1??3¨¬
	KeSetEvent(
		&devContext->request_event,
		(KPRIORITY)0,
		FALSE
		);
	KeWaitForSingleObject(
		devContext->thread_pointer,//¦Ì¨¨¡äy??3¨¬???¨®?¨¢¨º?
		Executive,
		KernelMode,
		FALSE,
		NULL
		);
	ObDereferenceObject(devContext->thread_pointer);
#endif
	GSLEvtDeviceDestroyI2CConnection(Device);

	if (devContext->DpcLock != NULL)
		WdfObjectDelete(devContext->DpcLock);

	return STATUS_SUCCESS;
}

NTSTATUS
HidGSLEvtDevicePrepareHardware(
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
	//BOOLEAN fConnectionIdFound = FALSE;
	BOOLEAN I2ResourceFound = FALSE;
	BOOLEAN GPIOResourceFound = FALSE;
	ULONG resourceCount;
	NTSTATUS status = STATUS_SUCCESS;

	PDEVICE_EXTENSION             devContext = NULL;
	ULONG ix = 0;
	//DbgPrintEx(DPFLTR_IHVDRIVER_ID, 0, "%s SSSSSSSSSSSSSSSS  HidGSLEvtDevicePrepareHardware Entry\n", __FUNCTION__);
	DbgPrintGSL("HidGSLEvtDevicePrepareHardware Entry\n");
	UNREFERENCED_PARAMETER(ResourceList);
	
	//mapping resource.
	//get the number of resource that the system has assigned to its device
	resourceCount = WdfCmResourceListGetCount(ResourceListTranslated);
	devContext = GetDeviceContext(Device);
	// Loop through the resources and save the relevant ones.
	devContext->InterruptCount = 0;
	for (ix = 0; ix < resourceCount; ix++)
	{
		//get details about a particular resource from the list.
		PCM_PARTIAL_RESOURCE_DESCRIPTOR pDescriptor;
		pDescriptor = WdfCmResourceListGetDescriptor(ResourceListTranslated, ix);
		if (pDescriptor == NULL)
		{
			break;
		}
		switch (pDescriptor->Type)
		{
			case CmResourceTypeConnection:
			{
				UCHAR Class = pDescriptor->u.Connection.Class;
				UCHAR Type = pDescriptor->u.Connection.Type;
				if (Class == CM_RESOURCE_CONNECTION_CLASS_SERIAL)
				{
					if (Type == CM_RESOURCE_CONNECTION_TYPE_SERIAL_I2C)
					{
						if (I2ResourceFound == FALSE)
						{
							// Save the SPB connection ID.
							devContext->ConnectionIds[I2C_RESOURCE_INDEX].LowPart =
								pDescriptor->u.Connection.IdLowPart;
							devContext->ConnectionIds[I2C_RESOURCE_INDEX].HighPart =
								pDescriptor->u.Connection.IdHighPart;
							DbgPrint("I2Resource found.\n");
							I2ResourceFound = TRUE;
						}
					}
				}
				if (Class == CM_RESOURCE_CONNECTION_CLASS_GPIO)
				{
					// Check for GPIO pin resource.
					if (Type == CM_RESOURCE_CONNECTION_TYPE_GPIO_IO)
					{
						if (GPIOResourceFound == FALSE)
						{
							devContext->ConnectionIds[GPIO_RESOURCE_INDEX].LowPart =
								pDescriptor->u.Connection.IdLowPart;
							devContext->ConnectionIds[GPIO_RESOURCE_INDEX].HighPart =
								pDescriptor->u.Connection.IdHighPart;
							DbgPrint("GPIOResource found.\n");
							GPIOResourceFound = TRUE;
						}
					}
				}
			}
			break;

			case CmResourceTypeInterrupt:
			{
				devContext->InterruptCount++;
				// Check for interrupt resource.
				//create interrupt object.
			}
			break;

			default:
				// Don't care about other resource descriptors.
			break;
		}//end switch
	}
	
	DbgPrintGSL("Resource values! I2C = %d, GPIO = %d, InterruptCount = %d\n",
		I2ResourceFound,
		GPIOResourceFound,
		devContext->InterruptCount);

	//GPIO_CLIENT_REGISTRATION_PACKET RegistrationPacket;
	if ((devContext->InterruptCount != 1) || (I2ResourceFound == FALSE) || (GPIOResourceFound == FALSE)) {
		status = STATUS_UNSUCCESSFUL;
	}
	if (!NT_SUCCESS(status)) {
		goto DevicePrepareHardwareEnd;
	}

	//
	//  Store the number of GPIO IO connection strings
	//
	devContext->IoResourceCount = 2;

	status = GSLEvtDeviceSetupI2CConnection(Device);
	if (!NT_SUCCESS(status)) {
		DbgPrint("HidGSLEvtDevicePrepareHardware,SetupI2CConnection failed! status = %d\n", status);
		goto DevicePrepareHardwareEnd;
	}
	//init silead ic procedure.
	GPIOSHUTDOWN_LOW(Device);
	wd_delay(20);
	GPIOSHUTDOWN_HIGH(Device);
	wd_delay(20);
	//DbgPrintEx(DPFLTR_IHVDRIVER_ID, 0, "%s SSSSSSSSSSSSSSSSSSSSS  HidGSLEvtDevicePrepareHardware GSL_Init_Chip called before\n", __FUNCTION__);
	GSL_Init_Chip(Device);
	wd_delay(50);
	GSL_Check_Memdata(Device);//add for test.
	wd_delay(50);
	//DbgPrintEx(DPFLTR_IHVDRIVER_ID, 0, "%s SSSSSSSSSSSSSSSSSSSS  HidGSLEvtDevicePrepareHardware exit sucessful\n", __FUNCTION__);
DevicePrepareHardwareEnd:
	//DbgPrintEx(DPFLTR_IHVDRIVER_ID, 0, "%s SSSSSSSSSSSSSSSSSSSS  HidGSLEvtDevicePrepareHardware exit failed\n", __FUNCTION__);
	return status;
}


NTSTATUS GPIORstResetData(IN  WDFDEVICE Device, IN PUCHAR pData)
{
	WDF_OBJECT_ATTRIBUTES Attributes;
	WDFREQUEST IoctlRequest;
	WDFIOTARGET IoTarget;
	WDFMEMORY WdfMemory;
	WDF_OBJECT_ATTRIBUTES RequestAttributes;
	WDF_REQUEST_SEND_OPTIONS SendOptions;
	NTSTATUS Status;
	WDF_OBJECT_ATTRIBUTES ObjectAttributes;
	WDF_IO_TARGET_OPEN_PARAMS OpenParams;
	PDEVICE_EXTENSION             devContext = NULL;
	UNICODE_STRING WriteString;
	WCHAR WriteStringBuffer[100];

	DbgPrintGSL("GPIORstResetData begin.\n");
	WDF_OBJECT_ATTRIBUTES_INIT(&ObjectAttributes);
	ObjectAttributes.ParentObject = Device;

	IoctlRequest = NULL;
	IoTarget = NULL;

	devContext = GetDeviceContext(Device);

	if (pData == NULL) {
		Status = STATUS_INVALID_PARAMETER;
		DbgPrint("GPIORstResetData, STATUS_INVALID_PARAMETER.\n");
		goto GPIORstResetDataEnd;
	}

	RtlInitEmptyUnicodeString(&WriteString,
		WriteStringBuffer,
		sizeof(WriteStringBuffer));

	Status = RESOURCE_HUB_CREATE_PATH_FROM_ID(&WriteString,
		devContext->ConnectionIds[GPIO_RESOURCE_INDEX].LowPart,
		devContext->ConnectionIds[GPIO_RESOURCE_INDEX].HighPart);
	if (!NT_SUCCESS(Status)) {
		DbgPrint("GPIORstResetData, RESOURCE_HUB_CREATE_PATH_FROM_ID failed, status = 0x%x\n", Status);
		goto GPIORstResetDataEnd;
	}

	Status = WdfIoTargetCreate(Device,
		&ObjectAttributes,
		&IoTarget);

	if (!NT_SUCCESS(Status)) {
		DbgPrint("GPIORstResetData, WdfIoTargetCreate failed, status = 0x%x\n", Status);
		goto GPIORstResetDataEnd;
	}


	WDF_IO_TARGET_OPEN_PARAMS_INIT_OPEN_BY_NAME(&OpenParams,
		&WriteString,
		FILE_GENERIC_WRITE);

	Status = WdfIoTargetOpen(IoTarget, &OpenParams);
	if (!NT_SUCCESS(Status)) {
		DbgPrint("GPIORstResetData, WdfIoTargetOpen failed, status = 0x%x\n", Status);
		goto GPIORstResetDataEnd;
	}

	WDF_OBJECT_ATTRIBUTES_INIT(&RequestAttributes);
	Status = WdfRequestCreate(&RequestAttributes, IoTarget, &IoctlRequest);
	if (!NT_SUCCESS(Status)) {
		DbgPrint("GPIORstResetData, WdfRequestCreate failed, status = 0x%x\n", Status);
		goto GPIORstResetDataEnd;
	}

	WDF_OBJECT_ATTRIBUTES_INIT(&Attributes);
	Attributes.ParentObject = IoctlRequest;
	Status = WdfMemoryCreatePreallocated(&Attributes, pData, sizeof(*pData), &WdfMemory);
	if (!NT_SUCCESS(Status)) {
		DbgPrint("GPIORstResetData, WdfMemoryCreatePreallocated failed, status = 0x%x\n", Status);
		goto GPIORstResetDataEnd;
	}

	Status = WdfIoTargetFormatRequestForIoctl(IoTarget,
		IoctlRequest,
		IOCTL_GPIO_WRITE_PINS,
		WdfMemory,
		0,
		WdfMemory,
		0);

	if (!NT_SUCCESS(Status)) {
		DbgPrint("GPIORstResetData, WdfIoTargetFormatRequestForIoctl failed, status = 0x%x\n", Status);
		goto GPIORstResetDataEnd;
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
		DbgPrint("GPIORstResetData, WdfRequestAllocateTimer failed, status = 0x%x\n", Status);
		goto GPIORstResetDataEnd;
	}

	if (!WdfRequestSend(IoctlRequest, IoTarget, &SendOptions)) {
		Status = WdfRequestGetStatus(IoctlRequest);
	}

GPIORstResetDataEnd:
	if (IoctlRequest != NULL) {
		WdfObjectDelete(IoctlRequest);
	}

	if (IoTarget != NULL){
		WdfIoTargetClose(IoTarget);
		WdfObjectDelete(IoTarget);
	}
	DbgPrintGSL("GPIORstResetData end.\n");
	return Status;
}

NTSTATUS GPIORstReset(IN  WDFDEVICE Device)//write operation
{
	BYTE Data;

	Data = 0x0;
	GPIORstResetData(Device, &Data);
	wd_delay(20);
	Data = 0x1;
	GPIORstResetData(Device, &Data);
	wd_delay(20);
	return STATUS_SUCCESS;
}

NTSTATUS GPIOSHUTDOWN_LOW(IN WDFDEVICE Device)
{
	BYTE Data;

	Data = 0x0;
	GPIORstResetData(Device, &Data);
	return STATUS_SUCCESS;
}

NTSTATUS GPIOSHUTDOWN_HIGH(IN WDFDEVICE Device)
{
	BYTE Data;

	Data = 0x1;
	GPIORstResetData(Device, &Data);
	return STATUS_SUCCESS;
}
NTSTATUS
	HidGSLEvtDeviceD0Entry(
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
	PDEVICE_EXTENSION             devContext = NULL;

	UNREFERENCED_PARAMETER(PreviousState);
	DbgPrint("HidGSLEvtDeviceD0Entry\n");
	//DbgPrintEx(DPFLTR_IHVDRIVER_ID, 0, "%s SSSSSSSSSSSSSSSSSSSSS HidGSLEvtDeviceD0Entry enter\n", __FUNCTION__);
	devContext = GetDeviceContext(Device);
	//
	// Complete any pending Idle IRPs
	//
	TchCompleteIdleIrp(devContext);
#ifdef WINKEY_WAKEUP_SYSTEM
	GSL_Exit_Doze(Device);
#endif
	GPIOSHUTDOWN_LOW(Device);
	wd_delay(30);
	GPIOSHUTDOWN_HIGH(Device);	
	wd_delay(20);
	GSL_Reset_Chip(Device);
	GSL_Startup_Chip(Device);
	GSL_Check_Memdata(Device);
#ifdef GSL_TIMER
	{
		LARGE_INTEGER duetime = { 0 };
		KeCancelTimer(&devContext->EsdTimer);
		KeSetTimerEx(&devContext->EsdTimer, duetime, 3000, &devContext->EsdDPC);
	}
#endif
	DbgPrint("HidGSLEvtDeviceD0Entry finish.\n");
	return STATUS_SUCCESS;
}


NTSTATUS
	HidGSLEvtDeviceD0Exit(
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
		UNREFERENCED_PARAMETER(TargetState);
		DbgPrint("HidGSLEvtDeviceD0Exit\n");
#ifdef WINKEY_WAKEUP_SYSTEM
		GSL_Enter_Doze(Device);
#else
#if !defined(GSL2681)&& !defined(GSL1680)
		UCHAR tmp = 0xb5; 
#endif
#ifdef GSL_TIMER
	PDEVICE_EXTENSION devContext = GetDeviceContext(Device);
	KeCancelTimer(&devContext->EsdTimer);
#endif
	GSL_Reset_Chip(Device);
#if !defined(GSL2681)&& !defined(GSL1680)
	GSLDevWriteBuffer(Device, 0xe4, &tmp, 1);
#endif
	wd_delay(10);
	GPIOSHUTDOWN_LOW(Device);
#endif
	DbgPrint("HidGSLEvtDeviceD0Exit finish.\n");
	return STATUS_SUCCESS;
}


#ifdef GSL_TIMER
//#pragma LOCKEDCODE
VOID GSLDevESDRoutine(_In_ struct _KDPC *Dpc,_In_opt_ PVOID DeferredContext,
			_In_opt_ PVOID SystemArgument1,_In_opt_ PVOID SystemArgument2
)
{
	//DISPATCH_LEVEL.
	WDFDEVICE Device;
	PDEVICE_EXTENSION	devContext;
	//ASSERT(KeGetCurrentIrql() <= DISPATCH_LEVEL);
	Device = (WDFDEVICE)WdfObjectContextGetObject(DeferredContext);
	UNREFERENCED_PARAMETER(SystemArgument1);
	UNREFERENCED_PARAMETER(SystemArgument2);
	UNREFERENCED_PARAMETER(Dpc);
	devContext = GetDeviceContext(Device);
	KeSetEvent(&devContext->request_event,
		(KPRIORITY)0,
		FALSE);
}

VOID GSLDevESDDetail(IN WDFDEVICE    Device)
{
	PDEVICE_EXTENSION	devContext;
	UCHAR read_buf[4] = { 0 };
	UCHAR init_chip_flag = 0;
	devContext = GetDeviceContext(Device);
	DbgPrintGSL("%s Entry\n", __FUNCTION__);
	if (devContext->i2c_lock != 0)
	{
		return;
	}
	else
	{
		devContext->i2c_lock = 1;
	}	
	//read 0xb0.
	GSLDevReadBuffer(Device, 0xb0, read_buf, 4);
	if (read_buf[3] != 0x5a || read_buf[2] != 0x5a || read_buf[1] != 0x5a || read_buf[0] != 0x5a)
		devContext->b0_counter++;
	else
		devContext->b0_counter = 0;
	
	if (devContext->b0_counter > 1)
	{
		DbgPrint("======read 0xb0: %x %x %x %x ======\n", read_buf[3], read_buf[2], read_buf[1], read_buf[0]);
		init_chip_flag = 1;
		devContext->b0_counter = 0;
		goto init_chip_lable;
	}
	//read 0xb4.
	GSLDevReadBuffer(Device, 0xb4, read_buf, 4);
	devContext->int_2nd[3] = devContext->int_1st[3];
	devContext->int_2nd[2] = devContext->int_1st[2];
	devContext->int_2nd[1] = devContext->int_1st[1];
	devContext->int_2nd[0] = devContext->int_1st[0];
	devContext->int_1st[3] = read_buf[3];
	devContext->int_1st[2] = read_buf[2];
	devContext->int_1st[1] = read_buf[1];
	devContext->int_1st[0] = read_buf[0];
	
	if (devContext->int_1st[3] == devContext->int_2nd[3]
		&& devContext->int_1st[2] == devContext->int_2nd[2]
		&& devContext->int_1st[1] == devContext->int_2nd[1]
		&& devContext->int_1st[0] == devContext->int_2nd[0]
		)
	{
		DbgPrint("======int_1st: %x %x %x %x , int_2nd: %x %x %x %x ======\n",
			devContext->int_1st[3], devContext->int_1st[2], devContext->int_1st[1], devContext->int_1st[0],
			devContext->int_2nd[3], devContext->int_2nd[2], devContext->int_2nd[1], devContext->int_2nd[0]);
		init_chip_flag = 1;
		goto init_chip_lable;
	}
	//read 0xbc.
	GSLDevReadBuffer(Device, 0xbc, read_buf, 4);
	if (read_buf[3] != 0 || read_buf[2] != 0 || read_buf[1] != 0 || read_buf[0] != 0)
		devContext->bc_counter++;
	else
		devContext->bc_counter = 0;
	if (devContext->bc_counter > 1)
	{
		DbgPrint("======read 0xbc: %x %x %x %x======\n", read_buf[3], read_buf[2], read_buf[1], read_buf[0]);
		init_chip_flag = 1;
		devContext->bc_counter = 0;
	}	
init_chip_lable:
	if (init_chip_flag)
	{
		wd_delay(50);
		GSL_Init_Chip(Device);
		wd_delay(100); 
		GSL_Check_Memdata(Device);
	}
	devContext->i2c_lock = 0;
}

VOID ThreadFunc(IN PVOID Context)
{
	WDFDEVICE Device;
	PDEVICE_EXTENSION	devContext;
	//UINT32 i = 0;
	Device = (WDFDEVICE)WdfObjectContextGetObject(Context);
	devContext = GetDeviceContext(Device);

	KeSetPriorityThread(KeGetCurrentThread(), LOW_REALTIME_PRIORITY);
	for (;;)
	//for (; i < 10; i++)
	{
		KeWaitForSingleObject(
			&devContext->request_event,
			Executive,
			KernelMode,
			FALSE,
			NULL
			);
		GSLDevESDDetail(Device);
		KeResetEvent(&devContext->request_event);
		if (devContext->terminate_thread)
		{
			PsTerminateSystemThread(STATUS_SUCCESS);
		}
	}
}
#endif

NTSTATUS
	TestGPIOReadWrite(
	_In_ WDFDEVICE Device,
	_In_ PCUNICODE_STRING RequestString,
	_In_ BOOLEAN ReadOperation,
	_Inout_ PUCHAR Data,
	_In_ _In_range_(>, 0) ULONG Size,
	_Out_ WDFIOTARGET *IoTargetOut
	)

	/*++

	Routine Description:

	This is a utility routine to test read or write on a set of GPIO pins.

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
		WDF_OBJECT_ATTRIBUTES Attributes;
		WDFREQUEST IoctlRequest;
		WDFIOTARGET IoTarget;
		ULONG DesiredAccess;
		WDFMEMORY WdfMemory;
		WDF_OBJECT_ATTRIBUTES RequestAttributes;
		WDF_REQUEST_SEND_OPTIONS SendOptions;
		NTSTATUS Status;
		WDF_OBJECT_ATTRIBUTES ObjectAttributes;
		WDF_IO_TARGET_OPEN_PARAMS OpenParams;

		WDF_OBJECT_ATTRIBUTES_INIT(&ObjectAttributes);
		ObjectAttributes.ParentObject = Device;

		IoctlRequest = NULL;
		IoTarget = NULL;

		if ((Data == NULL) || (Size == 0)) {
			Status = STATUS_INVALID_PARAMETER;
			goto TestReadWriteEnd;
		}

		Status = WdfIoTargetCreate(Device,
			&ObjectAttributes,
			&IoTarget);

		if (!NT_SUCCESS(Status)) {
			goto TestReadWriteEnd;
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
			goto TestReadWriteEnd;
		}

		WDF_OBJECT_ATTRIBUTES_INIT(&RequestAttributes);
		Status = WdfRequestCreate(&RequestAttributes, IoTarget, &IoctlRequest);
		if (!NT_SUCCESS(Status)) {
			goto TestReadWriteEnd;
		}

		//
		// Set up a WDF memory object for the IOCTL request
		//

		WDF_OBJECT_ATTRIBUTES_INIT(&Attributes);
		Attributes.ParentObject = IoctlRequest;
		Status = WdfMemoryCreatePreallocated(&Attributes, Data, Size, &WdfMemory);
		if (!NT_SUCCESS(Status)) {
			goto TestReadWriteEnd;
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
			goto TestReadWriteEnd;
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
			goto TestReadWriteEnd;
		}

		if (!WdfRequestSend(IoctlRequest, IoTarget, &SendOptions)) {
			Status = WdfRequestGetStatus(IoctlRequest);
		}

		if (NT_SUCCESS(Status)) {
			*IoTargetOut = IoTarget;
		}

	TestReadWriteEnd:
		if (IoctlRequest != NULL) {
			WdfObjectDelete(IoctlRequest);
		}

		if (!NT_SUCCESS(Status) && (IoTarget != NULL)) {
			WdfIoTargetClose(IoTarget);
			WdfObjectDelete(IoTarget);
		}

		return Status;
	}

NTSTATUS
GPIOTestReadWrite(
	IN  WDFDEVICE Device)
{
	BYTE Data = 0;
	WDFIOTARGET ReadTarget = NULL;
	WDFIOTARGET WriteTarget = NULL;
	//PSAMPLE_DRV_DEVICE_EXTENSION SampleDrvExtension;
	PDEVICE_EXTENSION             devContext = NULL;
	UNICODE_STRING ReadString;
	WCHAR ReadStringBuffer[100];
	UNICODE_STRING WriteString;
	WCHAR WriteStringBuffer[100];
	NTSTATUS status;

	devContext = GetDeviceContext(Device);

	RtlInitEmptyUnicodeString(&ReadString,
		ReadStringBuffer,
		sizeof(ReadStringBuffer));

	RtlInitEmptyUnicodeString(&WriteString,
		WriteStringBuffer,
		sizeof(WriteStringBuffer));

	status = RESOURCE_HUB_CREATE_PATH_FROM_ID(&ReadString, devContext->ConnectionIds[GPIO_RESOURCE_INDEX].LowPart, devContext->ConnectionIds[GPIO_RESOURCE_INDEX].HighPart);

	if (!NT_SUCCESS(status)) {
		goto Cleanup;
	}
	status = RESOURCE_HUB_CREATE_PATH_FROM_ID(&WriteString,
		devContext->ConnectionIds[GPIO_RESOURCE_INDEX].LowPart,
		devContext->ConnectionIds[GPIO_RESOURCE_INDEX].HighPart);

	if (!NT_SUCCESS(status)) {
		goto Cleanup;
	}

	Data = 0x0;
	status = TestGPIOReadWrite(Device, &ReadString, TRUE, &Data, sizeof(Data), &ReadTarget);
	if (!NT_SUCCESS(status)) {
		goto Cleanup;
	}
	status = TestGPIOReadWrite(Device, &WriteString, FALSE, &Data, sizeof(Data), &WriteTarget);
	if (!NT_SUCCESS(status)) {
		goto Cleanup;
	}

Cleanup:

	if (ReadTarget != NULL) {
		WdfIoTargetClose(ReadTarget);
		WdfObjectDelete(ReadTarget);
	}
	if (WriteTarget != NULL) {
		WdfIoTargetClose(WriteTarget);
		WdfObjectDelete(WriteTarget);
	}
	return status;
}
