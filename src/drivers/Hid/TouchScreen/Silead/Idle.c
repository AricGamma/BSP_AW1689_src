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

#include "Device.h"
#include "Hiddesc.h"
#include "idle.h"
#ifdef DEBUG_C_TMH
#include "idle.tmh"
#endif
NTSTATUS
TchProcessIdleRequest(
    _In_  WDFDEVICE  Device,
    _In_  WDFREQUEST Request,
    _Out_ BOOLEAN    *Pending
)
{
	PDEVICE_EXTENSION    pDeviceContext;
	PHID_SUBMIT_IDLE_NOTIFICATION_CALLBACK_INFO idleCallbackInfo;
	PIRP               irp;
	PIO_STACK_LOCATION irpSp;
	NTSTATUS           status;

	DbgPrintGSL("TchProcessIdleRequest Entry\n");

	pDeviceContext = GetDeviceContext(Device);
	NT_ASSERT(pDeviceContext != NULL);
	NT_ASSERT(Pending != NULL);
	*Pending = FALSE;

	//
	// Retrieve request parameters and validate
	//
	irp = WdfRequestWdmGetIrp(Request);
	irpSp = IoGetCurrentIrpStackLocation(irp);

	if (irpSp->Parameters.DeviceIoControl.InputBufferLength <
		sizeof(HID_SUBMIT_IDLE_NOTIFICATION_CALLBACK_INFO))
	{
		status = STATUS_INVALID_BUFFER_SIZE;
		DbgPrint("Error: Input buffer is too small to process idle request status:%!STATUS!", status);
		goto exit;
	}

	//
	// Grab the callback
	//
	idleCallbackInfo = (PHID_SUBMIT_IDLE_NOTIFICATION_CALLBACK_INFO)
		irpSp->Parameters.DeviceIoControl.Type3InputBuffer;

	NT_ASSERT(idleCallbackInfo != NULL);

	if (NULL == idleCallbackInfo || NULL == idleCallbackInfo->IdleCallback)
	{
		status = STATUS_NO_CALLBACK_ACTIVE;
		DbgPrint("Error: Idle Notification request %p has no idle callback info status:%!STATUS!", Request, status);
		goto exit;
	}

	{
		//
		// Create a workitem for the idle callback
		//
		WDF_OBJECT_ATTRIBUTES  workItemAttributes;
		WDF_WORKITEM_CONFIG    workitemConfig;
		WDFWORKITEM            idleWorkItem;
		PIDLE_WORKITEM_CONTEXT idleWorkItemContext;

		WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&workItemAttributes, IDLE_WORKITEM_CONTEXT);
		workItemAttributes.ParentObject = Device;

		WDF_WORKITEM_CONFIG_INIT(&workitemConfig, TchIdleIrpWorkitem);

		status = WdfWorkItemCreate(
			&workitemConfig,
			&workItemAttributes,
			&idleWorkItem
			);

		if (!NT_SUCCESS(status)) 
		{
			DbgPrint("WdfWorkItemCreate failed creating creating idle work item status:%!STATUS!", status);
			goto exit;
		}

		//
		// Set the workitem context
		//
		idleWorkItemContext = GetWorkItemContext(idleWorkItem);
		idleWorkItemContext->Device = Device;
		idleWorkItemContext->Request = Request;

		//
		// Enqueue a workitem for the idle callback
		//
		WdfWorkItemEnqueue(idleWorkItem);

		//
		// Mark the request as pending so that 
		// we can complete it when we come out of idle
		//
		*Pending = TRUE;
	}

exit:

	DbgPrintGSL("TchProcessIdleRequest Exit\n");

	return status;
}

VOID
TchIdleIrpWorkitem(
    _In_ WDFWORKITEM IdleWorkItem
)
{
	NTSTATUS               status;
	PIDLE_WORKITEM_CONTEXT idleWorkItemContext;
	PDEVICE_EXTENSION        pDeviceContext;
	PHID_SUBMIT_IDLE_NOTIFICATION_CALLBACK_INFO idleCallbackInfo;

	DbgPrintGSL("TchIdleIrpWorkitem Entry\n");

	idleWorkItemContext = GetWorkItemContext(IdleWorkItem);
	NT_ASSERT(idleWorkItemContext != NULL);

	pDeviceContext = GetDeviceContext(idleWorkItemContext->Device);
	NT_ASSERT(pDeviceContext != NULL);

	//
	// Get the idle callback info from the workitem context
	//
	idleCallbackInfo = (PHID_SUBMIT_IDLE_NOTIFICATION_CALLBACK_INFO)
		IoGetCurrentIrpStackLocation(WdfRequestWdmGetIrp(idleWorkItemContext->Request))->\
		Parameters.DeviceIoControl.Type3InputBuffer;
	NT_ASSERT(idleCallbackInfo);

	//
	// idleCallbackInfo is validated already, so invoke idle callback
	//
	idleCallbackInfo->IdleCallback(idleCallbackInfo->IdleContext);

	//
	// Park this request in our IdleQueue and mark it as pending
	// This way if the IRP was cancelled, WDF will cancel it for us
	//
	status = WdfRequestForwardToIoQueue(
		idleWorkItemContext->Request,
		pDeviceContext->IdleQueue);

	if (!NT_SUCCESS(status))
	{
		//
		// IdleQueue is a manual-dispatch, non-power-managed queue. This should
		// *never* fail.
		//

		NT_ASSERTMSG("WdfRequestForwardToIoQueue to IdleQueue failed!", FALSE);

		DbgPrint("WdfRequestForwardToIoQueue failed forwarding idle notification Request:0x%p to IdleQueue:0x%p status:%!STATUS!",
			idleWorkItemContext->Request,
			pDeviceContext->IdleQueue,
			status);

		//
		// Complete the request if we couldnt forward to the Idle Queue
		//
		WdfRequestComplete(idleWorkItemContext->Request, status);
	}
	else
	{
		DbgPrint("Forwarded idle notification Request:0x%p to IdleQueue:0x%p status:%!STATUS!",
			idleWorkItemContext->Request,
			pDeviceContext->IdleQueue,
			status);
	}

	//
	// Delete the workitem since we're done with it
	//
	WdfObjectDelete(IdleWorkItem);

	DbgPrint("TchIdleIrpWorkitem Exit\n");

	return;
}


VOID
TchCompleteIdleIrp(
    _In_ PDEVICE_EXTENSION pDeviceContext
)
{
	NTSTATUS status;
	WDFREQUEST request = NULL;

	DbgPrintGSL("TchCompleteIdleIrp Entry\n");

	//
	// Lets try to retrieve the Idle IRP from the Idle queue
	//
	status = WdfIoQueueRetrieveNextRequest(
		pDeviceContext->IdleQueue,
		&request);

	//
	// We did not find the Idle IRP, maybe it was cancelled
	// 
	if (!NT_SUCCESS(status) || (NULL == request))
	{
		DbgPrint("Error finding idle notification request in IdleQueue:0x%p status:%!STATUS!",
			pDeviceContext->IdleQueue,
			status);
	}
	else
	{
		//
		// Complete the Idle IRP
		//
		WdfRequestComplete(request, status);

		DbgPrintGSL("Completed idle notification Request:0x%p from IdleQueue:0x%p status:%!STATUS!",
			request,
			pDeviceContext->IdleQueue,
			status);
	}

	DbgPrintGSL("TchCompleteIdleIrp Exit\n");

	return;
}
