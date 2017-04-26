#pragma once
#include "Driver.h"

typedef struct _INTERRUPT_CONTEXT
{
	WDFDEVICE Device;
	BOOLEAN IsFifoOverrun;
	BOOLEAN IsPacketEnd;
	BOOLEAN IsFifoAvailable;
	BOOLEAN IsIdle;
	UINT8 DataCount;
}INTERRUPT_CONTEXT, *PINTERRUPT_CONTEXT;

WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(INTERRUPT_CONTEXT, InterruptGetContext);

NTSTATUS CirInterruptCreate(_In_ WDFDEVICE Device, _In_ PCM_PARTIAL_RESOURCE_DESCRIPTOR InterruptRaw, _In_ PCM_PARTIAL_RESOURCE_DESCRIPTOR InterruptTrans);

VOID CirInterruptClearState(volatile ULONG * RegBase, UINT32 IntStateRegOffset, ULONG IntState);

EVT_WDF_INTERRUPT_ISR CirEvtInterruptIsr;

EVT_WDF_INTERRUPT_DPC CirEvtInterruptDpc;


