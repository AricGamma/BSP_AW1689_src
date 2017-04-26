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

#ifndef __DMA_H
#define __DMA_H
#include <nthalext.h>

#define Add2Ptr(Ptr, Value) ((PVOID)((PUCHAR)(Ptr) + (Value)))

//
// ---------------------------------------------------------------- Definitions
//

//
// Define register set size.
//

#define SUNXI_DMA_REGISTER_SIZE			(0x4E0)



//
// Define maximums.
//

#define SUNXI_DMA_MAX_REQUEST_LINES		(16)
#define SUNXI_DMA_MAX_CHANNELS          (16)

#define SUNXI_DMA_DESCRIPOR_SIZE_FOR_EACH (8)
#define SUNXI_DMA_MAXFRAGMENTS_COUNT (32)
#define SUNXI_DMA_DES_BUFFER_SIZE_PER_CHANNEL (SUNXI_DMA_DESCRIPOR_SIZE_FOR_EACH*SUNXI_DMA_MAXFRAGMENTS_COUNT)

//
// ------------------------------------------------------ Data Type Definitions
//

#pragma pack(push, 1)

//
// Register Definitions.
//

typedef struct _SUNXI_DMA_CHANNEL_COMMON {
	ULONG	DmaIrqEnable[2];
	ULONG   Reserve[2];
	ULONG	DmaIrqPending[2];
	ULONG   Reserve1[2];
	ULONG   DmaSec;
	ULONG   Reserve2[1];
	ULONG   DmaAutoGate;
	ULONG   Reserve3[1];
	ULONG	DmaChannelStatus;
} SUNXI_DMA_CHANNEL_COMMON, *PSUNXI_DMA_CHANNEL_COMMON;

typedef struct _SUNXI_DMA_CHANNEL_PRIVATE {
	ULONG	DmaEnable;
	ULONG	DmaPause;
	ULONG	DmaStartAddr;
	ULONG	DmaConfig;
	ULONG	DmaCurrentSrc;
	ULONG	DmaCurrentDest;
	ULONG	DmaLeftCount;
	ULONG	DmaPara;
	ULONG   Reserve[2];
	ULONG   DmaMode;
	ULONG   DmaFdescAddr;
	ULONG   DmaPkgNum;
} SUNXI_DMA_CHANNEL_PRIVATE, *PSUNXI_DMA_CHANNEL_PRIVATE;

#pragma pack(pop)

//
// CSRT resource descriptor types for chDMA.
//

typedef struct _RESOURCE_DMA_CONTROLLER {
	CSRT_RESOURCE_DESCRIPTOR_HEADER Header;
	ULONG BasePhysicalAddressLow;
	ULONG BasePhysicalAddressHight;
	ULONG MappingSize;
	ULONG InterruptGsi;
	ULONG InterruptMode;
	ULONG InterruptPolarity;
	ULONG ChannelCount;
	ULONG DmaChannelConfig[SUNXI_DMA_MAX_CHANNELS];
} RESOURCE_DMA_CONTROLLER, *PRESOURCE_DMA_CONTROLLER;

typedef struct _RD_DMA_CHANNEL {
	CSRT_RESOURCE_DESCRIPTOR_HEADER Header;
	ULONG ChannelNumber;
} RD_DMA_CHANNEL, *PRD_DMA_CHANNEL;

typedef struct _CH_MEMORY_ADDRESS {
	ULONG Address;
	ULONG Length;
}CH_MEMORY_ADDRESS, *PCH_MEMORY_ADDRESS;

typedef struct _CH_MEMORY_ADDRESS_LIST {
	CH_MEMORY_ADDRESS Elements[SUNXI_DMA_MAXFRAGMENTS_COUNT];
	ULONG NumberOfElements;
}CH_MEMORY_ADDRESS_LIST, *PCH_MEMORY_ADDRESS_LIST;


typedef struct _SUNXI_DMA_CHANNEl{
	ULONG			Used;
	PDMA_CHANNEL_T	ChannelContext;
	ULONG DmaChannelConfig;	//config vaule for cfg reg in descriptor, this value should be get from acpi
	ULONG DesBufferPhysicalAddress;
	PVOID DesBufferVirtualAddress;
	CH_MEMORY_ADDRESS_LIST MemoryAddressList;
	ULONG			IrqType;/* channel irq supported, eg: CHAN_IRQ_HD | CHAN_IRQ_FD */
	BOOLEAN			IsLoopTransfer;	/* Single loop type */
	ULONG           CurrentElementId;
	ULONG           Loopcounter;
	enum TRANSFER_STATE_E	State;/* Transfer state */
}SUNXI_DMA_CHANNEL, *PSUNXI_DMA_CHANNEL;

typedef struct _SUNXI_DMA_CONTROLLER {
	//
	// Virtual and physical base addresses of the controller.
	//

	PULONG ControllerBaseVa;
	PHYSICAL_ADDRESS ControllerBasePa;
	ULONG MappingSize;

	PSUNXI_DMA_CHANNEL_COMMON	CommonReg;
	PSUNXI_DMA_CHANNEL_PRIVATE	PrivateReg[SUNXI_DMA_MAX_CHANNELS];

	ULONG ChannelCount;
	ULONG MinimumRequestLine;
	ULONG MaximumRequestLine;

	SUNXI_DMA_CHANNEL ChannelInfo[SUNXI_DMA_MAX_CHANNELS];

} SUNXI_DMA_CONTROLLER, *PSUNXI_DMA_CONTROLLER;
#endif
