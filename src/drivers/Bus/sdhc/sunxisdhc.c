/** @file
*
*  Copyright (c) 2007-2014, Allwinner Technology Co., Ltd. All rights reserved.
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
#include <ntddk.h>
#include <sdport.h>
#include "sddef.h"
#include "sunxisdhc.h"

#ifdef ALLOC_PRAGMA
    #pragma alloc_text(INIT, DriverEntry)
#endif

ULONG DebugTrace[SUNXI_SDMMC_EMMC + 1];

NTSTATUS
DriverEntry(
    _In_ PVOID DriverObject,
    _In_ PVOID RegistryPath
    )

/*++

Routine Description:

    This routine is the entry point for the standard sdhsot miniport driver.

Arguments:

    DriverObject - DriverObject for the standard host controller.

    RegistryPath - Registry path for this standard host controller.

Return Value:

    NTSTATUS

--*/

{

    SDPORT_INITIALIZATION_DATA InitializationData;
	
    RtlZeroMemory(&InitializationData, sizeof(InitializationData));
    InitializationData.StructureSize = sizeof(InitializationData);

    //
    // Initialize the entry points/callbacks for the miniport interface.
    //

    InitializationData.GetSlotCount = SunxiGetSlotCount;
    InitializationData.GetSlotCapabilities = SunxiGetSlotCapabilities;
    InitializationData.Initialize = SunxiSlotInitialize;
    InitializationData.IssueBusOperation = SunxiSlotIssueBusOperation;
    InitializationData.GetCardDetectState = SunxiSlotGetCardDetectState;
    InitializationData.GetWriteProtectState = SunxiSlotGetWriteProtectState;
    InitializationData.Interrupt = SunxiSlotInterrupt;
    InitializationData.IssueRequest = SunxiSlotIssueRequest;
    InitializationData.GetResponse = SunxiSlotGetResponse;
    InitializationData.ToggleEvents = SunxiSlotToggleEvents;
    InitializationData.ClearEvents = SunxiSlotClearEvents;
    InitializationData.RequestDpc = SunxiRequestDpc;
    InitializationData.SaveContext = SunxiSaveContext;
    InitializationData.RestoreContext = SunxiRestoreContext;
    InitializationData.PowerControlCallback = SunxiPoFxPowerControlCallback;
    InitializationData.PrivateExtensionSize = sizeof(SUNXI_EXTENSION);

    return SdPortInitialize(DriverObject, RegistryPath, &InitializationData);
}

NTSTATUS
SunxiGetSlotCount(
    _In_ PSD_MINIPORT Miniport,
    _Out_ PUCHAR SlotCount
    )

/*++

Routine Description:

    Return the number of slots present on this controller.

Arguments:

    Argument - Functional device object for this controller.

Return value:

    STATUS_UNSUCCESSFUL - PCI config space was unable to be queried.

    STATUS_INVALID_PARAMETER - Invalid underlying bus type for the miniport.

    STATUS_SUCCESS - SlotCount returned properly.

--*/

{
	SDPORT_BUS_TYPE BusType;
	NTSTATUS Status;

	*SlotCount = 0;
	BusType = Miniport->ConfigurationInfo.BusType;

	switch (BusType) {
	case SdBusTypeAcpi:
		*SlotCount = 1;
		Status = STATUS_SUCCESS;
		break;

	case SdBusTypePci:
		*SlotCount = 1;
		Status = STATUS_SUCCESS;
		break;

	default:

		NT_ASSERT((BusType == SdBusTypeAcpi) || (BusType == SdBusTypePci));

		*SlotCount = 1;
		Status = STATUS_INVALID_PARAMETER;
		break;
	}

	return Status;
}

VOID
SunxiGetSlotCapabilities(
    _In_ PVOID PrivateExtension,
    _Out_ PSDPORT_CAPABILITIES Capabilities
    )

/*++

Routine Description:

    Override for miniport to provide host register mapping information if the
    memory range provideed by the underlying bus isn't sufficient.

Arguments:

    SlotExtension - SlotExtension interface between sdhost and this miniport.

    Capabilities - Miniport provided capabilities.

Return value:

    STATUS_SUCCESS - Capabilities returned successfully.

--*/

{

    PSUNXI_EXTENSION SunxiExtension;

    SunxiExtension = (PSUNXI_EXTENSION) PrivateExtension;

    RtlCopyMemory(Capabilities, 
                  &SunxiExtension->Capabilities,
                  sizeof(SunxiExtension->Capabilities));
}

NTSTATUS
SunxiSlotInitialize(
    _In_ PVOID PrivateExtension,
    _In_ PHYSICAL_ADDRESS PhysicalBase,
    _In_ PVOID VirtualBase,
    _In_ ULONG Length,
    _In_ BOOLEAN CrashdumpMode
    )

/*++

Routine Description:

    Initialize the miniport for standard host controllers.

Arguments:

    SlotExtension - SlotExtension interface between sdhost and this miniport.

Return value:

    NTSTATUS

--*/

{

    PSDPORT_CAPABILITIES Capabilities;
	ULONG CurrentLimitMax;
//    UCHAR Index;
    PSUNXI_EXTENSION SunxiExtension;

    SunxiExtension = (PSUNXI_EXTENSION) PrivateExtension;

	SunxiExtension->CmdSendCnt = 0;

    //
    // Initialize the SUNXI_EXTENSION register space.
    //
    SunxiExtension->PhysicalBaseAddress = PhysicalBase;
    SunxiExtension->BaseAddress = VirtualBase;
    SunxiExtension->BaseAddressSpaceSize = Length;

	SunxiExtension->CrashdumpMode = CrashdumpMode;

    NT_ASSERT(PhysicalBase.HighPart == 0);
    switch (PhysicalBase.LowPart)
    {
        case 0x01c0f000:
            SunxiExtension->Port = SUNXI_SDMMC_SD_CARD;
			SunxiExtension->PrintControl = 0;//BIT(DPFLTR_ERROR_LEVEL);
			// SunxiExtension->PrintControl = BIT(DPFLTR_ERROR_LEVEL) | BIT(DPFLTR_WARNING_LEVEL)
			//                                 | BIT(DPFLTR_TRACE_LEVEL) | BIT(DPFLTR_INFO_LEVEL);
            break;

        case 0x01c10000:
            SunxiExtension->Port = SUNXI_SDMMC_SDIO;
			SunxiExtension->PrintControl = BIT(DPFLTR_ERROR_LEVEL);
            SunxiExtension->PrintControl = 0 
				 | BIT(DPFLTR_ERROR_LEVEL) | BIT(DPFLTR_WARNING_LEVEL)
                                             // | BIT(DPFLTR_INFO_LEVEL)
                                             // | BIT(DPFLTR_TRACE_LEVEL)
			;
			//__debugbreak();
            break;

        case 0x01c11000:
            SunxiExtension->Port = SUNXI_SDMMC_EMMC;
            SunxiExtension->PrintControl = BIT(DPFLTR_ERROR_LEVEL); // | BIT(DPFLTR_WARNING_LEVEL);
            // SunxiExtension->PrintControl = BIT(DPFLTR_ERROR_LEVEL) | BIT(DPFLTR_WARNING_LEVEL)
            //                                 | BIT(DPFLTR_TRACE_LEVEL) | BIT(DPFLTR_INFO_LEVEL);
            break;

        default:
            SdPrintError("[%s] Error: PhysicalBase=%#x\n", __FUNCTION__, PhysicalBase);
    }

    Capabilities = (PSDPORT_CAPABILITIES) &SunxiExtension->Capabilities;
    RtlZeroMemory(Capabilities, sizeof(SDPORT_CAPABILITIES));

    if (IS_SD_CARD(SunxiExtension)) {
        SunxiExtension->DmaThreshold = SUNXI_DMA_TL_SDMMC0;
        SunxiExtension->Dat3Imask = 0; // TODO
        SunxiExtension->SdioImask = 0; // TODO
        SunxiExtension->DmaDesSizeBits = SUNXI_DES_SIZE_SDMMC0;
        SunxiExtension->SunxiSetThldCtl = SunxiSetThldCtl_0;

        Capabilities->BaseClockFrequencyKhz = BASE_CLOCK_FREQUENCY_KHZ;

		Capabilities->PioTransferMaxThreshold = 64;
	    Capabilities->Flags.UsePioForRead = FALSE;
	    Capabilities->Flags.UsePioForWrite = FALSE;
		Capabilities->Supported.BusWidth8Bit = 0;
    }
	
    if (IS_SDIO(SunxiExtension)) {
        SunxiExtension->DmaThreshold = SUNXI_DMA_TL_SDMMC1;
        SunxiExtension->Dat3Imask = 0; // TODO
        SunxiExtension->SdioImask = SDXC_SDIO_INTERRUPT; // TODO
        SunxiExtension->DmaDesSizeBits = SUNXI_DES_SIZE_SDMMC1;
        SunxiExtension->SunxiSetThldCtl = SunxiSetThldCtl_1;

		SunxiAllocateDmaSDIO(SunxiExtension);

        Capabilities->BaseClockFrequencyKhz = BASE_CLOCK_FREQUENCY_KHZ;

		if(SunxiExtension->UseSdBuffer)
		{
			Capabilities->PioTransferMaxThreshold = 0;
	    	Capabilities->Flags.UsePioForRead = FALSE;
	    	Capabilities->Flags.UsePioForWrite = FALSE;
		}
		else
		{
			Capabilities->PioTransferMaxThreshold = 4;
	    	Capabilities->Flags.UsePioForRead = FALSE;
	    	Capabilities->Flags.UsePioForWrite = TRUE;
		}
		Capabilities->Supported.BusWidth8Bit = 0;
    }
	
    if (IS_MMC_CARD(SunxiExtension)) {
        SunxiExtension->DmaThreshold = SUNXI_DMA_TL_SDMMC2;
        SunxiExtension->Dat3Imask = 0; // TODO
        SunxiExtension->SdioImask = 0; // TODO
        SunxiExtension->DmaDesSizeBits = SUNXI_DES_SIZE_SDMMC2;
        SunxiExtension->SunxiSetThldCtl = SunxiSetThldCtl_2;

        Capabilities->BaseClockFrequencyKhz = BASE_CLOCK_FREQUENCY_KHZ;

		Capabilities->PioTransferMaxThreshold = 64;
	    Capabilities->Flags.UsePioForRead = FALSE;
	    Capabilities->Flags.UsePioForWrite = FALSE;
		Capabilities->Supported.BusWidth8Bit = 1;
    }

    //
    // Initialize host capabilities.
    //
    Capabilities->SpecVersion = 0x04; // Versinon 3.00

    // Read and Write
    Capabilities->Supported.AutoCmd12 = 1;
    Capabilities->Supported.AutoCmd23 = 0;

    Capabilities->MaximumBlockSize = 4096; // 4KB
    Capabilities->MaximumBlockCount = 8192;

    Capabilities->Supported.ScatterGatherDma = 1;
    Capabilities->DmaDescriptorSize = sizeof(struct SunxiDmaDescriptor);
    Capabilities->AlignmentRequirement = sizeof(ULONG) - 1;
    Capabilities->Supported.Address64Bit = 0;

    Capabilities->Supported.HighSpeed = 1;
    // Capabilities->BaseClockFrequencyKhz = BASE_CLOCK_FREQUENCY_KHZ;


    // Voltage and Speed-mode. (Temporary setting)
    Capabilities->Supported.Voltage18V = 0;
    Capabilities->Supported.Voltage30V = 0;
    Capabilities->Supported.Voltage33V = 1;
    Capabilities->Supported.SignalingVoltage18V = 0;
    Capabilities->Supported.SDR50 = 0;
    Capabilities->Supported.DDR50 = 0;
    Capabilities->Supported.SDR104 = 0;
    Capabilities->Supported.HS200 = 0;
    Capabilities->Supported.HS400 = 0;
    
    // Current
    CurrentLimitMax = 1000; // TODO: Depended
    if (CurrentLimitMax >= 800) {
        Capabilities->Supported.Limit800mA = 1;
    }
    if (CurrentLimitMax >= 600) {
        Capabilities->Supported.Limit600mA = 1;
    }
    if (CurrentLimitMax >= 400) {
        Capabilities->Supported.Limit400mA = 1;
    }
    if (CurrentLimitMax >= 200) {
        Capabilities->Supported.Limit200mA = 1;
    }

    // Tuning
    Capabilities->Supported.SoftwareTuning = 0;
    Capabilities->Supported.TuningForSDR50 = 1;
    Capabilities->TuningTimerCountInSeconds = 0;

    Capabilities->Supported.DriverTypeA = 0;
    Capabilities->Supported.DriverTypeB = 0;
    Capabilities->Supported.DriverTypeC = 0;
    Capabilities->Supported.DriverTypeD = 0;

    // Other
    Capabilities->Supported.SaveContext = 0;

    //
    // Initialize our array of outstanding requests to keep track
    // of what operations are in flight.
    //
    Capabilities->MaximumOutstandingRequests = SDHC_MAX_OUTSTANDING_REQUESTS;

	SunxiInitializeDbgLog(SunxiExtension);
	
    return STATUS_SUCCESS;
}

NTSTATUS
SunxiSlotIssueBusOperation(
    _In_ PVOID PrivateExtension,
    _In_ PSDPORT_BUS_OPERATION BusOperation
    )

/*++

Routine Description:

    Issue host bus operation specified by BusOperation.

Arguments:

    SlotExtension - SlotExtension interface between sdhost and this miniport.

    BusOperation - Bus operation to perform.

Return value:

    NTSTATUS - Return value of the bus operation performed.

--*/

{

    PSUNXI_EXTENSION SunxiExtension;
    NTSTATUS Status;

    SunxiExtension = (PSUNXI_EXTENSION) PrivateExtension;

	SunxiAddDbgLog(SunxiExtension, 'BusO', BusOperation->Type, BusOperation->Parameters.ResetType, 0);

    Status = STATUS_INVALID_PARAMETER;
    switch (BusOperation->Type) {
    case SdResetHw:
		 Status = SunxiResetHw(SunxiExtension, BusOperation->Parameters.ResetType);
         break;
		
	case SdResetHost:
		 //SdPortWait(10);
		 Status = STATUS_SUCCESS;
         break;

    case SdSetClock:
        Status = SunxiSetClock(SunxiExtension,
                              BusOperation->Parameters.FrequencyKhz);
        break;

    case SdSetVoltage:
        Status = SunxiSetVoltage(SunxiExtension,
                                BusOperation->Parameters.Voltage);

        break;

    case SdSetBusWidth:
        Status = SunxiSetBusWidth(SunxiExtension,
                                 BusOperation->Parameters.BusWidth);

        break;

    case SdSetBusSpeed:
        Status = SunxiSetSpeed(SunxiExtension, BusOperation->Parameters.BusSpeed);
        break;

    case SdSetSignalingVoltage:
        Status = SunxiSetSignaling(SunxiExtension, 
                                  BusOperation->Parameters.SignalingVoltage);

        break;

    case SdSetDriveStrength:
        //Status = SunxiSetDriveStrength(SunxiExtension,
        //                            BusOperation->Parameters.DriveStrength);
        break;

    case SdSetDriverType:
        //Status = SunxiSetDriverType(SunxiExtension,
        //                         BusOperation->Parameters.DriverType);

         break;

    case SdSetPresetValue:
         break;

    case SdSetBlockGapInterrupt:
         break;

    case SdExecuteTuning:
         break;

    default:
        Status = STATUS_INVALID_PARAMETER;
        break; 
    }

    return Status;
}

BOOLEAN
SunxiSlotGetCardDetectState(
    _In_ PVOID PrivateExtension
    )

/*++

Routine Description:

    Determine whether a card is inserted in the slot.

Arguments:

    SlotExtension - SlotExtension interface between sdhost and this miniport.

Return value:

    TRUE - Card is inserted.

    FALSE - Slot is empty.

--*/

{
	PSUNXI_EXTENSION SunxiExtension;

	SunxiExtension = (PSUNXI_EXTENSION)PrivateExtension;

    return TRUE; // TODO: to fix
}

BOOLEAN
SunxiSlotGetWriteProtectState(
    _In_ PVOID PrivateExtension
    )

/*++

Routine Description:

    Determine whether the slot write protection is engaged.

Arguments:

    SlotExtension - SlotExtension interface between sdhost and this miniport.

Return value:

    TRUE - Slot is write protected.

    FALSE - Slot is writable.

--*/

{

    PSUNXI_EXTENSION SunxiExtension;

    SunxiExtension = (PSUNXI_EXTENSION) PrivateExtension;

    return SunxiIsWriteProtected(SunxiExtension);
}

ULONG
SunxiGetErrorStatus (
    _In_ ULONG Err
    )
{
    ULONG Ret = 0;

    if ((Err & SDXC_RESP_ERROR) || (Err & SDXC_RESP_CRC_ERROR))
            Ret |= SDHC_ES_CMD_CRC_ERROR;

    if (Err & SDXC_DATA_CRC_ERROR)
            Ret |= SDHC_ES_DATA_CRC_ERROR;

    if (Err & SDXC_RESP_TIMEOUT)
            Ret |= SDHC_ES_CMD_TIMEOUT;

    if ((Err & SDXC_DATA_TIMEOUT) || (Err & SDXC_HARD_WARE_LOCKED) || (Err & SDXC_FIFO_RUN_ERROR))
            Ret |= SDHC_ES_DATA_TIMEOUT;

    if ((Err & SDXC_END_BIT_ERROR) || (Err & SDXC_START_BIT_ERROR))
            Ret |= SDHC_ES_CMD_END_BIT_ERROR;

    return Ret;
}

BOOLEAN
SunxiSlotInterrupt(
    _In_ PVOID PrivateExtension,
    _Out_ PULONG Events,
    _Out_ PULONG Errors,
    _Out_ PBOOLEAN NotifyCardChange,
    _Out_ PBOOLEAN NotifySdioInterrupt,
    _Out_ PBOOLEAN NotifyTuning
    )

/*++

Routine Description:

    Level triggered DIRQL interrupt handler (ISR) for this controller.

Arguments:

    SlotExtension - SlotExtension interface between sdhost and this miniport.

Return value:

    Whether the interrupt is handled.

--*/

{
    PSUNXI_EXTENSION SunxiExtension;
    ULONG RawInterruptStatus, MaskInterruptStatus, DmaStatus, Err=0;

    UNREFERENCED_PARAMETER(NotifyTuning);
    UNREFERENCED_PARAMETER(NotifyCardChange);

    SunxiExtension = (PSUNXI_EXTENSION) PrivateExtension;
	if ((IS_SDIO(SunxiExtension)) 
		//|| (IS_SD_CARD(SunxiExtension))
		)
	{
		return SunxiSlotInterruptSDIO(
			    PrivateExtension,
			    Events,
			    Errors,
			    NotifyCardChange,
			    NotifySdioInterrupt,
			    NotifyTuning
			    );
	}
	
    DmaStatus = SunxiReadRegisterUlong(SunxiExtension, SDXC_REG_IDST);
    MaskInterruptStatus = SunxiReadRegisterUlong(SunxiExtension, SDXC_REG_MISTA);
    RawInterruptStatus = SunxiReadRegisterUlong(SunxiExtension, SDXC_REG_RINTR);

	SunxiWriteRegisterUlong(SunxiExtension, SDXC_REG_RINTR, MaskInterruptStatus);
	SunxiWriteRegisterUlong(SunxiExtension, SDXC_REG_IDST, DmaStatus);

	SunxiAddDbgLog(SunxiExtension, 'Intr', MaskInterruptStatus, RawInterruptStatus, DmaStatus);

    if ((MaskInterruptStatus == 0) && (DmaStatus == 0))
        return FALSE;

    if (MaskInterruptStatus || DmaStatus){

        if (DmaStatus & SDXC_IDMAC_RECEIVE_INTERRUPT)
            SunxiExtension->ReadWaitDma = FALSE;

		if ((IS_SDIO(SunxiExtension)) && (DmaStatus & SDXC_IDMAC_TRANSMIT_INTERRUPT))
            SunxiExtension->ReadWaitDma = FALSE;

        SunxiExtension->IntrBak |= MaskInterruptStatus;

        // if cmd rsp timeout, enable and wait for command done irq
        if ((SunxiExtension->IntrBak & SDXC_RESP_TIMEOUT) &&
            !(SunxiExtension->IntrBak & SDXC_COMMAND_DONE)) {
            SunxiWriteRegisterUlong(SunxiExtension, SDXC_REG_IMASK, SunxiExtension->SdioImask
                    | SunxiExtension->Dat3Imask | SDXC_COMMAND_DONE);
        /* Don't wait for dma on error */
        } else if (SunxiExtension->IntrBak & SDXC_INTERRUPT_ERROR_BIT) {

            Err = (ULONG) SunxiGetErrorStatus(SunxiExtension->IntrBak & SDXC_INTERRUPT_ERROR_BIT);
        } else if ((SunxiExtension->IntrBak & SDXC_INTERRUPT_DONE_BIT)
                && !SunxiExtension->ReadWaitDma) {
            if (SunxiExtension->IntrBak & SDXC_COMMAND_DONE)
                *Events = SDHC_IS_CMD_COMPLETE; // cmd is completed
            else
                *Events = SDHC_IS_TRANSFER_COMPLETE; // data transfer is completed
        }
		
    }

	*Errors = Err;

    return ((*Events != 0) || Err);
}

NTSTATUS
SunxiSlotIssueRequest(
    _In_ PVOID PrivateExtension,
    _In_ PSDPORT_REQUEST Request
    )

/*++

Routine Description:

    Issue hardware request specified by Request.

Arguments:

    PrivateExtension - This driver's device extension (SunxiExtension).

    Request - Request operation to perform.

Return value:

    NTSTATUS - Return value of the request performed.

--*/

{

//    ULONG Index;
    PSUNXI_EXTENSION SunxiExtension;
    NTSTATUS Status;

    SunxiExtension = (PSUNXI_EXTENSION) PrivateExtension;

    //
    // Dispatch the request based off of the request type.
    //

	//SunxiAddDbgLog(SunxiExtension, 'Requ', Request->Type, Request->Command.TransferType, Request->Command.TransferMethod);

    switch (Request->Type) {
    case SdRequestTypeCommandNoTransfer:
    case SdRequestTypeCommandWithTransfer:
        Status = SunxiSendCmd(SunxiExtension, Request);
        break;

    case SdRequestTypeStartTransfer:
        Status = SunxiStartTransfer(SunxiExtension, Request);
        break;

    default:
        Status = STATUS_NOT_SUPPORTED;
        break;
    }

    return Status;
}

VOID
SunxiSlotGetResponse(
    _In_ PVOID PrivateExtension,
    _In_ PSDPORT_COMMAND Command,
    _Out_ PVOID ResponseBuffer
    )

/*++

Routine Description:

    Return the response data for a given command back to the port driver.

Arguments:

    PrivateExtension - This driver's device extension (SunxiExtension).

    Command - Command for which we're getting the response.

    ResponseBuffer - Response data for the given command.

Return value:

    None.

--*/

{

    PSUNXI_EXTENSION SunxiExtension;
    PULONG Response;
	SD_RW_DIRECT_ARGUMENT Argument;
	char D;

    SunxiExtension = (PSUNXI_EXTENSION) PrivateExtension;
    // SdPrintInfoEx(SunxiExtension, "Enter\n");

    if ((Command->ResponseType == SdResponseTypeUndefined)
            || (Command->ResponseType == SdResponseTypeNone))
        return ; // do nothing

    Response = (PULONG) ResponseBuffer;
    if (Command->ResponseType == SdResponseTypeR2) {
		ULONG Cnt;
		UCHAR temp;
		PUCHAR buf;
		Response[0] = SunxiReadRegisterUlong(SunxiExtension, SDXC_REG_RESP0);
		Response[1] = SunxiReadRegisterUlong(SunxiExtension, SDXC_REG_RESP1);
		Response[2] = SunxiReadRegisterUlong(SunxiExtension, SDXC_REG_RESP2);
		Response[3] = SunxiReadRegisterUlong(SunxiExtension, SDXC_REG_RESP3);
		if (((Command->Index & 0x3F) == 9) && (SunxiExtension->Port == SUNXI_SDMMC_SD_CARD))
		{
			// Skip 1 byte for SD CSD register
			buf = (PUCHAR)ResponseBuffer;
			temp = buf[0];
			for (Cnt = 0; Cnt < 16-1; Cnt++)
			{
				buf[Cnt] = buf[Cnt + 1];
			}
			buf[15] = temp;
		}

		SunxiAddDbgLog(SunxiExtension, 'Re4 ', Command->Index, Command->Argument, Response[0]);
		SunxiAddDbgLog(SunxiExtension, 'Re4 ', Response[1], Response[2], Response[3]);
    } 
	else 
	{
        Response[0] = SunxiReadRegisterUlong(SunxiExtension, SDXC_REG_RESP0);
		SunxiAddDbgLog(SunxiExtension, 'Re1 ', Command->Index, Command->Argument, Response[0]);
//#if 0		
		if ((IS_SDIO(SunxiExtension)))
		{
			if ((Command->Index & 0x3f) == 52)
			{
				Argument.u.AsULONG = Command->Argument;
				D = (Command->TransferDirection == SdTransferDirectionRead)?'R':'W';
				SdPrintInfoEx(SunxiExtension, "%c52 A %d R %x\n", D, Argument.u.bits.Address, (Response[0])&0xff);	
				SunxiAddDbgLog(SunxiExtension, 'Re52', Argument.u.bits.Address, Argument.u.bits.WriteToDevice, Response[0]&0xff);
			}
			if ((Command->Index & 0x3f) == 5)
			{
				if(SunxiExtension->SdioRespCmd5 == 0)
				{
					SunxiExtension->SdioRespCmd5 = Response[0];
				}
				if(Response[0] != SunxiExtension->SdioRespCmd5)
				{
					SdPrintErrorEx(SunxiExtension, "C5R RO %x RW %x\n", (Response[0]), SunxiExtension->SdioRespCmd5);	
					Response[0] = SunxiExtension->SdioRespCmd5;
				}
			}	
		}
//#endif
	}
}

// NTSTATUS
ULONG
__SunxiWaitDat0Busy (
    _In_ PSUNXI_EXTENSION SunxiExtension,
    _In_ ULONG MaxCnt,
    _In_ ULONG Unit
    )
{
	ULONG Expire;
    ULONG Rval;

	if (!Unit)
		Unit = 1;
	
    Expire = MaxCnt;
    do {
        Rval = SunxiReadRegisterUlong(SunxiExtension, SDXC_REG_STAS);
        if (!(Rval & SDXC_CARD_DATA_BUSY))
            break;

        SdPortWait(Unit);
        Expire--;
    } while (Expire);

    if (Rval & SDXC_CARD_DATA_BUSY) {
		SdPrintErrorEx(SunxiExtension,
			"Error: Wait data0 busy timeout\n");
        // return STATUS_IO_DEVICE_ERROR;
        return MaxCnt - Expire;
    }

	// return STATUS_SUCCESS;
    return MaxCnt - Expire;
}

NTSTATUS
SunxiWaitDat0Busy(
    _In_ PSUNXI_EXTENSION SunxiExtension,
    _In_ PSDPORT_COMMAND Command,
    _In_ BOOLEAN IsError
    )
{
    ULONG CmdIndex = Command->Index & 0x3f;
    ULONG WaitCnt, Unit = 1, MaxCnt;
    NTSTATUS Status = STATUS_SUCCESS;

    if ((CmdIndex == 24) || (CmdIndex == 25)) // for data_write cmd
        Unit = 100;

    if (IsError)
        MaxCnt = 10000;
    else
        MaxCnt = 100000;

    WaitCnt = __SunxiWaitDat0Busy(SunxiExtension, MaxCnt, Unit);

    if (WaitCnt == MaxCnt) {
        SdPrintErrorEx(SunxiExtension, "Cmd%d, arg %#x, blksz=%d, blk=%d, RawInterruptStatus=%#x\n",
                CmdIndex, Command->Argument, Command->BlockSize,
                Command->BlockCount, SunxiReadRegisterUlong(SunxiExtension, SDXC_REG_RINTR));
        Status = STATUS_IO_DEVICE_ERROR;
    } else {
        static ULONG WaitCntMax_Unit1 = 0, WaitCntMax_Unit100 = 0;
        ULONG Flag = FALSE;
        PCHAR Str = "";

        if (Unit == 1) {
            if (WaitCnt > WaitCntMax_Unit1) {
                WaitCntMax_Unit1 = WaitCnt;
                if (WaitCntMax_Unit1 > 400)
                    Flag = TRUE;
            }
        }
        else {
            if (WaitCnt > WaitCntMax_Unit100) {
                WaitCntMax_Unit100 = WaitCnt;
                if (WaitCntMax_Unit100 > 9000)
                    Flag = TRUE;
                Str = "00";
            }
        }

        if (Flag)
            SdPrintErrorEx(SunxiExtension, "Cmd%d, arg %#x, blksz=%d, blk=%d, WaitCntMax_Unit1%s=%d\n",
                    CmdIndex, Command->Argument, Command->BlockSize, Command->BlockCount, Str, WaitCnt);
    }

    return Status;
}

#define SDIO_CCCR_ABORT		        0x06	/* function abort/card reset   */
#define MMC_STOP_TRANSMISSION       12      /* ac                      R1b */
#define SD_IO_RW_DIRECT             52      /* ac   [31:0] See below   R5  */
#define SD_IO_RW_EXTENDED           53      /* adtc [31:0] See below   R5  */
static VOID
SunxiSendStop(
        _In_ PSUNXI_EXTENSION SunxiExtension,
        _In_ PSDPORT_COMMAND Command
        )
{
	LONG Arg, Cmd, Expire, Rval;

	Cmd = SDXC_START | SDXC_RESP_EXPECT
	     | SDXC_STOP_ABORT_CMD | SDXC_CHECK_RESPONSE_CRC;

	if ((Command->Index & 0x3f) == SD_IO_RW_EXTENDED) {
		Cmd |= SD_IO_RW_DIRECT;
		Arg = (1 << 31) | (0 << 28) | (SDIO_CCCR_ABORT << 9) |
		    ((Command->Argument >> 28) & 0x7);
	} else {
		Cmd |= MMC_STOP_TRANSMISSION;
		Arg = 0;
	}

	SunxiWriteRegisterUlong(SunxiExtension, SDXC_REG_CARG, Arg);
	SunxiWriteRegisterUlong(SunxiExtension, SDXC_REG_CMDR, Cmd);

    Expire = 1000;
    do {
        Rval = SunxiReadRegisterUlong(SunxiExtension, SDXC_REG_RINTR);
        if (Rval & (SDXC_COMMAND_DONE | SDXC_INTERRUPT_ERROR_BIT))
            break;

        SdPortWait(1);
        Expire--;
    } while (Expire);

    if (!(Rval & SDXC_COMMAND_DONE) || (Rval & SDXC_INTERRUPT_ERROR_BIT)) {
        SdPrintErrorEx(SunxiExtension, "Failed to send stop cmd%d, arg %x, Rval=%x, "
                "RawInterruptStatus=%x\n",
                Cmd & 0x3f, Arg, Rval, SunxiReadRegisterUlong(SunxiExtension, SDXC_REG_RINTR));
	} else
        SdPrintErrorEx(SunxiExtension, "Cmd%d, arg %x, RawInterruptStatus=%x, Expire=%d\n",
                Cmd & 0x3f, Arg, SunxiReadRegisterUlong(SunxiExtension, SDXC_REG_RINTR), 1000 - Expire);

	SunxiWriteRegisterUlong(SunxiExtension, SDXC_REG_RINTR, 0xffff);
}

VOID
SunxiRequestDpc(
    _In_ PVOID PrivateExtension,
    _Inout_ PSDPORT_REQUEST Request,
    _In_ ULONG Events,
    _In_ ULONG Errors
    )

/*++

Routine Description:

    DPC for interrupts associated with the given request.

Arguments:

    PrivateExtension - This driver's device extension (SunxiExtension).

    Request - Request operation to perform.

Return value:

    NTSTATUS - Return value of the request performed.

--*/

{
    PSUNXI_EXTENSION SunxiExtension;
    NTSTATUS Status = STATUS_SUCCESS;
	ULONG CmdIndex; 
	PSDPORT_COMMAND Command = &Request->Command;
	//PSCATTER_GATHER_ELEMENT SglistElement;
	//ULONG Item = 0;
	//ULONG Offset = 0;
	//PULONG Address = 0;
	//PVOID VirtualAddress;

    SunxiExtension = (PSUNXI_EXTENSION) PrivateExtension;
	if ((IS_SDIO(SunxiExtension)) 
		//|| (IS_SD_CARD(SunxiExtension))
		)
	{
		 SunxiRequestDpcSDIO(
		    	PrivateExtension,
		    	Request,
		    	Events,
		    	Errors
		    	);
		 return;
	}
    CmdIndex = Command->Index & 0x3f; 

	SunxiAddDbgLog(SunxiExtension, 'DPCE', Request->RequiredEvents, Events, Errors);

    if ((Events & (SDHC_IS_CMD_COMPLETE | SDHC_IS_TRANSFER_COMPLETE))
            || (Errors)) { // cmd done or errors happen
        //
        // Clear the request's required events if they have completed.
        //
        Request->RequiredEvents &= ~Events;
		
        SunxiWriteRegisterUlong(SunxiExtension, SDXC_REG_IMASK,
			SunxiExtension->SdioImask | SunxiExtension->Dat3Imask);
        SunxiWriteRegisterUlong(SunxiExtension, SDXC_REG_IDIE, 0);

        if (Errors) { // error
        	Request->RequiredEvents = 0;
            Status = SunxiConvertErrorToStatus((USHORT) Errors);
        } else 
        {
            // wait busy for busy cmd and data_write cmd.
			{
	            if ((Command->ResponseType == SdResponseTypeR1B) 
	                    || (Command->ResponseType == SdResponseTypeR5B) // busy cmd
	                    || ((Command->TransferType != SdTransferTypeNone) // data write
	                        && (Command->TransferDirection == SdTransferDirectionWrite)))
	                Status = SunxiWaitDat0Busy(SunxiExtension, Command, FALSE);
			}			 
        }

        Request->Status = Status;
        SunxiWriteRegisterUlong(SunxiExtension, SDXC_REG_RINTR, 0xffff); // clear raw interrupt status

        SunxiExtension->IntrBak = 0;
        SunxiExtension->ReadWaitDma = FALSE;

        if (Errors && (Command->TransferType != SdTransferTypeNone)) { // data transfer
            SunxiWaitDat0Busy(SunxiExtension, Command, TRUE);
            SunxiSendStop(SunxiExtension, Command); // CMD12
        }

		{
        	SdPortCompleteRequest(Request, Status);
		}
    }
	
}

VOID
SunxiSlotToggleEvents(
    _In_ PVOID PrivateExtension,
    _In_ ULONG EventMask,
    _In_ BOOLEAN Enable
    )

/*++

Routine Description:

    Enable or disable the given event mask.

Arguments:

    SlotExtension - SlotExtension interface between sdhost and this miniport.

    Events - The event mask to toggle.

    Enable - TRUE to enable, FALSE to disable.

Return value:

    None.

--*/

{
	PSUNXI_EXTENSION SunxiExtension;
	USHORT Mask = 0;

	SunxiExtension = (PSUNXI_EXTENSION)PrivateExtension;

    UNREFERENCED_PARAMETER(EventMask);
    UNREFERENCED_PARAMETER(Enable);

    SdPrintInfoEx(SunxiExtension, "EventMask=%#x, Enable=%d\n", EventMask, Enable);
	
	if(EventMask & SDPORT_EVENT_CARD_INTERRUPT)
	{
		if(Enable)
		{
			Mask = 1;
		}
		else
		{
			Mask = 0;
		}
		SunxiWriteRegisterUshort(SunxiExtension, SDXC_REG_IMASK + 2, Mask);

	}
}

VOID
SunxiSlotClearEvents(
    _In_ PVOID PrivateExtension,
    _In_ ULONG EventMask
    )

{
	PSUNXI_EXTENSION SunxiExtension;

	SunxiExtension = (PSUNXI_EXTENSION)PrivateExtension;

    UNREFERENCED_PARAMETER(EventMask);

    SdPrintInfoEx(SunxiExtension, "EventMask=%#x\n", EventMask);
	SunxiWriteRegisterUlong(SunxiExtension, SDXC_REG_RINTR, EventMask);
}

VOID
SunxiSaveContext(
    _In_ PVOID PrivateExtension
    )

/*++

Routine Description:

    Save slot register context.

Arguments:

    SlotExtension - SlotExtension interface between sdhost and this miniport.

Return value:

    None.

--*/

{
	PSUNXI_EXTENSION SunxiExtension;

	SunxiExtension = (PSUNXI_EXTENSION)PrivateExtension;
}

VOID
SunxiRestoreContext(
    _In_ PVOID PrivateExtension
    )

/*++

Routine Description:

    Restore slot register context from a previously saved context.

Arguments:

    SlotExtension - SlotExtension interface between sdhost and this miniport.

Return value:

    None.

--*/

{
	PSUNXI_EXTENSION SunxiExtension;

	SunxiExtension = (PSUNXI_EXTENSION)PrivateExtension;
}

NTSTATUS
SunxiPoFxPowerControlCallback(
    _In_ PSD_MINIPORT Miniport,
    _In_ LPCGUID PowerControlCode,
    _In_reads_bytes_opt_(InputBufferSize) PVOID InputBuffer,
    _In_ SIZE_T InputBufferSize,
    _Out_writes_bytes_opt_(OutputBufferSize) PVOID OutputBuffer,
    _In_ SIZE_T OutputBufferSize,
    _Out_opt_ PSIZE_T BytesReturned
    )

/*++

Routine Description:

    Handle any PoFxPowerControl callbacks.

Arguments:

    Miniport - Miniport interface for the controller.

    PowerControlCode - GUID defining a platform-specific PoFxPowerControl
                       method.

    InputBuffer - Buffer containing any input arguments.

    InputBufferSize - Size of InputBuffer in bytes.

    OutputBuffer - Buffer containing any output results.

    OutputBufferSize - Size of OutputBuffer in bytes.

    BytesReturned - Number of bytes returned.

Return value:

    NTSTATUS

--*/

{

    UNREFERENCED_PARAMETER(Miniport);
    UNREFERENCED_PARAMETER(PowerControlCode);
    UNREFERENCED_PARAMETER(InputBuffer);
    UNREFERENCED_PARAMETER(InputBufferSize);
    UNREFERENCED_PARAMETER(OutputBuffer);
    UNREFERENCED_PARAMETER(OutputBufferSize);
    UNREFERENCED_PARAMETER(BytesReturned);
    return STATUS_NOT_IMPLEMENTED;
}
    

//-----------------------------------------------------------------------------
// Host routine implementations.
//-----------------------------------------------------------------------------

NTSTATUS
SunxiSoftResetDmaCtl(
    _In_ PSUNXI_EXTENSION SunxiExtension
    )
{
    UCHAR Expire;
    ULONG Rval;

    Expire = 250;
	Rval = SunxiReadRegisterUlong(SunxiExtension, SDXC_REG_DMAC);
	SunxiWriteRegisterUlong(SunxiExtension, SDXC_REG_DMAC, Rval | SDXC_IDMAC_SOFT_RESET);
    do {
        Rval = SunxiReadRegisterUlong(SunxiExtension, SDXC_REG_DMAC);
        if (!(Rval & SDXC_IDMAC_SOFT_RESET))
            break;

        SdPortWait(1);
        Expire--;
    } while (Expire);
    if (Expire == 0) {
        return STATUS_TIMEOUT;
    }

    if (Rval & SDXC_IDMAC_SOFT_RESET) {
		SdPrintErrorEx(SunxiExtension,
			"fatal err reset dma contol timeout\n");
        return STATUS_IO_DEVICE_ERROR;
    }

	return STATUS_SUCCESS;
}


_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS
SunxiResetHw(
	_In_ PSUNXI_EXTENSION SunxiExtension,
	_In_ UCHAR ResetType
)

{
	UCHAR Expire;
	ULONG Rval;
	NTSTATUS Status;
	//KIRQL OldIrql;

	UNREFERENCED_PARAMETER(ResetType);

	Expire = 250;
	SunxiWriteRegisterUlong(SunxiExtension, SDXC_REG_GCTRL, SDXC_HARDWARE_RESET);
	do {
		Rval = SunxiReadRegisterUlong(SunxiExtension, SDXC_REG_GCTRL);
		if (!(Rval & SDXC_HARDWARE_RESET))
			break;

		SdPortWait(1);
		Expire--;
	} while (Expire);
	if (Expire == 0) {
		return STATUS_TIMEOUT;
	}

	if (Rval & SDXC_HARDWARE_RESET) {
		SdPrintErrorEx(SunxiExtension, "fatal err reset timeout\n");
		return STATUS_IO_DEVICE_ERROR;
	}

	Status = SunxiSoftResetDmaCtl(SunxiExtension);
	if (!NT_SUCCESS(Status)) {
		return Status;
	}
	// Set the max HW timeout for bus operations.
	SunxiWriteRegisterUlong(SunxiExtension, SDXC_REG_TMOUT, 0xffffffff);
	SunxiWriteRegisterUlong(SunxiExtension, SDXC_REG_DBGC, 0xdeb); // Turn on controller debug mode
	//OldIrql = KeGetCurrentIrql();
	//KeAcquireSpinLock(&SunxiExtension->IntSpinLock, &OldIrql);
	SunxiWriteRegisterUlong(SunxiExtension, SDXC_REG_IMASK,
		SunxiExtension->SdioImask | SunxiExtension->Dat3Imask 
		| SDXC_INTERRUPT_ERROR_BIT | SDXC_DATA_OVER | SDXC_COMMAND_DONE | SDXC_VOLTAGE_CHANGE_DONE);
	//KeReleaseSpinLock(&SunxiExtension->IntSpinLock, OldIrql);
	SunxiWriteRegisterUlong(SunxiExtension, SDXC_REG_FTRGL,
		SunxiExtension->DmaThreshold ? SunxiExtension->DmaThreshold : 0x20070008);
	SunxiWriteRegisterUlong(SunxiExtension, SDXC_REG_RINTR, 0xffffffff); // clear all raw interrupt status

	Rval = SunxiReadRegisterUlong(SunxiExtension, SDXC_REG_GCTRL);
	Rval |= SDXC_INTERRUPT_ENABLE_BIT; // enable interrupt
	Rval &= ~SDXC_ACCESS_DONE_DIRECT;
	if (SunxiExtension->Dat3Imask) {
		Rval |= SDXC_DEBOUNCE_ENABLE_BIT;
	}
	SunxiWriteRegisterUlong(SunxiExtension, SDXC_REG_GCTRL, Rval);

	// set 1 bit bus width
	Rval = SunxiReadRegisterUlong(SunxiExtension, SDXC_REG_WIDTH);
	Rval &= ~(0x03);
	SunxiWriteRegisterUlong(SunxiExtension, SDXC_REG_WIDTH, Rval);

	SunxiWriteRegisterUlong(SunxiExtension, SDXC_REG_A12A, 0);

	return STATUS_SUCCESS;
}



#define SDXC_REG_DL_NEW 0x5c
static void
SunxiMmcSetClkDlyChain (
    _In_ PSUNXI_EXTENSION SunxiExtension,
    _In_ ULONG Frequency
    )
{
    ULONG Reg;
	ULONG cmd_drv_ph	= 1;
	ULONG dat_drv_ph	= 0;
    ULONG sam_dly	= 0;
    ULONG ds_dly	= 0;

    UNREFERENCED_PARAMETER(Frequency);

    if (IS_SD_CARD(SunxiExtension)||IS_SDIO(SunxiExtension)) {
        Reg = SunxiReadRegisterUlong(SunxiExtension,SDXC_REG_DL_NEW);
        Reg |= (1 << 31);
		Reg &= ~(0x3 << 4);
        SunxiWriteRegisterUlong(SunxiExtension,SDXC_REG_DL_NEW,Reg);
        return ; // TODO
    }

	Reg = SunxiReadRegisterUlong(SunxiExtension, SDXC_REG_DRV_DL);
	if(cmd_drv_ph){
		Reg |= SDXC_CMD_DRV_PH_SEL;//180 phase
	}else{
		Reg &= ~SDXC_CMD_DRV_PH_SEL;//90 phase
	}

	if(dat_drv_ph){
		Reg |= SDXC_DAT_DRV_PH_SEL;//180 phase
	}else{
		Reg &= ~SDXC_DAT_DRV_PH_SEL;//90 phase
	}
	SunxiWriteRegisterUlong(SunxiExtension, SDXC_REG_DRV_DL, Reg);

    if (Frequency > CLOCK_25MHZ)
        sam_dly = 0x1c;

	Reg = SunxiReadRegisterUlong(SunxiExtension, SDXC_REG_SAMP_DL);
	Reg &= ~SDXC_SAMP_DL_SW_MASK;
	Reg |= sam_dly & SDXC_SAMP_DL_SW_MASK;
	Reg |= SDXC_SAMP_DL_SW_EN;
	SunxiWriteRegisterUlong(SunxiExtension,SDXC_REG_SAMP_DL,Reg);

	Reg = SunxiReadRegisterUlong(SunxiExtension,SDXC_REG_DS_DL);
	Reg &= ~SDXC_DS_DL_SW_MASK;
	Reg |= ds_dly & SDXC_DS_DL_SW_MASK;
	Reg |= SDXC_DS_DL_SW_EN;
	SunxiWriteRegisterUlong(SunxiExtension,SDXC_REG_DS_DL,Reg);
}

static NTSTATUS
SunxiClkSwitch (
    _In_ PSUNXI_EXTENSION SunxiExtension,
    _In_ ULONG TurnOn,
    _In_ ULONG PwrSave,
    _In_ ULONG IgnoreDat0
    )
{
    UCHAR Expire;
    ULONG Reg;

    Reg = SunxiReadRegisterUlong(SunxiExtension, SDXC_REG_CLKCR);
	if (TurnOn)
	{
		Reg |= SDXC_CARD_CLOCK_ON;
	}
	else
	{
		Reg &= ~SDXC_CARD_CLOCK_ON;
	}
	if(PwrSave)
		Reg |= SDXC_LOW_POWER_ON;
	if(IgnoreDat0)
		Reg |= SDXC_MASK_DATA0;
    SunxiWriteRegisterUlong(SunxiExtension, SDXC_REG_CLKCR, Reg);

    Reg = (ULONG)(SDXC_START | SDXC_UPCLK_ONLY | SDXC_WAIT_PRE_OVER);
    SunxiWriteRegisterUlong(SunxiExtension, SDXC_REG_CMDR, Reg);

    Expire = 250;
    do {
        Reg = SunxiReadRegisterUlong(SunxiExtension, SDXC_REG_CMDR);
        if (!(Reg & SDXC_START))
            break;

        SdPortWait(1);
        Expire--;
    } while (Expire);

    /* clear irq status bits set by the command */
    SunxiWriteRegisterUlong(SunxiExtension, SDXC_REG_RINTR, 
            (SunxiReadRegisterUlong(SunxiExtension, SDXC_REG_RINTR) 
             & ~(SDXC_SDIO_INTERRUPT | SDXC_CARD_INSERT | SDXC_CARD_REMOVE)));

    if (Reg & SDXC_START) {
        SdPrintErrorEx(SunxiExtension, "fatal err update clk timeout\n");
        return STATUS_IO_DEVICE_ERROR;
	}
    
    /*only use mask data0 when update clk, clear it when not update clk*/
	if (IgnoreDat0 && TurnOn)
	{
		SunxiWriteRegisterUlong(SunxiExtension, SDXC_REG_CLKCR, 
			SunxiReadRegisterUlong(SunxiExtension, SDXC_REG_CLKCR) & ~SDXC_MASK_DATA0);
	}

	if(TurnOn)
	{
		SdPortWait(100);
	}
    return STATUS_SUCCESS;
}

VOID
SunxiXmodOnOff (
    _In_ PSUNXI_EXTENSION SunxiExtension,
    _In_ ULONG NewModeEnable)
{
	ULONG Ret = SunxiReadRegisterUlong(SunxiExtension, SDXC_REG_SD_NTSR);

	if(NewModeEnable){
		Ret |= SDXC_2X_TIMING_MODE;
	}else{
		Ret &= ~SDXC_2X_TIMING_MODE;
	}
	SunxiWriteRegisterUlong(SunxiExtension, SDXC_REG_SD_NTSR, Ret);
}

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS
SunxiSetClock(
    _In_ PSUNXI_EXTENSION SunxiExtension,
    _In_ ULONG Frequency
    )

/*++

Routine Description:

    Set the clock to a given frequency.

Arguments:

    SunxiExtension - Host controller specific driver context.

    Frequency - The target frequency.

Return value:

    STATUS_SUCCESS - The clock was successfuly set.

    STATUS_TIMEOUT - The clock did not stabilize in time.

--*/

{
    ULONG SrcFreq = SunxiExtension->Capabilities.BaseClockFrequencyKhz; // TODO: fix me
    ULONG Reg;
    ULONG Div;
	ULONG FrequencyLimit = CLOCK_52MHZ;

	SdPrintInfoEx(SunxiExtension, "**Frequency=%dKHz\n", Frequency);
	
	if (Frequency == 0) {
		return 0;
	}

    // Ture off clock before setting freq
    SunxiClkSwitch(SunxiExtension, FALSE, FALSE, TRUE);

	if (Frequency > FrequencyLimit)
        Frequency = FrequencyLimit;
	
	if (Frequency > SrcFreq)
        Frequency = SrcFreq;
	
    if (Frequency >= SrcFreq)
        Div = 0;
    else {
        Div = (SrcFreq + Frequency - 1) / Frequency;
        Div = (Div + 1) / 2;
    }
    Div = MIN(Div, 255);
    Reg = SunxiReadRegisterUlong(SunxiExtension, SDXC_REG_CLKCR);
    Reg &= ~0xff;
    Reg |= (Div & 0xff);
    SunxiWriteRegisterUlong(SunxiExtension, SDXC_REG_CLKCR, Reg);
    SdPrintInfoEx(SunxiExtension, "ActualFrequency=%dKHz, Src=%dKHz, Div=%d, Reg=%#x\n", Frequency, SrcFreq, Div, Reg);

    if (IS_MMC_CARD(SunxiExtension)) {
        if ((SunxiExtension->BusWidth == 8)
                && (SunxiExtension->SpeedMode == SdBusSpeedHS400)) {
            Reg = SunxiReadRegisterUlong(SunxiExtension, SDXC_REG_EDSD);
            Reg |= SDXC_HS400_MD_EN;
            SunxiWriteRegisterUlong(SunxiExtension, SDXC_REG_EDSD, Reg);

            Reg = SunxiReadRegisterUlong(SunxiExtension, SDXC_REG_CSDC);
            Reg &= ~SDXC_CRC_DET_PARA_MASK;
            Reg |= SDXC_CRC_DET_PARA_HS400;
            SunxiWriteRegisterUlong(SunxiExtension, SDXC_REG_CSDC, Reg);
        } else {
            Reg = SunxiReadRegisterUlong(SunxiExtension, SDXC_REG_EDSD);
            Reg &= ~ SDXC_HS400_MD_EN;
            SunxiWriteRegisterUlong(SunxiExtension, SDXC_REG_EDSD, Reg);

            Reg = SunxiReadRegisterUlong(SunxiExtension, SDXC_REG_CSDC);
            Reg &= ~SDXC_CRC_DET_PARA_MASK;
            Reg |= SDXC_CRC_DET_PARA_OTHER;
            SunxiWriteRegisterUlong(SunxiExtension, SDXC_REG_CSDC, Reg);
        }
    }

    if (IS_SD_CARD(SunxiExtension))
        SunxiXmodOnOff(SunxiExtension, TRUE);
    
    SunxiMmcSetClkDlyChain(SunxiExtension, Frequency);

    return SunxiClkSwitch(SunxiExtension, TRUE, FALSE, TRUE);
}

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS
SunxiSetVoltage(
    _In_ PSUNXI_EXTENSION SunxiExtension,
    _In_ SDPORT_BUS_VOLTAGE Voltage
    )

/*++

Routine Description:

    Set the slot's voltage profile.

Arguments:

    SunxiExtension - Host controller specific driver context.

    VoltageProfile - Indicates which power voltage to use. If 0, turn off the
                     power

Return value:

    STATUS_SUCCESS - The bus voltage was successfully switched.

    STATUS_INVALID_PARAMETER - The voltage profile provided was invalid.

    STATUS_TIMEOUT - The bus voltage did not stabilize in time.

--*/

{

    UNREFERENCED_PARAMETER(SunxiExtension);
    UNREFERENCED_PARAMETER(Voltage);
	
	SdPrintInfoEx(SunxiExtension, "Voltage=%d\n", Voltage);

    return STATUS_SUCCESS;
}

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS
SunxiSetBusWidth(
    _In_ PSUNXI_EXTENSION SunxiExtension,
    _In_ UCHAR Width
    )

/*++

Routine Description:

    Set bus width for host controller.

Arguments:

    SunxiExtension - Host controller specific driver context.

    Width - The data bus width of the slot.

Return value:

    STATUS_SUCCESS - The bus width was properly changed.

--*/

{
	SdPrintInfoEx(SunxiExtension, "Width=%d\n", Width);
	
    SunxiExtension->BusWidth = Width;
    switch (Width) {
    case 1:
		SunxiWriteRegisterUlong(SunxiExtension, SDXC_REG_WIDTH, SDXC_WIDTH1);
        break;

    case 4:
		SunxiWriteRegisterUlong(SunxiExtension, SDXC_REG_WIDTH, SDXC_WIDTH4);
        break;

    case 8:
		SunxiWriteRegisterUlong(SunxiExtension, SDXC_REG_WIDTH, SDXC_WIDTH8);
        break;

    default:
        SunxiExtension->BusWidth = 0;
        NT_ASSERTMSG("SDHC - Provided bus width is invalid", FALSE);
        break;
    }

    return STATUS_SUCCESS;
}

NTSTATUS
SunxiSetSpeed(
    _In_ PSUNXI_EXTENSION SunxiExtension,
    _In_ SDPORT_BUS_SPEED Speed
    )

/*++

Routine Description:

    Based on the capabilities of card and host, turns on the maximum performing
    speed mode for the host. Caller is expected to know the capabilities of the
    card before setting the speed mode.

Arguments:

    SunxiExtension - Host controller specific driver context.

    Speed - Bus speed to set.

Return value:

    STATUS_SUCCESS - The selected speed mode was successfully set.

    STATUS_INVALID_PARAMETER - The speed mode selected is not valid.

--*/

{

    NTSTATUS Status = STATUS_SUCCESS;
    ULONG Rval;

	SdPrintInfoEx(SunxiExtension, "Speed=%d\n", Speed);
	
    SunxiExtension->SpeedMode = Speed;
    Rval = SunxiReadRegisterUlong(SunxiExtension, SDXC_REG_GCTRL);
    switch (Speed) {
    case SdBusSpeedDDR50:
        Rval |= SDXC_DDR_MODE;
        break;

    case SdBusSpeedNormal:
		// SunxiSetClock(SunxiExtension, 50*1000); 
		// break;
		
    case SdBusSpeedHigh:
    case SdBusSpeedSDR12:
    case SdBusSpeedSDR25:
    case SdBusSpeedSDR50:
    case SdBusSpeedSDR104:
    case SdBusSpeedHS200:
    case SdBusSpeedHS400:
        Rval &= ~SDXC_DDR_MODE;
        break;

    default:
        SunxiExtension->SpeedMode = SdBusSpeedUndefined;
        NT_ASSERTMSG("SDHC - Invalid speed mode selected.", FALSE);
        Status = STATUS_INVALID_PARAMETER;
        break;
    }
    SunxiWriteRegisterUlong(SunxiExtension, SDXC_REG_GCTRL, Rval);

    return Status;
}

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS
SunxiSetSignaling(
    _In_ PSUNXI_EXTENSION SunxiExtension,
    _In_ BOOLEAN Enable
    )

/*++

Routine Description:

    Set signaling voltage (1.8V or 3.3V).

Arguments:

    SunxiExtension - Host controller specific driver context.

    Enable - TRUE for 1.8V signaling, FALSE for default 3.3V.

Return value:

    STATUS_SUCCESS - Signaling voltage switch successful.

    STATUS_UNSUCCESSFUL - Signaling voltage switch unsuccessful.

--*/

{
    UNREFERENCED_PARAMETER(SunxiExtension);
    UNREFERENCED_PARAMETER(Enable);

    return STATUS_SUCCESS;
}

BOOLEAN
SunxiIsWriteProtected(
    _In_ PSUNXI_EXTENSION SunxiExtension
    )
{

    UNREFERENCED_PARAMETER(SunxiExtension);

    return FALSE; 
}

VOID
SunxiSetThldCtl_0 (
    _In_ PVOID PrivateExtension,
    _In_ PSDPORT_COMMAND Command
    )
{
    PSUNXI_EXTENSION SunxiExtension = (PSUNXI_EXTENSION) PrivateExtension;
	ULONG BlockSize = Command->BlockSize;
	ULONG RdTl = ((SunxiExtension->DmaThreshold & SDXC_RX_TL_MASK)>>16)<<2;//unit:byte
	ULONG Ret = 0;

    if ((Command->TransferDirection == SdTransferDirectionRead) // read
		&& (BlockSize <= SDXC_CARD_RD_THLD_SIZE)
		&& ((SDXC_FIFO_DETH<<2) >= (RdTl+BlockSize))      //((SDXC_FIFO_DETH<<2)-BlockSize) >= (RdTl)
		&& (SunxiExtension->SpeedMode == SdBusSpeedHS200)) {
		Ret = SunxiReadRegisterUlong(SunxiExtension,SDXC_REG_THLD);
		Ret &= ~SDXC_CARD_RD_THLD_MASK;
		Ret |= BlockSize << SDXC_CARD_RD_THLD_SIZE_SHIFT;
		Ret |= SDXC_CARD_RD_THLD_ENB;
		SunxiWriteRegisterUlong(SunxiExtension,SDXC_REG_THLD,Ret);
	} else {
		Ret = SunxiReadRegisterUlong(SunxiExtension,SDXC_REG_THLD);
		Ret &= ~SDXC_CARD_RD_THLD_ENB;
		SunxiWriteRegisterUlong(SunxiExtension,SDXC_REG_THLD,Ret);
	}
}

VOID
SunxiSetThldCtl_1 (
    _In_ PVOID PrivateExtension,
    _In_ PSDPORT_COMMAND Command
    )
{
    PSUNXI_EXTENSION SunxiExtension = (PSUNXI_EXTENSION) PrivateExtension;
	ULONG BlockSize = Command->BlockSize;
	ULONG RdTl = ((SunxiExtension->DmaThreshold & SDXC_RX_TL_MASK)>>16)<<2;//unit:byte
	ULONG Ret = 0;

    if ((Command->TransferDirection == SdTransferDirectionRead) // read
		&& (BlockSize <= SDXC_CARD_RD_THLD_SIZE)
		&& ((SDXC_FIFO_DETH<<2) >= (RdTl+BlockSize))      //((SDXC_FIFO_DETH<<2)-BlockSize) >= (RdTl)
		&& ((SunxiExtension->SpeedMode == SdBusSpeedHS200)
            || (SunxiExtension->SpeedMode == SdBusSpeedSDR50)
			|| (SunxiExtension->SpeedMode == SdBusSpeedSDR104))) {
		Ret = SunxiReadRegisterUlong(SunxiExtension,SDXC_REG_THLD);
		Ret &= ~SDXC_CARD_RD_THLD_MASK;
		Ret |= BlockSize << SDXC_CARD_RD_THLD_SIZE_SHIFT;
		Ret |= SDXC_CARD_RD_THLD_ENB;
		SunxiWriteRegisterUlong(SunxiExtension,SDXC_REG_THLD,Ret);
	} else {
		Ret = SunxiReadRegisterUlong(SunxiExtension,SDXC_REG_THLD);
		Ret &= ~SDXC_CARD_RD_THLD_ENB;
		SunxiWriteRegisterUlong(SunxiExtension,SDXC_REG_THLD,Ret);
	}
}

VOID
SunxiSetThldCtl_2 (
    _In_ PVOID PrivateExtension,
    _In_ PSDPORT_COMMAND Command
    )
{
    PSUNXI_EXTENSION SunxiExtension = (PSUNXI_EXTENSION) PrivateExtension;
	ULONG BlockSize = Command->BlockSize;
	ULONG TdTl = (SunxiExtension->DmaThreshold & SDXC_TX_TL_MASK)<<2;		//unit:byte
	ULONG RdTl = ((SunxiExtension->DmaThreshold & SDXC_RX_TL_MASK)>>16)<<2;//unit:byte
	ULONG Ret = 0;


    if ((Command->TransferDirection == SdTransferDirectionWrite) // write
		&& (BlockSize <= SDXC_CARD_RD_THLD_SIZE)
		&& (BlockSize <= TdTl) ){
		Ret = SunxiReadRegisterUlong(SunxiExtension,SDXC_REG_THLD);
		Ret &=~SDXC_CARD_RD_THLD_MASK;
		Ret |= BlockSize << SDXC_CARD_RD_THLD_SIZE_SHIFT;
		Ret |= SDXC_CARD_WR_THLD_ENB;
		SunxiWriteRegisterUlong(SunxiExtension,SDXC_REG_THLD,Ret);
	}else{
		Ret = SunxiReadRegisterUlong(SunxiExtension,SDXC_REG_THLD);
		Ret &= ~SDXC_CARD_WR_THLD_ENB;
		SunxiWriteRegisterUlong(SunxiExtension,SDXC_REG_THLD,Ret);
	}

    if ((Command->TransferDirection == SdTransferDirectionRead) // read
		&& (BlockSize <= SDXC_CARD_RD_THLD_SIZE)
		&& ((SDXC_FIFO_DETH<<2) >= (RdTl+BlockSize))      //((SDXC_FIFO_DETH<<2)-BlockSize) >= (RdTl)
		&& ((SunxiExtension->SpeedMode == SdBusSpeedHS200)
			||(SunxiExtension->SpeedMode == SdBusSpeedHS400))) {
		Ret = SunxiReadRegisterUlong(SunxiExtension,SDXC_REG_THLD);
		Ret &= ~SDXC_CARD_RD_THLD_MASK;
		Ret |= BlockSize << SDXC_CARD_RD_THLD_SIZE_SHIFT;
		Ret |= SDXC_CARD_RD_THLD_ENB;
		SunxiWriteRegisterUlong(SunxiExtension,SDXC_REG_THLD,Ret);
	} else {
		Ret = SunxiReadRegisterUlong(SunxiExtension,SDXC_REG_THLD);
		Ret &= ~SDXC_CARD_RD_THLD_ENB;
		SunxiWriteRegisterUlong(SunxiExtension,SDXC_REG_THLD,Ret);
	}
}

NTSTATUS
SunxiStartDma (
    _In_ PSUNXI_EXTENSION SunxiExtension,
    _In_ PSDPORT_COMMAND Command
    )
{
    NTSTATUS Status = STATUS_SUCCESS;
    ULONG Rval;

	// Reset FIFO and DMA control
	Rval = SunxiReadRegisterUlong(SunxiExtension, SDXC_REG_GCTRL);
	Rval &= ~SDXC_ACCESS_BY_AHB;
	Rval |= SDXC_DMA_ENABLE_BIT;
	SunxiWriteRegisterUlong(SunxiExtension, SDXC_REG_GCTRL, Rval | SDXC_DMA_RESET);

	Rval = SunxiReadRegisterUlong(SunxiExtension, SDXC_REG_DMAC);
	SunxiWriteRegisterUlong(SunxiExtension, SDXC_REG_DMAC, Rval | SDXC_IDMAC_SOFT_RESET);

	if(IS_SDIO(SunxiExtension) && (Command->TransferDirection == SdTransferDirectionWrite))
	{	
		Status = SunxiCreateAdmaDescriptorTableSDIOForWrite(SunxiExtension, Command);	
	}
	else
	{
    	Status = SunxiCreateAdmaDescriptorTable(SunxiExtension, Command);
	}
    if (!NT_SUCCESS(Status)) {
        return Status;
    }

    if (Command->TransferDirection == SdTransferDirectionRead) // read
    {
        SunxiWriteRegisterUlong(SunxiExtension, SDXC_REG_IDIE, 0x0000014 | SDXC_IDMAC_RECEIVE_INTERRUPT); // SDXC_IDMAC_RECEIVE_INTERRUPT
    }

	if (Command->TransferDirection == SdTransferDirectionWrite) // write
	{
    	SunxiWriteRegisterUlong(SunxiExtension, SDXC_REG_IDIE, 0x0000014 | SDXC_IDMAC_TRANSMIT_INTERRUPT); // SDXC_IDMAC_RECEIVE_INTERRUPT
	}

    SunxiWriteRegisterUlong(SunxiExtension, SDXC_REG_DMAC, (ULONG)(SDXC_IDMAC_FIX_BURST | SDXC_IDMAC_IDMA_ON | SDXC_IDMAC_REFETCH_DES));

    return Status;
}

NTSTATUS
SunxiSendCmd(
    _In_ PSUNXI_EXTENSION SunxiExtension,
    _In_ PSDPORT_REQUEST Request
    )

/*++

Routine Description:

    This routine takes the SD command package and writes it to the appropriate
    registers on the host controller. It also computes the proper flag
    settings.

Arguments:

    SunxiExtension - Host controller specific driver context.

    Request - Supplies the descriptor for this SD command

Return value:

    STATUS_SUCCESS - Command successfully sent.

    STATUS_INVALID_PARAMETER - Invalid command response type specified.

--*/

{

    PSDPORT_COMMAND Command = &Request->Command;
    NTSTATUS Status;
    ULONG Reg;
	ULONG InterruptMask = SDXC_INTERRUPT_ERROR_BIT;
    UCHAR CmdIndex = Command->Index & 0x3f;
	ULONG CmdReg = SDXC_START | CmdIndex;

    NT_ASSERT(Command->TransferType != SdTransferTypeUndefined);

	if (IS_SDIO(SunxiExtension) 
		//|| (IS_SD_CARD(SunxiExtension))
		)
    {
        return SunxiSendCmdSDIO(SunxiExtension, Request);
    }

	SunxiAddDbgLog(SunxiExtension, 'CMDS', Command->Index, Command->Argument, (ULONG)Request);
    
    // Let SD port handle the timeout
    if ((IS_MMC_CARD(SunxiExtension))&& 
		((CmdIndex == 5)|| ((CmdIndex == 8) && (Command->TransferType == SdTransferTypeNone))))
    {
    	SdPortCompleteRequest(Request, STATUS_IO_TIMEOUT);
        return STATUS_PENDING;
    }

    if (IS_SD_CARD(SunxiExtension) && (CmdIndex == 5))
    {
    	SdPortCompleteRequest(Request, STATUS_IO_TIMEOUT);
        return STATUS_PENDING;
    }

    if (IS_SDIO(SunxiExtension) && (CmdIndex == 8))
    {
    	SdPortCompleteRequest(Request, STATUS_IO_TIMEOUT);
        return STATUS_PENDING;
    }

	if (CmdIndex == 0) { // CMD0
		CmdReg |= SDXC_SEND_INIT_SEQUENCE;
		InterruptMask |= SDXC_COMMAND_DONE;
	}

    if (IS_MMC_CARD(SunxiExtension) && (CmdIndex == 19)) // CMD19
		InterruptMask &= ~SDXC_END_BIT_ERROR;

    switch (Command->ResponseType) {
        case SdResponseTypeNone:
            InterruptMask |= SDXC_COMMAND_DONE;
            break;

        case SdResponseTypeR1:
        case SdResponseTypeR5:
        case SdResponseTypeR6:
        case SdResponseTypeR1B:
        case SdResponseTypeR5B:
            CmdReg |= SDXC_RESP_EXPECT | SDXC_CHECK_RESPONSE_CRC;
            break;

        case SdResponseTypeR2:
            CmdReg |= SDXC_RESP_EXPECT | SDXC_CHECK_RESPONSE_CRC | SDXC_LONG_RESPONSE;
            break;

        case SdResponseTypeR3:
        case SdResponseTypeR4:
            CmdReg |= SDXC_RESP_EXPECT;
            break;

        default:
            NT_ASSERTMSG("SDHC - Invalid response type", FALSE);
            return STATUS_INVALID_PARAMETER;
    }
	
	SunxiExtension->ReadWaitDma = FALSE;
    if (Command->TransferType != SdTransferTypeNone) { // transfer with data
        if ((Request->Command.TransferDirection != SdTransferDirectionRead) &&
	        (Request->Command.TransferDirection != SdTransferDirectionWrite)) {

	        return STATUS_INVALID_PARAMETER;
	    }
		
        CmdReg |= SDXC_DATA_EXPECT | SDXC_WAIT_PRE_OVER;

        if (Command->TransferType == SdTransferTypeMultiBlock) { /// need stop
            InterruptMask |= SDXC_AUTO_COMMAND_DONE;
            CmdReg |= SDXC_SEND_AUTO_STOP;
        } else {
            InterruptMask |= SDXC_DATA_OVER;
        }

        if (Command->TransferDirection == SdTransferDirectionRead) // read
            SunxiExtension->ReadWaitDma = TRUE;
        else // write
        {
            CmdReg |= SDXC_WRITE;
        }


        SunxiWriteRegisterUlong(SunxiExtension, SDXC_REG_BLKSZ, Command->BlockSize);
		SunxiWriteRegisterUlong(SunxiExtension, SDXC_REG_BCNTR, Command->BlockSize * Command->BlockCount);

        if (Command->TransferMethod == SdTransferMethodPio) {
            Reg = SunxiReadRegisterUlong(SunxiExtension, SDXC_REG_GCTRL);
            Reg |= (SDXC_ACCESS_BY_AHB);
			Reg &= ~SDXC_DMA_ENABLE_BIT;
            SunxiWriteRegisterUlong(SunxiExtension, SDXC_REG_GCTRL, Reg);

			SunxiExtension->ReadWaitDma = FALSE;

			// Clear inter mask set before for PIO mode
			InterruptMask = 0;
            InterruptMask |= SDXC_COMMAND_DONE;
			if(Command->TransferDirection == SdTransferDirectionRead)
			{
				InterruptMask |= SDXC_RX_DATA_REQUEST;
			}
			if(Command->TransferDirection == SdTransferDirectionWrite)
			{
				InterruptMask |= SDXC_TX_DATA_REQUEST;
			}
        } 
		else 
		{ // SdTransferMethodSgDma
            if (SunxiExtension->SunxiSetThldCtl)
                SunxiExtension->SunxiSetThldCtl((PVOID) SunxiExtension, Command);
            Status = SunxiStartDma(SunxiExtension, Command);
            if (!NT_SUCCESS(Status)) 
			{
                return Status;
            }
        }        
    } else { // transfer without data
        InterruptMask |= SDXC_COMMAND_DONE;
    }

	SunxiExtension->CmdSendCnt++;

    {
        //
        // Set the bitmask for the required events that will fire after
        // writing to the command register. Depending on the response
        // type or whether the command involves data transfer, we will need
        // to wait on a number of different events.
        //
        Request->RequiredEvents = 0;
        if (InterruptMask & SDXC_COMMAND_DONE)
            Request->RequiredEvents |= SDHC_IS_CMD_COMPLETE;

        if (InterruptMask & (SDXC_DATA_OVER | SDXC_AUTO_COMMAND_DONE))
            Request->RequiredEvents |= SDHC_IS_TRANSFER_COMPLETE;

		if (InterruptMask & SDXC_RX_DATA_REQUEST)
		{
			Request->RequiredEvents |= SDHC_IS_BUFFER_READ_READY;
		}
		if (InterruptMask & SDXC_TX_DATA_REQUEST)
		{
			Request->RequiredEvents |= SDHC_IS_BUFFER_WRITE_READY;
		}

        SunxiWriteRegisterUlong(SunxiExtension, SDXC_REG_IMASK,
                SunxiExtension->SdioImask | SunxiExtension->Dat3Imask | InterruptMask);

    }

    SunxiWriteRegisterUlong(SunxiExtension, SDXC_REG_CARG, Command->Argument);
    SunxiWriteRegisterUlong(SunxiExtension, SDXC_REG_CMDR, CmdReg);
	
    return STATUS_PENDING;
}

NTSTATUS
SunxiPioTrans(
    _In_ PSUNXI_EXTENSION SunxiExtension,
    _In_ PSDPORT_COMMAND Command
    )
{
    NTSTATUS Ret = STATUS_SUCCESS;
    USHORT Remain, Len, BlockCount, Size;
    PUCHAR Address;

	ULONG Rval;

    Address = Command->DataBuffer;
	
    BlockCount = Command->BlockCount;
	Size = Command->BlockSize;
	Len = BlockCount * Size;
    Remain = Len;

    while (Remain >= sizeof(ULONG)) {
        if (Command->TransferDirection == SdTransferDirectionRead) {
                SunxiReadRegisterBufferUlong(SunxiExtension, SDXC_REG_FIFO, (PULONG) Address, 1);
        }
		if (Command->TransferDirection == SdTransferDirectionWrite) {
                SunxiWriteRegisterBufferUlong(SunxiExtension, SDXC_REG_FIFO, (PULONG) Address, 1);
        }
		Remain -= sizeof(ULONG);
		Address += sizeof(ULONG);
    }

	if(Remain)
	{
		if (Command->TransferDirection == SdTransferDirectionRead) {
	        SunxiReadRegisterBufferUlong(SunxiExtension, SDXC_REG_FIFO, &Rval, 1);
			while(Remain)
			{
				*Address = Rval & 0xff;
				Address ++;
				Rval = Rval >> 8;
				Remain --;
			}
	    }
		if (Command->TransferDirection == SdTransferDirectionWrite) {
			Rval = 0;
			while(Remain)
			{
				Rval += *Address & 0xff;
				Address ++;
				Rval = Rval << 8;
				Remain --;
			}
	    	SunxiWriteRegisterBufferUlong(SunxiExtension, SDXC_REG_FIFO, &Rval, 1);
	    }
	}

	SdPortWait(1);
	return Ret;
}

NTSTATUS
SunxiStartTransfer(
    _In_ PSUNXI_EXTENSION SunxiExtension,
    _In_ PSDPORT_REQUEST Request
    )

/*++

Routine Description:

    Execute the transfer request.

Arguments:

    SunxiExtension - Host controller specific driver context.

    Request - The command for which we're building the transfer request.

Return value:

    NTSTATUS

--*/

{
	UNREFERENCED_PARAMETER(SunxiExtension);
	UNREFERENCED_PARAMETER(Request);

    NTSTATUS Status;

    NT_ASSERT(Request->Command.TransferType != SdTransferTypeNone);

    switch (Request->Command.TransferMethod) {
    case SdTransferMethodPio:
        Status = SunxiStartPioTransfer(SunxiExtension, Request);
        break;

    case SdTransferMethodSgDma:
        Status = SunxiStartAdmaTransfer(SunxiExtension, Request);
        break;

    default:
        Status = STATUS_NOT_SUPPORTED;
        break;
    }

    return Status;
}

NTSTATUS
SunxiStartPioTransfer(
    _In_ PSUNXI_EXTENSION SunxiExtension,
    _In_ PSDPORT_REQUEST Request
    )

/*++

Routine Description:

    Execute the PIO transfer request.

Arguments:

    SunxiExtension - Host controller specific driver context.

    Request - The command for which we're building the transfer.

Return value:

    NTSTATUS

--*/

{
    NTSTATUS Ret = STATUS_SUCCESS;
    USHORT Reg;

    NT_ASSERT((Request->Command.TransferDirection == SdTransferDirectionRead) ||
              (Request->Command.TransferDirection == SdTransferDirectionWrite));

    Ret = SunxiPioTrans(SunxiExtension, &Request->Command);
    if (NT_SUCCESS(Ret)) {
        Request->Status = STATUS_SUCCESS;
    } else { // failed
        Request->Status = Ret;
    }

    Reg = SunxiReadRegisterUshort(SunxiExtension, SDXC_REG_RINTR);
	SunxiWriteRegisterUshort(SunxiExtension, SDXC_REG_RINTR, Reg);

	SdPortCompleteRequest(Request, STATUS_SUCCESS);

    return STATUS_PENDING;
}

NTSTATUS
SunxiStartAdmaTransfer(
    _In_ PSUNXI_EXTENSION SunxiExtension,
    _In_ PSDPORT_REQUEST Request
    )

/*++

Routine Description:

    Execute the ADMA2 transfer request.

Arguments:

    SunxiExtension - Host controller specific driver context.

    Request - The command for which we're building the transfer.

Return value:

    NTSTATUS

--*/

{

    UNREFERENCED_PARAMETER(SunxiExtension);

    Request->Status = STATUS_SUCCESS;
    SdPortCompleteRequest(Request, Request->Status);
    return STATUS_SUCCESS;
}



NTSTATUS
SunxiCreateAdmaDescriptorTable(
    _In_ PSUNXI_EXTENSION SunxiExtension,
    _In_ PSDPORT_COMMAND Command
    )

/*++

Routine Description:

    This routine creates a ADMA descriptor table based on scattor gatther list
    given by Sglist.

Arguments:

    Socket - Supplies the pointer to the socket

    Request - Data transfer request for which to build the descriptor table.

    TotalTransferLength - Supplies the pointer to return the total transfer 
                          length of the descriptor table

Return value:

    Whether the table was successfully created.

--*/

{
    ULONG NumberOfElements;
    ULONG NextLength;
    ULONG RemainingLength;
    PSCATTER_GATHER_ELEMENT SglistElement;
    PHYSICAL_ADDRESS NextAddress;
	struct SunxiDmaDescriptor *Descriptor = (struct SunxiDmaDescriptor *)Command->DmaVirtualAddress;
	struct SunxiDmaDescriptor *DescriptorPhysicalAddress = (struct SunxiDmaDescriptor *)Command->DmaPhysicalAddress.LowPart;
    ULONG Cnt;
    ULONG MaxLen = (1 << SunxiExtension->DmaDesSizeBits);

    NumberOfElements = Command->ScatterGatherList->NumberOfElements;
    SglistElement = &Command->ScatterGatherList->Elements[0];

    NT_ASSERT(NumberOfElements > 0);
    
    //
    // Iterate through each element in the SG list and convert it into the
    // descriptor table required by the controller.
    //
    Cnt = 0;
    while (NumberOfElements > 0) {
        RemainingLength = SglistElement->Length;
        NextAddress.QuadPart = SglistElement->Address.QuadPart;

        NT_ASSERT((RemainingLength > 0) && ((RemainingLength & 0x03) == 0x00));
        while (RemainingLength > 0) {
            Descriptor[Cnt].Config = (ULONG)(SDXC_IDMAC_DES0_CH | SDXC_IDMAC_DES0_OWN |
                SDXC_IDMAC_DES0_DIC);

            NextLength = MIN(MaxLen, RemainingLength);
            Descriptor[Cnt].BufSize = NextLength;
            RemainingLength -= NextLength;

            NT_ASSERT(NextAddress.HighPart == 0);
            Descriptor[Cnt].BufAddr = NextAddress.LowPart;
            NextAddress.LowPart += NextLength;

            Descriptor[Cnt].NextDescriptorAddr = (ULONG)&DescriptorPhysicalAddress[Cnt + 1];

            Cnt++;
        }

        SglistElement += 1;
        NumberOfElements -= 1;
    }
	Descriptor[0].Config |= SDXC_IDMAC_DES0_FD;
	if(Cnt == 1)
	{
		Descriptor[0].Config &= ~SDXC_IDMAC_DES0_CH;
		Descriptor[0].NextDescriptorAddr = 0;
	}
	Descriptor[Cnt - 1].Config |= SDXC_IDMAC_DES0_LD;
	Descriptor[Cnt - 1].Config &= ~SDXC_IDMAC_DES0_DIC;
	
    NT_ASSERT(Command->DmaPhysicalAddress.HighPart == 0);
    SunxiWriteRegisterUlong(SunxiExtension, SDXC_REG_DLBA, Command->DmaPhysicalAddress.LowPart);

    return STATUS_SUCCESS;
}

NTSTATUS
SunxiSendCmdSDIO(
    _In_ PSUNXI_EXTENSION SunxiExtension,
    _In_ PSDPORT_REQUEST Request
    )

/*++

Routine Description:

    This routine takes the SD command package and writes it to the appropriate
    registers on the host controller. It also computes the proper flag
    settings.

Arguments:

    SunxiExtension - Host controller specific driver context.

    Request - Supplies the descriptor for this SD command

Return value:

    STATUS_SUCCESS - Command successfully sent.

    STATUS_INVALID_PARAMETER - Invalid command response type specified.

--*/

{

    PSDPORT_COMMAND Command = &Request->Command;
    NTSTATUS Status;
    ULONG Reg;
	ULONG InterruptMask = SDXC_INTERRUPT_ERROR_BIT;
    UCHAR CmdIndex = Command->Index & 0x3f;
	ULONG CmdReg = SDXC_START | SDXC_WAIT_PRE_OVER | CmdIndex;

    NT_ASSERT(Command->TransferType != SdTransferTypeUndefined);

	SunxiAddDbgLog(SunxiExtension, 'CMDS', Command->Index, Command->Argument, (ULONG)Request);
    
    // Let SD port handle the timeout
    if ((IS_MMC_CARD(SunxiExtension))&& 
		((CmdIndex == 5)|| ((CmdIndex == 8) && (Command->TransferType == SdTransferTypeNone))))
    {
    	SdPortCompleteRequest(Request, STATUS_IO_TIMEOUT);
        return STATUS_PENDING;
    }

    if (IS_SD_CARD(SunxiExtension) && (CmdIndex == 5))
    {
    	SdPortCompleteRequest(Request, STATUS_IO_TIMEOUT);
        return STATUS_PENDING;
    }

    if (IS_SDIO(SunxiExtension) && (CmdIndex == 8))
    {
    	SdPortCompleteRequest(Request, STATUS_IO_TIMEOUT);
        return STATUS_PENDING;
    }	

	if (CmdIndex == 0) { // CMD0
		CmdReg |= SDXC_SEND_INIT_SEQUENCE | SDXC_STOP_ABORT_CMD;
	}

    if (IS_MMC_CARD(SunxiExtension) && (CmdIndex == 19)) // CMD19
		InterruptMask &= ~SDXC_END_BIT_ERROR;

    switch (Command->ResponseType) {
        case SdResponseTypeNone:
            break;

        case SdResponseTypeR1:
        case SdResponseTypeR5:
        case SdResponseTypeR6:
        case SdResponseTypeR1B:
        case SdResponseTypeR5B:
            CmdReg |= SDXC_RESP_EXPECT | SDXC_CHECK_RESPONSE_CRC;
            break;

        case SdResponseTypeR2:
            CmdReg |= SDXC_RESP_EXPECT | SDXC_CHECK_RESPONSE_CRC | SDXC_LONG_RESPONSE;
            break;

        case SdResponseTypeR3:
        case SdResponseTypeR4:
            CmdReg |= SDXC_RESP_EXPECT;
            break;

        default:
            NT_ASSERTMSG("SDHC - Invalid response type", FALSE);
            return STATUS_INVALID_PARAMETER;
    }
	
    if (Command->TransferType == SdTransferTypeNone) 
    {
    	InterruptMask |= SDXC_COMMAND_DONE;
    }
	else
	{ // transfer with data
        if ((Request->Command.TransferDirection != SdTransferDirectionRead) &&
	        (Request->Command.TransferDirection != SdTransferDirectionWrite))
	    {
	        return STATUS_INVALID_PARAMETER;
	    }
		
        CmdReg |= SDXC_DATA_EXPECT | SDXC_WAIT_PRE_OVER;
		InterruptMask |= SDXC_DATA_OVER;

        if (Command->TransferType == SdTransferTypeMultiBlock)
		{ /// need stop
            CmdReg |= SDXC_SEND_AUTO_STOP;
        }

        if (Command->TransferDirection == SdTransferDirectionWrite) // write
        {
            CmdReg |= SDXC_WRITE;
        }
		SunxiAddDbgLog(SunxiExtension, 'CMDd', Request->Command.TransferDirection, Command->BlockSize, Command->BlockCount);

        SunxiWriteRegisterUlong(SunxiExtension, SDXC_REG_BLKSZ, Command->BlockSize);
		SunxiWriteRegisterUlong(SunxiExtension, SDXC_REG_BCNTR, Command->BlockSize * Command->BlockCount);

        if (Command->TransferMethod == SdTransferMethodPio) 
		{
            Reg = SunxiReadRegisterUlong(SunxiExtension, SDXC_REG_GCTRL);
            Reg |= (SDXC_ACCESS_BY_AHB | SDXC_FIFO_RESET | SDXC_DMA_RESET);
			Reg &= ~SDXC_DMA_ENABLE_BIT;
            SunxiWriteRegisterUlong(SunxiExtension, SDXC_REG_GCTRL, Reg);

            InterruptMask |= SDXC_COMMAND_DONE;
			if(Command->TransferDirection == SdTransferDirectionWrite)
			{
				InterruptMask &= ~SDXC_DATA_OVER;
			}
        } 
		else 
		{ // SdTransferMethodSgDma
            if (SunxiExtension->SunxiSetThldCtl)
            {
                SunxiExtension->SunxiSetThldCtl((PVOID) SunxiExtension, Command);
            }

			Status = SunxiStartDma(SunxiExtension, Command);
            if (!NT_SUCCESS(Status)) 
			{
                return Status;
            }
        }
    } 

	SunxiExtension->CmdSendCnt++;

    //
    // Set the bitmask for the required events that will fire after
    // writing to the command register. Depending on the response
    // type or whether the command involves data transfer, we will need
    // to wait on a number of different events.
    //
    Request->RequiredEvents = 0;
    if (InterruptMask & SDXC_COMMAND_DONE)
    {
        Request->RequiredEvents |= SDHC_IS_CMD_COMPLETE;
    }
    if (InterruptMask & (SDXC_DATA_OVER))
    {
        Request->RequiredEvents |= SDHC_IS_TRANSFER_COMPLETE;
    }  

    SunxiWriteRegisterUlong(SunxiExtension, SDXC_REG_CARG, Command->Argument);
    SunxiWriteRegisterUlong(SunxiExtension, SDXC_REG_CMDR, CmdReg);

    //
    // We must wait until the request completes.
    //
    return STATUS_PENDING;
}

BOOLEAN
SunxiSlotInterruptSDIO(
    _In_ PVOID PrivateExtension,
    _Out_ PULONG Events,
    _Out_ PULONG Errors,
    _Out_ PBOOLEAN NotifyCardChange,
    _Out_ PBOOLEAN NotifySdioInterrupt,
    _Out_ PBOOLEAN NotifyTuning
    )

/*++

Routine Description:

    Level triggered DIRQL interrupt handler (ISR) for this controller.

Arguments:

    SlotExtension - SlotExtension interface between sdhost and this miniport.

Return value:

    Whether the interrupt is handled.

--*/

{
    PSUNXI_EXTENSION SunxiExtension;
    ULONG RawInterruptStatus, MaskInterruptStatus, DmaStatus;

    UNREFERENCED_PARAMETER(NotifyTuning);
    UNREFERENCED_PARAMETER(NotifyCardChange);

    SunxiExtension = (PSUNXI_EXTENSION) PrivateExtension;
	*NotifySdioInterrupt = FALSE;
	
    DmaStatus = SunxiReadRegisterUlong(SunxiExtension, SDXC_REG_IDST);
    MaskInterruptStatus = SunxiReadRegisterUlong(SunxiExtension, SDXC_REG_MISTA);
    RawInterruptStatus = SunxiReadRegisterUlong(SunxiExtension, SDXC_REG_RINTR);

	SunxiAddDbgLog(SunxiExtension, 'Intr', MaskInterruptStatus, RawInterruptStatus, DmaStatus);

	if ((MaskInterruptStatus == 0) && (DmaStatus == 0))
    {
        return FALSE;
    }
	if (MaskInterruptStatus & SDXC_COMMAND_DONE)
	{
    	*Events |= SDHC_IS_CMD_COMPLETE; // cmd is completed
	}
	if (MaskInterruptStatus & SDXC_DATA_OVER)
	{
    	*Events |= SDHC_IS_TRANSFER_COMPLETE; // cmd is completed
	}

    if (MaskInterruptStatus & SDXC_INTERRUPT_ERROR_BIT) 
	{
        *Errors = (ULONG) SunxiGetErrorStatus(MaskInterruptStatus & SDXC_INTERRUPT_ERROR_BIT);
		SdPrintErrorEx2(SunxiExtension, "D=%#x, M=%#x\n", DmaStatus, MaskInterruptStatus);
    }

	if ((MaskInterruptStatus & SunxiExtension->SdioImask))
	{ // SDIO
       *NotifySdioInterrupt = TRUE;
	   *Events |= SDHC_IS_CARD_INTERRUPT;
    }

	SunxiWriteRegisterUlong(SunxiExtension, SDXC_REG_RINTR, MaskInterruptStatus);
	SunxiWriteRegisterUlong(SunxiExtension, SDXC_REG_IDST, DmaStatus);

    return ((*Events != 0) || (*Errors != 0));
}

VOID
SunxiRequestDpcSDIO(
    _In_ PVOID PrivateExtension,
    _Inout_ PSDPORT_REQUEST Request,
    _In_ ULONG Events,
    _In_ ULONG Errors
    )

/*++

Routine Description:

    DPC for interrupts associated with the given request.

Arguments:

    PrivateExtension - This driver's device extension (SunxiExtension).

    Request - Request operation to perform.

Return value:

    NTSTATUS - Return value of the request performed.

--*/

{
    PSUNXI_EXTENSION SunxiExtension;
    NTSTATUS Status = STATUS_SUCCESS;
	ULONG CmdIndex; 
	PSDPORT_COMMAND Command = &Request->Command;

	SunxiExtension = (PSUNXI_EXTENSION) PrivateExtension;
	SunxiAddDbgLog(SunxiExtension, 'DPCE', Request->RequiredEvents, Events, Errors);

	if((Events == SDHC_IS_CARD_INTERRUPT) || (Events == 0))
    {
    	return;  // Only SD event
    }

	if(Request->RequiredEvents == 0)
	{
		// Already completed
		return;
	}

    CmdIndex = Command->Index & 0x3f; 

    //
    // Clear the request's required events if they have completed.
    //
    Request->RequiredEvents &= ~Events;

    if (Errors) 
	{ // error
    	Request->RequiredEvents = 0;
        Status = SunxiConvertErrorToStatus((USHORT) Errors);
		if (Command->TransferType != SdTransferTypeNone) 
		{ // data transfer
            //SunxiWaitDat0Busy(SunxiExtension, Command, TRUE);
            SunxiSendStop(SunxiExtension, Command); // CMD12
    	}
    }  

	if (Request->RequiredEvents == 0)
	{
		if ((Command->ResponseType == SdResponseTypeR1B) 
	         || (Command->ResponseType == SdResponseTypeR5B) // busy cmd
	        )
		{
	    	Status = SunxiWaitDat0Busy(SunxiExtension, Command, FALSE);
		}

		Request->Status = Status;
		SdPortCompleteRequest(Request, Status);
	}
}

NTSTATUS
SunxiAllocateDmaSDIO (
    _In_ PSUNXI_EXTENSION SunxiExtension    
)
{
	PHYSICAL_ADDRESS PhyAddress;

	SunxiExtension->DmaDesc = 0;
	SunxiExtension->DataBuffer = 0;
	
	SunxiExtension->DmaDesc = MmAllocateNonCachedMemory(sizeof(struct SunxiDmaDescriptor) * DMA_DESC_NUM);
	if(SunxiExtension->DmaDesc)
	{
		SunxiExtension->DataBuffer = MmAllocateNonCachedMemory(DMA_BUFFER_SIZE * DMA_DESC_NUM);
	}

	if((SunxiExtension->DmaDesc) && (SunxiExtension->DataBuffer))
	{
		PhyAddress = MmGetPhysicalAddress(SunxiExtension->DmaDesc);
		SunxiExtension->PhyDmaDesc = PhyAddress.LowPart;
		PhyAddress = MmGetPhysicalAddress(SunxiExtension->DataBuffer);
		SunxiExtension->PhyDataBuffer = PhyAddress.LowPart;
		RtlZeroMemory(SunxiExtension->DmaDesc, sizeof(struct SunxiDmaDescriptor) * DMA_DESC_NUM);
		RtlZeroMemory(SunxiExtension->DataBuffer, DMA_BUFFER_SIZE * DMA_DESC_NUM);
		SunxiExtension->UseSdBuffer = TRUE;
	}
	else
	{
		SunxiExtension->UseSdBuffer = FALSE;
		if(SunxiExtension->DmaDesc)
		{
			MmFreeNonCachedMemory(SunxiExtension->DmaDesc, sizeof(struct SunxiDmaDescriptor) * DMA_DESC_NUM);
		}
		if(SunxiExtension->DataBuffer)
		{
			MmFreeNonCachedMemory(SunxiExtension->DataBuffer, DMA_BUFFER_SIZE * DMA_DESC_NUM);
		}
	}

	return STATUS_SUCCESS;	
}

NTSTATUS
SunxiCreateAdmaDescriptorTableSDIOForWriteSmall(
    _In_ PSUNXI_EXTENSION SunxiExtension,
    _In_ PSDPORT_COMMAND Command
    )

/*++

Routine Description:

    This routine creates a ADMA descriptor table based on scattor gatther list
    given by Sglist.

Arguments:

    Socket - Supplies the pointer to the socket

    Request - Data transfer request for which to build the descriptor table.

    TotalTransferLength - Supplies the pointer to return the total transfer 
                          length of the descriptor table

Return value:

    Whether the table was successfully created.

--*/

{
    ULONG NumberOfElements;
    ULONG RemainingLength;
	ULONG Offset = 0;
	ULONG TotalLength = 0;
    PSCATTER_GATHER_ELEMENT SglistElement;
    PHYSICAL_ADDRESS NextAddress;
	PVOID	VirtualAddress;
	struct SunxiDmaDescriptor *Descriptor = (struct SunxiDmaDescriptor *)SunxiExtension->DmaDesc;
	struct SunxiDmaDescriptor *DescriptorPhysicalAddress = (struct SunxiDmaDescriptor *)SunxiExtension->PhyDmaDesc;
    ULONG Cnt;
	PVOID DataBuffer = SunxiExtension->DataBuffer;

    NumberOfElements = Command->ScatterGatherList->NumberOfElements;
    SglistElement = &Command->ScatterGatherList->Elements[0];

    NT_ASSERT(NumberOfElements > 0);
    
    //
    // Iterate through each element in the SG list and convert it into the
    // descriptor table required by the controller.
    //
    Cnt = 0;
	RtlZeroMemory(SunxiExtension->DmaDesc, (NumberOfElements + 1) * sizeof(struct SunxiDmaDescriptor));
	while ((NumberOfElements > 0))
	{
        RemainingLength = SglistElement->Length;
		TotalLength += SglistElement->Length;

		VirtualAddress = MmMapIoSpace(SglistElement->Address, SglistElement->Length, MmNonCached);
		if(VirtualAddress)
		{
        	RtlCopyMemory((PVOID)((PUCHAR)DataBuffer + Offset), VirtualAddress, RemainingLength);
			Offset += RemainingLength;
			MmUnmapIoSpace(VirtualAddress, SglistElement->Length);
		}
        
        SglistElement += 1;
        NumberOfElements -= 1;
    }

	Descriptor[0].Config = (ULONG)(SDXC_IDMAC_DES0_OWN | SDXC_IDMAC_DES0_FD | 
								SDXC_IDMAC_DES0_LD );
	Descriptor[0].BufSize = TotalLength;
	NextAddress = MmGetPhysicalAddress(DataBuffer);
	Descriptor[0].BufAddr = NextAddress.LowPart;
	Descriptor[0].NextDescriptorAddr = (ULONG)&DescriptorPhysicalAddress[1];

    SunxiWriteRegisterUlong(SunxiExtension, SDXC_REG_DLBA, SunxiExtension->PhyDmaDesc);

    return STATUS_SUCCESS;
}

NTSTATUS
SunxiCreateAdmaDescriptorTableSDIOForWrite(
    _In_ PSUNXI_EXTENSION SunxiExtension,
    _In_ PSDPORT_COMMAND Command
    )

/*++

Routine Description:

    This routine creates a ADMA descriptor table based on scattor gatther list
    given by Sglist.

Arguments:

    Socket - Supplies the pointer to the socket

    Request - Data transfer request for which to build the descriptor table.

    TotalTransferLength - Supplies the pointer to return the total transfer 
                          length of the descriptor table

Return value:

    Whether the table was successfully created.

--*/

{
    ULONG NumberOfElements;
	ULONG TotalLength = 0;
    PSCATTER_GATHER_ELEMENT SglistElement;
    NumberOfElements = Command->ScatterGatherList->NumberOfElements;
    SglistElement = &Command->ScatterGatherList->Elements[0];

    NT_ASSERT(NumberOfElements > 0);

	if(SunxiExtension->UseSdBuffer)
	{
		while (NumberOfElements > 0)
		{
			TotalLength += SglistElement->Length;
			SglistElement += 1;
			NumberOfElements -= 1;
		}

		if(TotalLength <= SUNXI_SDIO_FIFO_LENGTH)
		{
			return SunxiCreateAdmaDescriptorTableSDIOForWriteSmall(SunxiExtension, Command);
		}
		else
		{
			return SunxiCreateAdmaDescriptorTable(SunxiExtension, Command);
		}
	}
	else
	{
		return SunxiCreateAdmaDescriptorTable(SunxiExtension, Command);
	}

}

VOID
SunxiInitializeDbgLog(
_Inout_ PSUNXI_EXTENSION SunxiExtension
)

/*++

Routine Description:

Initialize the slot's circular debug log used to track bus activity.

Arguments:

MshcExtension - Host controller specific driver context.

Return value:

None.

--*/
{
    ULONG LogSize;

    LogSize = MSHC_DBGLOG_SIZE;
    SunxiExtension->Debug.Log.Index = 0;
    SunxiExtension->Debug.Log.IndexMask = LogSize / sizeof(MSHC_DBGLOG_ENTRY) - 1;
    SunxiExtension->Debug.Log.LogStart = (PMSHC_DBGLOG_ENTRY)&SunxiExtension->Debug.LogEntry[0];
    SunxiExtension->Debug.Log.LogCurrent = NULL;
    SunxiExtension->Debug.Log.LogEnd =
        SunxiExtension->Debug.Log.LogStart +
        (LogSize / sizeof(MSHC_DBGLOG_ENTRY)) - 1;

	DebugTrace[SunxiExtension->Port] = (ULONG)&SunxiExtension->Debug;
}

VOID
SunxiAddDbgLog(
	_Inout_ PSUNXI_EXTENSION SunxiExtension,
	_In_ ULONG Sig,
	_In_ ULONG_PTR Data1,
	_In_ ULONG_PTR Data2,
	_In_ ULONG_PTR Data3
)

/*++

Routine Description:

	Add an entry to the slot's circular debug log.

Arguments:

	MshcExtension - Host controller specific driver context.

Return value:

	None.

--*/
{
    PMSHC_DBGLOG DbgLog;
    PMSHC_DBGLOG_ENTRY Entry;
    ULONG Index;

    NT_ASSERT(SunxiExtension && SunxiExtension->Debug.Log.LogStart);

    DbgLog = &SunxiExtension->Debug.Log;
    Index = InterlockedDecrement((volatile LONG *)&(DbgLog->Index));
    Index &= DbgLog->IndexMask;

    Entry = DbgLog->LogStart + Index;
    DbgLog->LogCurrent = Entry;

    Entry->Sig = ((Sig & 0xFF) << 24) |
        ((Sig & 0xFF00) << 8) |
        ((Sig & 0xFF0000) >> 8) |
        ((Sig & 0xFF000000) >> 24);

    Entry->Data1 = Data1;
    Entry->Data2 = Data2;
    Entry->Data3 = Data3;
}

