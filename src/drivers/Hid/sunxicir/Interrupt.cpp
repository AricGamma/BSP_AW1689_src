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

#include "Interrupt.h"
#include "Device.h"
#include "AwCIR.h"
#include "IRDecoder.h"
#include "SendInput.h"
#include "WinVK.h"


#include "Trace.h"
#include "Interrupt.tmh"


NTSTATUS CirInterruptCreate(WDFDEVICE Device, PCM_PARTIAL_RESOURCE_DESCRIPTOR InterruptRaw, PCM_PARTIAL_RESOURCE_DESCRIPTOR InterruptTrans)
{
	NTSTATUS status;
	PDEVICE_CONTEXT pDevContext = NULL;
	WDF_INTERRUPT_CONFIG interruptConfig;
	WDF_OBJECT_ATTRIBUTES spinLockAttr;
	WDFSPINLOCK interruptSpinLock;
	WDF_OBJECT_ATTRIBUTES attr = { 0 };

	FunctionEnter();

	pDevContext = DeviceGetContext(Device);


	WDF_OBJECT_ATTRIBUTES_INIT(&spinLockAttr);
	spinLockAttr.ParentObject = Device;

	status = WdfSpinLockCreate(&spinLockAttr, &interruptSpinLock);
	if (!NT_SUCCESS(status))
	{
		DbgPrint_E("Failed to create spin lock for interrupt 0x%x\n", status);
		goto Exit;
	}


	WDF_INTERRUPT_CONFIG_INIT(&interruptConfig, CirEvtInterruptIsr, CirEvtInterruptDpc);
	interruptConfig.SpinLock = interruptSpinLock;
	interruptConfig.InterruptRaw = InterruptRaw;
	interruptConfig.InterruptTranslated = InterruptTrans;

	WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&attr, INTERRUPT_CONTEXT);

	status = WdfInterruptCreate(Device, &interruptConfig, &attr, &pDevContext->Interrupt);
	if (!NT_SUCCESS(status))
	{
		DbgPrint_E("Failed to create interrupt 0x%x\n", status);
		goto Exit;
	}


Exit:


	return status;
}

VOID CirInterruptClearState(ULONG * RegBase, UINT32 IntStateRegOffset, ULONG IntState)
{
	ULONG tmp = ReadReg(RegBase, IntStateRegOffset);

	tmp &= ~0xff;
	tmp |= IntState & 0xff;
	WRITE_REGISTER_ULONG((volatile ULONG*)((ULONG)RegBase + IntStateRegOffset), tmp);
}

BOOLEAN CirEvtInterruptIsr(_In_ WDFINTERRUPT Interrupt, _In_ ULONG MessageID)
{
	ULONG intState;

	PDEVICE_CONTEXT pDevContext = NULL;
	WDFDEVICE device;
	PINTERRUPT_CONTEXT pIntContext = NULL;

	FunctionEnter();
	UNREFERENCED_PARAMETER(MessageID);

	device = WdfInterruptGetDevice(Interrupt);
	pDevContext = DeviceGetContext(device);
	pIntContext = InterruptGetContext(Interrupt);

	pIntContext->Device = device;

	intState = ReadReg(pDevContext->RegisterBase, CIR_RX_STATE_REG_OFFSET);
	{
		ULONG tmp = ReadReg(pDevContext->RegisterBase, CIR_RX_STATE_REG_OFFSET);

		tmp &= ~0xff;
		tmp |= intState & 0xff;
		WRITE_REGISTER_ULONG((volatile ULONG*)((ULONG)pDevContext->RegisterBase + CIR_RX_STATE_REG_OFFSET), tmp);
	}
	ULONG tmp = ReadReg(pDevContext->RegisterBase, CIR_RX_STATE_REG_OFFSET);

	tmp &= ~0xff;
	tmp |= intState & 0xff;
	WRITE_REGISTER_ULONG((volatile ULONG*)((ULONG)pDevContext->RegisterBase + CIR_RX_STATE_REG_OFFSET), tmp);

	pIntContext->IsFifoOverrun = ((intState >> CIR_RX_STATE_ROI_MASK) & CIR_RX_STATE_ROI) ? TRUE : FALSE;
	pIntContext->IsPacketEnd = ((intState >> CIR_RX_STATE_RPE_MASK) & CIR_RX_STATE_RPE) ? TRUE : FALSE;
	pIntContext->IsFifoAvailable = ((intState >> CIR_RX_STATE_RA_MASK) & CIR_RX_STATE_RA) ? TRUE : FALSE;
	pIntContext->IsIdle = ((intState >> CIR_RX_STATE_IDLE_MASK) & CIR_RX_STATE_BUSY) ? FALSE : TRUE;
	pIntContext->DataCount = (UINT8)(intState >> CIR_RX_STATE_RAC_MASK);

	if (!pIntContext->IsFifoOverrun &&
		!pIntContext->IsPacketEnd &&
		!pIntContext->IsFifoAvailable
		)
	{
		return FALSE;
	}

	WRITE_REGISTER_ULONG((volatile ULONG*)((ULONG)pDevContext->RegisterBase + 0x20), 0x0);


	BOOL pulseNow = 0;
	ULONG irDuration = 0;

	int i = 0;
	for (i = 0; i < pIntContext->DataCount; i++)
	{
		ULONG data = ReadReg(pDevContext->RegisterBase, CIR_RXFIFO_REG_OFFSET);

		pulseNow = data >> 7;
		irDuration = data & 0x7f;

		if (pDevContext->PulsePrevious == pulseNow)
		{
			pDevContext->DataBuffer[pDevContext->DataSize] = irDuration;
			pDevContext->PulseBuffer[pDevContext->DataSize] = pulseNow;
			pDevContext->DataSize++;
		}
		else
		{
			if (pDevContext->IsReceiving)
			{
				pDevContext->DataBuffer[pDevContext->DataSize] = irDuration;
				pDevContext->PulseBuffer[pDevContext->DataSize] = pulseNow;
				pDevContext->DataSize++;
			}
			else
			{
				pDevContext->IsReceiving = TRUE;

				pDevContext->DataSize = 0;

				pDevContext->DataBuffer[pDevContext->DataSize] = irDuration;
				pDevContext->PulseBuffer[pDevContext->DataSize] = pulseNow;
				pDevContext->DataSize++;
			}
			pDevContext->PulsePrevious = pulseNow;
		}


	}



	if (pIntContext->IsPacketEnd)
	{
		if (pDevContext->DataSize > 70)
			WdfInterruptQueueDpcForIsr(Interrupt);
		pDevContext->IsReceiving = FALSE;
	}
	if (pIntContext->IsFifoOverrun)
	{
		pDevContext->IsReceiving = 0;
		pDevContext->PulsePrevious = 0;
	}

	return TRUE;
}

void CirEvtInterruptDpc(_In_ WDFINTERRUPT Interrupt, _In_ WDFOBJECT AssociatedObject)
{
	PDEVICE_CONTEXT pDevContext = NULL;
	PINTERRUPT_CONTEXT pIntContext = NULL;
	UINT8 dataCode = 0;
	UINT8 addrCode = 0;

	UNREFERENCED_PARAMETER(AssociatedObject);

	//FunctionEnter();

	pIntContext = InterruptGetContext(Interrupt);
	pDevContext = DeviceGetContext(pIntContext->Device);


	PD6121G_F_Decoder(pDevContext->DataBuffer, pDevContext->DataSize, &addrCode, &dataCode);

	pDevContext->DataSize = 0;
	if (addrCode == SKYWORTH_ADDR_CODE)
	{
		DbgPrint_I("Received data: %d", dataCode);
		
		switch (dataCode)
		{
		case PD6121G_NUM0:
			InjectUnicode('0', pDevContext);
			break;
		case PD6121G_NUM1:
			InjectUnicode('1', pDevContext);
			break;
		case PD6121G_NUM2:
			InjectUnicode('2', pDevContext);
			break;
		case PD6121G_NUM3:
			InjectUnicode('3', pDevContext);
			break;
		case PD6121G_NUM4:
			InjectUnicode('4', pDevContext);
			break;
		case PD6121G_NUM5:
			InjectUnicode('5', pDevContext);
			break;
		case PD6121G_NUM6:
			InjectUnicode('6', pDevContext);
			break;
		case PD6121G_NUM7:
			InjectUnicode('7', pDevContext);
			break;
		case PD6121G_NUM8:
			InjectUnicode('8', pDevContext);
			break;
		case PD6121G_NUM9:
			InjectUnicode('9', pDevContext);
			break;
		case PD6121G_POWER:
			break;
		case PD6121G_MUTE:
			InjectKeyDown(VK_VOLUME_MUTE, pDevContext);
			InjectKeyUp(VK_VOLUME_MUTE, pDevContext);
			break;
		case PD6121G_MENU:
			break;
		case PD6121G_CH_UP:
			break;
		case PD6121G_CH_DOWN:
			break;
		case PD6121G_VOL_UP:
			InjectKeyDown(VK_VOLUME_UP, pDevContext);
			InjectKeyUp(VK_VOLUME_UP, pDevContext);
			break;
		case PD6121G_VOL_DOWN:
			InjectKeyDown(VK_VOLUME_DOWN, pDevContext);
			InjectKeyUp(VK_VOLUME_DOWN, pDevContext);
			break;
		case PD6121G_BACK:
			break;
		case PD6121G_HOME:
			break;
		case PD6121G_UP:
			InjectKeyDown(VK_UP, pDevContext);
			InjectKeyUp(VK_UP, pDevContext);
			break;
		case PD6121G_DOWN:
			InjectKeyDown(VK_DOWN, pDevContext);
			InjectKeyUp(VK_DOWN, pDevContext);
			break;
		case PD6121G_LEFT:
			InjectKeyDown(VK_LEFT, pDevContext);
			InjectKeyUp(VK_LEFT, pDevContext);
			break;
		case PD6121G_RIGHT:
			InjectKeyDown(VK_RIGHT, pDevContext);
			InjectKeyUp(VK_RIGHT, pDevContext);
			break;
		case PD6121G_OK:
			InjectKeyDown(VK_RETURN, pDevContext);
			InjectKeyUp(VK_RETURN, pDevContext);
			break;
		}
	}

}
