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

#define USE_HARDCODED_HID_REPORT_DESCRIPTOR

#include <hidctp.h>

#if defined(EVENT_TRACING)
#include "hid.tmh"
#endif

#ifdef ALLOC_PRAGMA
#pragma alloc_text( PAGE, HidCtpSetFeature)
#pragma alloc_text( PAGE, HidCtpGetFeature)
#endif

VOID
HidCtpEvtInternalDeviceControl(
IN WDFQUEUE     Queue,
IN WDFREQUEST   Request,
IN size_t       OutputBufferLength,
IN size_t       InputBufferLength,
IN ULONG        IoControlCode
)
/*++

Routine Description:

This event is called when the framework receives
IRP_MJ_INTERNAL DEVICE_CONTROL requests from the system.

Arguments:

Queue - Handle to the framework queue object that is associated
with the I/O request.
Request - Handle to a framework request object.

OutputBufferLength - length of the request's output buffer,
if an output buffer is available.
InputBufferLength - length of the request's input buffer,
if an input buffer is available.

IoControlCode - the driver-defined or system-defined I/O control code
(IOCTL) that is associated with the request.
Return Value:

VOID

--*/

{
	NTSTATUS            status = STATUS_SUCCESS;
	WDFDEVICE           device;
	PDEVICE_EXTENSION   devContext = NULL;
	ULONG               bytesReturned = 0;

	UNREFERENCED_PARAMETER(OutputBufferLength);
	UNREFERENCED_PARAMETER(InputBufferLength);



	device = WdfIoQueueGetDevice(Queue);
	devContext = GetDeviceContext(device);
	if ((devContext == NULL) || (Queue == NULL) || (Request == NULL)) {
		TraceEvents(TRACE_LEVEL_ERROR, DBG_IOCTL,
			"devContext or Queue or Request is NULL!\n");
		return;
	}

	TraceEvents(TRACE_LEVEL_VERBOSE, DBG_IOCTL,
		"%s, Queue:0x%p, Request:0x%p\n",
		DbgHidInternalIoctlString(IoControlCode),
		Queue,
		Request
		);

	//
	// Please note that HIDCLASS provides the buffer in the Irp->UserBuffer
	// field irrespective of the ioctl buffer type. However, framework is very
	// strict about type checking. You cannot get Irp->UserBuffer by using
	// WdfRequestRetrieveOutputMemory if the ioctl is not a METHOD_NEITHER
	// internal ioctl. So depending on the ioctl code, we will either
	// use retreive function or escape to WDM to get the UserBuffer.
	//

	switch (IoControlCode) {

	case IOCTL_HID_GET_DEVICE_DESCRIPTOR:
		//
		// Retrieves the device's HID descriptor.
		//
		status = HidCtpGetHidDescriptor(device, Request);
		devContext->Report |= 0x01;
		break;

	case IOCTL_HID_GET_DEVICE_ATTRIBUTES:
		//
		//Retrieves a device's attributes in a HID_DEVICE_ATTRIBUTES structure.
		//
		status = HidCtpGetDeviceAttributes(Request);
		devContext->Report |= 0x02;
		break;

	case IOCTL_HID_GET_REPORT_DESCRIPTOR:
		//
		//Obtains the report descriptor for the HID device.
		//
		status = HidCtpGetReportDescriptor(device, Request);
		devContext->Report |= 0x04;
		break;

	case IOCTL_HID_READ_REPORT:
		//
		// Returns a report from the device into a class driver-supplied buffer.
		// For now queue the request to the manual queue. The request will
		// be retrived and completd when continuous reader reads new data
		// from the device.
		//
		status = WdfRequestForwardToIoQueue(Request, devContext->InterruptMsgQueue);

		if (!NT_SUCCESS(status)){
			TraceEvents(TRACE_LEVEL_ERROR, DBG_IOCTL,
				"WdfRequestForwardToIoQueue failed with status: 0x%x\n", status);

			WdfRequestComplete(Request, status);
		}

		return;

		//
		// This feature is only supported on WinXp and later. Compiling in W2K 
		// build environment will fail without this conditional preprocessor statement.
		//
	case IOCTL_HID_SEND_IDLE_NOTIFICATION_REQUEST:

		//
		// Hidclass sends this IOCTL for devices that have opted-in for Selective
		// Suspend feature. This feature is enabled by adding a registry value
		// "SelectiveSuspendEnabled" = 1 in the hardware key through inf file 
		// (see hidctp.inf). Since hidclass is the power policy owner for 
		// this stack, it controls when to send idle notification and when to 
		// cancel it. This IOCTL is passed to USB stack. USB stack pends it. 
		// USB stack completes the request when it determines that the device is
		// idle. Hidclass's idle notification callback get called that requests a 
		// wait-wake Irp and subsequently powers down the device. 
		// The device is powered-up either when a handle is opened for the PDOs 
		// exposed by hidclass, or when usb stack completes wait
		// wake request. In the first case, hidclass cancels the notification 
		// request (pended with usb stack), cancels wait-wake Irp and powers up
		// the device. In the second case, an external wake event triggers completion
		// of wait-wake irp and powering up of device.
		//
		//status = HidFx2SendIdleNotification(Request);

		//if (!NT_SUCCESS(status)) {
		//    TraceEvents(TRACE_LEVEL_ERROR, DBG_IOCTL,
		//        "SendIdleNotification failed with status: 0x%x\n", status);
		//    
		//    WdfRequestComplete(Request, status);
		//} 

		return;

	case IOCTL_HID_SET_FEATURE:
		//
		// This sends a HID class feature report to a top-level collection of
		// a HID class device.
		//
		status = HidCtpSetFeature(Request);
		WdfRequestComplete(Request, status);
		return;

	case IOCTL_HID_GET_FEATURE:
		//
		// Get a HID class feature report from a top-level collection of
		// a HID class device.
		//
		status = HidCtpGetFeature(Request, &bytesReturned);
		WdfRequestCompleteWithInformation(Request, status, bytesReturned);
		devContext->Report |= 0x08;
		return;

	case IOCTL_HID_WRITE_REPORT:
		//
		//Transmits a class driver-supplied report to the device.
		//
	case IOCTL_HID_GET_STRING:
		//
		// Requests that the HID minidriver retrieve a human-readable string
		// for either the manufacturer ID, the product ID, or the serial number
		// from the string descriptor of the device. The minidriver must send
		// a Get String Descriptor request to the device, in order to retrieve
		// the string descriptor, then it must extract the string at the
		// appropriate index from the string descriptor and return it in the
		// output buffer indicated by the IRP. Before sending the Get String
		// Descriptor request, the minidriver must retrieve the appropriate
		// index for the manufacturer ID, the product ID or the serial number
		// from the device extension of a top level collection associated with
		// the device.
		//
	case IOCTL_HID_ACTIVATE_DEVICE:
		//
		// Makes the device ready for I/O operations.
		//
	case IOCTL_HID_DEACTIVATE_DEVICE:
		//
		// Causes the device to cease operations and terminate all outstanding
		// I/O requests.
		//
	default:
		status = STATUS_NOT_SUPPORTED;
		break;
	}

	WdfRequestComplete(Request, status);

	TraceEvents(TRACE_LEVEL_ERROR, DBG_IOCTL,
		"report: %d\n", devContext->Report);
	return;
}


NTSTATUS
HidCtpSetFeature(
IN WDFREQUEST Request
)
/*++

Routine Description

This routine sets the state of the Feature: in this
case Segment Display on the USB FX2 board.

Arguments:

Request - Wdf Request

Return Value:

NT status value

--*/
{
	NTSTATUS                     status = STATUS_SUCCESS;
	PHID_XFER_PACKET             transferPacket = NULL;
	WDF_REQUEST_PARAMETERS       params;
	//WDFDEVICE                    device;
	//UCHAR                        featureUsage = 0;
	//PHIDFX2_FEATURE_REPORT_COUNT featureReportCount = NULL;
	//PHIDFX2_FEATURE_REPORT_VENDOR featureReportVendor = NULL;

	PAGED_CODE();

	TraceEvents(TRACE_LEVEL_VERBOSE, DBG_IOCTL, "HidFx2SetFeature Enter\n");

	//device = WdfIoQueueGetDevice(WdfRequestGetIoQueue(Request));

	WDF_REQUEST_PARAMETERS_INIT(&params);
	WdfRequestGetParameters(Request, &params);

	//
	// IOCTL_HID_SET_FEATURE & IOCTL_HID_GET_FEATURE are not METHOD_NIEHTER
	// IOCTLs. So you cannot retreive UserBuffer from the IRP using Wdf
	// function. As a result we have to escape out to WDM to get the UserBuffer
	// directly from the IRP. 
	//
	if (params.Parameters.DeviceIoControl.InputBufferLength < sizeof(HID_XFER_PACKET)) {
		status = STATUS_BUFFER_TOO_SMALL;
		TraceEvents(TRACE_LEVEL_ERROR, DBG_IOCTL,
			"Userbuffer is small 0x%x\n", status);
		return status;
	}

	//
	// This is a kernel buffer so no need for try/except block when accesssing
	// Irp->UserBuffer.
	//
	transferPacket = (PHID_XFER_PACKET) WdfRequestWdmGetIrp(Request)->UserBuffer;
	if (transferPacket == NULL) {
		status = STATUS_INVALID_DEVICE_REQUEST;
		TraceEvents(TRACE_LEVEL_ERROR, DBG_IOCTL,
			"Irp->UserBuffer is NULL 0x%x\n", status);
		return status;
	}

	if (transferPacket->reportBufferLen == 0){
		status = STATUS_BUFFER_TOO_SMALL;
		TraceEvents(TRACE_LEVEL_ERROR, DBG_IOCTL,
			"HID_XFER_PACKET->reportBufferLen is 0, 0x%x\n", status);
		return status;
	}

	if (transferPacket->reportBufferLen < sizeof(UCHAR)){
		status = STATUS_BUFFER_TOO_SMALL;
		TraceEvents(TRACE_LEVEL_ERROR, DBG_IOCTL,
			"HID_XFER_PACKET->reportBufferLen is too small, 0x%x\n", status);
		return status;
	}

	//featureReport = (PHIDFX2_FEATURE_REPORT)transferPacket->reportBuffer;
	//featureUsage = featureReport->ContactCountMaximum;

	//
	// The feature reports map directly to the command
	// data that is sent down to the device.
	//
	//if (transferPacket->reportId == SEVEN_SEGMENT_REPORT_ID)
	//{
	//       status = SendVendorCommand(
	//           device, 
	//           HIDFX2_SET_7SEGMENT_DISPLAY,
	//           &featureUsage 
	//           );
	//}
	//else if (transferPacket->reportId == BARGRAPH_REPORT_ID)
	//{
	//       status = SendVendorCommand(
	//           device, 
	//           HIDFX2_SET_BARGRAPH_DISPLAY,
	//           &featureUsage
	//           );
	//}
	//else
	//{
	//       status = STATUS_INVALID_DEVICE_REQUEST;
	//       TraceEvents(TRACE_LEVEL_ERROR, DBG_IOCTL,
	//           "Incorrect report ID, 0x%x\n", status);
	//       return status;
	//}

	TraceEvents(TRACE_LEVEL_VERBOSE, DBG_IOCTL, "HidFx2SetFeature Exit\n");
	return status;
}

NTSTATUS
HidCtpGetFeature(
IN WDFREQUEST Request,
OUT PULONG BytesReturned
)
/*++

Routine Description

This routine gets the state of the Feature: in this
case Segment Display or bargraph display on the USB FX2 board.

Arguments:

Request - Wdf Request

BytesReturned - Size of buffer returned for the request

Return Value:

NT status value

--*/
{
	NTSTATUS                     status = STATUS_SUCCESS;
	PHID_XFER_PACKET             transferPacket = NULL;
	WDF_REQUEST_PARAMETERS       params;
	PHIDFX2_FEATURE_REPORT_COUNT       featureReportCount = NULL;
	PHIDFX2_FEATURE_REPORT_VENDOR       featureReportVendor = NULL;
	//WDFDEVICE                    device;

	PAGED_CODE();

	TraceEvents(TRACE_LEVEL_VERBOSE, DBG_IOCTL, "HidFx2GetFeature Enter\n");

	*BytesReturned = 0;

	//device = WdfIoQueueGetDevice(WdfRequestGetIoQueue(Request));

	WDF_REQUEST_PARAMETERS_INIT(&params);
	WdfRequestGetParameters(Request, &params);

	//
	// IOCTL_HID_SET_FEATURE & IOCTL_HID_GET_FEATURE are not METHOD_NIEHTER
	// IOCTLs. So you cannot retreive UserBuffer from the IRP using Wdf
	// function. As a result we have to escape out to WDM to get the UserBuffer
	// directly from the IRP. 
	//
	if (params.Parameters.DeviceIoControl.OutputBufferLength < sizeof(HID_XFER_PACKET)) {
		status = STATUS_BUFFER_TOO_SMALL;
		TraceEvents(TRACE_LEVEL_ERROR, DBG_IOCTL,
			"Userbuffer is small 0x%x\n", status);
		return status;
	}

	//
	// This is a kernel buffer so no need for try/except block when accesssing
	// Irp->UserBuffer.
	//
	transferPacket = (PHID_XFER_PACKET) WdfRequestWdmGetIrp(Request)->UserBuffer;
	if (transferPacket == NULL) {
		status = STATUS_INVALID_DEVICE_REQUEST;
		TraceEvents(TRACE_LEVEL_ERROR, DBG_IOCTL,
			"Irp->UserBuffer is NULL 0x%x\n", status);
		return status;
	}

	if (transferPacket->reportBufferLen == 0){
		status = STATUS_BUFFER_TOO_SMALL;
		TraceEvents(TRACE_LEVEL_ERROR, DBG_IOCTL,
			"HID_XFER_PACKET->reportBufferLen is 0, 0x%x\n", status);
		return status;
	}

	if (transferPacket->reportBufferLen < sizeof(UCHAR)){
		status = STATUS_BUFFER_TOO_SMALL;
		TraceEvents(TRACE_LEVEL_ERROR, DBG_IOCTL,
			"HID_XFER_PACKET->reportBufferLen is too small, 0x%x\n", status);
		return status;
	}

	if (transferPacket->reportId == 0x05)
	{
		featureReportCount = (PHIDFX2_FEATURE_REPORT_COUNT) transferPacket->reportBuffer;
		featureReportCount->ContactCountMaximum = 0x05;
		*BytesReturned = sizeof (HIDFX2_FEATURE_REPORT_COUNT);
	}
	else if (transferPacket->reportId == 0x44)
	{
		featureReportVendor = (PHIDFX2_FEATURE_REPORT_VENDOR) transferPacket->reportBuffer;
		for (ULONG i = 0; i < 0x100; i++)
			featureReportVendor->Vendor[i] = 0;
		*BytesReturned = sizeof (HIDFX2_FEATURE_REPORT_VENDOR);

	}
	else
	{
		status = STATUS_INVALID_DEVICE_REQUEST;
		TraceEvents(TRACE_LEVEL_ERROR, DBG_IOCTL,
			"Incorrect report ID, 0x%x\n", status);
		return status;
	}

	TraceEvents(TRACE_LEVEL_VERBOSE, DBG_IOCTL, "HidFx2GetFeature Exit\n");
	return status;
}

NTSTATUS
HidCtpGetHidDescriptor(
IN WDFDEVICE Device,
IN WDFREQUEST Request
)
/*++

Routine Description:

Finds the HID descriptor and copies it into the buffer provided by the
Request.

Arguments:

Device - Handle to WDF Device Object

Request - Handle to request object

Return Value:

NT status code.

--*/
{
	NTSTATUS            status;
	size_t              bytesToCopy = 0;
	WDFMEMORY           memory;

	UNREFERENCED_PARAMETER(Device);

	TraceEvents(TRACE_LEVEL_VERBOSE, DBG_IOCTL,
		"HidFx2GetHidDescriptor Entry\n");

	//
	// This IOCTL is METHOD_NEITHER so WdfRequestRetrieveOutputMemory
	// will correctly retrieve buffer from Irp->UserBuffer. 
	// Remember that HIDCLASS provides the buffer in the Irp->UserBuffer
	// field irrespective of the ioctl buffer type. However, framework is very
	// strict about type checking. You cannot get Irp->UserBuffer by using
	// WdfRequestRetrieveOutputMemory if the ioctl is not a METHOD_NEITHER
	// internal ioctl.
	//
	status = WdfRequestRetrieveOutputMemory(Request, &memory);
	if (!NT_SUCCESS(status)) {
		TraceEvents(TRACE_LEVEL_ERROR, DBG_IOCTL,
			"WdfRequestRetrieveOutputMemory failed 0x%x\n", status);
		return status;
	}

	//
	// Use hardcoded "HID Descriptor" 
	//
	bytesToCopy = G_DefaultHidDescriptor.bLength;

	if (bytesToCopy == 0) {
		status = STATUS_INVALID_DEVICE_STATE;
		TraceEvents(TRACE_LEVEL_ERROR, DBG_IOCTL,
			"G_DefaultHidDescriptor is zero, 0x%x\n", status);
		return status;
	}

	status = WdfMemoryCopyFromBuffer(memory,
		0, // Offset
		(PVOID) &G_DefaultHidDescriptor,
		bytesToCopy);
	if (!NT_SUCCESS(status)) {
		TraceEvents(TRACE_LEVEL_ERROR, DBG_IOCTL,
			"WdfMemoryCopyFromBuffer failed 0x%x\n", status);
		return status;
	}

	//
	// Report how many bytes were copied
	//
	WdfRequestSetInformation(Request, bytesToCopy);

	TraceEvents(TRACE_LEVEL_VERBOSE, DBG_IOCTL,
		"HidFx2GetHidDescriptor Exit = 0x%x\n", status);
	return status;
}

NTSTATUS
HidCtpGetReportDescriptor(
IN WDFDEVICE Device,
IN WDFREQUEST Request
)
/*++

Routine Description:

Finds the Report descriptor and copies it into the buffer provided by the
Request.

Arguments:

Device - Handle to WDF Device Object

Request - Handle to request object

Return Value:

NT status code.

--*/
{
	NTSTATUS            status;
	ULONG_PTR           bytesToCopy;
	WDFMEMORY           memory;

	UNREFERENCED_PARAMETER(Device);

	TraceEvents(TRACE_LEVEL_VERBOSE, DBG_IOCTL,
		"HidFx2GetReportDescriptor Entry\n");

	//
	// This IOCTL is METHOD_NEITHER so WdfRequestRetrieveOutputMemory
	// will correctly retrieve buffer from Irp->UserBuffer. 
	// Remember that HIDCLASS provides the buffer in the Irp->UserBuffer
	// field irrespective of the ioctl buffer type. However, framework is very
	// strict about type checking. You cannot get Irp->UserBuffer by using
	// WdfRequestRetrieveOutputMemory if the ioctl is not a METHOD_NEITHER
	// internal ioctl.
	//
	status = WdfRequestRetrieveOutputMemory(Request, &memory);
	if (!NT_SUCCESS(status)) {
		TraceEvents(TRACE_LEVEL_ERROR, DBG_IOCTL,
			"WdfRequestRetrieveOutputMemory failed 0x%x\n", status);
		return status;
	}

	//
	// Use hardcoded Report descriptor
	//
	bytesToCopy = G_DefaultHidDescriptor.DescriptorList[0].wReportLength;

	if (bytesToCopy == 0) {
		status = STATUS_INVALID_DEVICE_STATE;
		TraceEvents(TRACE_LEVEL_ERROR, DBG_IOCTL,
			"G_DefaultHidDescriptor's reportLenght is zero, 0x%x\n", status);
		return status;
	}

	status = WdfMemoryCopyFromBuffer(memory,
		0,
		(PVOID) G_DefaultReportDescriptor,
		bytesToCopy);
	if (!NT_SUCCESS(status)) {
		TraceEvents(TRACE_LEVEL_ERROR, DBG_IOCTL,
			"WdfMemoryCopyFromBuffer failed 0x%x\n", status);
		return status;
	}

	//
	// Report how many bytes were copied
	//
	WdfRequestSetInformation(Request, bytesToCopy);

	TraceEvents(TRACE_LEVEL_VERBOSE, DBG_IOCTL,
		"HidFx2GetReportDescriptor Exit = 0x%x\n", status);
	return status;
}


NTSTATUS
HidCtpGetDeviceAttributes(
IN WDFREQUEST Request
)
/*++

Routine Description:

Fill in the given struct _HID_DEVICE_ATTRIBUTES

Arguments:

Request - Pointer to Request object.

Return Value:

NT status code.

--*/
{
	NTSTATUS                 status;
	PHID_DEVICE_ATTRIBUTES   deviceAttributes = NULL;

	TraceEvents(TRACE_LEVEL_VERBOSE, DBG_IOCTL,
		"HidFx2GetDeviceAttributes Entry\n");

	//
	// This IOCTL is METHOD_NEITHER so WdfRequestRetrieveOutputMemory
	// will correctly retrieve buffer from Irp->UserBuffer. 
	// Remember that HIDCLASS provides the buffer in the Irp->UserBuffer
	// field irrespective of the ioctl buffer type. However, framework is very
	// strict about type checking. You cannot get Irp->UserBuffer by using
	// WdfRequestRetrieveOutputMemory if the ioctl is not a METHOD_NEITHER
	// internal ioctl.
	//
	status = WdfRequestRetrieveOutputBuffer(Request,
		sizeof (HID_DEVICE_ATTRIBUTES),
		&deviceAttributes,
		NULL);
	if (!NT_SUCCESS(status)) {
		TraceEvents(TRACE_LEVEL_ERROR, DBG_IOCTL,
			"WdfRequestRetrieveOutputBuffer failed 0x%x\n", status);
		return status;
	}

	//
	// Retrieve USB device descriptor saved in device context
	//
	// usbDeviceDescriptor = WdfMemoryGetBuffer(deviceInfo->DeviceDescriptor, NULL);

	deviceAttributes->Size = sizeof (HID_DEVICE_ATTRIBUTES);
	deviceAttributes->VendorID = G_DefauleDeviceAttributes.VendorID;
	deviceAttributes->ProductID = G_DefauleDeviceAttributes.ProductID;
	deviceAttributes->VersionNumber = G_DefauleDeviceAttributes.VersionNumber;

	//
	// Report how many bytes were copied
	//
	WdfRequestSetInformation(Request, sizeof (HID_DEVICE_ATTRIBUTES));

	TraceEvents(TRACE_LEVEL_VERBOSE, DBG_IOCTL,
		"HidFx2GetDeviceAttributes Exit = 0x%x\n", status);
	return status;
}


NTSTATUS
HidCtpSendIdleNotification(
IN WDFREQUEST Request
)
/*++

Routine Description:

Pass down Idle notification request to lower driver

Arguments:

Request - Pointer to Request object.

Return Value:

NT status code.

--*/
{
	NTSTATUS                   status = STATUS_SUCCESS;
	BOOLEAN                    sendStatus = FALSE;
	WDF_REQUEST_SEND_OPTIONS   options;
	WDFIOTARGET                nextLowerDriver;
	WDFDEVICE                  device;
	PIO_STACK_LOCATION         currentIrpStack = NULL;
	IO_STACK_LOCATION          nextIrpStack;

	device = WdfIoQueueGetDevice(WdfRequestGetIoQueue(Request));
	currentIrpStack = IoGetCurrentIrpStackLocation(WdfRequestWdmGetIrp(Request));

	//
	// Convert the request to corresponding USB Idle notification request
	//
	if (currentIrpStack->Parameters.DeviceIoControl.InputBufferLength <
		sizeof(HID_SUBMIT_IDLE_NOTIFICATION_CALLBACK_INFO)) {

		status = STATUS_BUFFER_TOO_SMALL;
		TraceEvents(TRACE_LEVEL_ERROR, DBG_IOCTL,
			"DeviceIoControl.InputBufferLength too small, 0x%x\n", status);
		return status;
	}

	ASSERT(sizeof(HID_SUBMIT_IDLE_NOTIFICATION_CALLBACK_INFO)
		== sizeof(USB_IDLE_CALLBACK_INFO));

#pragma warning(suppress :4127)  // conditional expression is constant warning
	if (sizeof(HID_SUBMIT_IDLE_NOTIFICATION_CALLBACK_INFO) != sizeof(USB_IDLE_CALLBACK_INFO)) {

		status = STATUS_INFO_LENGTH_MISMATCH;
		TraceEvents(TRACE_LEVEL_ERROR, DBG_IOCTL,
			"Incorrect DeviceIoControl.InputBufferLength, 0x%x\n", status);
		return status;
	}

	//
	// prepare next stack location
	//
	RtlZeroMemory(&nextIrpStack, sizeof(IO_STACK_LOCATION));

	nextIrpStack.MajorFunction = currentIrpStack->MajorFunction;
	nextIrpStack.Parameters.DeviceIoControl.InputBufferLength =
		currentIrpStack->Parameters.DeviceIoControl.InputBufferLength;
	nextIrpStack.Parameters.DeviceIoControl.Type3InputBuffer =
		currentIrpStack->Parameters.DeviceIoControl.Type3InputBuffer;
	nextIrpStack.Parameters.DeviceIoControl.IoControlCode =
		IOCTL_INTERNAL_USB_SUBMIT_IDLE_NOTIFICATION;
	nextIrpStack.DeviceObject =
		WdfIoTargetWdmGetTargetDeviceObject(WdfDeviceGetIoTarget(device));

	//
	// Format the I/O request for the driver's local I/O target by using the
	// contents of the specified WDM I/O stack location structure.
	//
	WdfRequestWdmFormatUsingStackLocation(
		Request,
		&nextIrpStack
		);

	//
	// Send the request down using Fire and forget option.
	//
	WDF_REQUEST_SEND_OPTIONS_INIT(
		&options,
		WDF_REQUEST_SEND_OPTION_SEND_AND_FORGET
		);

	nextLowerDriver = WdfDeviceGetIoTarget(device);

	sendStatus = WdfRequestSend(
		Request,
		nextLowerDriver,
		&options
		);

	if (sendStatus == FALSE) {
		status = STATUS_UNSUCCESSFUL;
	}

	return status;
}

PCHAR
DbgHidInternalIoctlString(
IN ULONG        IoControlCode
)
/*++

Routine Description:

Returns Ioctl string helpful for debugging

Arguments:

IoControlCode - IO Control code

Return Value:

Name String

--*/
{
	switch (IoControlCode)
	{
	case IOCTL_HID_GET_DEVICE_DESCRIPTOR:
		return "IOCTL_HID_GET_DEVICE_DESCRIPTOR";
	case IOCTL_HID_GET_REPORT_DESCRIPTOR:
		return "IOCTL_HID_GET_REPORT_DESCRIPTOR";
	case IOCTL_HID_READ_REPORT:
		return "IOCTL_HID_READ_REPORT";
	case IOCTL_HID_GET_DEVICE_ATTRIBUTES:
		return "IOCTL_HID_GET_DEVICE_ATTRIBUTES";
	case IOCTL_HID_WRITE_REPORT:
		return "IOCTL_HID_WRITE_REPORT";
	case IOCTL_HID_SET_FEATURE:
		return "IOCTL_HID_SET_FEATURE";
	case IOCTL_HID_GET_FEATURE:
		return "IOCTL_HID_GET_FEATURE";
	case IOCTL_HID_GET_STRING:
		return "IOCTL_HID_GET_STRING";
	case IOCTL_HID_ACTIVATE_DEVICE:
		return "IOCTL_HID_ACTIVATE_DEVICE";
	case IOCTL_HID_DEACTIVATE_DEVICE:
		return "IOCTL_HID_DEACTIVATE_DEVICE";
	case IOCTL_HID_SEND_IDLE_NOTIFICATION_REQUEST:
		return "IOCTL_HID_SEND_IDLE_NOTIFICATION_REQUEST";
	default:
		return "Unknown IOCTL";
	}
}


