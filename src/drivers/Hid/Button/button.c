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
#include "hidbutton.h"

#define RESHUB_USE_HELPER_ROUTINES
#include "reshub.h"

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, HidButtonEvtDevicePrepareHardware)
#pragma alloc_text(PAGE, HidButtonEvtDeviceD0Exit)
#endif

NTSTATUS
HidButtonEvtDevicePrepareHardware(
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
	NTSTATUS status = STATUS_SUCCESS;
	PDEVICE_EXTENSION pDevice = GetDeviceContext(Device);

	UNREFERENCED_PARAMETER(ResourceList);

	PAGED_CODE();

	//
	// Parse the peripheral's resources.
	//

	ULONG resourceCount = WdfCmResourceListGetCount(ResourceListTranslated);

	for (ULONG i = 0; i < resourceCount; i++) {
		PCM_PARTIAL_RESOURCE_DESCRIPTOR res;
		res = WdfCmResourceListGetDescriptor(ResourceListTranslated, i);
		if (res->Type == CmResourceTypeMemory) {
			pDevice->PhysicalBaseAddress = res->u.Memory.Start;
			pDevice->Registers = MmMapIoSpace(res->u.Memory.Start,
				res->u.Memory.Length,
				MmNonCached);
			if (pDevice->Registers == NULL) {
				status = STATUS_INSUFFICIENT_RESOURCES;
			}
		}
	}

	return status;
}

NTSTATUS
HidButtonEvtDeviceD0Entry(
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
	NTSTATUS status = STATUS_SUCCESS;
	ULONG value = 0x0;

	pDevice->KeyCnt = 0;
	pDevice->KeyVul = 0;
	pDevice->CompareBuffer[0] = 0;
	pDevice->CompareBuffer[1] = 0;
	pDevice->ScanCode = 0;
	pDevice->TransferCode = INITIAL_VALUE;
	pDevice->JudgeFlag = 0;

	value = LRADC_ADC0_DOWN_EN | LRADC_ADC0_UP_EN | LRADC_ADC0_DATA_EN;
	WRITE_REGISTER_ULONG(&pDevice->Registers->LradcIntc,value);

	value = FIRST_CONCERT_DLY | LEVELB_VOL | KEY_MODE_SELECT | LRADC_HOLD_EN | ADC_CHAN_SELECT | LRADC_SAMPLE_250HZ | LRADC_EN;
	WRITE_REGISTER_ULONG(&pDevice->Registers->LradcCtrl, value);

	return status;
}


NTSTATUS
HidButtonEvtDeviceD0Exit(
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
	//PDEVICE_EXTENSION         devContext;
	UNREFERENCED_PARAMETER(Device);
	UNREFERENCED_PARAMETER(TargetState);

	PAGED_CODE();

	//devContext = GetDeviceContext(Device);

	return STATUS_SUCCESS;
}

VOID
KeyReport(
_In_ PDEVICE_EXTENSION pDevice,
_In_ BYTE	Id,
_In_ BYTE	Vul
)
{

	ULONG				 bytesToCopy = 0;
	size_t				 bytesReturned = 0;
	WDFREQUEST           request;
	PHID_INPUT_REPORT	inputReport = NULL;
	NTSTATUS status;

	status = WdfIoQueueRetrieveNextRequest(pDevice->InterruptMsgQueue, &request);
	if (NT_SUCCESS(status)) {

		bytesToCopy = sizeof(HID_INPUT_REPORT);
		if (bytesToCopy == 0) {
			goto exit_bytes;
		}

		status = WdfRequestRetrieveOutputBuffer(request,
			bytesToCopy,
			&inputReport,
			&bytesReturned);// BufferLength

		if (NT_SUCCESS(status)) {

			inputReport->ReportId = Id;
			inputReport->State |= Vul;

		}
		else {
			bytesToCopy = 0;
		}
	}

exit_bytes:
	bytesReturned = bytesToCopy;
	WdfRequestCompleteWithInformation(request, status, bytesReturned);
}



VOID 
KeyDown(
	_In_ PDEVICE_EXTENSION pDevice
)
{

	pDevice->KeyVul = READ_REGISTER_ULONG(&pDevice->Registers->LradcData0);

	if (pDevice->KeyVul < 0x3f) {
		pDevice->CompareBuffer[pDevice->KeyCnt] = pDevice->KeyVul & 0x3f;
	}

	if ((pDevice->KeyCnt + 1) < REPORT_START_NUM) {
		pDevice->KeyCnt++;
		/* do not report key message */

	}
	else {
		if (pDevice->CompareBuffer[0] == pDevice->CompareBuffer[1]) {
			pDevice->KeyVul = pDevice->CompareBuffer[0];
			pDevice->ScanCode = keypad_mapindex[pDevice->KeyVul & 0x3f];
			pDevice->JudgeFlag = 1;
		}
		else{
			pDevice->JudgeFlag = 0;
		}

		pDevice->KeyCnt = 0;

		if (pDevice->JudgeFlag){
			if (pDevice->TransferCode == pDevice->ScanCode) {
			}
			else if (INITIAL_VALUE != pDevice->TransferCode) {
				KeyReport(pDevice, REPORT_ID_KEYBOARD, 0);
				KeyReport(pDevice, REPORT_ID_KEYBOARD, 0x04);
				pDevice->TransferCode = pDevice->ScanCode;
			}
			else {
				KeyReport(pDevice, REPORT_ID_KEYBOARD, 0x04);
				pDevice->TransferCode = pDevice->ScanCode;
			
			}
		}
	}

}

VOID
KeyUp(
_In_ PDEVICE_EXTENSION pDevice
)
{
	if (INITIAL_VALUE != pDevice->TransferCode) {
		KeyReport(pDevice, REPORT_ID_KEYBOARD, 0);
	}

	pDevice->KeyCnt = 0;
	pDevice->JudgeFlag = 0;
	pDevice->TransferCode = INITIAL_VALUE;
}





BOOLEAN
ButtonInterruptIsr(
_In_  WDFINTERRUPT Interrupt,
_In_  ULONG        MessageID
)
{
	BOOLEAN fInterruptRecognized = TRUE;

	WDFDEVICE device;
	PDEVICE_EXTENSION pDevice;

	UNREFERENCED_PARAMETER(MessageID);

	device = WdfInterruptGetDevice(Interrupt);
	pDevice = GetDeviceContext(device);
	if (pDevice == NULL) {
		return fInterruptRecognized;
	}

	pDevice->RegVul = READ_REGISTER_ULONG(&pDevice->Registers->LradcInts);

	if (pDevice->RegVul & LRADC_ADC0_DOWNPEND)
		KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, " key Down!\n"));

	if (pDevice->RegVul & LRADC_ADC0_DATAPEND) {
		KeyDown(pDevice);
	}

	if (pDevice->RegVul & LRADC_ADC0_UPPEND) {
		KeyUp(pDevice);
	}

	WRITE_REGISTER_ULONG(&pDevice->Registers->LradcInts, pDevice->RegVul);

	return fInterruptRecognized;

}

