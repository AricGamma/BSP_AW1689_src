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

#include "SunxiDma.h"
#include "DmaCsp.h"
#include "Trace.h"

unsigned long g_DebugLevel = DEBUG_LEVEL_INFORMATION;


//
// ------------- Prototypes ---------------------------------------------------------
//

VOID
AwInitializeController(
	__in PVOID ControllerContext
	);

BOOLEAN
AwValidateRequestLineBinding(
	__in PVOID ControllerContext,
	__in PDMA_REQUEST_LINE_BINDING_DESCRIPTION BindingDescription
	);

ULONG
AwQueryMaxFragments(
	__in PVOID ControllerContext,
	__in ULONG ChannelNumber,
	__in ULONG MaxFragmentsRequested
	);

VOID
AwProgramChannel(
	__in PVOID ControllerContext,
	__in ULONG ChannelNumber,
	__in ULONG RequestLine,
	__in PDMA_SCATTER_GATHER_LIST MemoryAddresses,
	__in PHYSICAL_ADDRESS DeviceAddress,
	__in BOOLEAN WriteToDevice,
	__in BOOLEAN LoopTransfer
	);

BOOLEAN
AwCancelTransfer(
	__in PVOID ControllerContext,
	__in ULONG ChannelNumber
	); 

NTSTATUS
AwConfigureChannel(
	__in PVOID ControllerContext,
	__in ULONG ChannelNumber,
	__in ULONG FunctionNumber,
	__in PVOID Context
	);

VOID
AwFlushChannel(
	__in PVOID ControllerContext,
	__in ULONG ChannelNumber
	);

BOOLEAN
AwHandleInterrupt(
	__in PVOID ControllerContext,
	__out PULONG ChannelNumber,
	__out PDMA_INTERRUPT_TYPE InterruptType
	);

VOID AwReportCommonBuffer(
	_In_  PVOID ControllerContext,
	_In_  ULONG ChannelNumber,
	_In_  PVOID VirtualAddress,
	_In_  PHYSICAL_ADDRESS PhysicalAddress
	);

ULONG
AwReadDmaCounter(
	__in PVOID ControllerContext,
	__in ULONG ChannelNumber
	);

//
// ------------- Globals ---------------------------------------------------
//

DMA_FUNCTION_TABLE AwFunctionTable =
{
	AwInitializeController,
	AwValidateRequestLineBinding,
	AwQueryMaxFragments,
	AwProgramChannel,
	AwConfigureChannel,
	AwFlushChannel,
	AwHandleInterrupt,
	AwReadDmaCounter,
	AwReportCommonBuffer, /* ReportCommonBuffer */
	AwCancelTransfer
};


//
// -------------- Routines -------------------------------------------
//


VOID
AwInitializeController(
	__in PVOID ControllerContext
)

/*++

Routine Description:

	This routine provides an opportunity for DMA controllers to initialize.

Arguments:

	ControllerContext - Supplies a pointer to the controller's internel data.

Return Value:

	None.

--*/

{

	PSUNXI_DMA_CONTROLLER Controller;
	ULONG Index = 0;
	ULONG Offset = 0;
	FunctionEnter();
	Controller = (PSUNXI_DMA_CONTROLLER) ControllerContext;

	//
	// Map the controller base iff this is the first call to init (and it is
	// therefore not already mapped.)
	//

	if (Controller->ControllerBaseVa == NULL) {
		Controller->ControllerBaseVa =
			(PULONG) HalMapIoSpace(Controller->ControllerBasePa,
			Controller->MappingSize,
			MmNonCached);

		NT_ASSERT(Controller->ControllerBaseVa != NULL);

		//common register0
		Offset = 0x0;
		Controller->CommonReg = Add2Ptr(Controller->ControllerBaseVa, Offset);

		//
		// Initialize each channel.
		//
		for (Index = 0; Index < Controller->ChannelCount; Index += 1) {
			Offset = 0x100 + Index * 0x40 + 0x00;
			Controller->PrivateReg[Index] = Add2Ptr(Controller->ControllerBaseVa, Offset);
		}
		 
	}

	if (Controller->ControllerBaseVa == NULL) {
		return;
	}

	/* Disable & clear all interrupts */
	WRITE_REGISTER_ULONG(&Controller->CommonReg->DmaIrqEnable[0], 0x0);
	WRITE_REGISTER_ULONG(&Controller->CommonReg->DmaIrqEnable[1], 0x0);
	WRITE_REGISTER_ULONG(&Controller->CommonReg->DmaIrqPending[0], 0xffffffff);
	WRITE_REGISTER_ULONG(&Controller->CommonReg->DmaIrqPending[1], 0xffffffff);

	/* init enable reg */
	for (Index = 0; Index < Controller->ChannelCount; Index++) {
		WRITE_REGISTER_ULONG(&Controller->PrivateReg[Index]->DmaEnable, 0);
		//config each channel with data from acpi tatble£¬actually we do not need to do this as it is a read only register
		//WRITE_REGISTER_ULONG(&Controller->PrivateReg[Index]->DmaConfig,Controller->ChannelInfo[Index].DmaChannelConfig);
	}
	//set non-secure mode
	WRITE_REGISTER_ULONG(&Controller->CommonReg->DmaSec, 0xff);
	FunctionExit(0);
	return;
}

BOOLEAN
AwValidateRequestLineBinding(
	__in PVOID ControllerContext,
	__in PDMA_REQUEST_LINE_BINDING_DESCRIPTION BindingDescription
)

/*++

Routine Description:

	This routine queries a DMA controller extension to test the validity of a
	request line binding.

Arguments:

	ControllerContext - Supplies a pointer to the controller's internal data.

	DeviceDescription - Supplies a pointer to the request information.

Return Value:

	TRUE if the request line binding is valid and supported by the controller.

	FALSE if the binding is invalid.

Environment:

	PASSIVE_LEVEL.

--*/

{

	PSUNXI_DMA_CONTROLLER Controller;
	FunctionEnter();
	Controller = (PSUNXI_DMA_CONTROLLER) ControllerContext;
	if (BindingDescription->ChannelNumber > Controller->ChannelCount) {
		return FALSE;
	}

	if ((BindingDescription->RequestLine > Controller->MaximumRequestLine) ||
		(BindingDescription->RequestLine < Controller->MinimumRequestLine)) {
		return FALSE;
	}

	if (Controller->ChannelInfo[BindingDescription->ChannelNumber].Used == 1) {
		return FALSE;
	}

	Controller->ChannelInfo[BindingDescription->ChannelNumber].Used = 1;
	FunctionExit(TRUE);
	return TRUE;
}

ULONG
AwQueryMaxFragments(
	__in PVOID ControllerContext,
	__in ULONG ChannelNumber,
	__in ULONG MaxFragmentsRequested
)

/*++

Routine Description:

	This routine queries the DMA extension to determine the number of
	scatter gather fragments that the next transfer can support.

Arguments:

	ControllerContext - Supplies a pointer to the controller's internal data.

	ChannelNumber - Supplies the number of the channel to program.

	MaxFragmentsRequested - Supplies a hint to the maximum fragments useful to
		this transfer.

Return Value:

	Number of fragments the next transfer on this channel can support.

--*/

{
	ULONG MaxFragmentsCanBeSupport = 0;
	PSUNXI_DMA_CONTROLLER Controller = (PSUNXI_DMA_CONTROLLER) ControllerContext;
	UNREFERENCED_PARAMETER(Controller);
	//UNREFERENCED_PARAMETER(ChannelNumber);

	MaxFragmentsCanBeSupport= (MaxFragmentsRequested > SUNXI_DMA_MAXFRAGMENTS_COUNT) ? SUNXI_DMA_MAXFRAGMENTS_COUNT : MaxFragmentsRequested;
	DbgPrint_V("ChannelNumber 0x%d,MaxFragmentsRequested is %d\n", ChannelNumber, MaxFragmentsCanBeSupport);

	return MaxFragmentsCanBeSupport;
}

VOID DumpDmaCommonReg(__in PVOID ControllerContext) 
{
	PSUNXI_DMA_CONTROLLER Controller = (PSUNXI_DMA_CONTROLLER)ControllerContext;
	DbgPrint_V("Common register:\n"
		"\t_IrqEnable0: 0x%08x\n"
		"\t___IrqMask0: 0x%08x\n"
		"\t___AutoGate: 0x%08x\n"
		"\tChannelStat: 0x%08x\n"
		"\t______stats: 0x%08x\n",
		READ_REGISTER_ULONG(&Controller->CommonReg->DmaIrqEnable[0]),
		READ_REGISTER_ULONG(&Controller->CommonReg->DmaIrqPending[0]),
		READ_REGISTER_ULONG(&Controller->CommonReg->DmaAutoGate),
		READ_REGISTER_ULONG(&Controller->CommonReg->DmaChannelStatus));

}

VOID DumpDmaChannelReg(__in PVOID ControllerContext, __in ULONG ChannelNumber)
{

	PSUNXI_DMA_CONTROLLER Controller = (PSUNXI_DMA_CONTROLLER)ControllerContext;
	DbgPrint_V("Chan %d reg:\n"
		"\t___en: \t0x%08x\n"
		"\tpause: \t0x%08x\n"
		"\tstart: \t0x%08x\n"
		"\t__cfg: \t0x%08x\n"
		"\t__src: \t0x%08x\n"
		"\t__dst: \t0x%08x\n"
		"\tcount: \t0x%08x\n"
		"\t_para: \t0x%08x\n\n",
		ChannelNumber,
		READ_REGISTER_ULONG(&Controller->PrivateReg[ChannelNumber]->DmaEnable),
		READ_REGISTER_ULONG(&Controller->PrivateReg[ChannelNumber]->DmaPause),
		READ_REGISTER_ULONG(&Controller->PrivateReg[ChannelNumber]->DmaStartAddr),
		READ_REGISTER_ULONG(&Controller->PrivateReg[ChannelNumber]->DmaConfig),
		READ_REGISTER_ULONG(&Controller->PrivateReg[ChannelNumber]->DmaCurrentSrc),
		READ_REGISTER_ULONG(&Controller->PrivateReg[ChannelNumber]->DmaCurrentDest),
		READ_REGISTER_ULONG(&Controller->PrivateReg[ChannelNumber]->DmaLeftCount),
		READ_REGISTER_ULONG(&Controller->PrivateReg[ChannelNumber]->DmaPara));
		
}

NTSTATUS SplitMemoryAddress(__in PDMA_SCATTER_GATHER_LIST MemoryAddresses, __in PSUNXI_DMA_CHANNEL pCurrentChannel)
{
	ULONG i, j;
	BOOLEAN IsSplited = FALSE;
	ULONG TotalTransferSize = 0;
	ULONG HalfTransferSize = 0;
	NTSTATUS Status = STATUS_SUCCESS;
	//if it is not looptransfer,we just copy the memory list into our data structure.
	if (!pCurrentChannel->IsLoopTransfer)
	{
		for (i = 0; i < MemoryAddresses->NumberOfElements; i++)
		{

			pCurrentChannel->MemoryAddressList.Elements[i].Address = MemoryAddresses->Elements[i].Address.LowPart;
			pCurrentChannel->MemoryAddressList.Elements[i].Length = MemoryAddresses->Elements[i].Length;
		}
		pCurrentChannel->MemoryAddressList.NumberOfElements = MemoryAddresses->NumberOfElements;
		return Status;
	}
	//caculate the total transfer size 
	for (i = 0; i < MemoryAddresses->NumberOfElements; i++)
	{
		TotalTransferSize = MemoryAddresses->Elements[i].Length;
	}

	//if the transfer size is not a multiple of 2 and 
	//if total transfer size is less than 2, then we think this transfer could not be done by our
	//hardware as we can not perfectly split this transfer into twice.
	if ((TotalTransferSize & 0x1) || (TotalTransferSize < 2))
	{
		 Status = STATUS_UNSUCCESSFUL;
		 return Status;
	}
	
	HalfTransferSize = TotalTransferSize >> 1;
	TotalTransferSize = 0;

	for (i = 0,j=0; i < MemoryAddresses->NumberOfElements; i++,j++)
	{
		TotalTransferSize += MemoryAddresses->Elements[i].Length;
		if (!IsSplited && TotalTransferSize > HalfTransferSize)
		{
			ULONG FirstSplitSize, SecondSplitSize;
			FirstSplitSize = HalfTransferSize - (TotalTransferSize - MemoryAddresses->Elements[i].Length);
			SecondSplitSize = MemoryAddresses->Elements[i].Length - FirstSplitSize;

			//let's split this memory element;
			pCurrentChannel->MemoryAddressList.Elements[j].Address = MemoryAddresses->Elements[i].Address.LowPart;
			pCurrentChannel->MemoryAddressList.Elements[j].Length = FirstSplitSize;
			j++;
			pCurrentChannel->MemoryAddressList.Elements[j].Address = MemoryAddresses->Elements[i].Address.LowPart+ FirstSplitSize;
			pCurrentChannel->MemoryAddressList.Elements[j].Length = SecondSplitSize;
			IsSplited = TRUE;
		}
		else
		{
			pCurrentChannel->MemoryAddressList.Elements[j].Address = MemoryAddresses->Elements[i].Address.LowPart;
			pCurrentChannel->MemoryAddressList.Elements[j].Length = MemoryAddresses->Elements[i].Length;
		}
	}

	pCurrentChannel->MemoryAddressList.NumberOfElements = j;

	return Status;
}


VOID
AwProgramChannel(
	__in PVOID ControllerContext,
	__in ULONG ChannelNumber,
	__in ULONG RequestLine,
	__in PDMA_SCATTER_GATHER_LIST MemoryAddresses,
	__in PHYSICAL_ADDRESS DeviceAddress,
	__in BOOLEAN WriteToDevice,
	__in BOOLEAN LoopTransfer
)

/*++

Routine Description:

	This routine programs a DMA controller channel for a specific transfer.

Arguments:

	ControllerContext - Supplies a pointer to the controller's internal data.

	ChannelNumber - Supplies the number of the channel to program.

	RequestLine - Supplies the request line number to program.  This request
		line number is system-unique (as provided to the HAL during
		registration) and must be translated by the extension.

	MemoryAddress - Supplies the address to be programmed into the memory
		side of the channel configuration.

	DeviceAddress - Supplies the address to be programmed into the device
		side of the channel configuration.

	WriteToDevice - Supplies the direction of the transfer.

	LoopTransfer - Supplies whether AutoInitialize has been set in the
		adapter making this request.

Return Value:

	None.

--*/

{
	PSUNXI_DMA_CONTROLLER Controller = (PSUNXI_DMA_CONTROLLER) ControllerContext;
	PTRANSFER_DES pDes = NULL;
	ULONG i = 0;
	PSUNXI_DMA_CHANNEL pCurrentChannel;
	NTSTATUS Status;
	DbgPrint_T("ChannelNumber=%d,RequestLine=%d,DeviceAddress=%lx,WriteToDevice=%d,LoopTransfer=%d\n", \
		ChannelNumber, RequestLine, DeviceAddress.LowPart, WriteToDevice, LoopTransfer);
	
	NT_ASSERT(RequestLine >= Controller->MinimumRequestLine && RequestLine<= Controller->MaximumRequestLine);
	NT_ASSERT(ChannelNumber < Controller->ChannelCount);
	NT_ASSERT(MemoryAddresses->NumberOfElements <= SUNXI_DMA_MAXFRAGMENTS_COUNT);

	pCurrentChannel = &(Controller->ChannelInfo[ChannelNumber]);
	pCurrentChannel->IsLoopTransfer = LoopTransfer;
	pCurrentChannel->IrqType = DMA_IRQ_QD;

	pDes = (PTRANSFER_DES)pCurrentChannel->DesBufferVirtualAddress;

	RtlZeroMemory(pDes, SUNXI_DMA_DES_BUFFER_SIZE_PER_CHANNEL);

	Status = SplitMemoryAddress(MemoryAddresses, pCurrentChannel);
	if (Status != STATUS_SUCCESS) {
		NT_ASSERT(Status);
	}
	
	pCurrentChannel->CurrentElementId = 0;

	for(i = 0; i < pCurrentChannel->MemoryAddressList.NumberOfElements; i++) {
		pDes->cofig = pCurrentChannel->DmaChannelConfig;
		pDes->saddr = WriteToDevice ? pCurrentChannel->MemoryAddressList.Elements[i].Address: DeviceAddress.LowPart;
		pDes->daddr = WriteToDevice ? DeviceAddress.LowPart: pCurrentChannel->MemoryAddressList.Elements[i].Address;
		pDes->bcnt = pCurrentChannel->MemoryAddressList.Elements[i].Length;
		pDes->param = DMA_PARA_NORMAL_WAIT;
		if (i != (pCurrentChannel->MemoryAddressList.NumberOfElements - 1)) { //let's go on...
			pDes->pnext = LoopTransfer ? DMA_END_DES_LINK :(pCurrentChannel->DesBufferPhysicalAddress + (ULONG)((i+1)*sizeof(TRANSFER_DES)));
			pDes++;
		}
		else  //last descriptor
		{
			
			pDes->pnext = DMA_END_DES_LINK;
		}
	}

	/*set start address*/
	CspDmaSetStartAddr(Controller, ChannelNumber, pCurrentChannel->DesBufferPhysicalAddress);

	//clear interrupt pendings
	CspDmaClearIrqPend(Controller, ChannelNumber, DMA_IRQ_HD | DMA_IRQ_FD | DMA_IRQ_QD);

	
	/* Enable all relevant interrupt on this channel */
	CspDmaIrqEnable(Controller,
		ChannelNumber,
		pCurrentChannel->IrqType,
		TRUE);

	Controller->ChannelInfo[ChannelNumber].State = TRANSFER_STATE_RUNING;
	/*set channel enable*/
	CspDmaStart(Controller, ChannelNumber);
	DumpDmaCommonReg(ControllerContext);
	DumpDmaChannelReg(ControllerContext, ChannelNumber);
	FunctionExit(0);
}

BOOLEAN
AwCancelTransfer(
	__in PVOID ControllerContext,
	__in ULONG ChannelNumber
)

/*++

Routine Description:

	This routine must disable the selected channel.  The channel must not be
	capable of interrupting for this transfer after being cleared in this way.

Arguments:

	ControllerContext - Supplies a pointer to the controller's internal data.

	ChannelNumber - Supplies the channel number.

Return Value:

	FALSE if the channel is already idle or if the channel is already asserting
	an interrupt.  TRUE is the channel is active and no interrupt is asserted.

--*/

{

	PSUNXI_DMA_CONTROLLER Controller;
	DbgPrint_T("cancel channel%d\n", ChannelNumber);
	Controller = (PSUNXI_DMA_CONTROLLER) ControllerContext;
	//
	// Disable the channel.
	//
	CspDmaStop(Controller, ChannelNumber);

	/* Disable & clear all interrupts */

	CspDmaIrqEnable(Controller,
		ChannelNumber,
		Controller->ChannelInfo[ChannelNumber].IrqType,
		FALSE);

	CspDmaClearIrqPend(Controller, ChannelNumber, DMA_IRQ_HD | DMA_IRQ_FD | DMA_IRQ_QD);

	Controller->ChannelInfo[ChannelNumber].IrqType = DMA_IRQ_NO;
	Controller->ChannelInfo[ChannelNumber].State = TRANSFER_STATE_IDLE;
	Controller->ChannelInfo[ChannelNumber].Used = 0;
	return TRUE;
}


VOID SunxiStartDma(
	__in PSUNXI_DMA_CONTROLLER Controller,
	__in ULONG ChannelNumber
	)
/*++

Routine Description:

	This routine start dma operation.

Arguments:

	Controller - Supplies a pointer to the controller's internal data.

	ChannelNumber - Supplies the channel number.

Return Value:
	None

--*/

{

	FunctionEnter();
	/* set start address*/
	CspDmaSetStartAddr(Controller, 
					  ChannelNumber, 
					   Controller->ChannelInfo[ChannelNumber].ChannelContext->CurDesItem.paddr);

	/* set enable register, start transfer*/
	CspDmaStart(Controller, ChannelNumber);

	Controller->ChannelInfo[ChannelNumber].ChannelContext->State = TRANSFER_STATE_RUNING;
}

VOID SunxiPauseDma(
	__in PSUNXI_DMA_CONTROLLER Controller,
	__in ULONG ChannelNumber
	)
/*++

Routine Description:

	This routine Pause dma operation.

Arguments:

	Controller - Supplies a pointer to the controller's internal data.

	ChannelNumber - Supplies the channel number.

Return Value:
	None

--*/
{
	CspDmaPause(Controller, ChannelNumber);
}

VOID SunxiResumeDma(
	__in PSUNXI_DMA_CONTROLLER Controller,
	__in ULONG ChannelNumber
	)
/*++

Routine Description:

	This routine resume dma operation.

Arguments:

	Controller - Supplies a pointer to the controller's internal data.

	ChannelNumber - Supplies the channel number.

Return Value:
	None

--*/
{

	FunctionEnter();
	CspDmaResume(Controller, ChannelNumber);
}

VOID SunxiStopDma(
	__in PSUNXI_DMA_CONTROLLER Controller,
	__in ULONG ChannelNumber
	)
/*++

Routine Description:

	This routine stop dma operation.

Arguments:

	Controller - Supplies a pointer to the controller's internal data.

	ChannelNumber - Supplies the channel number.

Return Value:
	None

--*/
{
	/* stop dma channle and clear irq pending */
	CspDmaStop(Controller, ChannelNumber);
	CspDmaClearIrqPend(Controller, ChannelNumber,  DMA_IRQ_HD | DMA_IRQ_FD | DMA_IRQ_QD);

	Controller->ChannelInfo[ChannelNumber].ChannelContext->State = TRANSFER_STATE_IDLE;
}

VOID SunxiGetDmaConfig(
	__in PDMA_CHANNEL_T ChannelContext
	)
/*++

Routine Description:

	This routine by setting information, obtain the configuration register setting value.

Arguments:

	ChannelContext - Supplies a pointer to the channel's internal data.

Return Value:
	None

--*/
{
	UNREFERENCED_PARAMETER(ChannelContext);
}

VOID
SunxiEnableDmaIrq(
	__in PSUNXI_DMA_CONTROLLER Controller,
	__in PDMA_CHANNEL_T ChannelContext
)
/*++

Routine Description:

	This routine enable dma interrupt.

Arguments:

	Controller - Supplies a pointer to the controller's internal data.

	ChannelContext - Supplies a pointer to the channel's internal data.

Return Value:
	None

--*/
{
	/*set Irq enable*/
	FunctionEnter();
	CspDmaIrqEnable(Controller, ChannelContext->ChannelNumber, ChannelContext->CurDesItem.IrqType, ChannelContext->IsEnableIrq);

}


NTSTATUS
AwConfigureChannel(
	__in PVOID ControllerContext,
	__in ULONG ChannelNumber,
	__in ULONG FunctionNumber,
	__in PVOID Context
)

/*++

Routine Description:

	This routine  configures the channel for a DMA extension specific operation.

Arguments:

	ControllerContext - Supplies a pointer to the controller's internal data.

	ChannelNumber - Supplies the channel to configure.

	FunctionNumber - Supplies the ID of the operation to perform.

	Context - Supplies parameters for this operation.

Return Value:

	NTSTATUS code.

--*/

{
	NTSTATUS Status;
	PSUNXI_DMA_CONTROLLER Controller;
	PDMA_CHANNEL_T ChannelContext;
	FunctionEnter();
	Controller = ControllerContext;
	ChannelContext = Context;

	ChannelContext->ChannelNumber = ChannelNumber;
	Controller->ChannelInfo[ChannelNumber].ChannelContext = ChannelContext;

	Controller = (PSUNXI_DMA_CONTROLLER) ControllerContext;
	switch (FunctionNumber) {
	case DMA_OP_START:
		SunxiStartDma(Controller, ChannelNumber);
			break;
	case DMA_OP_PAUSE:
		SunxiPauseDma(Controller, ChannelNumber);
		break;
	case DMA_OP_RESUME:
		SunxiResumeDma(Controller, ChannelNumber);
		break;
	case DMA_OP_STOP:
		SunxiStopDma(Controller, ChannelNumber);
		break;
	case DMA_OP_GET_CONFIGURE:
		SunxiGetDmaConfig(ChannelContext);
		break;
	case DMA_OP_ENABLE_IRQ:
		SunxiEnableDmaIrq(Controller, ChannelContext);
		break;
	case DMA_OP_GET_STATUS:
		*(ULONG *) ChannelContext->parg = CspDmaGetStatus(Controller, ChannelContext->ChannelNumber);
		break;
	case DMA_OP_GET_CUR_SRC_ADDR:
		*(ULONG *) ChannelContext->parg = CspDmaGetCurSrcAddr(Controller, ChannelContext->ChannelNumber);
		break;
	case DMA_OP_GET_CUR_DST_ADDR:
		*(ULONG *) ChannelContext->parg = CspDmaGetCurDstAddr(Controller, ChannelContext->ChannelNumber);
		break;
	case DMA_OP_GET_BYTECNT_LEFT:
		*(ULONG *) ChannelContext->parg = CspDmaGetLeftByteCnt(Controller, ChannelContext->ChannelNumber);
		break;
	default:
		Status = STATUS_NOT_IMPLEMENTED;
	}

	Status = STATUS_SUCCESS;
	FunctionExit(Status);
	return Status;
}

VOID
AwFlushChannel(
	__in PVOID ControllerContext,
	__in ULONG ChannelNumber
)

/*++

Routine Description:

	This routine flushes a previous transfer from a channel and returns the
	channel to a state ready for the next ProgramChannel call.

Arguments:

	ControllerContext - Supplies a pointer to the controller's internal data.

	ChannelNumber - Supplies the channel to flush.

Return Value:

	None.

--*/

{
	UNREFERENCED_PARAMETER(ControllerContext);
	UNREFERENCED_PARAMETER(ChannelNumber);
	DbgPrint_T("Flash channel %d\n",ChannelNumber);

}

BOOLEAN
AwHandleInterrupt(
	__in PVOID ControllerContext,
	__out PULONG ChannelNumber,
	__out PDMA_INTERRUPT_TYPE InterruptType
)

/*++

Routine Description:

	This routine probes a controller for interrupts, clears any interrupts
	found, fills in channel and interrupt type information.  This routine
	will be called repeatedly until FALSE is returned.

Arguments:

	ControllerContext - Supplies a pointer to the controller's internal data.

	ChannelNumber - Supplies a placeholder for the extension to fill in which
	channel is interrupting.

	InterruptType - Supplies a placeholder for the extension to fill in the
	interrupt type.

Return Value:

	TRUE if an interrupt was found on this controller.

FALSE otherwise.

--*/

{

	PSUNXI_DMA_CONTROLLER Controller;
	ULONG Index;
	ULONG Val;
	Controller = (PSUNXI_DMA_CONTROLLER) ControllerContext;
	PSUNXI_DMA_CHANNEL pCurrentChannel;

	for (Index = 0; Index < Controller->ChannelCount; Index++) {
		Val = CspDmaGetIrqPend(Controller, Index);
		if ((Val & DMA_IRQ_QD)&&(Controller->ChannelInfo[Index].IrqType & DMA_IRQ_QD)) {
			CspDmaClearIrqPend(Controller, Index, DMA_IRQ_QD);
			pCurrentChannel = &(Controller->ChannelInfo[Index]);
			if (pCurrentChannel->IsLoopTransfer) {
				pCurrentChannel->CurrentElementId++;
				if (pCurrentChannel->CurrentElementId == pCurrentChannel->MemoryAddressList.NumberOfElements) {
					pCurrentChannel->CurrentElementId = 0;
					pCurrentChannel->Loopcounter++;
				}
				CspDmaStop(Controller, Index);
				CspDmaSetStartAddr(Controller, Index, pCurrentChannel->DesBufferPhysicalAddress+(ULONG)(pCurrentChannel->CurrentElementId*sizeof(TRANSFER_DES)));
				CspDmaClearIrqPend(Controller, Index, DMA_IRQ_HD | DMA_IRQ_FD | DMA_IRQ_QD);
				DumpDmaCommonReg(ControllerContext);
				DumpDmaChannelReg(ControllerContext, Index);
				CspDmaStart(Controller, Index);	
				
			}
			goto Done;
		}
		
	}

	return FALSE;

Done:	
	*ChannelNumber = Index;
	*InterruptType = InterruptTypeCompletion;
	return TRUE;

}

ULONG AwReadDmaCounter(
	__in PVOID ControllerContext,
	__in ULONG ChannelNumber
)

/*++

Routine Description:

	This routine determines how many bytes remain to be transferred on the
	given channel.  If the current transfer is set to loop, this routine
	will return the number of bytes remaining in the current iteration.

Arguments:

	ControllerContext - Supplies a pointer to the controller's internal data.

	ChannelNumber - Supplies the channel number.

Return Value:

	Returns the number of bytes remaining to be transferred on the given
	channel.

--*/

{

	PSUNXI_DMA_CONTROLLER Controller;
	Controller = (PSUNXI_DMA_CONTROLLER) ControllerContext;
	PSUNXI_DMA_CHANNEL pCurrentChannel;
	ULONG i,Size = 0;
	ULONG Pos;
	BOOLEAN Count = FALSE;
	PTRANSFER_DES pDes;
	ULONG PhyDes;
	FunctionEnter();

	pCurrentChannel = &(Controller->ChannelInfo[ChannelNumber]);

	if (pCurrentChannel->IsLoopTransfer)
	{
		for (i = pCurrentChannel->CurrentElementId; i < pCurrentChannel->MemoryAddressList.NumberOfElements; i++)
		{
			Size += pCurrentChannel->MemoryAddressList.Elements[i].Length;
		}
		return Size;
	}

	pDes = (PTRANSFER_DES)(pCurrentChannel->DesBufferVirtualAddress);
	PhyDes =(pCurrentChannel->DesBufferPhysicalAddress);
	Pos = CspDmaGetStartAddr(Controller, ChannelNumber);
	Size = CspDmaGetLeftByteCnt(Controller, ChannelNumber);;
	/* It is the last package, and just read count register */
	if (Pos == DMA_END_DES_LINK)
		return Size;

	for (i = 0; pDes->bcnt; pDes++, i++) {
		/* Ok, found next lli that is ready be transported */
		if ((PhyDes+(i*sizeof(TRANSFER_DES))) == Pos) {
			Count = TRUE;
			continue;
		}

		if (Count)
			Size += pDes->bcnt;
			
	}

	return Size;
	
}

VOID AwReportCommonBuffer(
	_In_  PVOID ControllerContext,
	_In_  ULONG ChannelNumber,
	_In_  PVOID VirtualAddress,
	_In_  PHYSICAL_ADDRESS PhysicalAddress
	)
{
	PSUNXI_DMA_CONTROLLER Controller;
	FunctionEnter();

	Controller = (PSUNXI_DMA_CONTROLLER)ControllerContext;
	NT_ASSERT(ChannelNumber <=Controller->ChannelCount);

	Controller = (PSUNXI_DMA_CONTROLLER)ControllerContext;

	Controller->ChannelInfo[ChannelNumber].DesBufferVirtualAddress = VirtualAddress;
	Controller->ChannelInfo[ChannelNumber].DesBufferPhysicalAddress= PhysicalAddress.LowPart;

	DbgPrint_V("AwReportCommonBuffer: ChannelNumber %d, Va 0x%p, Pa 0x%x\n",
	ChannelNumber, VirtualAddress, PhysicalAddress.LowPart);
}

NTSTATUS
RegisterDmaController(
	__in ULONG Handle,
	__in PCSRT_RESOURCE_GROUP_HEADER ResourceGroup,
	__in PCSRT_RESOURCE_DESCRIPTOR_HEADER ResourceDescriptor,
	__out PULONG ControllerId
)

/*++

Routine Description:

	This routine takes a DMA resource descriptor with subtype controller.
	The controller is then registered with the HAL.

Arguments:

	Handle - Supplies handle passed to extension.

	ResourceGroup - Supplies resource group containing this descriptor.

	ResourceDescrpitor - Supplies the resource descriptor.

	ControllerId - Supplies a placeholder for the controller ID returned from
	the HAL after registration.

Return Value:

	NTSTATUS Value.

--*/

{
	NTSTATUS Status;
	PRESOURCE_DMA_CONTROLLER ControllerResource;
	SUNXI_DMA_CONTROLLER Controller;	
	DMA_INITIALIZATION_BLOCK DmaInitBlock;
	ULONG i;

	FunctionEnter();
	RtlZeroMemory(&Controller, sizeof(SUNXI_DMA_CONTROLLER));

	ControllerResource = (PRESOURCE_DMA_CONTROLLER) ResourceDescriptor;

	Controller.ControllerBasePa.QuadPart = (ControllerResource->BasePhysicalAddressHight << 8) | (ControllerResource->BasePhysicalAddressLow);
	Controller.ChannelCount = ControllerResource->ChannelCount;
	Controller.MappingSize = ControllerResource->MappingSize;
	Controller.MinimumRequestLine = 0;
	Controller.MaximumRequestLine = Controller.ChannelCount;

	INITIALIZE_DMA_HEADER(&DmaInitBlock);
	DmaInitBlock.ChannelCount = Controller.ChannelCount;
	DmaInitBlock.MinimumTransferUnit = 1;
	DmaInitBlock.MinimumRequestLine = Controller.MinimumRequestLine;
	DmaInitBlock.MaximumRequestLine = Controller.MaximumRequestLine;
	DmaInitBlock.CacheCoherent = TRUE;
	DmaInitBlock.GeneratesInterrupt = TRUE;
	DmaInitBlock.InternalData = (PVOID) &Controller;
	DmaInitBlock.InternalDataSize = sizeof(SUNXI_DMA_CONTROLLER);
	DmaInitBlock.DmaAddressWidth = 32;
	DmaInitBlock.Gsi = ControllerResource->InterruptGsi;
	DmaInitBlock.InterruptPolarity = ControllerResource->InterruptPolarity;
	DmaInitBlock.InterruptMode = ControllerResource->InterruptMode;
	DmaInitBlock.Operations = &AwFunctionTable;
	DmaInitBlock.ControllerId = ControllerResource->Header.Uid;

	for (i = 0; i < Controller.ChannelCount; i++) {
		Controller.ChannelInfo[i].DmaChannelConfig = ControllerResource->DmaChannelConfig[i];
		Controller.ChannelInfo[i].IrqType = DMA_IRQ_HD | DMA_IRQ_FD|DMA_IRQ_QD;
	}
	//
	// Register physical address space with the HAL.
	//

	HalRegisterPermanentAddressUsage(Controller.ControllerBasePa,
		ControllerResource->MappingSize);

	//
	// Register controller.
	//

	Status = RegisterResourceDescriptor(Handle,
		ResourceGroup,
		ResourceDescriptor,
		&DmaInitBlock);

	*ControllerId = DmaInitBlock.ControllerId;
	return Status;

}

NTSTATUS
RegisterDmaChannel(
	__in ULONG Handle,
	__in PCSRT_RESOURCE_GROUP_HEADER ResourceGroup,
	__in PCSRT_RESOURCE_DESCRIPTOR_HEADER ResourceDescriptor,
	__in ULONG ControllerId
)

/*++

Routine Description:

	This routine takes a DMA resource descriptor with subtype channel.
	The channel is then registered with the HAL.

Arguments:

	Handle - Supplies handle passed to extension.

	ResourceGroup - Supplies resource group containing this descriptor.

	ResourceDescrpitor - Supplies the resource descriptor.

	ControllerId - Supplies the controller ID this channel is registered with.

Return Value:

	NTSTATUS Value.

--*/

{

	DMA_CHANNEL_INITIALIZATION_BLOCK DmaChannelInitBlock;
	PRD_DMA_CHANNEL DmaDesc;

	NT_ASSERT(ControllerId != 0);


	DmaDesc = (PRD_DMA_CHANNEL) ResourceDescriptor;
	INITIALIZE_DMA_CHANNEL_HEADER(&DmaChannelInitBlock);
	DmaChannelInitBlock.ControllerId = ControllerId;
	DmaChannelInitBlock.GeneratesInterrupt = FALSE;
	DmaChannelInitBlock.ChannelNumber = DmaDesc->ChannelNumber;
	DmaChannelInitBlock.CommonBufferLength = SUNXI_DMA_DES_BUFFER_SIZE_PER_CHANNEL;

	return RegisterResourceDescriptor(Handle,
		ResourceGroup,
		ResourceDescriptor,
		&DmaChannelInitBlock);
}

NTSTATUS
AddResourceGroup(
	__in ULONG Handle,
	__in PCSRT_RESOURCE_GROUP_HEADER ResourceGroup
)

{

	ULONG ControllerId;
	PCSRT_RESOURCE_DESCRIPTOR_HEADER ResourceDescriptor;
	FunctionEnter();
	ResourceDescriptor = NULL;
	ControllerId = 0;
	for (;;) {
		ResourceDescriptor =
			GetNextResourceDescriptor(Handle,
			ResourceGroup,
			ResourceDescriptor,
			CSRT_RD_TYPE_DMA,
			CSRT_RD_SUBTYPE_ANY,
			CSRT_RD_UID_ANY);

		if (ResourceDescriptor == NULL) {
			break;
		}

		if (ResourceDescriptor->Subtype == CSRT_RD_SUBTYPE_DMA_CONTROLLER) {
			RegisterDmaController(Handle,
				ResourceGroup,
				ResourceDescriptor,
				&ControllerId);

		}
		else if (ResourceDescriptor->Subtype == CSRT_RD_SUBTYPE_DMA_CHANNEL) {
			RegisterDmaChannel(Handle,
				ResourceGroup,
				ResourceDescriptor,
				ControllerId);
		}
		else {

			NT_ASSERT(FALSE);
		}
	}

	return STATUS_SUCCESS;
}
