#include "vhfhid.h"

#include "../sunxicir/Trace.h"
#include "vhfhid.tmh"

unsigned long g_DebugLevel = DEBUG_LEVEL_VERBOSE;

//
// This is the default report descriptor for the virtual Hid device returned
// by the mini driver in response to IOCTL_HID_GET_REPORT_DESCRIPTOR.
//
HID_REPORT_DESCRIPTOR       G_DefaultReportDescriptor[] =
{
	// Keyboard(STD101)
	0x05,   0x01,       // Usage Page(Generic Desktop),
	0x09,   0x06,       // Usage(Keyboard),
	0xA1,   0x01,       // Collection(Application),
	0x85,   0x01,       //  Report Id(1)
	0x05,   0x07,       //  usage page key codes
	0x19,   0xe0,       //  usage min left control
	0x29,   0xe7,       //  usage max keyboard right gui
	0x15,   0x00,       //  Logical Minimum(0),
	0x25,   0x01,       //  Logical Maximum(1),
	0x75,   0x01,       //  report size 1
	0x95,   0x08,       //  report count 8
	0x81,   0x02,       //  input(Variable)
	0x19,   0x00,       //  usage min 0
	0x29,   0x91,       //  usage max 91
	0x15,   0x00,       //  Logical Minimum(0),
	0x26,   0xff, 0x00, //  logical max 0xff
	0x75,   0x08,       //  report size 8
	0x95,   0x04,       //  report count 4
	0x81,   0x00,       //  Input(Data, Array),
	0x95,   0x05,       //  REPORT_COUNT (2)
	0x75,   0x08,       //  REPORT_SIZE (8)
	0x81,   0x03,       //  INPUT (Cnst,Var,Abs)
	0xC0,				// End Collection,

						// Mouse
	0x05, 0x01,                    // USAGE_PAGE (Generic Desktop)
	0x09, 0x02,                    // USAGE (Mouse)
	0xa1, 0x01,                    // COLLECTION (Application)
	0x85, 0x02,                    //  Report Id(2)
	0x09, 0x01,                    //   USAGE (Pointer)
	0xa1, 0x00,                    //   COLLECTION (Physical)
	0x05, 0x09,                    //     USAGE_PAGE (Button)
	0x19, 0x01,                    //     USAGE_MINIMUM (Button 1)
	0x29, 0x03,                    //     USAGE_MAXIMUM (Button 3)
	0x15, 0x00,                    //     LOGICAL_MINIMUM (0)
	0x25, 0x01,                    //     LOGICAL_MAXIMUM (1)
	0x95, 0x03,                    //     REPORT_COUNT (3)
	0x75, 0x01,                    //     REPORT_SIZE (1)
	0x81, 0x02,                    //     INPUT (Data,Var,Abs)
	0x95, 0x01,                    //     REPORT_COUNT (1)
	0x75, 0x05,                    //     REPORT_SIZE (5)
	0x81, 0x03,                    //     INPUT (Cnst,Var,Abs)
	0x05, 0x01,                    //     USAGE_PAGE (Generic Desktop)
	0x09, 0x30,                    //     USAGE (X)
	0x09, 0x31,                    //     USAGE (Y)
	0x16, 0x00, 0x00,              //     Logical Minimum(0)
	0x26, 0xFF, 0x7F,              //     Logical Maximum(32767)
	0x36, 0x00, 0x00,              //     Physical Minimum(0)
	0x46, 0xFF, 0x7F,              //     Physical Maximum(32767)
	0x66, 0x00, 0x00,              //     Unit(None)
	0x75, 0x10,                    //     Report Size(16)
	0x95, 0x02,                    //     Report Count(2)
	0x81, 0x62,                    //     Input(Data, Variable, Absolute, No Preferred, Null State)
	0x95, 0x05,                    //     REPORT_COUNT (2)
	0x75, 0x08,                    //     REPORT_SIZE (8)
	0x81, 0x03,                    //     INPUT (Cnst,Var,Abs)
	0xc0,                          //   END_COLLECTION
	0xc0,                          // END_COLLECTION

								   // Touch
	0x05, 0x0d,                     // USAGE_PAGE (Digitizers)
	0x09, 0x04,                     // USAGE (Touch Screen)
	0xa1, 0x01,                     // COLLECTION (Application)
	0x85, 0x03,                     //  Report Id(3)
	0x09, 0x22,                     //   USAGE (Finger)
	0xa1, 0x02,                     //   COLLECTION (Logical)
	0x09, 0x42,                     //     USAGE (Tip Switch)
	0x09, 0x32,                     //     USAGE (In Range)
	0x15, 0x00,                     //     LOGICAL_MINIMUM (0)
	0x25, 0x01,                     //     LOGICAL_MAXIMUM (1)
	0x75, 0x01,                     //     REPORT_SIZE (1)
	0x95, 0x02,                     //     REPORT_COUNT (2)
	0x81, 0x02,                     //     INPUT (Data,Var,Abs)
	0x95, 0x06,                     //     REPORT_COUNT (6)
	0x81, 0x01,                     //     INPUT (Cnst,Ary,Abs)
	0x09, 0x51,                     //     USAGE (Contact Identifier)
	0x75, 0x08,                     //     REPORT_SIZE (8)
	0x95, 0x01,                     //     REPORT_COUNT (1)
	0x81, 0x02,                     //     INPUT (Data,Var,Abs)
	0x05, 0x01,                     //     USAGE_PAGE (Generic Desktop)
	0x09, 0x30,                     //     USAGE (X)
	0x09, 0x31,                     //     USAGE (Y)
	0x16, 0x00, 0x00,               //     Logical Minimum(0)
	0x26, 0xFF, 0x7F,               //     Logical Maximum(32767)
	0x36, 0x00, 0x00,               //     Physical Minimum(0)
	0x46, 0xFF, 0x7F,               //     Physical Maximum(32767)
	0x66, 0x00, 0x00,               //     Unit(None)
	0x75, 0x10,                     //     REPORT_SIZE(16)
	0x95, 0x02,                     //     REPORT_COUNT(2)
	0x81, 0x62,                     //     Input(Data, Variable, Absolute, No Preferred, Null State)
	0xc0,                           //   END_COLLECTION
	0x05, 0x0d,                     //   USAGE_PAGE (Digitizers)
	0x09, 0x54,                     //   USAGE (Contact count)
	0x25, 0x7f,                     //   LOGICAL_MAXIMUM (127) 
	0x95, 0x01,                     //   REPORT_COUNT (1)
	0x75, 0x08,                     //   REPORT_SIZE (8)    
	0x81, 0x02,                     //   INPUT (Data,Var,Abs)
	0x85, MAX_COUNT_REPORT_ID,      //   REPORT_ID (Feature)              
	0x09, 0x55,                     //   USAGE(Contact Count Maximum)
	0x25, 0x0a,                     //   LOGICAL_MAXIMUM (10) 
	0x95, 0x01,                     //   REPORT_COUNT (1)
	0x75, 0x08,                     //   REPORT_SIZE (8)
	0xb1, 0x02,                     //   FEATURE (Data,Var,Abs)
	0xc0,                           // END_COLLECTION
};

#define MAX_HID_REPORT_SIZE sizeof(HIDINJECTOR_INPUT_REPORT)

//
// This is the default HID descriptor returned by the mini driver
// in response to IOCTL_HID_GET_DEVICE_DESCRIPTOR. The size
// of report descriptor is currently the size of G_DefaultReportDescriptor.
//

HID_DESCRIPTOR              G_DefaultHidDescriptor =
{
	0x09,   // length of HID descriptor
	0x21,   // descriptor type == HID  0x21
	0x0100, // hid spec release
	0x00,   // country code == Not Specified
	0x01,   // number of HID class descriptors
	{                                       //DescriptorList[0]
		0x22,                               //report descriptor type 0x22
		sizeof(G_DefaultReportDescriptor)   //total length of report descriptor
	}
};

NTSTATUS DriverEntry(_In_ PDRIVER_OBJECT DriverObject, _In_ PUNICODE_STRING RegistryPath)
{
	WDF_DRIVER_CONFIG config;
	NTSTATUS status = STATUS_SUCCESS;


	WPP_INIT_TRACING(DriverObject, RegistryPath);
	FunctionEnter();

	WDF_DRIVER_CONFIG_INIT(&config, VhfHidEvtDriverDeviceAdd);

	status = WdfDriverCreate(DriverObject, RegistryPath, WDF_NO_OBJECT_ATTRIBUTES, &config, WDF_NO_HANDLE);

	if (!NT_SUCCESS(status))
	{
		DbgPrint_E("Failed to create driver 0x%x\n", status);
	}


	return status;
}


NTSTATUS VhfHidEvtDriverDeviceAdd(_In_ WDFDRIVER Driver, _Inout_ PWDFDEVICE_INIT DeviceInit)
{
	NTSTATUS status = STATUS_SUCCESS;
	PHID_DEVICE_CONTEXT pDevContext = NULL;
	WDFDEVICE device;
	WDF_OBJECT_ATTRIBUTES attr;
	WDF_PNPPOWER_EVENT_CALLBACKS pnpCallbacks;

	UNREFERENCED_PARAMETER(Driver);

	FunctionEnter();

	WdfFdoInitSetFilter(DeviceInit);

	WDF_PNPPOWER_EVENT_CALLBACKS_INIT(&pnpCallbacks);
	pnpCallbacks.EvtDeviceSelfManagedIoInit = VhfHidEvtDeviceSelfManagedIoInit;
	pnpCallbacks.EvtDeviceSelfManagedIoCleanup = VhfHidEvtDeviceSelfManagedIoCleanup;
	WdfDeviceInitSetPnpPowerEventCallbacks(DeviceInit, &pnpCallbacks);

	WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&attr, HID_DEVICE_CONTEXT);

	status = WdfDeviceCreate(&DeviceInit, &attr, &device);
	if (!NT_SUCCESS(status))
	{
		DbgPrint_E("Failed to create device 0x%x\n", status);
		goto Exit;
	}

	pDevContext = GetHidDeviceContext(device);
	pDevContext->Device = device;
	pDevContext->HidDescriptor = G_DefaultHidDescriptor;
	pDevContext->ReportDescriptor = G_DefaultReportDescriptor;

	//Initialize Vhf
	status = VhfInitialize(device);
	if (!NT_SUCCESS(status))
	{
		DbgPrint_E("Failed to init vhf 0x%x\n", status);
		goto Exit;
	}


	//Create a new queue to handle IOCTLs forward to us from rawPDO.
	status = RawQueueCreate(device, &pDevContext->RawPdoQueue);
	if (!NT_SUCCESS(status))
	{
		DbgPrint_E("QueueCreate failed 0x%x\n", status);
		goto Exit;
	}



	//Create raw PDO.


Exit:

	return status;
}


NTSTATUS VhfHidEvtDeviceSelfManagedIoInit(_In_ WDFDEVICE Device)
{
	NTSTATUS status;
	PHID_DEVICE_CONTEXT pDevContext;

	FunctionEnter();

	pDevContext = GetHidDeviceContext(Device);

	status = VhfStart(pDevContext->VhfHandle);

	if (!NT_SUCCESS(status))
	{
		DbgPrint_E("Failed to start vhf 0x%x\n", status);
	}

	return status;
}


VOID VhfHidEvtDeviceSelfManagedIoCleanup(_In_ WDFDEVICE Device)
{
	PHID_DEVICE_CONTEXT pDevContext;

	FunctionEnter();

	pDevContext = GetHidDeviceContext(Device);

	VhfDelete(pDevContext->VhfHandle, TRUE);

	return;
}

NTSTATUS VhfInitialize(WDFDEVICE Device)
{
	NTSTATUS status;
	PHID_DEVICE_CONTEXT pDevContext;
	VHF_CONFIG vhfConfig;

	FunctionEnter();

	pDevContext = GetHidDeviceContext(Device);

	VHF_CONFIG_INIT(&vhfConfig,
		WdfDeviceWdmGetDeviceObject(Device),
		pDevContext->HidDescriptor.DescriptorList[0].wReportLength,
		pDevContext->ReportDescriptor);

	vhfConfig.VhfClientContext = pDevContext;
	vhfConfig.OperationContextSize = 11;
	vhfConfig.EvtVhfAsyncOperationGetFeature = VhfHidEvtVhfAsyncOperationGetFeature;

	status = VhfCreate(&vhfConfig, &pDevContext->VhfHandle);
	if (!NT_SUCCESS(status))
	{
		DbgPrint_E("Failed to create vhf 0x%x\n", status);
	}


	return status;
}

VOID VhfHidEvtVhfAsyncOperationGetFeature(PVOID VhfClientContext, VHFOPERATIONHANDLE VhfOperationHandle, PVOID VhfOperationContext, PHID_XFER_PACKET HidTransferPacket)
{
	NTSTATUS status = STATUS_SUCCESS;
	PHID_DEVICE_CONTEXT context;

	UNREFERENCED_PARAMETER(VhfOperationContext);

	FunctionEnter();

	context = (PHID_DEVICE_CONTEXT)VhfClientContext;

	switch (HidTransferPacket->reportId)
	{
		case MAX_COUNT_REPORT_ID:
		{

			PHIDINJECTOR_MAX_COUNT_REPORT maxCountReport = (PHIDINJECTOR_MAX_COUNT_REPORT)HidTransferPacket->reportBuffer;

			if (HidTransferPacket->reportBufferLen < sizeof(HIDINJECTOR_MAX_COUNT_REPORT))
			{
				status = STATUS_BUFFER_TOO_SMALL;
				DbgPrint_E("HIDINJECTOR_GetFeatureReport Error: reportBufferLen too small");
				goto GetFeatureReportExit;
			}
			maxCountReport->MaxCount = MAX_COUNT_REPORT_ID;
			maxCountReport->ReportID = TOUCH_MAX_FINGER;
			break;
		};
		default:
		{
			DbgPrint_E("HIDINJECTOR_GetFeatureReport Error: parameter %Xh is invalid", (*(PUCHAR)HidTransferPacket->reportBuffer));
			status = STATUS_INVALID_PARAMETER;
			goto GetFeatureReportExit;
		};
	}

GetFeatureReportExit:
	VhfAsyncOperationComplete(VhfOperationHandle, status);
	return;
}

NTSTATUS RawQueueCreate(WDFDEVICE Device, WDFQUEUE * Queue)
{
	NTSTATUS status;
	WDF_IO_QUEUE_CONFIG queueConfig;
	WDF_OBJECT_ATTRIBUTES attr;
	WDFQUEUE queue;
	PQUEUE_CONTEXT queueContext;

	FunctionEnter();

	WDF_IO_QUEUE_CONFIG_INIT(&queueConfig, WdfIoQueueDispatchParallel);

	queueConfig.EvtIoWrite = VhfHidEvtIoWriteFromRawPdo;

	WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&attr, QUEUE_CONTEXT);

	status = WdfIoQueueCreate(Device, &queueConfig, &attr, &queue);
	if (!NT_SUCCESS(status))
	{
		DbgPrint_E("Failed to create io queue 0x%x\n", status);
		goto IoQueueCreateExit;
	}

	queueContext = GetQueueContext(queue);
	queueContext->Queue = queue;
	queueContext->DeviceContext = GetHidDeviceContext(Device);
	*Queue = queue;


IoQueueCreateExit:

	return status;
}

VOID VhfHidSubmitReadReport(PHID_DEVICE_CONTEXT DeviceContext, PUCHAR Report, size_t ReportSize)
{
	NTSTATUS status;
	HID_XFER_PACKET transferPacket;


	FunctionEnter();

	transferPacket.reportId = *Report;
	transferPacket.reportBuffer = Report;
	transferPacket.reportBufferLen = ReportSize;

	status = VhfReadReportSubmit(DeviceContext->VhfHandle, &transferPacket);
	if (!NT_SUCCESS(status))
	{
		DbgPrint_E("Failed to submit read report 0x%x\n", status);
	}
}



VOID VhfHidEvtIoWriteFromRawPdo(
	_In_ WDFQUEUE   Queue,
	_In_ WDFREQUEST Request,
	_In_ size_t     Length
)
{
	PQUEUE_CONTEXT pQueueContext;
	WDFMEMORY memory;
	void *pvoid;
	size_t length;
	NTSTATUS status;

	FunctionEnter();

	pQueueContext = GetQueueContext(Queue);

	if (Length < sizeof(HIDINJECTOR_INPUT_REPORT))
	{
		DbgPrint_E("WriteReport: invalid input buffer. size %d, expect %d\n",
			Length, sizeof(HIDINJECTOR_INPUT_REPORT));
		WdfRequestComplete(Request, STATUS_INVALID_BUFFER_SIZE);
		return;
	}

	status = WdfRequestRetrieveInputMemory(Request, &memory);
	if (!NT_SUCCESS(status))
	{
		DbgPrint_E("Retrieve input memory failed 0x%x\n", status);
		WdfRequestComplete(Request, status);
		return;
	}

	pvoid = WdfMemoryGetBuffer(memory, &length);
	if (pvoid == NULL)
	{
		WdfRequestComplete(Request, STATUS_INVALID_BUFFER_SIZE);
		return;
	}

	//submit report
	VhfHidSubmitReadReport(pQueueContext->DeviceContext, pvoid, Length);

	WdfRequestCompleteWithInformation(Request, STATUS_SUCCESS, Length);

}


