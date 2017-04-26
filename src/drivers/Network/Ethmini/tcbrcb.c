/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

    THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
    KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
    IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
    PURPOSE.

Module Name:

        TcbRcb.C

Abstract:

    This module contains miniport functions for handling Send & Receive
    packets and other helper routines called by these miniport functions.

    In order to excercise the send and receive code path of this driver,
    you should install more than one instance of the miniport. If there
    is only one instance installed, the driver throws the send packet on
    the floor and completes the send successfully. If there are more
    instances present, it indicates the incoming send packet to the other
    instances. For example, if there 3 instances: A, B, & C installed.
    Packets coming in for A instance would be indicated to B & C; packets
    coming into B would be indicated to C, & A; and packets coming to C
    would be indicated to A & B.

Revision History:

Notes:

--*/

#include "Ethmini.h"
#include "tcbrcb.tmh"


VOID
ReturnTCB(
    _In_  PMP_ADAPTER  Adapter,
    _In_  PTCB         Tcb)
{
    TXNblRelease(Adapter, NBL_FROM_SEND_NB(Tcb->NetBuffer), TRUE);
    Tcb->NetBuffer = NULL;
	Tcb->BytesSent = 0;

    NdisInterlockedInsertTailList(
            &Adapter->FreeTcbList,
            &Tcb->TcbLink,
            &Adapter->FreeTcbListLock);
}

VOID
ReturnRCB(
    _In_  PMP_ADAPTER   Adapter,
    _In_  PRCB          Rcb)
/*++

Routine Description:

    This routine frees an RCB back to the unused pool, recovers relevant memory used in the RCB.

    Runs at IRQL <= DISPATCH_LEVEL

Arguments:

    Adapter     - The receiving adapter (the one that owns the RCB).
    Rcb         - The RCB to be freed.

Return Value:

    None.

--*/
{
	PDMA_DESC Desc = Rcb->DmaDesc;
	
    DEBUGP(MP_TRACE, "[%p] ---> ReturnRCB. RCB: %p\n", Adapter, Rcb);
   
	Rcb->RecvLen = 0;
	HW_DMA_Set_Buffer(Desc, Rcb->PhyAddress, NIC_RECV_BUFFER_DMA_USE_SIZE);
	HW_DMA_Clear_Status(Desc);
	HW_DMA_Set_Own(Desc);
	//HW_DMA_Set_Rx_Int(Desc);
    
    DEBUGP(MP_TRACE, "[%p] <--- ReturnRCB.\n", Adapter);

}


