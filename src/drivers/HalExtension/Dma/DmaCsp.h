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

#ifndef __DMA_CSP_H
#define __DMA_CSP_H
#include "Dma.h"
//
//register Routines
//

VOID CspDmaSetStartAddr(
	__in PSUNXI_DMA_CONTROLLER Controller,
	__in ULONG ChannelNumber,
	__in ULONG StartAddr
)
{
	WRITE_REGISTER_ULONG(&Controller->PrivateReg[ChannelNumber]->DmaStartAddr, StartAddr);
}

VOID CspDmaPause(
	__in PSUNXI_DMA_CONTROLLER Controller,
	__in ULONG ChannelNumber
)
{
	WRITE_REGISTER_ULONG(&Controller->PrivateReg[ChannelNumber]->DmaPause, 1);
}

VOID CspDmaStart(
	__in PSUNXI_DMA_CONTROLLER Controller,
	__in ULONG ChannelNumber
	)
{
	WRITE_REGISTER_ULONG(&Controller->PrivateReg[ChannelNumber]->DmaEnable, 1);
}

VOID CspDmaResume(
	__in PSUNXI_DMA_CONTROLLER Controller,
	__in ULONG ChannelNumber
	)
{
	WRITE_REGISTER_ULONG(&Controller->PrivateReg[ChannelNumber]->DmaPause, 0);
}

VOID CspDmaStop(
	__in PSUNXI_DMA_CONTROLLER Controller,
	__in ULONG ChannelNumber
	)
{
	WRITE_REGISTER_ULONG(&Controller->PrivateReg[ChannelNumber]->DmaEnable, 0);
}

/**
* CspDmaGetStatus - get dma channel status
* @Address:	the register address
*
* Returns 1 indicate channel is busy, 0 idle
*/
ULONG CspDmaGetStatus(
	__in PSUNXI_DMA_CONTROLLER Controller,
	__in ULONG ChannelNumber
	)
{
	return ((READ_REGISTER_ULONG(&Controller->CommonReg->DmaChannelStatus) >> ChannelNumber) & 1);
}

/**
* CspDmaGetCurSrcaddr - get dma channel's cur src addr reg value
*
* Returns the channel's cur src addr reg value
*/
ULONG CspDmaGetCurSrcAddr(
	__in PSUNXI_DMA_CONTROLLER Controller,
	__in ULONG ChannelNumber
	)
{
	return READ_REGISTER_ULONG(&Controller->PrivateReg[ChannelNumber]->DmaCurrentSrc);
}

/**
* CspDmaGetCurDstaddr - get dma channel's cur dst addr reg value
* @Address:	the current destaddress register address
*
* Returns the channel's cur dst addr reg value
*/
 ULONG CspDmaGetCurDstAddr(
	__in PSUNXI_DMA_CONTROLLER Controller,
	__in ULONG ChannelNumber
	)
{
	return READ_REGISTER_ULONG(&Controller->PrivateReg[ChannelNumber]->DmaCurrentDest);
}

/**
* CspDmaGetLeftBytecnt - get dma channel's left byte cnt
* @Address:the left bytecnt register address
*
* Returns the channel's left byte cnt
*/
ULONG  CspDmaGetLeftByteCnt(
	__in PSUNXI_DMA_CONTROLLER Controller,
	__in ULONG ChannelNumber
	)
{
	return READ_REGISTER_ULONG(&Controller->PrivateReg[ChannelNumber]->DmaLeftCount);
}

/**
* CspDmaGetStartAddr - get dma channel's start address reg value
*
* Returns the dma channel's start address reg value
*/
ULONG CspDmaGetStartAddr(
	__in PSUNXI_DMA_CONTROLLER Controller,
	__in ULONG ChannelNumber
	)
{
	return READ_REGISTER_ULONG(&Controller->PrivateReg[ChannelNumber]->DmaStartAddr);
}

/**
* CspDmaIrqEnable - enable dma channel irq
* @Address: the enable register address
* @channelNumber:	dma channel Number
* @irq_type:	irq type that will be enabled
*/
VOID CspDmaIrqEnable(
	__in PSUNXI_DMA_CONTROLLER Controller,
	__in ULONG ChannelNumber,
	__in ULONG IrqType,
	__in BOOLEAN Enable
	)
{
	ULONG uTemp = 0;
	ULONG	*Address = 0;

	if (ChannelNumber > 7) {
		ChannelNumber -= 8;
		Address = &Controller->CommonReg->DmaIrqEnable[1];
	}
	else {
		Address = &Controller->CommonReg->DmaIrqEnable[0];
	}

	uTemp = READ_REGISTER_ULONG(Address);

	if (Enable) {
		uTemp |= (IrqType << (ChannelNumber * 4 ));
		WRITE_REGISTER_ULONG(Address, uTemp);
	}
	else {
		uTemp &= ~(IrqType << (ChannelNumber * 4));
		WRITE_REGISTER_ULONG(Address, uTemp);

	}
}

/**
* CspDmaGetStatus - get dma channel irq pending val
* @Address:	the irq pending register address
* @channelNumber:	dma channel Number
* Returns the irq pend value, eg: 0b101
*/
ULONG CspDmaGetIrqPend(
	__in PSUNXI_DMA_CONTROLLER Controller,
	__in ULONG ChannelNumber
	)
{
	ULONG 	uret = 0;
	ULONG 	utemp = 0;
	ULONG   *Address = 0;

	if (ChannelNumber > 7) {
		ChannelNumber -= 8;
		Address = &Controller->CommonReg->DmaIrqPending[1];
	}
	else {
		Address = &Controller->CommonReg->DmaIrqPending[0];
	}

	utemp = READ_REGISTER_ULONG(Address);
	uret = (utemp >> (ChannelNumber * 4)) & 0x7;
	return uret;
}

/**
* CspDmaClearIrqpend - clear the dma channel irq pending
*/
VOID CspDmaClearIrqPend(
	__in PSUNXI_DMA_CONTROLLER Controller,
	__in ULONG ChannelNumber,
	__in ULONG IrqType
	)
{
	ULONG  	uTemp = 0;
	ULONG	*Address = 0;

	if (ChannelNumber > 7) {
		ChannelNumber -= 8;
		Address = &Controller->CommonReg->DmaIrqPending[1];
	}
	else {
		Address = &Controller->CommonReg->DmaIrqPending[0];
	}

	uTemp = READ_REGISTER_ULONG(Address);
	uTemp &= IrqType << (ChannelNumber * 4);
	WRITE_REGISTER_ULONG(Address, uTemp);
}

/**
* CspDmaClearAllIrqPend - clear dma irq pending register
* @index:	irq pend reg index, 0 or 1
*
* Returns the irq pend value before cleared, 0xffffffff if failed
*/
ULONG CspDmaClearAllIrqPend(
	__in PSUNXI_DMA_CONTROLLER Controller,
	__in ULONG Index
	)
{
	ULONG 	uret = 0;

	uret = READ_REGISTER_ULONG(&Controller->CommonReg->DmaIrqPending[Index]);
	WRITE_REGISTER_ULONG(&Controller->CommonReg->DmaIrqPending[Index], uret);
	return uret;
}

#endif  /* __DMA_CSP_H */

