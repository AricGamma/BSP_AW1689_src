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


