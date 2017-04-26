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

#if defined (_MSC_VER) && (_MSC_VER > 1000)
#pragma once
#endif

#pragma warning(disable:4214)   // bit field types other than int
#pragma warning(disable:4201)   // nameless struct/union
#pragma warning(disable:4115)   // named type definition in parentheses

#define MIN(x,y) ((x) > (y) ? (y) : (x))        // return minimum among x & y
#define MAX(x,y) ((x) > (y) ? (x) : (y))        // return maximum among x & y
#define DIV_CEIL(x, y) (((x) + (y) - 1) / (y))

// ======================================================================================================

/* register offset definitions */
#define SDXC_REG_GCTRL	(0x00) /* SMC Global Control Register */
#define SDXC_REG_CLKCR	(0x04) /* SMC Clock Control Register */
#define SDXC_REG_TMOUT	(0x08) /* SMC Time Out Register */
#define SDXC_REG_WIDTH	(0x0C) /* SMC Bus Width Register */
#define SDXC_REG_BLKSZ	(0x10) /* SMC Block Size Register */
#define SDXC_REG_BCNTR	(0x14) /* SMC Byte Count Register */
#define SDXC_REG_CMDR	(0x18) /* SMC Command Register */
#define SDXC_REG_CARG	(0x1C) /* SMC Argument Register */
#define SDXC_REG_RESP0	(0x20) /* SMC Response Register 0 */
#define SDXC_REG_RESP1	(0x24) /* SMC Response Register 1 */
#define SDXC_REG_RESP2	(0x28) /* SMC Response Register 2 */
#define SDXC_REG_RESP3	(0x2C) /* SMC Response Register 3 */
#define SDXC_REG_IMASK	(0x30) /* SMC Interrupt Mask Register */
#define SDXC_REG_MISTA	(0x34) /* SMC Masked Interrupt Status Register */
#define SDXC_REG_RINTR	(0x38) /* SMC Raw Interrupt Status Register */
#define SDXC_REG_STAS	(0x3C) /* SMC Status Register */
#define SDXC_REG_FTRGL	(0x40) /* SMC FIFO Threshold Watermark Registe */
#define SDXC_REG_FUNS	(0x44) /* SMC Function Select Register */
#define SDXC_REG_CBCR	(0x48) /* SMC CIU Byte Count Register */
#define SDXC_REG_BBCR	(0x4C) /* SMC BIU Byte Count Register */
#define SDXC_REG_DBGC	(0x50) /* SMC Debug Enable Register */
#define SDXC_REG_A12A	(0x58 )		//auto cmd12 arg
#define SDXC_REG_HWRST	(0x78) /* SMC Card Hardware Reset for Register */
#define SDXC_REG_DMAC	(0x80) /* SMC IDMAC Control Register */
#define SDXC_REG_DLBA	(0x84) /* SMC IDMAC Descriptor List Base Addre */
#define SDXC_REG_IDST	(0x88) /* SMC IDMAC Status Register */
#define SDXC_REG_IDIE	(0x8C) /* SMC IDMAC Interrupt Enable Register */
#define SDXC_REG_CHDA	(0x90)
#define SDXC_REG_CBDA	(0x94)
#define SDXC_REG_FIFO	(0x200)

#define BIT(bit)                (1 << bit)

/* global control register bits */
#define SDXC_SOFT_RESET			BIT(0)
#define SDXC_FIFO_RESET			BIT(1)
#define SDXC_DMA_RESET			BIT(2)
#define SDXC_INTERRUPT_ENABLE_BIT	BIT(4)
#define SDXC_DMA_ENABLE_BIT		BIT(5)
#define SDXC_DEBOUNCE_ENABLE_BIT	BIT(8)
#define SDXC_POSEDGE_LATCH_DATA		BIT(9)
#define SDXC_DDR_MODE			BIT(10)
#define SDXC_MEMORY_ACCESS_DONE		BIT(29)
#define SDXC_ACCESS_DONE_DIRECT		BIT(30)
#define SDXC_ACCESS_BY_AHB		BIT(31)
#define SDXC_ACCESS_BY_DMA		(0 << 31)
#define SDXC_HARDWARE_RESET \
	(SDXC_SOFT_RESET | SDXC_FIFO_RESET | SDXC_DMA_RESET)

/* clock control bits */
#define SDXC_CARD_CLOCK_ON		BIT(16)
#define SDXC_LOW_POWER_ON		BIT(17)
#define SDXC_MASK_DATA0			BIT(31)


/* bus width */
#define SDXC_WIDTH1			0
#define SDXC_WIDTH4			1
#define SDXC_WIDTH8			2

/* smc command bits */
#define SDXC_RESP_EXPECT		BIT(6)
#define SDXC_LONG_RESPONSE		BIT(7)
#define SDXC_CHECK_RESPONSE_CRC		BIT(8)
#define SDXC_DATA_EXPECT		BIT(9)
#define SDXC_WRITE			BIT(10)
#define SDXC_SEQUENCE_MODE		BIT(11)
#define SDXC_SEND_AUTO_STOP		BIT(12)
#define SDXC_WAIT_PRE_OVER		BIT(13)
#define SDXC_STOP_ABORT_CMD		BIT(14)
#define SDXC_SEND_INIT_SEQUENCE		BIT(15)
#define SDXC_UPCLK_ONLY			BIT(21)
#define SDXC_READ_CEATA_DEV		BIT(22)
#define SDXC_CCS_EXPECT			BIT(23)
#define SDXC_ENABLE_BIT_BOOT		BIT(24)
#define SDXC_ALT_BOOT_OPTIONS		BIT(25)
#define SDXC_BOOT_ACK_EXPECT		BIT(26)
#define SDXC_BOOT_ABORT			BIT(27)
#define SDXC_VOLTAGE_SWITCH	        BIT(28)
#define SDXC_USE_HOLD_REGISTER	        BIT(29)
#define SDXC_START			BIT(31)

/* interrupt bits */
#define SDXC_RESP_ERROR			BIT(1)
#define SDXC_COMMAND_DONE		BIT(2)
#define SDXC_DATA_OVER			BIT(3)
#define SDXC_TX_DATA_REQUEST		BIT(4)
#define SDXC_RX_DATA_REQUEST		BIT(5)
#define SDXC_RESP_CRC_ERROR		BIT(6)
#define SDXC_DATA_CRC_ERROR		BIT(7)
#define SDXC_RESP_TIMEOUT		BIT(8)
#define SDXC_DATA_TIMEOUT		BIT(9)
#define SDXC_VOLTAGE_CHANGE_DONE	BIT(10)
#define SDXC_FIFO_RUN_ERROR		BIT(11)
#define SDXC_HARD_WARE_LOCKED		BIT(12)
#define SDXC_START_BIT_ERROR		BIT(13)
#define SDXC_AUTO_COMMAND_DONE		BIT(14)
#define SDXC_END_BIT_ERROR		BIT(15)
#define SDXC_SDIO_INTERRUPT		BIT(16)
#define SDXC_CARD_INSERT		BIT(30)
#define SDXC_CARD_REMOVE		BIT(31)
#define SDXC_INTERRUPT_ERROR_BIT \
	(SDXC_RESP_ERROR | SDXC_RESP_CRC_ERROR | SDXC_DATA_CRC_ERROR | \
	 SDXC_RESP_TIMEOUT | SDXC_DATA_TIMEOUT | SDXC_FIFO_RUN_ERROR | \
	 SDXC_HARD_WARE_LOCKED | SDXC_START_BIT_ERROR | SDXC_END_BIT_ERROR | SDXC_VOLTAGE_CHANGE_DONE)
#define SDXC_INTERRUPT_DONE_BIT \
	(SDXC_AUTO_COMMAND_DONE | SDXC_DATA_OVER | SDXC_COMMAND_DONE)

/* status */
#define SDXC_RXWL_FLAG			BIT(0)
#define SDXC_TXWL_FLAG			BIT(1)
#define SDXC_FIFO_EMPTY			BIT(2)
#define SDXC_FIFO_FULL			BIT(3)
#define SDXC_CARD_PRESENT		BIT(8)
#define SDXC_CARD_DATA_BUSY		BIT(9)
#define SDXC_DATA_FSM_BUSY		BIT(10)
#define SDXC_DMA_REQUEST		BIT(31)
#define SDXC_FIFO_SIZE			16

/* Function select */
#define SDXC_CEATA_ON			(0xceaa << 16)
#define SDXC_SEND_IRQ_RESPONSE		BIT(0)
#define SDXC_SDIO_READ_WAIT		BIT(1)
#define SDXC_ABORT_READ_DATA		BIT(2)
#define SDXC_SEND_CCSD			BIT(8)
#define SDXC_SEND_AUTO_STOPCCSD		BIT(9)
#define SDXC_CEATA_DEV_IRQ_ENABLE	BIT(10)

/* IDMA controller bus mod bit field */
#define SDXC_IDMAC_SOFT_RESET		BIT(0)
#define SDXC_IDMAC_FIX_BURST		BIT(1)
#define SDXC_IDMAC_IDMA_ON		BIT(7)
#define SDXC_IDMAC_REFETCH_DES		BIT(31)

/* IDMA status bit field */
#define SDXC_IDMAC_TRANSMIT_INTERRUPT		BIT(0)
#define SDXC_IDMAC_RECEIVE_INTERRUPT		BIT(1)
#define SDXC_IDMAC_FATAL_BUS_ERROR		BIT(2)
#define SDXC_IDMAC_DESTINATION_INVALID		BIT(4)
#define SDXC_IDMAC_CARD_ERROR_SUM		BIT(5)
#define SDXC_IDMAC_NORMAL_INTERRUPT_SUM		BIT(8)
#define SDXC_IDMAC_ABNORMAL_INTERRUPT_SUM	BIT(9)
#define SDXC_IDMAC_HOST_ABORT_INTERRUPT		BIT(10)
#define SDXC_IDMAC_IDLE				(0 << 13)
#define SDXC_IDMAC_SUSPEND			(1 << 13)
#define SDXC_IDMAC_DESC_READ			(2 << 13)
#define SDXC_IDMAC_DESC_CHECK			(3 << 13)
#define SDXC_IDMAC_READ_REQUEST_WAIT		(4 << 13)
#define SDXC_IDMAC_WRITE_REQUEST_WAIT		(5 << 13)
#define SDXC_IDMAC_READ				(6 << 13)
#define SDXC_IDMAC_WRITE			(7 << 13)
#define SDXC_IDMAC_DESC_CLOSE			(8 << 13)

/*
* If the idma-des-size-bits of property is ie 13, bufsize bits are:
*  Bits  0-12: buf1 size
*  Bits 13-25: buf2 size
*  Bits 26-31: not used
* Since we only ever set buf1 size, we can simply store it directly.
*/
#define SDXC_IDMAC_DES0_DIC	BIT(1)  /* disable interrupt on completion */
#define SDXC_IDMAC_DES0_LD	BIT(2)  /* last descriptor */
#define SDXC_IDMAC_DES0_FD	BIT(3)  /* first descriptor */
#define SDXC_IDMAC_DES0_CH	BIT(4)  /* chain mode */
#define SDXC_IDMAC_DES0_ER	BIT(5)  /* end of ring */
#define SDXC_IDMAC_DES0_CES	BIT(30) /* card error summary */
#define SDXC_IDMAC_DES0_OWN	BIT(31) /* 1-idma owns it, 0-host owns it */

// =======================================================================================================
//reg
#define SDXC_REG_EDSD		(0x010C)    /*SMHC eMMC4.5 DDR Start Bit Detection Control Register*/
#define SDXC_REG_CSDC		(0x0054)    /*SMHC CRC Status Detect Control Register*/
#define SDXC_REG_THLD		(0x0100)    /*SMHC Card Threshold Control Register*/
#define SDXC_REG_DRV_DL	 	(0x0140)    /*SMHC Drive Delay Control Register*/
#define SDXC_REG_SAMP_DL	(0x0144)	/*SMHC Sample Delay Control Register*/
#define SDXC_REG_DS_DL		(0x0148)	/*SMHC Data Strobe Delay Control Register*/


#define SDXC_REG_SD_NTSR	(0x005C)	/*SMHC NewTiming Set Register*/
#define	SDXC_2X_TIMING_MODE			(1U<<31)

//bit
#define SDXC_HS400_MD_EN				(1U<<31)
#define SDXC_CARD_WR_THLD_ENB		(1U<<2)
#define SDXC_CARD_RD_THLD_ENB		(1U)

#define SDXC_DAT_DRV_PH_SEL			(1U<<17)
#define SDXC_CMD_DRV_PH_SEL			(1U<<16)
#define SDXC_SAMP_DL_SW_EN			(1u<<7)
#define SDXC_DS_DL_SW_EN			(1u<<7)


//mask
#define SDXC_CRC_DET_PARA_MASK		(0xf)
#define SDXC_CARD_RD_THLD_MASK		(0x0FFF0000)
#define SDXC_TX_TL_MASK				(0xff)
#define SDXC_RX_TL_MASK				(0x00FF0000)

#define SDXC_SAMP_DL_SW_MASK		(0x0000003F)
#define SDXC_DS_DL_SW_MASK			(0x0000003F)


//value
#define SDXC_CRC_DET_PARA_HS400		(6)
#define SDXC_CRC_DET_PARA_OTHER		(3)
#define SDXC_FIFO_DETH					(1024>>2)

//size
#define SDXC_CARD_RD_THLD_SIZE		(0x00000FFF)

//shit
#define SDXC_CARD_RD_THLD_SIZE_SHIFT		(16)

#define SDXC_RESERVE_SPACE          (84 * 1024 * 1024 / 512) // 84MB, unit: 512 bytes

#define BASE_CLOCK_FREQUENCY_KHZ    50*1000 // TODO: 100MHz / 2
#define CLOCK_25MHZ    				25*1000 // 25MHz
#define CLOCK_52MHZ    				52*1000 // 52MHz

// === Controller 0 (for SD Card) ====================================================================================================
//dma triger level setting
#define SUNXI_DMA_TL_SDMMC0 	((0x2<<28)|(7<<16)|248)
//one dma des can transfer data size = 1<<SUNXI_DES_SIZE_SDMMC0
#define SUNXI_DES_SIZE_SDMMC0	(15)

// === Controller 1 (for SDIO) ====================================================================================================
#define SUNXI_DMA_TL_SDMMC1 ((0x3<<28)|(15<<16)|240)
//one dma des can transfer data size = 1<<SUNXI_DES_SIZE_SDMMC1
#define SUNXI_DES_SIZE_SDMMC1	(15)

#define SUNXI_SDIO_FIFO_LENGTH	16

// === Controller 2 (for eMMC) ====================================================================================================
#define SUNXI_DMA_TL_SDMMC2		((0x3<<28)|(15<<16)|240)
//one dma des can transfer data size = 1<<SUNXI_DES_SIZE_SDMMC2
#define SUNXI_DES_SIZE_SDMMC2	(12)


typedef enum _SUNXI_SDMMC_PORT_NUM {
	SUNXI_SDMMC_SD_CARD = 0,
	SUNXI_SDMMC_SDIO,
	SUNXI_SDMMC_EMMC, // 2
} SUNXI_SDMMC_PORT_NUM;


#define IS_SD_CARD(SunxiExtension)  (SunxiExtension->Port == SUNXI_SDMMC_SD_CARD)
#define IS_SDIO(SunxiExtension)  (SunxiExtension->Port == SUNXI_SDMMC_SDIO)
#define IS_MMC_CARD(SunxiExtension)  (SunxiExtension->Port == SUNXI_SDMMC_EMMC)

//
// Bits defined in SDHC_INTERRUPT_STATUS
//                 SDHC_INTERRUPT_STATUS_ENABLE
//                 SDHC_INTERRUPT_SIGNAL_ENABLE
//

#define SDHC_IS_CMD_COMPLETE            0x0001
#define SDHC_IS_TRANSFER_COMPLETE       0x0002
#define SDHC_IS_BLOCKGAP_EVENT          0x0004
#define SDHC_IS_DMA_TRANSFER_COMPLETE   0x0008
#define SDHC_IS_BUFFER_WRITE_READY      0x0010
#define SDHC_IS_BUFFER_READ_READY       0x0020
#define SDHC_IS_CARD_INSERTION          0x0040
#define SDHC_IS_CARD_REMOVAL            0x0080
#define SDHC_IS_CARD_INTERRUPT          0x0100
#define SDHC_IS_TUNING_INTERRUPT        0x1000

#define SDHC_IS_ERROR_INTERRUPT         0x8000

#define SDHC_IS_CARD_DETECT             (SDHC_IS_CARD_INSERTION | SDHC_IS_CARD_REMOVAL)

//
// Bits defined in SDHC_ERROR_STATUS
// Bits defined in SDHC_ERROR_STATUS_ENABLE
// Bits defined in SDHC_ERROR_SIGNAL_ENABLE
//
// PLEASE NOTE: these values need to match exactly the
//              "generic" values in sdbus.h
//

#define SDHC_ES_CMD_TIMEOUT             0x0001
#define SDHC_ES_CMD_CRC_ERROR           0x0002
#define SDHC_ES_CMD_END_BIT_ERROR       0x0004
#define SDHC_ES_CMD_INDEX_ERROR         0x0008
#define SDHC_ES_DATA_TIMEOUT            0x0010
#define SDHC_ES_DATA_CRC_ERROR          0x0020
#define SDHC_ES_DATA_END_BIT_ERROR      0x0040
#define SDHC_ES_BUS_POWER_ERROR         0x0080
#define SDHC_ES_AUTO_CMD12_ERROR        0x0100
#define SDHC_ES_ADMA_ERROR              0x0200
#define SDHC_ES_BAD_DATA_SPACE_ACCESS   0x2000

#define SDHC_MAX_OUTSTANDING_REQUESTS 1

#define DMA_DESC_NUM	4
#define DMA_BUFFER_SIZE	4096


typedef enum _RequestState {
    StateIdle = 0,
    StateWaitInterrupt, // 1
    StateWaitMoreInterrupt, // 2
    StateWaitDpc // 3
} RequestState;

#define LARGER_DBGLOG	

#if defined(LARGER_DBGLOG)
#define MSHC_DBGLOG_SIZE (PAGE_SIZE * 64)
#else
#define MSHC_DBGLOG_SIZE PAGE_SIZE
#endif

//
// Debug log structures
// 

typedef struct _MSHC_DBGLOG_ENTRY {
    ULONG_PTR Sig;
    ULONG_PTR Data1;
    ULONG_PTR Data2;
    ULONG_PTR Data3;
} MSHC_DBGLOG_ENTRY, *PMSHC_DBGLOG_ENTRY;

typedef struct _MSHC_DBGLOG {
    ULONG Index;
    ULONG IndexMask;
    PMSHC_DBGLOG_ENTRY LogStart;
    PMSHC_DBGLOG_ENTRY LogCurrent;
    PMSHC_DBGLOG_ENTRY LogEnd;
} MSHC_DBGLOG, *PMSHC_DBGLOG;


typedef struct _SUNXI_EXTENSION {
    SUNXI_SDMMC_PORT_NUM Port;
	ULONG PrintControl;
	BOOLEAN CrashdumpMode;

	ULONG CmdSendCnt;
		
    BOOLEAN Removable;
    BOOLEAN ReadWaitDma;
	
	UCHAR DmaDesSizeBits;
    ULONG DmaThreshold;
	ULONG Dat3Imask;
	ULONG SdioImask;

	ULONG IntrBak;

    //
    // Host register space.
    //

    PHYSICAL_ADDRESS PhysicalBaseAddress;
    PVOID BaseAddress;
    SIZE_T BaseAddressSpaceSize;
    //PSD_HOST_CONTROLLER_REGISTERS BaseAddressDebug;
    KSPIN_LOCK IntSpinLock;

    //
    // Capabilities.
    //
    BOOLEAN UseSdBuffer;
    PVOID DmaDesc;
	ULONG PhyDmaDesc;
	PVOID DataBuffer;
	ULONG PhyDataBuffer;

    SDPORT_CAPABILITIES Capabilities;
    SDPORT_BUS_SPEED SpeedMode;
    UCHAR BusWidth;

	ULONG SdioRespCmd5;

	//
    // Debug log
    //
    struct {
        MSHC_DBGLOG Log;
        UCHAR LogEntry[MSHC_DBGLOG_SIZE];
    } Debug;

    VOID (*SunxiSetThldCtl) (
        _In_ PVOID PrivateExtension,
        _In_ PSDPORT_COMMAND Command
        );
} SUNXI_EXTENSION, *PSUNXI_EXTENSION;


// ---------------------------------------------------------------------------
// PCI Config definitions
// ---------------------------------------------------------------------------

#define SDHC_PCICFG_SLOT_INFORMATION    0x40

NTSTATUS
DriverEntry(
    _In_ PVOID DriverObject,
    _In_ PVOID RegistryPath
    );

//-----------------------------------------------------------------------------
// SlotExtension callbacks.
//-----------------------------------------------------------------------------

SDPORT_GET_SLOT_COUNT SunxiGetSlotCount;
/*
NTSTATUS
SunxiGetSlotCount(
    _In_ PVOID Argument,
    _In_ SDPORT_BUS_TYPE BusType
    );
*/

SDPORT_GET_SLOT_CAPABILITIES SunxiGetSlotCapabilities;
/*
VOID
SunxiGetSlotCapabilities(
    _In_ PSDPORT_SLOT_EXTENSION SlotExtension,
    _Inout_ PSDPORT_CAPABILITIES Capabilities
    );
*/

SDPORT_INITIALIZE SunxiSlotInitialize;
/*
NTSTATUS
SdhcSlotInitialize(
    _Inout_ PSDPORT_SLOT_EXTENSION SlotExtension
    );
*/

SDPORT_ISSUE_BUS_OPERATION SunxiSlotIssueBusOperation;
/*
NTSTATUS
SdhcSlotIssueBusOperation(
    _In_ PSDPORT_SLOT_EXTENSION SlotExtension,
    _In_ PSDPORT_REQUEST Request
    );
*/

SDPORT_GET_CARD_DETECT_STATE SunxiSlotGetCardDetectState;
/*
BOOLEAN
SdhcSlotGetCardDetectState(
    _In_ PSDPORT_SLOT_EXTENSION SlotExtension
    );
*/

SDPORT_GET_WRITE_PROTECT_STATE SunxiSlotGetWriteProtectState;
/*
BOOLEAN
SdhcSlotGetWriteProtectState(
    _In_ PSDPORT_SLOT_EXTENSION SlotExtension
    );
*/

SDPORT_INTERRUPT SunxiSlotInterrupt;
SDPORT_INTERRUPT SunxiSlotInterruptSDIO;

/*
BOOLEAN
SdhcSlotInterrupt(
    _In_ PSDPORT_SLOT_EXTENSION SlotExtension,
    _Out_ PULONG Events,
	_Out_ PULONG Errors,
    _Out_ PBOOLEAN NotifyCardChange,
    _Out_ PBOOLEAN NotifySdioInterrupt,
    _Out_ PBOOLEAN NotifyTuning
    );
*/

SDPORT_ISSUE_REQUEST SunxiSlotIssueRequest;
/*
NTSTATUS
SdhcSlotIssueRequest(
    _In_ PSDPORT_SLOT_EXTENSION SlotExtension, 
    _In_ PSDPORT_REQUEST Request
    );
*/

SDPORT_GET_RESPONSE SunxiSlotGetResponse;
/*
VOID
SdhcSlotGetResponse(
    _In_ PVOID PrivateExtension,
    _In_ PSDPORT_COMMAND Command,
    _Out_ PVOID ResponseBuffer
    );
*/

SDPORT_TOGGLE_EVENTS SunxiSlotToggleEvents;
/*
VOID
SdhcSlotToggleEevnts(
    _In_ PSDPORT_SLOT_EXTENSION SlotExtension,
    _In_ ULONG EventMask,
    _In_ BOOLEAN Enable
    );
*/

SDPORT_CLEAR_EVENTS SunxiSlotClearEvents;
/*
VOID
SdhcSlotClearEvents(
    _In_ PSDPORT_SLOT_EXTENSION SlotExtension,
    _In_ ULONG EventMask
    );
*/

SDPORT_REQUEST_DPC SunxiRequestDpc;
SDPORT_REQUEST_DPC SunxiRequestDpcSDIO;

/*
VOID
SdhcRequestDpc(
    _In_ PVOID PrivateExtension,
    _Inout_ PSDPORT_REQUEST Request,
    _In_ ULONG Events,
    _In_ ULONG Errors
    );
*/

SDPORT_SAVE_CONTEXT SunxiSaveContext;
/*
VOID
SdhcSaveContext(
    _In_ PSDPORT_SLOT_EXTENSION SlotExtension
    );
*/

SDPORT_RESTORE_CONTEXT SunxiRestoreContext;
/*
VOID
SdhcRestoreContext(
    _In_ PSDPORT_SLOT_EXTENSION SlotExtension
    );
*/

SDPORT_PO_FX_POWER_CONTROL_CALLBACK SunxiPoFxPowerControlCallback;
/*
NTSTATUS
SdhcPoFxPowerControlCallback(
    _In_ PSD_MINIPORT Miniport,
    _In_ LPCGUID PowerControlCode,
    _In_reads_bytes_opt_(InputBufferSize) PVOID InputBuffer,
    _In_ SIZE_T InputBufferSize,
    _Out_writes_bytes_opt_(OutputBufferSize) PVOID OutputBuffer,
    _In_ SIZE_T OutputBufferSize,
    _Out_opt_ PSIZE_T BytesReturned
    )
*/

//-----------------------------------------------------------------------------
// Hardware access routines.
//-----------------------------------------------------------------------------

NTSTATUS
SunxiResetHw(
    _In_ PSUNXI_EXTENSION SunxiExtension,
    _In_ UCHAR ResetType
    );

NTSTATUS
SunxiSetVoltage(
    _In_ PSUNXI_EXTENSION SunxiExtension,
    _In_ SDPORT_BUS_VOLTAGE VoltageProfile
    );

NTSTATUS
SunxiSetClock(
    _In_ PSUNXI_EXTENSION SunxiExtension,
    _In_ ULONG Frequency
    );

NTSTATUS
SunxiSetBusWidth(
    _In_ PSUNXI_EXTENSION SunxiExtension,
    _In_ UCHAR BusWidth
    );

NTSTATUS
SunxiSetSpeed(
    _In_ PSUNXI_EXTENSION SunxiExtension,
    _In_ SDPORT_BUS_SPEED Speed
    );

NTSTATUS
SunxiSetSignaling(
    _In_ PSUNXI_EXTENSION SunxiExtension,
    _In_ BOOLEAN Enable
    );

BOOLEAN
SunxiIsWriteProtected(
    _In_ PSUNXI_EXTENSION SunxiExtension
    );

NTSTATUS
SunxiSendCmd(
    _In_ PSUNXI_EXTENSION SunxiExtension,
    _In_ PSDPORT_REQUEST Request
    );

NTSTATUS
SunxiSendCmdSDIO(
    _In_ PSUNXI_EXTENSION SunxiExtension,
    _In_ PSDPORT_REQUEST Request
    );


NTSTATUS
SunxiGetResponse(
    _In_ PSUNXI_EXTENSION SunxiExtension,
    _In_ PSDPORT_COMMAND Command,
    _Out_ PVOID ResponseBuffer
    );

NTSTATUS
SunxiSetTransferMode(
    _In_ PSUNXI_EXTENSION SunxiExtension,
    _In_ PSDPORT_REQUEST Request
    );

NTSTATUS
SunxiStartTransfer(
    _In_ PSUNXI_EXTENSION SunxiExtension,
    _In_ PSDPORT_REQUEST Request
    );

NTSTATUS
SunxiStartPioTransfer(
    _In_ PSUNXI_EXTENSION SunxiExtension,
    _In_ PSDPORT_REQUEST Request
    );

NTSTATUS
SunxiStartAdmaTransfer(
    _In_ PSUNXI_EXTENSION SunxiExtension,
    _In_ PSDPORT_REQUEST Request
    );

//-----------------------------------------------------------------------------
// General utility functions.
//-----------------------------------------------------------------------------

NTSTATUS
SunxiConvertErrorToStatus(
    _In_ USHORT Error
    );

NTSTATUS
SunxiAllocateDmaSDIO (
    _In_ PSUNXI_EXTENSION SunxiExtension    
);

NTSTATUS
SunxiCreateAdmaDescriptorTable (
    _In_ PSUNXI_EXTENSION SunxiExtension,
    _In_ PSDPORT_COMMAND Command
    );

NTSTATUS
SunxiCreateAdmaDescriptorTableSDIOForWrite(
    _In_ PSUNXI_EXTENSION SunxiExtension,
    _In_ PSDPORT_COMMAND Command
    );

NTSTATUS
SunxiCreateAdmaDescriptorTableSDIOForWriteSmall(
    _In_ PSUNXI_EXTENSION SunxiExtension,
    _In_ PSDPORT_COMMAND Command
    );


__forceinline
NTSTATUS
SunxiConvertErrorToStatus(
    _In_ USHORT Error
    )

{

    if (Error == 0) {
        return STATUS_SUCCESS;
    }

    if (Error & (SDHC_ES_CMD_TIMEOUT | SDHC_ES_DATA_TIMEOUT)) {
        return STATUS_IO_TIMEOUT;
    }

    if (Error & (SDHC_ES_CMD_CRC_ERROR | SDHC_ES_DATA_CRC_ERROR)) {
        return STATUS_CRC_ERROR;
    }

    if (Error & (SDHC_ES_CMD_END_BIT_ERROR | SDHC_ES_DATA_END_BIT_ERROR)) {
        return STATUS_DEVICE_DATA_ERROR;
    }

    if (Error & SDHC_ES_CMD_INDEX_ERROR) {
        return STATUS_DEVICE_PROTOCOL_ERROR;
    }

    if (Error & SDHC_ES_BUS_POWER_ERROR) {
        return STATUS_DEVICE_POWER_FAILURE;
    }

    return STATUS_IO_DEVICE_ERROR;
}


//-----------------------------------------------------------------------------
// Register access macros.
//-----------------------------------------------------------------------------

__forceinline
VOID
SunxiWriteRegisterUlong(
    _In_ PSUNXI_EXTENSION SunxiExtension,
    _In_ ULONG Register,
    _In_ ULONG Data
    )

{

    SdPortWriteRegisterUlong(SunxiExtension->BaseAddress, Register, Data);
    return;
}

__forceinline
VOID
SunxiWriteRegisterUshort(
    _In_ PSUNXI_EXTENSION SunxiExtension,
    _In_ ULONG Register,
    _In_ USHORT Data
    )

{

    SdPortWriteRegisterUshort(SunxiExtension->BaseAddress, Register, Data);
    return;
}

__forceinline
VOID
SunxiWriteRegisterUchar(
    _In_ PSUNXI_EXTENSION SunxiExtension,
    _In_ ULONG Register,
    _In_ UCHAR Data
    )

{

    SdPortWriteRegisterUchar(SunxiExtension->BaseAddress, Register, Data);
    return;
}

__forceinline
ULONG
SunxiReadRegisterUlong(
    _In_ PSUNXI_EXTENSION SunxiExtension,
    _In_ ULONG Register
    )

{
    return SdPortReadRegisterUlong(SunxiExtension->BaseAddress, Register);
}

__forceinline
USHORT
SunxiReadRegisterUshort(
    _In_ PSUNXI_EXTENSION SunxiExtension,
    _In_ ULONG Register
    )

{
    return SdPortReadRegisterUshort(SunxiExtension->BaseAddress, Register);
}

__forceinline
UCHAR
SunxiReadRegisterUchar(
    _In_ PSUNXI_EXTENSION SunxiExtension,
    _In_ ULONG Register
    )

{
    return SdPortReadRegisterUchar(SunxiExtension->BaseAddress, Register);
}

__forceinline
VOID
SunxiReadRegisterBufferUlong(
    _In_ PSUNXI_EXTENSION SunxiExtension,
    _In_ ULONG Register,
    _Out_writes_all_(Length) PULONG Buffer,
    _In_ ULONG Length)

{
    SdPortReadRegisterBufferUlong(SunxiExtension->BaseAddress,
                                  Register,
                                  Buffer,
                                  Length);
}

__forceinline
VOID
SunxiReadRegisterBufferUshort(
    _In_ PSUNXI_EXTENSION SunxiExtension,
    _In_ ULONG Register,
    _Out_writes_all_(Length) PUSHORT Buffer,
    _In_ ULONG Length)

{
    SdPortReadRegisterBufferUshort(SunxiExtension->BaseAddress,
                                   Register,
                                   Buffer,
                                   Length);
}

__forceinline
VOID
SunxiReadRegisterBufferUchar(
    _In_ PSUNXI_EXTENSION SunxiExtension,
    _In_ ULONG Register,
    _Out_writes_all_(Length) PUCHAR Buffer,
    _In_ ULONG Length)

{
    SdPortReadRegisterBufferUchar(SunxiExtension->BaseAddress,
                                  Register,
                                  Buffer,
                                  Length);
}

__forceinline
VOID
SunxiWriteRegisterBufferUlong(
    _In_ PSUNXI_EXTENSION SunxiExtension,
    _In_ ULONG Register,
    _In_reads_(Length) PULONG Buffer,
    _In_ ULONG Length
    )

{

    SdPortWriteRegisterBufferUlong(SunxiExtension->BaseAddress,
                                   Register,
                                   Buffer,
                                   Length);
}

__forceinline
VOID
SunxiWriteRegisterBufferUshort(
    _In_ PSUNXI_EXTENSION SunxiExtension,
    _In_ ULONG Register,
    _In_reads_(Length) PUSHORT Buffer,
    _In_ ULONG Length
    )

{

    SdPortWriteRegisterBufferUshort(SunxiExtension->BaseAddress,
                                    Register,
                                    Buffer,
                                    Length);
}

__forceinline
VOID
SunxiWriteRegisterBufferUchar(
    _In_ PSUNXI_EXTENSION SunxiExtension,
    _In_ ULONG Register,
    _In_reads_(Length) PUCHAR Buffer,
    _In_ ULONG Length
    )

{

    SdPortWriteRegisterBufferUchar(SunxiExtension->BaseAddress,
                                   Register,
                                   Buffer,
                                   Length);

}


struct SunxiDmaDescriptor {
	ULONG	Config;
	ULONG	BufSize;
	ULONG	BufAddr;
	ULONG	NextDescriptorAddr;
};

VOID
SunxiSetThldCtl_0 (
    _In_ PVOID PrivateExtension,
    _In_ PSDPORT_COMMAND Command
    );

VOID
SunxiSetThldCtl_1 (
    _In_ PVOID PrivateExtension,
    _In_ PSDPORT_COMMAND Command
    );

VOID
SunxiSetThldCtl_2 (
    _In_ PVOID PrivateExtension,
    _In_ PSDPORT_COMMAND Command
    );

// =======================================================================================================
#define SdPrintErrorEx(SunxiExtension, Format, ...)   \
    { \
        if (SunxiExtension->PrintControl & BIT(DPFLTR_ERROR_LEVEL)) \
            DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "[Ct%d Error: %s:] " Format, SunxiExtension->Port, __FUNCTION__, __VA_ARGS__); \
    }
#define SdPrintErrorEx2(SunxiExtension, Format, ...)   \
    { \
        if (SunxiExtension->PrintControl & BIT(DPFLTR_ERROR_LEVEL)) \
            DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, Format, __VA_ARGS__); \
    }

#define SdPrintWarningEx(SunxiExtension, Format, ...)   \
    { \
        if (SunxiExtension->PrintControl & BIT(DPFLTR_WARNING_LEVEL)) \
            DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "[Ct%d Warning: %s:] " Format, SunxiExtension->Port, __FUNCTION__, __VA_ARGS__); \
    }

#define SdPrintWarningEx2(SunxiExtension, Format, ...)   \
    { \
        if (SunxiExtension->PrintControl & BIT(DPFLTR_WARNING_LEVEL)) \
            DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, Format, __VA_ARGS__); \
    }

#define SdPrintTraceEx(SunxiExtension, Format, ...) \
    { \
        if (SunxiExtension->PrintControl & BIT(DPFLTR_TRACE_LEVEL)) \
            DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "[Ct%d Trace: %s:] " Format, SunxiExtension->Port, __FUNCTION__, __VA_ARGS__); \
    }
#define SdPrintInfoEx(SunxiExtension, Format, ...)  \
    { \
        if (SunxiExtension->PrintControl & BIT(DPFLTR_INFO_LEVEL)) \
            DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "[Ct%d Info: %s:] " Format, SunxiExtension->Port, __FUNCTION__, __VA_ARGS__); \
    }

#define SdPrintInfoEx2(SunxiExtension, Format, ...)  \
    { \
        if (SunxiExtension->PrintControl & BIT(DPFLTR_INFO_LEVEL)) \
            DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, Format,  __VA_ARGS__); \
    }

#define SdPrintError(Format, ...)   DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, Format, __VA_ARGS__)
#define SdPrintWarning(Format, ...) DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_WARNING_LEVEL, Format, __VA_ARGS__)
#define SdPrintTrace(Format, ...)   DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_TRACE_LEVEL, Format, __VA_ARGS__)
#define SdPrintInfo(Format, ...)    DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, Format, __VA_ARGS__)

VOID
SunxiInitializeDbgLog(
_Inout_ PSUNXI_EXTENSION SunxiExtension
);

VOID
SunxiAddDbgLog(
	_Inout_ PSUNXI_EXTENSION SunxiExtension,
	_In_ ULONG Sig,
	_In_ ULONG_PTR Data1,
	_In_ ULONG_PTR Data2,
	_In_ ULONG_PTR Data3
);

