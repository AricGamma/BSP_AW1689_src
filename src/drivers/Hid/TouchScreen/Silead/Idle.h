#ifndef IDLE_H
#define IDLE_H
//
// Power Idle Workitem context
// 
typedef struct _IDLE_WORKITEM_CONTEXT
{
	// Handle to a WDF device object
	WDFDEVICE Device;

	// Handle to a WDF request object
	WDFREQUEST Request;

} IDLE_WORKITEM_CONTEXT, *PIDLE_WORKITEM_CONTEXT;

WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(IDLE_WORKITEM_CONTEXT, GetWorkItemContext);

VOID
TchCompleteIdleIrp(
    _In_ PDEVICE_EXTENSION pDeviceContext
);

NTSTATUS
TchProcessIdleRequest(
    _In_  WDFDEVICE  Device,
    _In_  WDFREQUEST Request,
    _Out_ BOOLEAN    *Pending
);

EVT_WDF_WORKITEM TchIdleIrpWorkitem;
#endif