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

#ifndef _SUNXIDMA_H_
#define _SUNXIDMA_H_

#include <nthalext.h>


/* dma end des link */
#define DMA_END_DES_LINK	0xFFFFF800
#define DMA_PARA_NORMAL_WAIT (8 << 0)




/* dma operation type */
enum DMA_OP_TYPE_E {
	DMA_OP_START,  			/* start dma */
	DMA_OP_PAUSE,  			/* pause transferring */
	DMA_OP_RESUME,  		/* resume transferring */
	DMA_OP_STOP,  			/* stop dma */

	DMA_OP_GET_CONFIGURE,	/* Config */
	DMA_OP_ENABLE_IRQ,		/* Enable IRQ*/

	DMA_OP_GET_STATUS,  		/* get channel status: idle/busy */
	DMA_OP_GET_CUR_SRC_ADDR,  	/* get current src address */
	DMA_OP_GET_CUR_DST_ADDR,  	/* get current dst address */
	DMA_OP_GET_BYTECNT_LEFT,  	/* get byte cnt left */

};

/* dma channel irq type */
enum DMA_IRQ_TYPE {
	DMA_IRQ_NO = 0,			/* none */
	DMA_IRQ_HD = 0x01,	/* package half done irq */
	DMA_IRQ_FD = 0x02,	/* package full done irq */
	DMA_IRQ_QD = 0x04	/* queue end irq */
};


enum TRANSFER_STATE_E{
	TRANSFER_STATE_IDLE,		/* maybe before start or after stop */
	TRANSFER_STATE_RUNING,	/* transferring */
	TRANSFER_STATE_DONE		/* the last buffer has done,
					* in this state, any enqueueing will start dma
					*/
};

/*
* dma config information
*/
typedef struct _DMA_CONFIG_REGISTER{
	/*
	* data length and burst length combination in DDMA and NDMA
	* eg: DMAXFER_D_SWORD_S_SWORD, DMAXFER_D_SBYTE_S_BBYTE
	*/
	enum XFERUNIT_E	XferType;
	/*
	* NDMA/DDMA src/dst address type
	* eg: DMAADDRT_D_INC_S_INC(NDMA addr type),
	*     DMAADDRT_D_LN_S_LN / DMAADDRT_D_LN_S_IO(DDMA addr type)
	*/
	enum ADDRT_E	AddressType;
	ULONG 			IrqType; 
	ULONG			SrcDrqType; /* src drq type */
	ULONG			DstDrqType; /* dst drq type */
}DMA_CONFIG_REGISTER, *PDMA_CONFIG_REGISTER;

/* define dma config descriptor struct for hardware */
typedef struct _TRANSFER_DES {
	ULONG		cofig; /* dma configuration reg */
	ULONG		saddr; /* dma src addr reg */
	ULONG		daddr; /* dma dst addr reg */
	ULONG		bcnt;  /* dma byte cnt reg */
	ULONG		param; /* dma param reg,set to 8 as default*/
	ULONG       pnext; /* next descriptor address */
}TRANSFER_DES, *PTRANSFER_DES;

typedef struct _CURRENT_ITEM{
	TRANSFER_DES	TransferInfo;
	ULONG			paddr; /* DesItem address */
	ULONG			IrqType;/* channel irq supported, eg: CHAN_IRQ_HD | CHAN_IRQ_FD */
	BOOLEAN			LoopType;	/* Single loop type */
}CURRENT_ITEM, *PCURRENT_ITEM;

/* define dma channel struct */
typedef struct _DMA_CHANNEL_T{
	ULONG					ChannelNumber; /* channel id, 0~15 */
	
	DMA_CONFIG_REGISTER     ConfigRegisterInfo;/* Config register info */
	ULONG					ConfigRegisterValue;


	CURRENT_ITEM			CurDesItem;
	PMDL					TransferList;/* Transfer item list */

	ULONG					PackageNumber; /* Transfer package number */
		

	enum TRANSFER_STATE_E	State;/* Transfer state */
	VOID					*parg;/* get value */

	BOOLEAN					IsEnableIrq;/* TRUE:Enable, FALSE:disable */
	BOOLEAN					IsSingle; /* TRUE:Single packages, FALSE: Chain packages*/

}DMA_CHANNEL_T, *PDMA_CHANNEL_T;

#endif