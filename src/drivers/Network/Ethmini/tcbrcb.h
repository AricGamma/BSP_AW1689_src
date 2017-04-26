/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

    THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
    KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
    IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
    PURPOSE.

Module Name:

   TcbRcb.H

Abstract:

    This module declares the TCB and RCB structures, and the functions to
    manipulate them.

    See the comments in TcbRcb.c.

--*/


#ifndef _TCBRCB_H
#define _TCBRCB_H

//
// TCB (Transmit Control Block)
// -----------------------------------------------------------------------------
//

typedef struct _TCB
{
    LIST_ENTRY              TcbLink;
    PNET_BUFFER        		NetBuffer;
    ULONG                   FrameType;
	PDMA_DESC				DmaDesc;
    ULONG                   PhyAddress;
	PUCHAR					DataBuffer;
	ULONG					BufLen;
    ULONG                   BytesSent;
} TCB, *PTCB;

VOID
ReturnTCB(
    _In_  PMP_ADAPTER  Adapter,
    _In_  PTCB         Tcb);


//
// RCB (Receive Control Block)
// -----------------------------------------------------------------------------
//

typedef struct _RCB
{
    //LIST_ENTRY              RcbLink;
    PNET_BUFFER_LIST        Nbl;
	PMDL					Mdl;
	PDMA_DESC				DmaDesc;
    ULONG                   PhyAddress;
	PUCHAR					DataBuffer;
	ULONG					BufLen;
	ULONG					RecvLen;
} RCB, *PRCB;

VOID
ReturnRCB(
    _In_  PMP_ADAPTER   Adapter,
    _In_  PRCB          Rcb);

#endif // _TCBRCB_H

